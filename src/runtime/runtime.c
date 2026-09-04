/*
 * IronNet CLR Interpreter
 * runtime.c - Runtime type system implementation
 */

#include "iron/runtime.h"
#include "iron/exec.h"
#include "iron/pe.h"
#include "iron/debug.h"
#include "iron/thread.h"
#include <string.h>
#include <stdio.h>

static iron_runtime_type_t *resolve_signature_type(iron_assembly_t *assembly,
                                                   iron_sig_reader_t *reader,
                                                   iron_runtime_type_t **type_arguments,
                                                   iron_u32 type_argument_count,
                                                   iron_runtime_type_t **method_arguments,
                                                   iron_u32 method_argument_count,
                                                   iron_element_type_t *out_element_type,
                                                   iron_u32 *out_generic_param_index);
static iron_result_t load_generic_parameters(iron_assembly_t *assembly,
                                             iron_token_t owner_token,
                                             iron_runtime_type_t ***out_parameters,
                                             iron_u32 *out_parameter_count);
static iron_result_t load_method_signature(iron_assembly_t *assembly, iron_runtime_method_t *method, const iron_method_def_row_t *row);
static iron_result_t load_method_local_signature(iron_assembly_t *assembly, iron_runtime_method_t *method);
static iron_runtime_type_t *resolve_event_type(iron_assembly_t *assembly,
                                               iron_runtime_type_t *declaring_type,
                                               iron_token_t type_token);

static iron_u32 generic_method_inst_hash(const void *key, iron_size key_size)
{
    const struct iron_generic_method_inst *instantiation;
    iron_u32 hash;
    iron_u32 argument_index;

    (void)key_size;
    instantiation = (const struct iron_generic_method_inst *)key;
    hash = iron_hash_ptr(&instantiation->definition, sizeof(instantiation->definition));
    for (argument_index = 0; argument_index < instantiation->arg_count; argument_index++) {
        hash = hash * 16777619U ^ iron_hash_ptr(&instantiation->args[argument_index], sizeof(instantiation->args[argument_index]));
    }
    return hash ^ instantiation->arg_count;
}

static iron_bool generic_method_inst_equals(const void *left, const void *right, iron_size key_size)
{
    const struct iron_generic_method_inst *left_instantiation;
    const struct iron_generic_method_inst *right_instantiation;
    iron_u32 argument_index;

    (void)key_size;
    left_instantiation = (const struct iron_generic_method_inst *)left;
    right_instantiation = (const struct iron_generic_method_inst *)right;
    if (left_instantiation->definition != right_instantiation->definition || left_instantiation->arg_count != right_instantiation->arg_count) {
        return IRON_FALSE;
    }

    for (argument_index = 0; argument_index < left_instantiation->arg_count; argument_index++) {
        if (left_instantiation->args[argument_index] != right_instantiation->args[argument_index]) {
            return IRON_FALSE;
        }
    }
    return IRON_TRUE;
}

static iron_bool register_type_descriptor(iron_domain_t *domain, iron_runtime_type_t *type)
{
    void *key;
    iron_u8 present;

    if (!domain || !type) {
        return IRON_FALSE;
    }

    key = type;
    present = 1;
    return iron_hashmap_set(&domain->type_descriptors, &key, &present);
}

static void unregister_type_descriptor(iron_domain_t *domain, iron_runtime_type_t *type)
{
    void *key;

    if (!domain || !type) {
        return;
    }

    key = type;
    iron_hashmap_remove(&domain->type_descriptors, &key);
}

static void free_generic_parameters(iron_allocator_t *allocator, iron_runtime_type_t **parameters, iron_u32 parameter_count)
{
    iron_u32 parameter_index;

    if (!allocator || !parameters) {
        return;
    }

    for (parameter_index = 0; parameter_index < parameter_count; parameter_index++) {
        if (parameters[parameter_index]) {
            iron_domain_t *domain;

            domain = parameters[parameter_index]->module && parameters[parameter_index]->module->assembly ? parameters[parameter_index]->module->assembly->domain : NULL;
            unregister_type_descriptor(domain, parameters[parameter_index]);
            iron_free(allocator, parameters[parameter_index], sizeof(iron_runtime_type_t));
        }
    }
    iron_free(allocator, parameters, (iron_size)parameter_count * sizeof(iron_runtime_type_t *));
}

static void free_method_parameters(iron_allocator_t *allocator, iron_runtime_param_t **parameters, iron_u32 parameter_count)
{
    iron_u32 parameter_index;

    if (!allocator || !parameters) {
        return;
    }

    for (parameter_index = 0; parameter_index < parameter_count; parameter_index++) {
        if (parameters[parameter_index]) {
            iron_free(allocator, parameters[parameter_index], sizeof(iron_runtime_param_t));
        }
    }
    iron_free(allocator, parameters, (iron_size)parameter_count * sizeof(iron_runtime_param_t *));
}

static void free_property_parameters(iron_allocator_t *allocator, iron_runtime_param_t **parameters, iron_u32 parameter_count)
{
    free_method_parameters(allocator, parameters, parameter_count);
}

/* ============================================================================
 * Domain Implementation
 * ============================================================================ */

static iron_bool type_is_dynamically_constructed(const iron_runtime_type_t *type)
{
    return type && (type->is_generic_instance || type->kind == IRON_KIND_ARRAY || type->kind == IRON_KIND_POINTER || type->kind == IRON_KIND_BYREF || type->kind == IRON_KIND_FNPTR);
}

static void free_constructed_type(iron_domain_t *domain, iron_runtime_type_t *type)
{
    iron_u32 index;

    if (!domain || !type) {
        return;
    }

    unregister_type_descriptor(domain, type);

    if (type->fields) {
        for (index = 0; index < type->field_count; index++) {
            if (type->fields[index]) {
                iron_free(domain->allocator, type->fields[index], sizeof(iron_runtime_field_t));
            }
        }
        iron_free(domain->allocator, type->fields, type->field_count * sizeof(iron_runtime_field_t *));
    }

    if (type->methods) {
        for (index = 0; index < type->method_count; index++) {
            if (type->methods[index]) {
                if (type->methods[index]->params) {
                    free_method_parameters(domain->allocator, type->methods[index]->params, type->methods[index]->param_count);
                }
                if (type->methods[index]->locals) {
                    iron_free(domain->allocator,
                              type->methods[index]->locals,
                              (iron_size)type->methods[index]->local_count * sizeof(iron_runtime_type_t *));
                }
                if (type->methods[index]->generic_params) {
                    free_generic_parameters(domain->allocator, type->methods[index]->generic_params, type->methods[index]->generic_param_count);
                }
                iron_free(domain->allocator, type->methods[index], sizeof(iron_runtime_method_t));
            }
        }
        iron_free(domain->allocator, type->methods, type->method_count * sizeof(iron_runtime_method_t *));
    }

    if (type->properties) {
        for (index = 0; index < type->property_count; index++) {
            if (type->properties[index]) {
                if (type->properties[index]->index_params) {
                    free_property_parameters(domain->allocator, type->properties[index]->index_params, type->properties[index]->index_param_count);
                }
                iron_free(domain->allocator, type->properties[index], sizeof(iron_runtime_property_t));
            }
        }
        iron_free(domain->allocator, type->properties, type->property_count * sizeof(iron_runtime_property_t *));
    }

    if (type->events) {
        for (index = 0; index < type->event_count; index++) {
            if (type->events[index]) {
                iron_free(domain->allocator, type->events[index], sizeof(iron_runtime_event_t));
            }
        }
        iron_free(domain->allocator, type->events, type->event_count * sizeof(iron_runtime_event_t *));
    }

    if (type->nested_types) {
        iron_free(domain->allocator, type->nested_types, type->nested_type_count * sizeof(iron_runtime_type_t *));
    }

    if (type->interfaces) {
        iron_free(domain->allocator, type->interfaces, type->interface_count * sizeof(iron_runtime_type_t *));
    }
    if (type->vtable) {
        iron_free(domain->allocator, type->vtable, type->vtable_size * sizeof(iron_runtime_method_t *));
    }
    if (type->interface_map) {
        iron_free(domain->allocator, type->interface_map, type->interface_map_count * sizeof(*type->interface_map));
    }
    if (type->static_data) {
        iron_free(domain->allocator, type->static_data, type->static_data_size);
    }
    if (type->generic_args) {
        iron_free(domain->allocator, type->generic_args, type->generic_arg_count * sizeof(iron_runtime_type_t *));
    }
    if (type->generic_params) {
        free_generic_parameters(domain->allocator, type->generic_params, type->generic_param_count);
    }

    iron_free(domain->allocator, type, sizeof(iron_runtime_type_t));
}

iron_result_t iron_domain_create(iron_domain_t **out_domain,
                                 const iron_domain_config_t *config,
                                 iron_allocator_t *alloc)
{
    iron_domain_t *domain;

    if (!out_domain) {
        return IRON_ERROR(IRON_ERR_NULL_POINTER, "Invalid parameters");
    }

    if (!alloc) alloc = iron_system_allocator();

    domain = (iron_domain_t *)iron_alloc(alloc, sizeof(iron_domain_t));
    if (!domain) {
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate domain");
    }

    memset(domain, 0, sizeof(iron_domain_t));
    domain->allocator = alloc;
    
    /* Initialize interner FIRST before using it */
    iron_interner_init(&domain->interner, alloc);
    
    if (config) {
        iron_u32 path_index;

        domain->name = config->name ? 
            iron_intern_cstr(&domain->interner, config->name) : "DefaultDomain";
        domain->base_path = config->base_path ?
            iron_intern_cstr(&domain->interner, config->base_path) : ".";

        if (config->assembly_path_count > 0) {
            domain->assembly_paths = (const char **)iron_alloc(alloc, config->assembly_path_count * sizeof(const char *));
            if (!domain->assembly_paths) {
                iron_interner_destroy(&domain->interner);
                iron_free(alloc, domain, sizeof(iron_domain_t));
                return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to copy assembly search paths");
            }

            for (path_index = 0; path_index < config->assembly_path_count; path_index++) {
                domain->assembly_paths[path_index] = iron_intern_cstr(&domain->interner, config->assembly_paths[path_index]);
            }

            domain->assembly_path_count = config->assembly_path_count;
        }

        domain->load_callback = config->load_callback;
        domain->load_callback_data = config->load_callback_data;
    } else {
        domain->name = "DefaultDomain";
        domain->base_path = ".";
    }

    /* Initialize type cache */
    iron_hashmap_init(&domain->type_cache, alloc,
                      sizeof(const char*), sizeof(iron_runtime_type_t*),
                      iron_hash_string, iron_string_eq);

    iron_hashmap_init(&domain->type_descriptors, alloc,
                      sizeof(void *), sizeof(iron_u8),
                      iron_hash_ptr, iron_ptr_eq);
    
    /* Initialize generic instantiation cache */
    iron_hashmap_init(&domain->generic_inst_cache, alloc,
                      sizeof(iron_generic_inst_t), sizeof(iron_runtime_type_t*),
                      NULL, NULL);

    iron_hashmap_init(&domain->generic_method_cache, alloc,
                      sizeof(struct iron_generic_method_inst), sizeof(iron_runtime_method_t *),
                      generic_method_inst_hash, generic_method_inst_equals);
    
    *out_domain = domain;
    return IRON_SUCCESS;
}

void iron_domain_destroy(iron_domain_t *domain)
{
    iron_u32 i;
    
    if (!domain) return;

    for (i = 0; i < domain->generic_method_cache.capacity; i++) {
        iron_hashmap_entry_t *entry;
        iron_runtime_method_t *method;

        entry = &domain->generic_method_cache.entries[i];
        if (!entry->value) {
            continue;
        }

        method = *(iron_runtime_method_t **)entry->value;
        if (method->params) {
            free_method_parameters(domain->allocator, method->params, method->param_count);
        }
        if (method->locals) {
            iron_free(domain->allocator, method->locals, (iron_size)method->local_count * sizeof(iron_runtime_type_t *));
        }
        if (method->generic_args) {
            iron_free(domain->allocator, method->generic_args, (iron_size)method->generic_arg_count * sizeof(iron_runtime_type_t *));
        }
        iron_free(domain->allocator, method, sizeof(iron_runtime_method_t));
    }

    /* Constructed types are owned by the domain rather than an assembly. */
    for (i = 0; i < domain->type_cache.capacity; i++) {
        iron_hashmap_entry_t *entry;
        iron_runtime_type_t *type;

        entry = &domain->type_cache.entries[i];
        if (!entry->value) {
            continue;
        }

        type = *(iron_runtime_type_t **)entry->value;
        if (type_is_dynamically_constructed(type)) {
            free_constructed_type(domain, type);
        }
    }

    /* Free assemblies */
    for (i = 0; i < domain->assembly_count; i++) {
        iron_assembly_free(domain->assemblies[i]);
    }
    if (domain->assemblies) {
        iron_free(domain->allocator, domain->assemblies,
                  domain->assembly_capacity * sizeof(iron_assembly_t*));
    }

    if (domain->assembly_paths) {
        iron_free(domain->allocator, (void *)domain->assembly_paths, domain->assembly_path_count * sizeof(const char *));
    }

    /* Free caches */
    iron_hashmap_destroy(&domain->type_cache);
    iron_hashmap_destroy(&domain->type_descriptors);
    iron_hashmap_destroy(&domain->generic_inst_cache);
    iron_hashmap_destroy(&domain->generic_method_cache);
    iron_interner_destroy(&domain->interner);
    
    iron_free(domain->allocator, domain, sizeof(iron_domain_t));
}

void iron_domain_add_search_path(iron_domain_t *domain, const char *path)
{
    const char **new_paths;
    iron_u32 new_count;
    
    if (!domain || !path) return;
    
    new_count = domain->assembly_path_count + 1;
    new_paths = (const char **)iron_alloc(domain->allocator, 
                                           new_count * sizeof(const char *));
    if (!new_paths) return;
    
    /* Copy existing paths */
    if (domain->assembly_paths && domain->assembly_path_count > 0) {
        iron_u32 i;
        for (i = 0; i < domain->assembly_path_count; i++) {
            new_paths[i] = domain->assembly_paths[i];
        }
        iron_free(domain->allocator, (void *)domain->assembly_paths,
                  domain->assembly_path_count * sizeof(const char *));
    }

    /* Add new path (interned for memory efficiency) */
    new_paths[domain->assembly_path_count] = iron_intern_cstr(&domain->interner, path);
    
    domain->assembly_paths = new_paths;
    domain->assembly_path_count = new_count;
}

static iron_result_t domain_register_assembly(iron_domain_t *domain, iron_assembly_t *assembly)
{
    iron_assembly_t **new_list;
    iron_u32 new_capacity;

    if (!domain || !assembly) {
        return IRON_ERROR(IRON_ERR_NULL_POINTER, "Invalid domain or assembly");
    }

    if (domain->assembly_count < domain->assembly_capacity) {
        domain->assemblies[domain->assembly_count++] = assembly;
        return IRON_SUCCESS;
    }

    new_capacity = domain->assembly_capacity ? domain->assembly_capacity * 2 : 8;
    if (new_capacity < domain->assembly_capacity || (iron_size)new_capacity > ((iron_size)-1) / sizeof(iron_assembly_t *)) {
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Assembly list capacity overflow");
    }

    new_list = (iron_assembly_t **)iron_realloc(domain->allocator,
                                                 domain->assemblies,
                                                 domain->assembly_capacity * sizeof(iron_assembly_t *),
                                                 new_capacity * sizeof(iron_assembly_t *));
    if (!new_list) {
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to grow assembly list");
    }

    domain->assemblies = new_list;
    domain->assembly_capacity = new_capacity;
    domain->assemblies[domain->assembly_count++] = assembly;
    return IRON_SUCCESS;
}

iron_result_t iron_domain_load_assembly(iron_domain_t *domain,
                                        const char *path,
                                        iron_assembly_t **out_assembly)
{
    iron_result_t result;
    iron_assembly_t *assembly;
    
    if (!domain || !path || !out_assembly) {
        return IRON_ERROR(IRON_ERR_NULL_POINTER, "Invalid parameters");
    }
    
    result = iron_assembly_load(&assembly, domain, path);
    if (!IRON_RESULT_OK(result)) {
        return result;
    }
    
    result = domain_register_assembly(domain, assembly);
    if (!IRON_RESULT_OK(result)) {
        iron_assembly_free(assembly);
        return result;
    }

    *out_assembly = assembly;
    
    return IRON_SUCCESS;
}

iron_result_t iron_domain_load_assembly_memory(iron_domain_t *domain,
                                               const iron_u8 *data,
                                               iron_size size,
                                               iron_assembly_t **out_assembly)
{
    iron_assembly_t *assembly;
    iron_result_t result;

    if (!domain || !data || size == 0 || !out_assembly) {
        return IRON_ERROR(IRON_ERR_NULL_POINTER, "Invalid parameters");
    }

    result = iron_assembly_load_memory(&assembly, domain, data, size);
    if (!IRON_RESULT_OK(result)) {
        return result;
    }

    result = domain_register_assembly(domain, assembly);
    if (!IRON_RESULT_OK(result)) {
        iron_assembly_free(assembly);
        return result;
    }

    *out_assembly = assembly;
    return IRON_SUCCESS;
}

iron_assembly_t *iron_domain_find_assembly(iron_domain_t *domain, const char *name)
{
    iron_u32 i;
    
    if (!domain || !name) return NULL;
    
    for (i = 0; i < domain->assembly_count; i++) {
        if (domain->assemblies[i] && domain->assemblies[i]->name &&
            strcmp(domain->assemblies[i]->name, name) == 0) {
            return domain->assemblies[i];
        }
    }
    
    return NULL;
}

iron_runtime_type_t *iron_domain_find_type(iron_domain_t *domain,
                                            const char *full_name)
{
    iron_runtime_type_t *type = NULL;
    iron_u32 i;
    const char *dot_pos;
    const char *namespace_ = NULL;
    const char *name = full_name;
    char namespace_buf[512];
    
    if (!domain || !full_name) return NULL;
    
    /* Check cache first */
    if (iron_hashmap_get(&domain->type_cache, &full_name, &type)) {
        return type;
    }
    
    /* Parse namespace and name from full_name (e.g., "System.Console") */
    dot_pos = full_name;
    while (*dot_pos) {
        if (*dot_pos == '.') {
            name = dot_pos + 1;
        }
        dot_pos++;
    }
    
    if (name != full_name) {
        iron_size ns_len = (iron_size)(name - full_name - 1);
        if (ns_len < sizeof(namespace_buf)) {
            memcpy(namespace_buf, full_name, ns_len);
            namespace_buf[ns_len] = '\0';
            namespace_ = namespace_buf;
        }
    }
    
    /* Search in all assemblies */
    for (i = 0; i < domain->assembly_count; i++) {
        iron_assembly_t *assembly = domain->assemblies[i];
        if (!assembly) continue;

        type = iron_assembly_find_type(assembly, namespace_, name);
        if (type) {
            /* Cache the result */
            const char *interned_name = iron_intern_cstr(&domain->interner, full_name);
            iron_hashmap_set(&domain->type_cache, &interned_name, &type);
            return type;
        }
    }
    
    return NULL;
}

iron_bool iron_domain_is_type_descriptor(const iron_domain_t *domain, const void *pointer)
{
    void *key;
    iron_u8 present;

    if (!domain || !pointer) {
        return IRON_FALSE;
    }

    key = (void *)pointer;
    present = 0;
    return iron_hashmap_get(&domain->type_descriptors, &key, &present) && present != 0;
}

/* ============================================================================
 * Assembly Implementation
 * ============================================================================ */

static iron_result_t initialize_assembly_metadata(iron_assembly_t *assembly)
{
    iron_assembly_row_t assembly_row;
    iron_module_row_t module_row;
    iron_result_t result;

    result = iron_metadata_init(&assembly->metadata, &assembly->image, assembly->allocator);
    if (!IRON_RESULT_OK(result)) {
        return result;
    }

    if (iron_metadata_table_rows(&assembly->metadata, IRON_TABLE_ASSEMBLY) > 0) {
        result = iron_metadata_read_row(&assembly->metadata, IRON_MAKE_TOKEN(IRON_TABLE_ASSEMBLY, 1), &assembly_row);
        if (!IRON_RESULT_OK(result)) {
            return result;
        }

        assembly->name = iron_metadata_get_string(&assembly->metadata, assembly_row.name);
        assembly->culture = iron_metadata_get_string(&assembly->metadata, assembly_row.culture);
        assembly->major_version = assembly_row.major_version;
        assembly->minor_version = assembly_row.minor_version;
        assembly->build_number = assembly_row.build_number;
        assembly->revision_number = assembly_row.revision_number;
        assembly->flags = assembly_row.flags;
        if (assembly_row.public_key != 0) {
            result = iron_metadata_get_blob(&assembly->metadata, assembly_row.public_key, &assembly->public_key, &assembly->public_key_size);
            if (!IRON_RESULT_OK(result)) {
                return result;
            }
        }
    }

    assembly->module = (iron_module_t *)iron_alloc(assembly->allocator, sizeof(iron_module_t));
    if (!assembly->module) {
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate assembly module");
    }

    memset(assembly->module, 0, sizeof(iron_module_t));
    assembly->module->assembly = assembly;

    if (iron_metadata_table_rows(&assembly->metadata, IRON_TABLE_MODULE) == 0) {
        return IRON_SUCCESS;
    }

    result = iron_metadata_read_row(&assembly->metadata, IRON_MAKE_TOKEN(IRON_TABLE_MODULE, 1), &module_row);
    if (!IRON_RESULT_OK(result)) {
        return result;
    }

    assembly->module->name = iron_metadata_get_string(&assembly->metadata, module_row.name);
    {
        const iron_u8 *mvid;

        mvid = iron_metadata_get_guid(&assembly->metadata, module_row.mvid);
        if (mvid) {
            memcpy(assembly->module->mvid, mvid, sizeof(assembly->module->mvid));
        }
    }

    return IRON_SUCCESS;
}

static void free_unregistered_assembly(iron_assembly_t *assembly)
{
    if (!assembly) {
        return;
    }

    if (assembly->module) {
        iron_free(assembly->allocator, assembly->module, sizeof(iron_module_t));
        assembly->module = NULL;
    }

    iron_metadata_free(&assembly->metadata);
    iron_pe_free(&assembly->image);
    iron_free(assembly->allocator, assembly, sizeof(iron_assembly_t));
}

iron_result_t iron_assembly_load(iron_assembly_t **out_assembly,
                                 iron_domain_t *domain,
                                 const char *path)
{
    iron_result_t result;
    iron_assembly_t *assembly;
    
    if (!out_assembly || !domain || !path) {
        return IRON_ERROR(IRON_ERR_NULL_POINTER, "Invalid parameters");
    }
    
    assembly = (iron_assembly_t *)iron_alloc(domain->allocator, sizeof(iron_assembly_t));
    if (!assembly) {
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate assembly");
    }
    
    memset(assembly, 0, sizeof(iron_assembly_t));
    assembly->domain = domain;
    assembly->allocator = domain->allocator;
    assembly->location = iron_intern_cstr(&domain->interner, path);
    if (!assembly->location) {
        iron_free(domain->allocator, assembly, sizeof(iron_assembly_t));
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to retain the assembly location");
    }
    
    /* Load PE image */
    result = iron_pe_load_file(&assembly->image, path, domain->allocator);
    if (!IRON_RESULT_OK(result)) {
        iron_free(domain->allocator, assembly, sizeof(iron_assembly_t));
        return result;
    }
    
    /* Check if it's a CLI image */
    if (!iron_pe_is_cli(&assembly->image)) {
        iron_pe_free(&assembly->image);
        iron_free(domain->allocator, assembly, sizeof(iron_assembly_t));
        return IRON_ERROR(IRON_ERR_NOT_CLI_IMAGE, "Not a CLI assembly");
    }
    
    result = initialize_assembly_metadata(assembly);
    if (!IRON_RESULT_OK(result)) {
        free_unregistered_assembly(assembly);
        return result;
    }
    
    *out_assembly = assembly;
    return IRON_SUCCESS;
}

iron_result_t iron_assembly_load_memory(iron_assembly_t **out_assembly,
                                        iron_domain_t *domain,
                                        const iron_u8 *data,
                                        iron_size size)
{
    iron_result_t result;
    iron_assembly_t *assembly;
    
    if (!out_assembly || !domain || !data || size == 0) {
        return IRON_ERROR(IRON_ERR_NULL_POINTER, "Invalid parameters");
    }
    
    assembly = (iron_assembly_t *)iron_alloc(domain->allocator, sizeof(iron_assembly_t));
    if (!assembly) {
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate assembly");
    }
    
    memset(assembly, 0, sizeof(iron_assembly_t));
    assembly->domain = domain;
    assembly->allocator = domain->allocator;
    assembly->location = iron_intern_cstr(&domain->interner, "");
    if (!assembly->location) {
        iron_free(domain->allocator, assembly, sizeof(iron_assembly_t));
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to initialize the in-memory assembly location");
    }
    
    /* Load PE image from memory */
    result = iron_pe_load(&assembly->image, data, size, IRON_TRUE, domain->allocator);
    if (!IRON_RESULT_OK(result)) {
        iron_free(domain->allocator, assembly, sizeof(iron_assembly_t));
        return result;
    }
    
    /* Check if it's a CLI image */
    if (!iron_pe_is_cli(&assembly->image)) {
        iron_pe_free(&assembly->image);
        iron_free(domain->allocator, assembly, sizeof(iron_assembly_t));
        return IRON_ERROR(IRON_ERR_NOT_CLI_IMAGE, "Not a CLI assembly");
    }
    
    result = initialize_assembly_metadata(assembly);
    if (!IRON_RESULT_OK(result)) {
        free_unregistered_assembly(assembly);
        return result;
    }
    
    *out_assembly = assembly;
    return IRON_SUCCESS;
}

void iron_assembly_free(iron_assembly_t *assembly)
{
    iron_u32 type_index;
    iron_u32 method_index;

    if (!assembly) return;

    if (assembly->types) {
        for (type_index = 0; type_index < assembly->type_count; type_index++) {
            iron_runtime_type_t *type;

            type = &assembly->types[type_index];
            unregister_type_descriptor(assembly->domain, type);
            if (type->fields) {
                iron_free(assembly->allocator, type->fields, type->field_count * sizeof(iron_runtime_field_t *));
            }
            if (type->methods) {
                iron_free(assembly->allocator, type->methods, type->method_count * sizeof(iron_runtime_method_t *));
            }
            if (type->properties) {
                iron_free(assembly->allocator, type->properties, type->property_count * sizeof(iron_runtime_property_t *));
            }
            if (type->events) {
                iron_free(assembly->allocator, type->events, type->event_count * sizeof(iron_runtime_event_t *));
            }
            if (type->nested_types) {
                iron_free(assembly->allocator, type->nested_types, type->nested_type_count * sizeof(iron_runtime_type_t *));
            }
            if (type->interfaces) {
                iron_free(assembly->allocator, type->interfaces, type->interface_count * sizeof(iron_runtime_type_t *));
            }
            if (type->vtable) {
                iron_free(assembly->allocator, type->vtable, type->vtable_size * sizeof(iron_runtime_method_t *));
            }
            if (type->interface_map) {
                iron_free(assembly->allocator, type->interface_map, type->interface_map_count * sizeof(*type->interface_map));
            }
            if (type->static_data) {
                iron_free(assembly->allocator, type->static_data, type->static_data_size);
            }
            if (type->generic_params) {
                free_generic_parameters(assembly->allocator, type->generic_params, type->generic_param_count);
            }
        }

        iron_free(assembly->allocator, assembly->types, assembly->type_count * sizeof(iron_runtime_type_t));
    }

    if (assembly->methods) {
        for (method_index = 0; method_index < assembly->method_count; method_index++) {
            iron_runtime_method_t *method;

            method = &assembly->methods[method_index];
            if (method->body) {
                iron_free_method_body(method->body, assembly->allocator);
                iron_free(assembly->allocator, method->body, sizeof(iron_method_body_t));
            }
            if (method->params) {
                free_method_parameters(assembly->allocator, method->params, method->param_count);
            }
            if (method->locals) {
                iron_free(assembly->allocator, method->locals, method->local_count * sizeof(iron_runtime_type_t *));
            }
            if (method->generic_params) {
                free_generic_parameters(assembly->allocator, method->generic_params, method->generic_param_count);
            }
        }

        iron_free(assembly->allocator, assembly->methods, assembly->method_count * sizeof(iron_runtime_method_t));
    }

    if (assembly->fields) {
        iron_free(assembly->allocator, assembly->fields, assembly->field_count * sizeof(iron_runtime_field_t));
    }
    if (assembly->properties) {
        iron_u32 property_index;

        for (property_index = 0; property_index < assembly->property_count; property_index++) {
            if (assembly->properties[property_index].index_params) {
                free_property_parameters(assembly->allocator,
                                         assembly->properties[property_index].index_params,
                                         assembly->properties[property_index].index_param_count);
            }
        }
        iron_free(assembly->allocator, assembly->properties, assembly->property_count * sizeof(iron_runtime_property_t));
    }
    if (assembly->events) {
        iron_free(assembly->allocator, assembly->events, assembly->event_count * sizeof(iron_runtime_event_t));
    }

    if (assembly->module) {
        if (assembly->module->types) {
            iron_free(assembly->allocator, assembly->module->types,
                      assembly->module->type_count * sizeof(iron_runtime_type_t*));
        }
        iron_free(assembly->allocator, assembly->module, sizeof(iron_module_t));
    }
    
    if (assembly->references) {
        iron_free(assembly->allocator, assembly->references,
                  assembly->reference_count * sizeof(iron_assembly_t*));
    }
    
    iron_metadata_free(&assembly->metadata);
    iron_pe_free(&assembly->image);
    iron_free(assembly->allocator, assembly, sizeof(iron_assembly_t));
}

iron_runtime_type_t *iron_assembly_find_type(iron_assembly_t *assembly,
                                             const char *namespace_,
                                             const char *name)
{
    iron_u32 i;
    iron_u32 type_count;
    
    if (!assembly || !assembly->module || !name) {
        return NULL;
    }
    
    type_count = iron_metadata_table_rows(&assembly->metadata, IRON_TABLE_TYPE_DEF);
    for (i = 0; i < type_count; i++) {
        iron_runtime_type_t *type;

        type = iron_type_resolve_token(assembly->module, IRON_MAKE_TOKEN(IRON_TABLE_TYPE_DEF, i + 1));
        if (!type) {
            continue;
        }

        if (type->name && strcmp(type->name, name) == 0) {
            if ((!namespace_ || namespace_[0] == '\0') && (!type->namespace_ || type->namespace_[0] == '\0')) {
                return type;
            }

            if (namespace_ && type->namespace_ && strcmp(type->namespace_, namespace_) == 0) {
                return type;
            }
        }
    }
    
    return NULL;
}

iron_runtime_method_t *iron_assembly_get_entry_point(iron_assembly_t *assembly)
{
    iron_token_t entry_token;
    iron_runtime_method_t *method;
    
    if (!assembly) return NULL;
    
    /* Check cached entry point */
    if (assembly->entry_point) {
        return assembly->entry_point;
    }
    
    /* Get entry point token from CLI header */
    entry_token = assembly->image.cli_header.entry_point_token;
    if (entry_token == 0) {
        return NULL;
    }
    
    /* Resolve token to method */
    method = iron_resolve_method_token(assembly, entry_token);
    if (method) {
        assembly->entry_point = method;
    }
    
    return method;
}

/* Resolve a method token to a runtime method */
static iron_result_t read_effective_element_type(iron_sig_reader_t *reader, iron_element_type_t *out_element_type)
{
    iron_element_type_t element_type;
    iron_result_t result;

    result = iron_sig_read_element_type(reader, &element_type);
    if (!IRON_RESULT_OK(result)) {
        return result;
    }

    while (element_type == IRON_TYPE_CMOD_OPT || element_type == IRON_TYPE_CMOD_REQD || element_type == IRON_TYPE_PINNED || element_type == IRON_TYPE_SENTINEL) {
        if (element_type == IRON_TYPE_CMOD_OPT || element_type == IRON_TYPE_CMOD_REQD) {
            iron_token_t modifier_token;

            result = iron_sig_read_type_def_or_ref(reader, &modifier_token);
            if (!IRON_RESULT_OK(result)) {
                return result;
            }
        }

        result = iron_sig_read_element_type(reader, &element_type);
        if (!IRON_RESULT_OK(result)) {
            return result;
        }
    }

    *out_element_type = element_type;
    return IRON_SUCCESS;
}

static iron_bool signature_type_skip(iron_sig_reader_t *reader)
{
    iron_element_type_t element_type;
    iron_result_t result;

    result = read_effective_element_type(reader, &element_type);
    if (!IRON_RESULT_OK(result)) {
        return IRON_FALSE;
    }

    switch (element_type) {
        case IRON_TYPE_CLASS:
        case IRON_TYPE_VALUETYPE: {
            iron_token_t type_token;
            return IRON_RESULT_OK(iron_sig_read_type_def_or_ref(reader, &type_token));
        }
        case IRON_TYPE_VAR:
        case IRON_TYPE_MVAR: {
            iron_u32 generic_index;
            return IRON_RESULT_OK(iron_sig_read_compressed_u32(reader, &generic_index));
        }
        case IRON_TYPE_PTR:
        case IRON_TYPE_BYREF:
        case IRON_TYPE_SZARRAY:
            return signature_type_skip(reader);
        case IRON_TYPE_GENERICINST: {
            iron_element_type_t class_or_value;
            iron_token_t definition_token;
            iron_u32 argument_count;
            iron_u32 argument_index;

            result = iron_sig_read_element_type(reader, &class_or_value);
            if (!IRON_RESULT_OK(result) || (class_or_value != IRON_TYPE_CLASS && class_or_value != IRON_TYPE_VALUETYPE)) {
                return IRON_FALSE;
            }
            result = iron_sig_read_type_def_or_ref(reader, &definition_token);
            if (!IRON_RESULT_OK(result)) {
                return IRON_FALSE;
            }
            result = iron_sig_read_compressed_u32(reader, &argument_count);
            if (!IRON_RESULT_OK(result)) {
                return IRON_FALSE;
            }
            for (argument_index = 0; argument_index < argument_count; argument_index++) {
                if (!signature_type_skip(reader)) {
                    return IRON_FALSE;
                }
            }
            return IRON_TRUE;
        }
        case IRON_TYPE_ARRAY: {
            iron_u32 rank;
            iron_u32 count;
            iron_u32 index;

            if (!signature_type_skip(reader) || !IRON_RESULT_OK(iron_sig_read_compressed_u32(reader, &rank)) ||
                !IRON_RESULT_OK(iron_sig_read_compressed_u32(reader, &count))) {
                return IRON_FALSE;
            }
            for (index = 0; index < count; index++) {
                iron_u32 size;
                if (!IRON_RESULT_OK(iron_sig_read_compressed_u32(reader, &size))) {
                    return IRON_FALSE;
                }
            }
            if (!IRON_RESULT_OK(iron_sig_read_compressed_u32(reader, &count))) {
                return IRON_FALSE;
            }
            for (index = 0; index < count; index++) {
                iron_i32 lower_bound;
                if (!IRON_RESULT_OK(iron_sig_read_compressed_i32(reader, &lower_bound))) {
                    return IRON_FALSE;
                }
            }
            return IRON_TRUE;
        }
        case IRON_TYPE_FNPTR: {
            iron_call_conv_t calling_convention;
            iron_u32 generic_param_count;
            iron_u32 param_count;
            iron_u32 param_index;

            if (!IRON_RESULT_OK(iron_sig_read_method_header(reader, &calling_convention, &generic_param_count, &param_count)) || !signature_type_skip(reader)) {
                return IRON_FALSE;
            }
            for (param_index = 0; param_index < param_count; param_index++) {
                if (!signature_type_skip(reader)) {
                    return IRON_FALSE;
                }
            }
            return IRON_TRUE;
        }
        default:
            return IRON_TRUE;
    }
}

static iron_bool metadata_type_name(const iron_metadata_t *metadata, iron_token_t token, const char **out_namespace, const char **out_name)
{
    return IRON_RESULT_OK(iron_metadata_get_type_name(metadata, token, out_namespace, out_name));
}

static iron_bool metadata_type_tokens_equal(const iron_metadata_t *left_metadata,
                                            iron_token_t left_token,
                                            const iron_metadata_t *right_metadata,
                                            iron_token_t right_token)
{
    const char *left_namespace;
    const char *left_name;
    const char *right_namespace;
    const char *right_name;

    if (!metadata_type_name(left_metadata, left_token, &left_namespace, &left_name) ||
        !metadata_type_name(right_metadata, right_token, &right_namespace, &right_name)) {
        return IRON_FALSE;
    }

    if (!left_name || !right_name || strcmp(left_name, right_name) != 0) {
        return IRON_FALSE;
    }

    left_namespace = left_namespace ? left_namespace : "";
    right_namespace = right_namespace ? right_namespace : "";
    return strcmp(left_namespace, right_namespace) == 0;
}

typedef struct iron_signature_type_context {
    const iron_metadata_t *metadata;
    const iron_u8 *signature;
    iron_u32 signature_size;
} iron_signature_type_context_t;

static iron_bool signature_get_type_argument(const iron_signature_type_context_t *context, iron_u32 argument_index, iron_sig_reader_t *out_reader)
{
    iron_sig_reader_t reader;
    iron_element_type_t element_type;
    iron_element_type_t class_or_value;
    iron_token_t definition_token;
    iron_u32 argument_count;
    iron_u32 index;
    iron_size argument_start;

    if (!context || !context->signature || !out_reader) {
        return IRON_FALSE;
    }

    iron_sig_init(&reader, context->signature, context->signature_size);
    if (!IRON_RESULT_OK(read_effective_element_type(&reader, &element_type)) || element_type != IRON_TYPE_GENERICINST ||
        !IRON_RESULT_OK(iron_sig_read_element_type(&reader, &class_or_value)) ||
        (class_or_value != IRON_TYPE_CLASS && class_or_value != IRON_TYPE_VALUETYPE) ||
        !IRON_RESULT_OK(iron_sig_read_type_def_or_ref(&reader, &definition_token)) ||
        !IRON_RESULT_OK(iron_sig_read_compressed_u32(&reader, &argument_count)) || argument_index >= argument_count) {
        return IRON_FALSE;
    }

    for (index = 0; index < argument_index; index++) {
        if (!signature_type_skip(&reader)) {
            return IRON_FALSE;
        }
    }

    argument_start = reader.pos;
    if (!signature_type_skip(&reader)) {
        return IRON_FALSE;
    }

    iron_sig_init(out_reader, context->signature + argument_start, reader.pos - argument_start);
    return IRON_TRUE;
}

static iron_bool signature_type_matches(const iron_metadata_t *definition_metadata,
                                        iron_sig_reader_t *definition_reader,
                                        const iron_signature_type_context_t *definition_type_context,
                                        const iron_metadata_t *reference_metadata,
                                        iron_sig_reader_t *reference_reader)
{
    iron_element_type_t definition_type;
    iron_element_type_t reference_type;
    iron_size reference_type_start;

    reference_type_start = reference_reader->pos;

    if (!IRON_RESULT_OK(read_effective_element_type(definition_reader, &definition_type)) ||
        !IRON_RESULT_OK(read_effective_element_type(reference_reader, &reference_type))) {
        return IRON_FALSE;
    }

    if (definition_type == IRON_TYPE_VAR) {
        iron_u32 definition_index;
        iron_u32 reference_index;
        iron_sig_reader_t type_argument_reader;

        if (!IRON_RESULT_OK(iron_sig_read_compressed_u32(definition_reader, &definition_index))) {
            return IRON_FALSE;
        }

        if (reference_type == IRON_TYPE_VAR) {
            return IRON_RESULT_OK(iron_sig_read_compressed_u32(reference_reader, &reference_index)) && definition_index == reference_index;
        }

        if (signature_get_type_argument(definition_type_context, definition_index, &type_argument_reader)) {
            reference_reader->pos = reference_type_start;
            return signature_type_matches(definition_type_context->metadata, &type_argument_reader, NULL, reference_metadata, reference_reader);
        }

        return IRON_FALSE;
    }

    if (definition_type == IRON_TYPE_MVAR) {
        iron_u32 definition_index;
        iron_u32 reference_index;

        return reference_type == IRON_TYPE_MVAR && IRON_RESULT_OK(iron_sig_read_compressed_u32(definition_reader, &definition_index)) &&
               IRON_RESULT_OK(iron_sig_read_compressed_u32(reference_reader, &reference_index)) && definition_index == reference_index;
    }

    if (definition_type != reference_type && !((definition_type == IRON_TYPE_CLASS || definition_type == IRON_TYPE_VALUETYPE) &&
                                                (reference_type == IRON_TYPE_CLASS || reference_type == IRON_TYPE_VALUETYPE))) {
        return IRON_FALSE;
    }

    switch (definition_type) {
        case IRON_TYPE_CLASS:
        case IRON_TYPE_VALUETYPE: {
            iron_token_t definition_token;
            iron_token_t reference_token;

            return IRON_RESULT_OK(iron_sig_read_type_def_or_ref(definition_reader, &definition_token)) &&
                   IRON_RESULT_OK(iron_sig_read_type_def_or_ref(reference_reader, &reference_token)) &&
                   metadata_type_tokens_equal(definition_metadata, definition_token, reference_metadata, reference_token);
        }
        case IRON_TYPE_VAR:
        case IRON_TYPE_MVAR: {
            iron_u32 definition_index;
            iron_u32 reference_index;

            return IRON_RESULT_OK(iron_sig_read_compressed_u32(definition_reader, &definition_index)) &&
                   IRON_RESULT_OK(iron_sig_read_compressed_u32(reference_reader, &reference_index)) && definition_index == reference_index;
        }
        case IRON_TYPE_PTR:
        case IRON_TYPE_BYREF:
        case IRON_TYPE_SZARRAY:
            return signature_type_matches(definition_metadata, definition_reader, definition_type_context, reference_metadata, reference_reader);
        case IRON_TYPE_GENERICINST: {
            iron_element_type_t definition_kind;
            iron_element_type_t reference_kind;
            iron_token_t definition_token;
            iron_token_t reference_token;
            iron_u32 definition_count;
            iron_u32 reference_count;
            iron_u32 argument_index;

            if (!IRON_RESULT_OK(iron_sig_read_element_type(definition_reader, &definition_kind)) ||
                !IRON_RESULT_OK(iron_sig_read_element_type(reference_reader, &reference_kind)) ||
                definition_kind != reference_kind || (definition_kind != IRON_TYPE_CLASS && definition_kind != IRON_TYPE_VALUETYPE) ||
                !IRON_RESULT_OK(iron_sig_read_type_def_or_ref(definition_reader, &definition_token)) ||
                !IRON_RESULT_OK(iron_sig_read_type_def_or_ref(reference_reader, &reference_token)) ||
                !metadata_type_tokens_equal(definition_metadata, definition_token, reference_metadata, reference_token) ||
                !IRON_RESULT_OK(iron_sig_read_compressed_u32(definition_reader, &definition_count)) ||
                !IRON_RESULT_OK(iron_sig_read_compressed_u32(reference_reader, &reference_count)) || definition_count != reference_count) {
                return IRON_FALSE;
            }

            for (argument_index = 0; argument_index < definition_count; argument_index++) {
                if (!signature_type_matches(definition_metadata, definition_reader, definition_type_context, reference_metadata, reference_reader)) {
                    return IRON_FALSE;
                }
            }
            return IRON_TRUE;
        }
        case IRON_TYPE_ARRAY: {
            iron_u32 definition_rank;
            iron_u32 reference_rank;
            iron_u32 definition_count;
            iron_u32 reference_count;
            iron_u32 index;

            if (!signature_type_matches(definition_metadata, definition_reader, definition_type_context, reference_metadata, reference_reader) ||
                !IRON_RESULT_OK(iron_sig_read_compressed_u32(definition_reader, &definition_rank)) ||
                !IRON_RESULT_OK(iron_sig_read_compressed_u32(reference_reader, &reference_rank)) || definition_rank != reference_rank ||
                !IRON_RESULT_OK(iron_sig_read_compressed_u32(definition_reader, &definition_count)) ||
                !IRON_RESULT_OK(iron_sig_read_compressed_u32(reference_reader, &reference_count)) || definition_count != reference_count) {
                return IRON_FALSE;
            }
            for (index = 0; index < definition_count; index++) {
                iron_u32 definition_size;
                iron_u32 reference_size;

                if (!IRON_RESULT_OK(iron_sig_read_compressed_u32(definition_reader, &definition_size)) ||
                    !IRON_RESULT_OK(iron_sig_read_compressed_u32(reference_reader, &reference_size)) || definition_size != reference_size) {
                    return IRON_FALSE;
                }
            }
            if (!IRON_RESULT_OK(iron_sig_read_compressed_u32(definition_reader, &definition_count)) ||
                !IRON_RESULT_OK(iron_sig_read_compressed_u32(reference_reader, &reference_count)) || definition_count != reference_count) {
                return IRON_FALSE;
            }
            for (index = 0; index < definition_count; index++) {
                iron_i32 definition_bound;
                iron_i32 reference_bound;

                if (!IRON_RESULT_OK(iron_sig_read_compressed_i32(definition_reader, &definition_bound)) ||
                    !IRON_RESULT_OK(iron_sig_read_compressed_i32(reference_reader, &reference_bound)) || definition_bound != reference_bound) {
                    return IRON_FALSE;
                }
            }
            return IRON_TRUE;
        }
        case IRON_TYPE_FNPTR: {
            iron_call_conv_t definition_calling_convention;
            iron_call_conv_t reference_calling_convention;
            iron_u32 definition_generic_count;
            iron_u32 reference_generic_count;
            iron_u32 definition_param_count;
            iron_u32 reference_param_count;
            iron_u32 param_index;

            if (!IRON_RESULT_OK(iron_sig_read_method_header(definition_reader, &definition_calling_convention, &definition_generic_count, &definition_param_count)) ||
                !IRON_RESULT_OK(iron_sig_read_method_header(reference_reader, &reference_calling_convention, &reference_generic_count, &reference_param_count)) ||
                definition_calling_convention != reference_calling_convention || definition_generic_count != reference_generic_count ||
                definition_param_count != reference_param_count ||
                !signature_type_matches(definition_metadata, definition_reader, definition_type_context, reference_metadata, reference_reader)) {
                return IRON_FALSE;
            }
            for (param_index = 0; param_index < definition_param_count; param_index++) {
                if (!signature_type_matches(definition_metadata, definition_reader, definition_type_context, reference_metadata, reference_reader)) {
                    return IRON_FALSE;
                }
            }
            return IRON_TRUE;
        }
        default:
            return IRON_TRUE;
    }
}

static iron_bool method_matches_member_signature(iron_assembly_t *assembly, const iron_member_ref_row_t *member_ref, const iron_runtime_method_t *method)
{
    iron_assembly_t *definition_assembly;
    iron_method_def_row_t definition_row;
    const iron_u8 *definition_signature;
    const iron_u8 *reference_signature;
    iron_u32 definition_signature_size;
    iron_u32 reference_signature_size;
    iron_sig_reader_t definition_reader;
    iron_sig_reader_t reference_reader;
    iron_call_conv_t definition_calling_convention;
    iron_call_conv_t reference_calling_convention;
    iron_u32 definition_generic_count;
    iron_u32 reference_generic_count;
    iron_u32 definition_param_count;
    iron_u32 reference_param_count;
    iron_u32 param_index;
    iron_result_t result;
    iron_signature_type_context_t type_context;
    const iron_signature_type_context_t *type_context_pointer;

    if (!assembly || !member_ref || !method || member_ref->signature == 0 || !method->declaring_type || !method->declaring_type->module) {
        return IRON_FALSE;
    }

    definition_assembly = method->declaring_type->module->assembly;
    result = iron_metadata_read_row(&definition_assembly->metadata, method->token, &definition_row);
    if (!IRON_RESULT_OK(result) || definition_row.signature == 0) {
        return IRON_FALSE;
    }

    if (!IRON_RESULT_OK(iron_metadata_get_blob(&definition_assembly->metadata, definition_row.signature, &definition_signature, &definition_signature_size)) ||
        !IRON_RESULT_OK(iron_metadata_get_blob(&assembly->metadata, member_ref->signature, &reference_signature, &reference_signature_size))) {
        return IRON_FALSE;
    }

    iron_sig_init(&definition_reader, definition_signature, definition_signature_size);
    iron_sig_init(&reference_reader, reference_signature, reference_signature_size);
    type_context_pointer = NULL;

    {
        iron_token_t parent_token;

        parent_token = iron_metadata_decode_coded(&assembly->metadata, IRON_CODED_MEMBER_REF_PARENT, member_ref->class_);
        if (IRON_TOKEN_TABLE(parent_token) == IRON_TABLE_TYPE_SPEC) {
            iron_type_spec_row_t type_spec;

            if (IRON_RESULT_OK(iron_metadata_read_row(&assembly->metadata, parent_token, &type_spec)) &&
                IRON_RESULT_OK(iron_metadata_get_blob(&assembly->metadata, type_spec.signature, &type_context.signature, &type_context.signature_size))) {
                type_context.metadata = &assembly->metadata;
                type_context_pointer = &type_context;
            }
        }
    }

    if (!IRON_RESULT_OK(iron_sig_read_method_header(&definition_reader, &definition_calling_convention, &definition_generic_count, &definition_param_count)) ||
        !IRON_RESULT_OK(iron_sig_read_method_header(&reference_reader, &reference_calling_convention, &reference_generic_count, &reference_param_count)) ||
        definition_generic_count != reference_generic_count || definition_param_count != reference_param_count ||
        (definition_calling_convention & (IRON_CALL_HAS_THIS | 0x0F)) != (reference_calling_convention & (IRON_CALL_HAS_THIS | 0x0F))) {
        return IRON_FALSE;
    }

    if (!signature_type_matches(&definition_assembly->metadata, &definition_reader, type_context_pointer, &assembly->metadata, &reference_reader)) {
        return IRON_FALSE;
    }

    for (param_index = 0; param_index < definition_param_count; param_index++) {
        if (!signature_type_matches(&definition_assembly->metadata, &definition_reader, type_context_pointer, &assembly->metadata, &reference_reader)) {
            return IRON_FALSE;
        }
    }

    return definition_reader.pos == definition_reader.size && reference_reader.pos == reference_reader.size;
}

static iron_runtime_method_t *find_member_reference_method(iron_assembly_t *assembly,
                                                          const iron_member_ref_row_t *member_ref,
                                                          iron_runtime_type_t *type,
                                                          const char *name)
{
    iron_runtime_type_t *current;

    if (!name) {
        return NULL;
    }

    for (current = type; current; current = current->base_type) {
        iron_u32 method_index;

        for (method_index = 0; method_index < current->method_count; method_index++) {
            iron_runtime_method_t *candidate;

            candidate = current->methods[method_index];
            if (candidate && candidate->name && strcmp(candidate->name, name) == 0 && method_matches_member_signature(assembly, member_ref, candidate)) {
                return candidate;
            }
        }
    }

    return NULL;
}

iron_runtime_method_t *iron_resolve_method_token(iron_assembly_t *assembly, iron_token_t token)
{
    iron_u32 table_id;
    iron_u32 row_index;
    iron_u32 method_count;
    iron_runtime_method_t *method;
    iron_method_def_row_t row;
    iron_result_t result;

    if (!assembly) return NULL;

    /* Extract table ID and row index from token */
    table_id = (token >> 24) & 0xFF;
    row_index = token & 0x00FFFFFF;

    if (row_index == 0) return NULL;
    row_index--; /* Convert to 0-based index */

    /* Table 0x06 = MethodDef */
    if (table_id == IRON_TABLE_MEMBER_REF) {
        /* Handle MemberRef (0x0A) for cross-assembly references */
        iron_member_ref_row_t member_ref;
        iron_result_t res;
        const char *method_name;
        iron_u32 class_token;
        iron_u32 class_table;

        res = iron_metadata_read_row(&assembly->metadata, token, &member_ref);
        if (!IRON_RESULT_OK(res)) {
            return NULL;
        }

        method_name = iron_metadata_get_string(&assembly->metadata, member_ref.name);

        /* Decode the class (MemberRefParent coded index) */
        class_token = iron_metadata_decode_coded(&assembly->metadata,
                                                  IRON_CODED_MEMBER_REF_PARENT,
                                                  member_ref.class_);
        class_table = (class_token >> 24) & 0xFF;

        IRON_DEBUG_META("MemberRef: method=%s class_=0x%X class_token=0x%08X class_table=0x%02X",
                       method_name, member_ref.class_, class_token, class_table);

        if (class_table == IRON_TABLE_TYPE_REF) {
            iron_runtime_type_t *type;

            type = iron_type_resolve_token(assembly->module, class_token);
            return find_member_reference_method(assembly, &member_ref, type, method_name);
        }
        else if (class_table == IRON_TABLE_TYPE_DEF) {
            /* Reference to type in same assembly */
            iron_type_def_row_t type_def;
            res = iron_metadata_read_row(&assembly->metadata, class_token, &type_def);
            if (IRON_RESULT_OK(res)) {
                /* Find method in the type's method list */
                iron_u32 method_start = type_def.method_list;
                iron_u32 method_end;
                iron_u32 type_count = iron_metadata_table_rows(&assembly->metadata, IRON_TABLE_TYPE_DEF);
                iron_u32 type_row = (class_token & 0x00FFFFFF);
                iron_u32 m;
                
                if (type_row < type_count) {
                    iron_type_def_row_t next_type;
                    iron_token_t next_token = IRON_MAKE_TOKEN(IRON_TABLE_TYPE_DEF, type_row + 1);
                    if (IRON_RESULT_OK(iron_metadata_read_row(&assembly->metadata, next_token, &next_type))) {
                        method_end = next_type.method_list;
                    } else {
                        method_end = iron_metadata_table_rows(&assembly->metadata, IRON_TABLE_METHOD_DEF) + 1;
                    }
                } else {
                    method_end = iron_metadata_table_rows(&assembly->metadata, IRON_TABLE_METHOD_DEF) + 1;
                }
                
                for (m = method_start; m < method_end; m++) {
                    iron_token_t method_token = IRON_MAKE_TOKEN(IRON_TABLE_METHOD_DEF, m);
                    iron_runtime_method_t *meth = iron_resolve_method_token(assembly, method_token);
                    if (meth && meth->name && strcmp(meth->name, method_name) == 0 && method_matches_member_signature(assembly, &member_ref, meth)) {
                        return meth;
                    }
                }
            }
        }
        else if (class_table == IRON_TABLE_TYPE_SPEC) {
            iron_type_spec_row_t type_spec;
            const iron_u8 *signature;
            iron_u32 signature_size;
            iron_sig_reader_t reader;
            iron_sig_reader_t validation_reader;
            iron_element_type_t element_type;
            iron_element_type_t class_or_value;
            iron_token_t definition_token;
            iron_runtime_type_t *type;

            if (!IRON_RESULT_OK(iron_metadata_read_row(&assembly->metadata, class_token, &type_spec)) ||
                !IRON_RESULT_OK(iron_metadata_get_blob(&assembly->metadata, type_spec.signature, &signature, &signature_size))) {
                return NULL;
            }

            iron_sig_init(&reader, signature, signature_size);
            validation_reader = reader;
            if (!signature_type_skip(&validation_reader) || validation_reader.pos != validation_reader.size ||
                !IRON_RESULT_OK(read_effective_element_type(&reader, &element_type)) || element_type != IRON_TYPE_GENERICINST ||
                !IRON_RESULT_OK(iron_sig_read_element_type(&reader, &class_or_value)) ||
                (class_or_value != IRON_TYPE_CLASS && class_or_value != IRON_TYPE_VALUETYPE) ||
                !IRON_RESULT_OK(iron_sig_read_type_def_or_ref(&reader, &definition_token))) {
                return NULL;
            }

            /* Preserve the declaring-type chain for nested TypeRefs, and use the
             * shared signature reader for all compressed token widths. */
            type = iron_type_resolve_token(assembly->module, definition_token);
            return find_member_reference_method(assembly, &member_ref, type, method_name);
        }

        return NULL;
    }
    
    if (table_id != IRON_TABLE_METHOD_DEF) {
        return NULL;
    }
    
    /* Get method count from metadata */
    method_count = iron_metadata_table_rows(&assembly->metadata, IRON_TABLE_METHOD_DEF);
    if (method_count == 0 || row_index >= method_count) {
        return NULL;
    }
    
    /* Allocate methods array if needed */
    if (!assembly->methods) {
        assembly->methods = (iron_runtime_method_t *)iron_alloc(
            assembly->domain->allocator,
            method_count * sizeof(iron_runtime_method_t));
        if (!assembly->methods) return NULL;
        memset(assembly->methods, 0, method_count * sizeof(iron_runtime_method_t));
        assembly->method_count = method_count;
    }
    
    method = &assembly->methods[row_index];
    
    /* Load method metadata if not already loaded */
    if (!method->name) {
        iron_u32 type_count;
        iron_u32 method_row = row_index + 1; /* 1-based */

        /* Read the MethodDef row */
        result = iron_metadata_read_row(&assembly->metadata, token, &row);
        if (!IRON_RESULT_OK(result)) {
            return NULL;
        }
        
        method->name = iron_metadata_get_string(&assembly->metadata, row.name);
        method->token = token;
        method->attrs = row.flags;
        method->impl_attrs = row.impl_flags;
        
        /* Find declaring type by checking method_list ranges in TypeDef table */
        type_count = assembly->metadata.tables[IRON_TABLE_TYPE_DEF].row_count;
        {
            iron_u32 i;
            for (i = 1; i <= type_count; i++) {
                iron_type_def_row_t type_def;
                iron_u32 type_token = (IRON_TABLE_TYPE_DEF << 24) | i;
                iron_u32 method_start, method_end;
                
                if (!IRON_RESULT_OK(iron_metadata_read_row(&assembly->metadata, type_token, &type_def))) {
                    continue;
                }
                
                method_start = type_def.method_list;
                
                if (i < type_count) {
                    iron_type_def_row_t next_type;
                    iron_u32 next_token = (IRON_TABLE_TYPE_DEF << 24) | (i + 1);
                    if (IRON_RESULT_OK(iron_metadata_read_row(&assembly->metadata, next_token, &next_type))) {
                        method_end = next_type.method_list;
                    } else {
                        method_end = method_count + 1;
                    }
                } else {
                    method_end = method_count + 1;
                }
                
                if (method_row >= method_start && method_row < method_end) {
                    /* Found the declaring type - resolve it */
                    method->declaring_type = iron_type_resolve_token(assembly->module, type_token);
                    break;
                }
            }
        }
        
        if (!method->declaring_type) {
            method->name = NULL;
            return NULL;
        }

        result = load_generic_parameters(assembly, method->token, &method->generic_params, &method->generic_param_count);
        if (!IRON_RESULT_OK(result)) {
            method->name = NULL;
            return NULL;
        }
        {
            iron_u32 parameter_index;

            for (parameter_index = 0; parameter_index < method->generic_param_count; parameter_index++) {
                method->generic_params[parameter_index]->declaring_method = method;
            }
        }
        method->is_generic_definition = method->generic_param_count != 0;

        if (row.signature > 0) {
            const iron_u8 *signature;
            iron_u32 signature_size;
            iron_sig_reader_t reader;
            iron_u32 signature_generic_count;

            result = iron_metadata_get_blob(&assembly->metadata, row.signature, &signature, &signature_size);
            if (!IRON_RESULT_OK(result)) {
                method->name = NULL;
                return NULL;
            }

            iron_sig_init(&reader, signature, signature_size);
            result = iron_sig_read_method_header(&reader, &method->calling_convention, &signature_generic_count, &method->param_count);
            if (!IRON_RESULT_OK(result) || signature_generic_count != method->generic_param_count) {
                method->name = NULL;
                return NULL;
            }
        }
        
        /* Check if this is an internal call */
        if ((row.impl_flags & 0x1000) != 0) { /* InternalCall flag */
            method->is_internal_call = IRON_TRUE;
        }
        
        /* Load method body if it has IL code */
        if (row.rva != 0 && !method->is_internal_call) {
            /* Parse method body from RVA */
            method->body = (iron_method_body_t *)iron_alloc(
                assembly->domain->allocator, sizeof(iron_method_body_t));
            if (method->body) {
                iron_result_t body_res = iron_parse_method_body(
                    &assembly->image, row.rva, method->body, assembly->domain->allocator);
                if (!IRON_RESULT_OK(body_res)) {
                    iron_free(assembly->domain->allocator, method->body, sizeof(iron_method_body_t));
                    method->body = NULL;
                } else if (!method->is_generic_definition &&
                           (!method->declaring_type || !iron_type_contains_generic_parameters(method->declaring_type))) {
                    body_res = load_method_local_signature(assembly, method);
                    if (!IRON_RESULT_OK(body_res)) {
                        iron_free(assembly->domain->allocator, method->body, sizeof(iron_method_body_t));
                        method->body = NULL;
                    }
                }
            }
        }
    }
    
    return method;
}

/* ============================================================================
 * Generic Instantiation
 * ============================================================================ */

iron_u32 iron_generic_inst_hash(const iron_generic_inst_t *inst)
{
    iron_u32 hash = 0;
    iron_u32 i;
    
    if (!inst) return 0;
    
    for (i = 0; i < inst->arg_count; i++) {
        hash = hash * 31 + (iron_u32)(iron_size)inst->args[i];
    }
    
    return hash;
}

iron_bool iron_generic_inst_equals(const iron_generic_inst_t *a,
                                   const iron_generic_inst_t *b)
{
    iron_u32 i;
    
    if (!a || !b) return a == b;
    if (a->arg_count != b->arg_count) return IRON_FALSE;
    
    for (i = 0; i < a->arg_count; i++) {
        if (a->args[i] != b->args[i]) return IRON_FALSE;
    }
    
    return IRON_TRUE;
}

/* ============================================================================
 * Type Resolution
 * ============================================================================ */

static iron_runtime_type_t *resolve_signature_type(iron_assembly_t *assembly,
                                                   iron_sig_reader_t *reader,
                                                   iron_runtime_type_t **type_arguments,
                                                   iron_u32 type_argument_count,
                                                   iron_runtime_type_t **method_arguments,
                                                   iron_u32 method_argument_count,
                                                   iron_element_type_t *out_element_type,
                                                   iron_u32 *out_generic_param_index)
{
    iron_element_type_t element_type;
    iron_result_t result;
    const char *type_name;

    result = iron_sig_read_element_type(reader, &element_type);
    if (!IRON_RESULT_OK(result)) {
        return NULL;
    }

    while (element_type == IRON_TYPE_CMOD_OPT || element_type == IRON_TYPE_CMOD_REQD || element_type == IRON_TYPE_PINNED) {
        iron_token_t modifier_token;

        if (element_type != IRON_TYPE_PINNED) {
            result = iron_sig_read_type_def_or_ref(reader, &modifier_token);
            if (!IRON_RESULT_OK(result)) {
                return NULL;
            }
        }

        result = iron_sig_read_element_type(reader, &element_type);
        if (!IRON_RESULT_OK(result)) {
            return NULL;
        }
    }

    if (out_element_type) {
        *out_element_type = element_type;
    }
    if (out_generic_param_index) {
        *out_generic_param_index = (iron_u32)-1;
    }

    type_name = iron_element_type_managed_name(element_type);
    if (type_name) {
        return iron_domain_find_type(assembly->domain, type_name);
    }

    if (element_type == IRON_TYPE_CLASS || element_type == IRON_TYPE_VALUETYPE) {
        iron_token_t type_token;

        result = iron_sig_read_type_def_or_ref(reader, &type_token);
        if (!IRON_RESULT_OK(result)) {
            return NULL;
        }

        return iron_type_resolve_token(assembly->module, type_token);
    }

    if (element_type == IRON_TYPE_SZARRAY) {
        iron_runtime_type_t *array_element;

        array_element = resolve_signature_type(assembly, reader, type_arguments, type_argument_count, method_arguments, method_argument_count, NULL, NULL);
        return array_element ? iron_type_make_array(assembly->domain, array_element, 1) : NULL;
    }

    if (element_type == IRON_TYPE_ARRAY) {
        iron_runtime_type_t *array_element;
        iron_u32 rank;
        iron_u32 size_count;
        iron_u32 lower_bound_count;
        iron_u32 index;

        array_element = resolve_signature_type(assembly, reader, type_arguments, type_argument_count, method_arguments, method_argument_count, NULL, NULL);
        if (!array_element || !IRON_RESULT_OK(iron_sig_read_compressed_u32(reader, &rank)) || rank == 0 || rank > 32 ||
            !IRON_RESULT_OK(iron_sig_read_compressed_u32(reader, &size_count)) || size_count > rank) {
            return NULL;
        }

        for (index = 0; index < size_count; index++) {
            iron_u32 size;

            if (!IRON_RESULT_OK(iron_sig_read_compressed_u32(reader, &size))) {
                return NULL;
            }
        }

        if (!IRON_RESULT_OK(iron_sig_read_compressed_u32(reader, &lower_bound_count)) || lower_bound_count > rank) {
            return NULL;
        }
        for (index = 0; index < lower_bound_count; index++) {
            iron_i32 lower_bound;

            if (!IRON_RESULT_OK(iron_sig_read_compressed_i32(reader, &lower_bound))) {
                return NULL;
            }
        }

        return iron_type_make_mdarray(assembly->domain, array_element, rank);
    }

    if (element_type == IRON_TYPE_PTR || element_type == IRON_TYPE_BYREF) {
        iron_runtime_type_t *pointed_type;

        pointed_type = resolve_signature_type(assembly, reader, type_arguments, type_argument_count, method_arguments, method_argument_count, NULL, NULL);
        if (!pointed_type) {
            return NULL;
        }

        return element_type == IRON_TYPE_PTR ? iron_type_make_pointer(assembly->domain, pointed_type) : iron_type_make_byref(assembly->domain, pointed_type);
    }

    if (element_type == IRON_TYPE_FNPTR) {
        iron_call_conv_t calling_convention;
        iron_u32 generic_parameter_count;
        iron_u32 parameter_count;
        iron_u32 parameter_index;

        result = iron_sig_read_method_header(reader, &calling_convention, &generic_parameter_count, &parameter_count);
        if (!IRON_RESULT_OK(result) || !resolve_signature_type(assembly, reader, type_arguments, type_argument_count, method_arguments, method_argument_count, NULL, NULL)) {
            return NULL;
        }
        for (parameter_index = 0; parameter_index < parameter_count; parameter_index++) {
            if (!resolve_signature_type(assembly, reader, type_arguments, type_argument_count, method_arguments, method_argument_count, NULL, NULL)) {
                return NULL;
            }
        }

        (void)calling_convention;
        (void)generic_parameter_count;
        return iron_domain_find_type(assembly->domain, "System.IntPtr");
    }

    if (element_type == IRON_TYPE_VAR || element_type == IRON_TYPE_MVAR) {
        iron_u32 generic_index;

        result = iron_sig_read_compressed_u32(reader, &generic_index);
        if (IRON_RESULT_OK(result) && out_generic_param_index) {
            *out_generic_param_index = generic_index;
        }
        if (IRON_RESULT_OK(result) && element_type == IRON_TYPE_VAR && generic_index < type_argument_count) {
            return type_arguments[generic_index];
        }
        if (IRON_RESULT_OK(result) && element_type == IRON_TYPE_MVAR && generic_index < method_argument_count) {
            return method_arguments[generic_index];
        }
        return NULL;
    }

    if (element_type == IRON_TYPE_GENERICINST) {
        iron_element_type_t definition_kind;
        iron_token_t definition_token;
        iron_runtime_type_t *definition;
        iron_runtime_type_t **generic_arguments;
        iron_runtime_type_t *instance;
        iron_u32 generic_argument_count;
        iron_u32 generic_index;

        result = iron_sig_read_element_type(reader, &definition_kind);
        if (!IRON_RESULT_OK(result) || (definition_kind != IRON_TYPE_CLASS && definition_kind != IRON_TYPE_VALUETYPE) ||
            !IRON_RESULT_OK(iron_sig_read_type_def_or_ref(reader, &definition_token))) {
            return NULL;
        }

        definition = iron_type_resolve_token(assembly->module, definition_token);
        result = iron_sig_read_compressed_u32(reader, &generic_argument_count);
        if (!definition || !IRON_RESULT_OK(result) || generic_argument_count == 0) {
            return NULL;
        }

        generic_arguments = (iron_runtime_type_t **)iron_alloc(assembly->allocator, generic_argument_count * sizeof(iron_runtime_type_t *));
        if (!generic_arguments) {
            return NULL;
        }

        for (generic_index = 0; generic_index < generic_argument_count; generic_index++) {
            generic_arguments[generic_index] = resolve_signature_type(assembly, reader, type_arguments, type_argument_count, method_arguments, method_argument_count, NULL, NULL);
            if (!generic_arguments[generic_index]) {
                iron_free(assembly->allocator, generic_arguments, generic_argument_count * sizeof(iron_runtime_type_t *));
                return NULL;
            }
        }

        instance = iron_type_make_generic(assembly->domain, definition, generic_arguments, generic_argument_count);
        iron_free(assembly->allocator, generic_arguments, generic_argument_count * sizeof(iron_runtime_type_t *));
        return instance;
    }

    return NULL;
}

static void load_parameter_default(iron_assembly_t *assembly, iron_runtime_param_t *parameter)
{
    iron_u32 constant_count;
    iron_u32 constant_index;

    if (!assembly || !parameter || parameter->token == 0 || (parameter->attrs & IRON_PARAM_HAS_DEFAULT) == 0) {
        return;
    }

    constant_count = iron_metadata_table_rows(&assembly->metadata, IRON_TABLE_CONSTANT);
    for (constant_index = 1; constant_index <= constant_count; constant_index++) {
        iron_constant_row_t row;
        iron_token_t parent_token;
        const iron_u8 *data;
        iron_u32 data_size;

        if (!IRON_RESULT_OK(iron_metadata_read_row(&assembly->metadata, IRON_MAKE_TOKEN(IRON_TABLE_CONSTANT, constant_index), &row))) {
            continue;
        }

        parent_token = iron_metadata_decode_coded(&assembly->metadata, IRON_CODED_HAS_CONSTANT, row.parent);
        if (parent_token != parameter->token || !IRON_RESULT_OK(iron_metadata_get_blob(&assembly->metadata, row.value, &data, &data_size))) {
            continue;
        }

        parameter->has_default = IRON_TRUE;
        parameter->default_type = (iron_element_type_t)row.type;
        parameter->default_data = data;
        parameter->default_data_size = data_size;

        switch (parameter->default_type) {
            case IRON_TYPE_BOOLEAN:
            case IRON_TYPE_U1:
                if (data_size >= 1) {
                    parameter->default_value.i32 = (iron_i32)data[0];
                }
                break;

            case IRON_TYPE_I1:
                if (data_size >= 1) {
                    parameter->default_value.i32 = (iron_i32)(iron_i8)data[0];
                }
                break;

            case IRON_TYPE_CHAR:
            case IRON_TYPE_U2:
                if (data_size >= 2) {
                    parameter->default_value.i32 = (iron_i32)iron_read_u16_le(data);
                }
                break;

            case IRON_TYPE_I2:
                if (data_size >= 2) {
                    parameter->default_value.i32 = (iron_i32)(iron_i16)iron_read_u16_le(data);
                }
                break;

            case IRON_TYPE_I4:
            case IRON_TYPE_U4:
                if (data_size >= 4) {
                    parameter->default_value.i32 = iron_read_i32_le(data);
                }
                break;

            case IRON_TYPE_I8:
            case IRON_TYPE_U8:
                if (data_size >= 8) {
                    parameter->default_value.i64 = iron_read_i64_le(data);
                }
                break;

            case IRON_TYPE_R4:
                if (data_size >= 4) {
                    parameter->default_value.f32 = iron_read_f32_le(data);
                }
                break;

            case IRON_TYPE_R8:
                if (data_size >= 8) {
                    parameter->default_value.f64 = iron_read_f64_le(data);
                }
                break;

            default:
                break;
        }
        return;
    }
}

static iron_result_t load_method_parameter_metadata(iron_assembly_t *assembly,
                                                    iron_runtime_method_t *method,
                                                    const iron_method_def_row_t *method_row)
{
    iron_u32 method_index;
    iron_u32 parameter_start;
    iron_u32 parameter_end;
    iron_u32 row_index;

    if (!assembly || !method || !method_row) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Invalid method parameter metadata arguments");
    }

    parameter_start = method_row->param_list;
    method_index = IRON_TOKEN_INDEX(method->token);
    if (method_index < iron_metadata_table_rows(&assembly->metadata, IRON_TABLE_METHOD_DEF)) {
        iron_method_def_row_t next_method;

        if (!IRON_RESULT_OK(iron_metadata_read_row(&assembly->metadata, IRON_MAKE_TOKEN(IRON_TABLE_METHOD_DEF, method_index + 1), &next_method))) {
            return IRON_ERROR(IRON_ERR_INVALID_METADATA, "Cannot read the next MethodDef parameter range");
        }
        parameter_end = next_method.param_list;
    } else {
        parameter_end = iron_metadata_table_rows(&assembly->metadata, IRON_TABLE_PARAM) + 1;
    }

    for (row_index = parameter_start; row_index < parameter_end; row_index++) {
        iron_param_row_t row;
        iron_runtime_param_t *parameter;

        if (!IRON_RESULT_OK(iron_metadata_read_row(&assembly->metadata, IRON_MAKE_TOKEN(IRON_TABLE_PARAM, row_index), &row))) {
            return IRON_ERROR(IRON_ERR_INVALID_METADATA, "Cannot read a Param row");
        }
        if (row.sequence == 0) {
            continue;
        }
        if (row.sequence > method->param_count || method->params[row.sequence - 1]->token != 0) {
            return IRON_ERROR(IRON_ERR_INVALID_METADATA, "Method parameter sequence is outside its signature or duplicated");
        }

        parameter = method->params[row.sequence - 1];
        parameter->token = IRON_MAKE_TOKEN(IRON_TABLE_PARAM, row_index);
        parameter->name = iron_metadata_get_string(&assembly->metadata, row.name);
        parameter->attrs = row.flags;
        parameter->sequence = row.sequence;
        load_parameter_default(assembly, parameter);
    }

    return IRON_SUCCESS;
}

static iron_result_t load_method_signature(iron_assembly_t *assembly, iron_runtime_method_t *method, const iron_method_def_row_t *row)
{
    const iron_u8 *signature;
    iron_u32 signature_size;
    iron_sig_reader_t reader;
    iron_runtime_type_t **type_arguments;
    iron_u32 type_argument_count;
    iron_u32 signature_generic_count;
    iron_u32 signature_parameter_count;
    iron_u32 parameter_index;
    iron_result_t result;

    if (!assembly || !method || !row) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Invalid method signature arguments");
    }

    result = iron_metadata_get_blob(&assembly->metadata, row->signature, &signature, &signature_size);
    if (!IRON_RESULT_OK(result)) {
        return result;
    }

    iron_sig_init(&reader, signature, signature_size);
    result = iron_sig_read_method_header(&reader, &method->calling_convention, &signature_generic_count, &signature_parameter_count);
    if (!IRON_RESULT_OK(result)) {
        return result;
    }
    if (signature_generic_count != (method->is_generic_instance ? method->generic_arg_count : method->generic_param_count)) {
        return IRON_ERROR(IRON_ERR_INVALID_SIGNATURE, "Method signature generic arity does not match GenericParam metadata");
    }

    method->param_count = signature_parameter_count;
    type_arguments = method->declaring_type && method->declaring_type->is_generic_instance ? method->declaring_type->generic_args :
                     method->declaring_type ? method->declaring_type->generic_params : NULL;
    type_argument_count = method->declaring_type ? method->declaring_type->generic_param_count : 0;
    if (method->declaring_type && method->declaring_type->is_generic_instance) {
        type_argument_count = method->declaring_type->generic_arg_count;
    }

    method->return_type = resolve_signature_type(assembly,
                                                 &reader,
                                                 type_arguments,
                                                 type_argument_count,
                                                 method->is_generic_instance ? method->generic_args : method->generic_params,
                                                 method->is_generic_instance ? method->generic_arg_count : method->generic_param_count,
                                                 NULL,
                                                 NULL);
    if (!method->return_type) {
        return IRON_ERROR(IRON_ERR_INVALID_SIGNATURE, "Cannot resolve a method return type");
    }

    if (method->param_count != 0) {
        method->params = (iron_runtime_param_t **)iron_alloc(assembly->allocator, (iron_size)method->param_count * sizeof(iron_runtime_param_t *));
        if (!method->params) {
            return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate method parameter descriptors");
        }
        memset(method->params, 0, (iron_size)method->param_count * sizeof(iron_runtime_param_t *));
    }

    for (parameter_index = 0; parameter_index < method->param_count; parameter_index++) {
        iron_runtime_param_t *parameter;

        if (reader.pos < reader.size && reader.data[reader.pos] == IRON_TYPE_SENTINEL) {
            reader.pos++;
        }

        parameter = (iron_runtime_param_t *)iron_alloc(assembly->allocator, sizeof(iron_runtime_param_t));
        if (!parameter) {
            free_method_parameters(assembly->allocator, method->params, method->param_count);
            method->params = NULL;
            return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate a method parameter descriptor");
        }

        memset(parameter, 0, sizeof(*parameter));
        parameter->method = method;
        parameter->sequence = (iron_u16)(parameter_index + 1);
        parameter->param_type = resolve_signature_type(assembly,
                                                       &reader,
                                                       type_arguments,
                                                       type_argument_count,
                                                       method->is_generic_instance ? method->generic_args : method->generic_params,
                                                       method->is_generic_instance ? method->generic_arg_count : method->generic_param_count,
                                                       NULL,
                                                       NULL);
        if (!parameter->param_type) {
            iron_free(assembly->allocator, parameter, sizeof(*parameter));
            free_method_parameters(assembly->allocator, method->params, method->param_count);
            method->params = NULL;
            return IRON_ERROR(IRON_ERR_INVALID_SIGNATURE, "Cannot resolve a method parameter type");
        }
        method->params[parameter_index] = parameter;
    }

    if (reader.pos != reader.size) {
        free_method_parameters(assembly->allocator, method->params, method->param_count);
        method->params = NULL;
        return IRON_ERROR(IRON_ERR_INVALID_SIGNATURE, "Method signature has trailing data");
    }

    result = load_method_parameter_metadata(assembly, method, row);
    if (!IRON_RESULT_OK(result)) {
        free_method_parameters(assembly->allocator, method->params, method->param_count);
        method->params = NULL;
        return result;
    }

    return IRON_SUCCESS;
}

iron_result_t iron_property_load_signature(iron_runtime_property_t *property)
{
    iron_assembly_t *assembly;
    iron_property_row_t row;
    const iron_u8 *signature;
    iron_u32 signature_size;
    iron_sig_reader_t reader;
    iron_call_conv_t calling_convention;
    iron_runtime_type_t **type_arguments;
    iron_u32 type_argument_count;
    iron_u32 generic_parameter_count;
    iron_u32 parameter_count;
    iron_u32 parameter_index;
    iron_result_t result;

    if (!property) {
        return IRON_ERROR(IRON_ERR_NULL_POINTER, "Null property");
    }
    if (property->signature_loaded) {
        return IRON_SUCCESS;
    }
    if (property->signature_loading) {
        return IRON_ERROR(IRON_ERR_INVALID_STATE, "Recursive property signature resolution");
    }
    if (!property->declaring_type || !property->declaring_type->module || !property->declaring_type->module->assembly) {
        return IRON_ERROR(IRON_ERR_INVALID_STATE, "Property has no metadata context");
    }

    assembly = property->declaring_type->module->assembly;
    result = iron_metadata_read_row(&assembly->metadata, property->token, &row);
    if (!IRON_RESULT_OK(result) || !IRON_RESULT_OK(iron_metadata_get_blob(&assembly->metadata, row.type, &signature, &signature_size))) {
        return !IRON_RESULT_OK(result) ? result : IRON_ERROR(IRON_ERR_INVALID_SIGNATURE, "Cannot read a property signature");
    }

    property->signature_loading = IRON_TRUE;
    iron_sig_init(&reader, signature, signature_size);
    result = iron_sig_read_method_header(&reader, &calling_convention, &generic_parameter_count, &parameter_count);
    if (!IRON_RESULT_OK(result) || (calling_convention & 0x0F) != IRON_CALL_PROPERTY || generic_parameter_count != 0) {
        property->signature_loading = IRON_FALSE;
        return IRON_ERROR(IRON_ERR_INVALID_SIGNATURE, "Invalid property signature header");
    }

    type_arguments = property->declaring_type->is_generic_instance ? property->declaring_type->generic_args : property->declaring_type->generic_params;
    type_argument_count = property->declaring_type->is_generic_instance ? property->declaring_type->generic_arg_count : property->declaring_type->generic_param_count;
    property->property_type = resolve_signature_type(assembly, &reader, type_arguments, type_argument_count, NULL, 0, NULL, NULL);
    if (!property->property_type) {
        property->signature_loading = IRON_FALSE;
        return IRON_ERROR(IRON_ERR_INVALID_SIGNATURE, "Cannot resolve a property type");
    }

    property->index_param_count = parameter_count;
    if (parameter_count != 0) {
        property->index_params = (iron_runtime_param_t **)iron_alloc(assembly->allocator, (iron_size)parameter_count * sizeof(iron_runtime_param_t *));
        if (!property->index_params) {
            property->signature_loading = IRON_FALSE;
            return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate property index parameters");
        }
        memset(property->index_params, 0, (iron_size)parameter_count * sizeof(iron_runtime_param_t *));
    }

    for (parameter_index = 0; parameter_index < parameter_count; parameter_index++) {
        iron_runtime_param_t *parameter;

        parameter = (iron_runtime_param_t *)iron_alloc(assembly->allocator, sizeof(iron_runtime_param_t));
        if (!parameter) {
            free_property_parameters(assembly->allocator, property->index_params, parameter_count);
            property->index_params = NULL;
            property->index_param_count = 0;
            property->signature_loading = IRON_FALSE;
            return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate a property index parameter");
        }

        memset(parameter, 0, sizeof(*parameter));
        parameter->sequence = (iron_u16)(parameter_index + 1);
        parameter->param_type = resolve_signature_type(assembly, &reader, type_arguments, type_argument_count, NULL, 0, NULL, NULL);
        if (!parameter->param_type) {
            iron_free(assembly->allocator, parameter, sizeof(*parameter));
            free_property_parameters(assembly->allocator, property->index_params, parameter_count);
            property->index_params = NULL;
            property->index_param_count = 0;
            property->signature_loading = IRON_FALSE;
            return IRON_ERROR(IRON_ERR_INVALID_SIGNATURE, "Cannot resolve a property index parameter type");
        }
        property->index_params[parameter_index] = parameter;
    }

    if (reader.pos != reader.size) {
        free_property_parameters(assembly->allocator, property->index_params, parameter_count);
        property->index_params = NULL;
        property->index_param_count = 0;
        property->signature_loading = IRON_FALSE;
        return IRON_ERROR(IRON_ERR_INVALID_SIGNATURE, "Property signature has trailing data");
    }

    property->signature_loading = IRON_FALSE;
    property->signature_loaded = IRON_TRUE;
    return IRON_SUCCESS;
}

static iron_result_t load_method_local_signature(iron_assembly_t *assembly, iron_runtime_method_t *method)
{
    iron_standalone_sig_row_t signature_row;
    const iron_u8 *signature;
    iron_u32 signature_size;
    iron_sig_reader_t reader;
    iron_runtime_type_t **locals;
    iron_runtime_type_t **type_arguments;
    iron_u32 type_argument_count;
    iron_u32 local_count;
    iron_u32 local_index;
    iron_result_t result;

    if (!assembly || !method || !method->body || method->body->local_var_sig_token == 0) {
        return IRON_SUCCESS;
    }
    if (IRON_TOKEN_TABLE(method->body->local_var_sig_token) != IRON_TABLE_STANDALONE_SIG) {
        return IRON_ERROR(IRON_ERR_INVALID_SIGNATURE, "Method local signature token does not reference StandAloneSig");
    }

    result = iron_metadata_read_row(&assembly->metadata, method->body->local_var_sig_token, &signature_row);
    if (!IRON_RESULT_OK(result)) {
        return result;
    }
    result = iron_metadata_get_blob(&assembly->metadata, signature_row.signature, &signature, &signature_size);
    if (!IRON_RESULT_OK(result)) {
        return result;
    }
    if (signature_size == 0 || signature[0] != IRON_CALL_LOCAL_SIG) {
        return IRON_ERROR(IRON_ERR_INVALID_SIGNATURE, "Invalid method local signature header");
    }

    iron_sig_init(&reader, signature, signature_size);
    reader.pos = 1;
    result = iron_sig_read_compressed_u32(&reader, &local_count);
    if (!IRON_RESULT_OK(result)) {
        return result;
    }
    if ((iron_size)local_count > (iron_size)-1 / sizeof(iron_runtime_type_t *)) {
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Method local signature is too large");
    }

    locals = NULL;
    if (local_count != 0) {
        locals = (iron_runtime_type_t **)iron_alloc(assembly->allocator, (iron_size)local_count * sizeof(iron_runtime_type_t *));
        if (!locals) {
            return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate method local types");
        }
        memset(locals, 0, (iron_size)local_count * sizeof(iron_runtime_type_t *));
    }

    type_arguments = method->declaring_type ? method->declaring_type->generic_args : NULL;
    type_argument_count = method->declaring_type ? method->declaring_type->generic_arg_count : 0;
    for (local_index = 0; local_index < local_count; local_index++) {
        iron_size signature_offset;

        signature_offset = reader.pos;
        locals[local_index] = resolve_signature_type(assembly, &reader, type_arguments, type_argument_count, method->generic_args, method->generic_arg_count, NULL, NULL);
        if (!locals[local_index]) {
            IRON_ERROR_META("Cannot resolve local %u of %s::%s at signature offset %zu (element 0x%02X)",
                            local_index,
                            method->declaring_type && method->declaring_type->full_name ? method->declaring_type->full_name : "<unknown type>",
                            method->name ? method->name : "<unknown method>",
                            signature_offset,
                            signature_offset < signature_size ? signature[signature_offset] : 0);
            iron_free(assembly->allocator, locals, (iron_size)local_count * sizeof(iron_runtime_type_t *));
            return IRON_ERROR(IRON_ERR_INVALID_SIGNATURE, "Cannot resolve a method local type");
        }
    }
    if (reader.pos != reader.size) {
        iron_free(assembly->allocator, locals, (iron_size)local_count * sizeof(iron_runtime_type_t *));
        return IRON_ERROR(IRON_ERR_INVALID_SIGNATURE, "Method local signature has trailing data");
    }

    if (method->locals) {
        iron_free(assembly->allocator, method->locals, (iron_size)method->local_count * sizeof(iron_runtime_type_t *));
    }
    method->locals = locals;
    method->local_count = local_count;
    return IRON_SUCCESS;
}

static void load_field_signature(iron_assembly_t *assembly, iron_runtime_field_t *field, iron_u32 signature_index)
{
    const iron_u8 *signature;
    iron_u32 signature_size;
    iron_sig_reader_t reader;
    iron_result_t result;

    field->element_type = IRON_TYPE_OBJECT;
    field->size = (iron_u32)sizeof(void *);
    field->generic_param_index = (iron_u32)-1;

    result = iron_metadata_get_blob(&assembly->metadata, signature_index, &signature, &signature_size);
    if (!IRON_RESULT_OK(result) || signature_size < 2 || signature[0] != IRON_CALL_FIELD) {
        return;
    }

    iron_sig_init(&reader, signature, signature_size);
    reader.pos = 1;
    field->field_type = resolve_signature_type(assembly, &reader, NULL, 0, NULL, 0, &field->element_type, &field->generic_param_index);

    if (field->field_type) {
        field->size = (iron_u32)iron_type_storage_size(field->field_type);
    } else {
        iron_size element_size;

        element_size = iron_element_type_size(field->element_type);
        if (element_size > 0) {
            field->size = (iron_u32)element_size;
        }
    }
}

static void load_field_rva_data(iron_assembly_t *assembly, iron_runtime_field_t *field, iron_u32 field_row_index)
{
    iron_u32 field_rva_count;
    iron_u32 field_rva_index;

    if (!assembly || !field || field_row_index == 0 || (field->attrs & IRON_FIELD_HAS_FIELD_RVA) == 0) {
        return;
    }

    field_rva_count = iron_metadata_table_rows(&assembly->metadata, IRON_TABLE_FIELD_RVA);
    for (field_rva_index = 1; field_rva_index <= field_rva_count; field_rva_index++) {
        iron_field_rva_row_t row;
        const iron_pe_section_t *section;
        iron_u32 section_offset;
        iron_size file_offset;
        iron_size available_size;

        if (!IRON_RESULT_OK(iron_metadata_read_row(&assembly->metadata, IRON_MAKE_TOKEN(IRON_TABLE_FIELD_RVA, field_rva_index), &row)) || row.field != field_row_index) {
            continue;
        }

        section = iron_pe_section_from_rva(&assembly->image, row.rva);
        if (!section || row.rva < section->virtual_address) {
            return;
        }

        section_offset = row.rva - section->virtual_address;
        if (section_offset >= section->file_size) {
            return;
        }

        file_offset = (iron_size)section->file_offset + section_offset;
        if (file_offset >= assembly->image.data_size) {
            return;
        }

        available_size = (iron_size)section->file_size - section_offset;
        if (available_size > assembly->image.data_size - file_offset) {
            available_size = assembly->image.data_size - file_offset;
        }
        if (available_size > UINT32_MAX) {
            available_size = UINT32_MAX;
        }

        field->rva_data = assembly->image.data + file_offset;
        field->rva_size = (iron_u32)available_size;
        return;
    }
}

static void load_field_constant(iron_assembly_t *assembly, iron_runtime_field_t *field, iron_u32 field_row_index)
{
    iron_u32 constant_count;
    iron_u32 constant_index;

    if (!assembly || !field || field_row_index == 0 || (field->attrs & IRON_FIELD_HAS_DEFAULT) == 0) {
        return;
    }

    constant_count = iron_metadata_table_rows(&assembly->metadata, IRON_TABLE_CONSTANT);
    for (constant_index = 1; constant_index <= constant_count; constant_index++) {
        iron_constant_row_t row;
        iron_token_t parent_token;
        const iron_u8 *data;
        iron_u32 data_size;

        if (!IRON_RESULT_OK(iron_metadata_read_row(&assembly->metadata, IRON_MAKE_TOKEN(IRON_TABLE_CONSTANT, constant_index), &row))) {
            continue;
        }

        parent_token = iron_metadata_decode_coded(&assembly->metadata, IRON_CODED_HAS_CONSTANT, row.parent);
        if (parent_token != IRON_MAKE_TOKEN(IRON_TABLE_FIELD, field_row_index)) {
            continue;
        }
        if (!IRON_RESULT_OK(iron_metadata_get_blob(&assembly->metadata, row.value, &data, &data_size))) {
            return;
        }

        field->has_constant = IRON_TRUE;
        field->constant_type = (iron_element_type_t)row.type;
        field->constant_data = data;
        field->constant_data_size = data_size;

        switch (field->constant_type) {
            case IRON_TYPE_BOOLEAN:
            case IRON_TYPE_I1:
            case IRON_TYPE_U1:
                if (data_size >= 1) {
                    field->constant_value.i32 = field->constant_type == IRON_TYPE_I1 ? (iron_i32)(iron_i8)data[0] : (iron_i32)data[0];
                }
                break;

            case IRON_TYPE_CHAR:
            case IRON_TYPE_I2:
            case IRON_TYPE_U2:
                if (data_size >= 2) {
                    iron_u16 value;

                    value = iron_read_u16_le(data);
                    field->constant_value.i32 = field->constant_type == IRON_TYPE_I2 ? (iron_i32)(iron_i16)value : (iron_i32)value;
                }
                break;

            case IRON_TYPE_I4:
            case IRON_TYPE_U4:
                if (data_size >= 4) {
                    field->constant_value.i32 = iron_read_i32_le(data);
                }
                break;

            case IRON_TYPE_I8:
            case IRON_TYPE_U8:
                if (data_size >= 8) {
                    field->constant_value.i64 = iron_read_i64_le(data);
                }
                break;

            case IRON_TYPE_R4:
                if (data_size >= 4) {
                    field->constant_value.f32 = iron_read_f32_le(data);
                }
                break;

            case IRON_TYPE_R8:
                if (data_size >= 8) {
                    field->constant_value.f64 = iron_read_f64_le(data);
                }
                break;

            default:
                break;
        }

        return;
    }
}

static iron_runtime_type_t *resolve_type_spec_definition(iron_assembly_t *assembly, iron_token_t token)
{
    iron_type_spec_row_t type_spec;
    const iron_u8 *signature;
    iron_u32 signature_size;
    iron_sig_reader_t reader;
    iron_element_type_t element_type;
    iron_token_t definition_token;
    iron_result_t result;

    result = iron_metadata_read_row(&assembly->metadata, token, &type_spec);
    if (!IRON_RESULT_OK(result)) {
        return NULL;
    }

    result = iron_metadata_get_blob(&assembly->metadata, type_spec.signature, &signature, &signature_size);
    if (!IRON_RESULT_OK(result)) {
        return NULL;
    }

    iron_sig_init(&reader, signature, signature_size);
    result = iron_sig_read_element_type(&reader, &element_type);
    if (!IRON_RESULT_OK(result) || element_type != IRON_TYPE_GENERICINST) {
        return NULL;
    }

    result = iron_sig_read_element_type(&reader, &element_type);
    if (!IRON_RESULT_OK(result) || (element_type != IRON_TYPE_CLASS && element_type != IRON_TYPE_VALUETYPE)) {
        return NULL;
    }

    result = iron_sig_read_type_def_or_ref(&reader, &definition_token);
    if (!IRON_RESULT_OK(result)) {
        return NULL;
    }

    return iron_type_resolve_token(assembly->module, definition_token);
}

static iron_runtime_type_t *resolve_type_spec_context(iron_assembly_t *assembly,
                                                      iron_token_t token,
                                                      iron_runtime_type_t **type_arguments,
                                                      iron_u32 type_argument_count,
                                                      iron_runtime_type_t **method_arguments,
                                                      iron_u32 method_argument_count)
{
    iron_type_spec_row_t type_spec;
    const iron_u8 *signature;
    iron_u32 signature_size;
    iron_sig_reader_t reader;
    iron_result_t result;

    if (!assembly || IRON_TOKEN_TABLE(token) != IRON_TABLE_TYPE_SPEC) {
        return NULL;
    }

    result = iron_metadata_read_row(&assembly->metadata, token, &type_spec);
    if (!IRON_RESULT_OK(result) || !IRON_RESULT_OK(iron_metadata_get_blob(&assembly->metadata, type_spec.signature, &signature, &signature_size))) {
        return NULL;
    }

    iron_sig_init(&reader, signature, signature_size);
    return resolve_signature_type(assembly, &reader, type_arguments, type_argument_count, method_arguments, method_argument_count, NULL, NULL);
}

static iron_runtime_type_t *resolve_type_spec_arguments(iron_assembly_t *assembly,
                                                        iron_token_t token,
                                                        iron_runtime_type_t **type_arguments,
                                                        iron_u32 type_argument_count)
{
    return resolve_type_spec_context(assembly, token, type_arguments, type_argument_count, NULL, 0);
}

static iron_bool generic_argument_is_reference_type(const iron_runtime_type_t *argument)
{
    if (argument->kind == IRON_KIND_GENERIC_PARAM) {
        return (argument->attrs & 0x0004U) != 0;
    }
    return iron_type_is_managed_reference(argument);
}

static iron_bool generic_argument_is_non_nullable_value_type(const iron_runtime_type_t *argument)
{
    const iron_runtime_type_t *definition;

    if (argument->kind == IRON_KIND_GENERIC_PARAM) {
        return (argument->attrs & 0x0008U) != 0;
    }
    if (argument->kind != IRON_KIND_VALUETYPE && argument->kind != IRON_KIND_ENUM) {
        return IRON_FALSE;
    }

    definition = argument->generic_definition ? argument->generic_definition : argument;
    return !definition->full_name || strcmp(definition->full_name, "System.Nullable`1") != 0;
}

static iron_result_t generic_argument_has_default_constructor(iron_runtime_type_t *argument, iron_bool *has_constructor)
{
    iron_u32 method_index;

    *has_constructor = IRON_FALSE;
    if (argument->kind == IRON_KIND_GENERIC_PARAM) {
        *has_constructor = (argument->attrs & (0x0008U | 0x0010U)) != 0;
        return IRON_SUCCESS;
    }
    if (argument->kind == IRON_KIND_VALUETYPE || argument->kind == IRON_KIND_ENUM) {
        *has_constructor = IRON_TRUE;
        return IRON_SUCCESS;
    }
    if ((argument->attrs & 0x0080U) != 0 || argument->kind == IRON_KIND_INTERFACE) {
        return IRON_SUCCESS;
    }

    for (method_index = 0; method_index < argument->method_count; method_index++) {
        iron_runtime_method_t *constructor;
        iron_result_t result;

        constructor = argument->methods[method_index];
        if (!constructor || !constructor->name || strcmp(constructor->name, ".ctor") != 0 || (constructor->attrs & 0x0007U) != 0x0006U || (constructor->attrs & 0x0010U) != 0) {
            continue;
        }

        result = iron_method_load_signature(constructor);
        if (!IRON_RESULT_OK(result)) {
            return result;
        }
        if (constructor->param_count == 0) {
            *has_constructor = IRON_TRUE;
            return IRON_SUCCESS;
        }
    }

    return IRON_SUCCESS;
}

static iron_result_t generic_parameter_satisfies_explicit_constraint(iron_runtime_type_t *parameter,
                                                                      iron_runtime_type_t *required_constraint,
                                                                      iron_u32 depth,
                                                                      iron_bool *satisfies_constraint)
{
    iron_runtime_type_t **constraints;
    iron_u32 constraint_count;
    iron_u32 constraint_index;
    iron_result_t result;

    *satisfies_constraint = IRON_FALSE;
    if (depth >= 32) {
        return IRON_ERROR(IRON_ERR_INVALID_METADATA, "Generic parameter constraints contain an excessive recursive chain");
    }

    constraints = NULL;
    constraint_count = 0;
    result = iron_generic_parameter_get_constraints(parameter, parameter->module->assembly->allocator, &constraints, &constraint_count);
    if (!IRON_RESULT_OK(result)) {
        return result;
    }

    for (constraint_index = 0; constraint_index < constraint_count; constraint_index++) {
        iron_runtime_type_t *candidate;

        candidate = constraints[constraint_index];
        if (candidate == required_constraint || iron_type_is_assignable_to(candidate, required_constraint)) {
            *satisfies_constraint = IRON_TRUE;
            break;
        }
        if (candidate->kind == IRON_KIND_GENERIC_PARAM) {
            result = generic_parameter_satisfies_explicit_constraint(candidate, required_constraint, depth + 1, satisfies_constraint);
            if (!IRON_RESULT_OK(result) || *satisfies_constraint) {
                break;
            }
        }
    }

    if (constraints) {
        iron_free(parameter->module->assembly->allocator, constraints, (iron_size)constraint_count * sizeof(iron_runtime_type_t *));
    }
    return result;
}

static iron_result_t validate_generic_argument_set(iron_assembly_t *assembly,
                                                   iron_runtime_type_t **parameters,
                                                   iron_u32 parameter_count,
                                                   iron_runtime_type_t **arguments,
                                                   iron_u32 argument_count,
                                                   iron_runtime_type_t **type_context,
                                                   iron_u32 type_context_count,
                                                   iron_runtime_type_t **method_context,
                                                   iron_u32 method_context_count)
{
    iron_u32 parameter_index;

    if (!assembly || !parameters || !arguments || parameter_count == 0 || parameter_count != argument_count) {
        return IRON_ERROR(IRON_ERR_INVALID_GENERIC_ARGS, "Generic argument count does not match the definition");
    }

    for (parameter_index = 0; parameter_index < parameter_count; parameter_index++) {
        iron_runtime_type_t *parameter;
        iron_runtime_type_t *argument;
        iron_u32 constraint_row_count;
        iron_u32 constraint_row_index;

        parameter = parameters[parameter_index];
        argument = arguments[parameter_index];
        if (!parameter || !argument || argument->element_type == IRON_TYPE_VOID || argument->kind == IRON_KIND_POINTER || argument->kind == IRON_KIND_BYREF) {
            return IRON_ERROR(IRON_ERR_INVALID_GENERIC_ARGS, "A generic argument is not a valid constructed type argument");
        }
        if ((parameter->attrs & 0x0004U) != 0 && !generic_argument_is_reference_type(argument)) {
            return IRON_ERROR(IRON_ERR_CONSTRAINT_VIOLATION, "A generic argument does not satisfy the reference-type constraint");
        }
        if ((parameter->attrs & 0x0008U) != 0 && !generic_argument_is_non_nullable_value_type(argument)) {
            return IRON_ERROR(IRON_ERR_CONSTRAINT_VIOLATION, "A generic argument does not satisfy the non-nullable value-type constraint");
        }
        if ((parameter->attrs & 0x0010U) != 0) {
            iron_bool has_constructor;
            iron_result_t constructor_result;

            constructor_result = generic_argument_has_default_constructor(argument, &has_constructor);
            if (!IRON_RESULT_OK(constructor_result)) {
                return constructor_result;
            }
            if (!has_constructor) {
                return IRON_ERROR(IRON_ERR_CONSTRAINT_VIOLATION, "A generic argument does not satisfy the public parameterless-constructor constraint");
            }
        }

        constraint_row_count = iron_metadata_table_rows(&assembly->metadata, IRON_TABLE_GENERIC_PARAM_CONSTRAINT);
        for (constraint_row_index = 1; constraint_row_index <= constraint_row_count; constraint_row_index++) {
            iron_generic_param_constraint_row_t row;
            iron_runtime_type_t *constraint;
            iron_token_t constraint_token;

            if (!IRON_RESULT_OK(iron_metadata_read_row(&assembly->metadata, IRON_MAKE_TOKEN(IRON_TABLE_GENERIC_PARAM_CONSTRAINT, constraint_row_index), &row)) ||
                row.owner != IRON_TOKEN_INDEX(parameter->token)) {
                continue;
            }

            constraint_token = iron_metadata_decode_coded(&assembly->metadata, IRON_CODED_TYPE_DEF_OR_REF, row.constraint);
            if (IRON_TOKEN_TABLE(constraint_token) == IRON_TABLE_TYPE_SPEC) {
                constraint = resolve_type_spec_context(assembly, constraint_token, type_context, type_context_count, method_context, method_context_count);
            } else {
                constraint = iron_type_resolve_token(assembly->module, constraint_token);
            }
            if (!constraint) {
                return IRON_ERROR(IRON_ERR_INVALID_METADATA, "Cannot resolve a generic parameter constraint");
            }
            if (argument != constraint && !iron_type_is_assignable_to(argument, constraint)) {
                iron_bool satisfies_constraint;

                satisfies_constraint = IRON_FALSE;
                if (argument->kind == IRON_KIND_GENERIC_PARAM) {
                    iron_result_t implication_result;

                    implication_result = generic_parameter_satisfies_explicit_constraint(argument, constraint, 0, &satisfies_constraint);
                    if (!IRON_RESULT_OK(implication_result)) {
                        return implication_result;
                    }
                }
                if (!satisfies_constraint) {
                    return IRON_ERROR(IRON_ERR_CONSTRAINT_VIOLATION, "A generic argument does not satisfy an explicit type constraint");
                }
            }
        }
    }

    return IRON_SUCCESS;
}

iron_result_t iron_type_validate_generic_arguments(iron_runtime_type_t *definition, iron_runtime_type_t **args, iron_u32 arg_count)
{
    if (!definition || !definition->module || !definition->module->assembly || !definition->is_generic_definition) {
        return IRON_ERROR(IRON_ERR_INVALID_GENERIC_ARGS, "Type is not a generic type definition");
    }

    return validate_generic_argument_set(definition->module->assembly,
                                         definition->generic_params,
                                         definition->generic_param_count,
                                         args,
                                         arg_count,
                                         args,
                                         arg_count,
                                         NULL,
                                         0);
}

iron_result_t iron_method_validate_generic_arguments(iron_runtime_method_t *definition, iron_runtime_type_t **args, iron_u32 arg_count)
{
    iron_runtime_type_t **type_context;
    iron_u32 type_context_count;

    if (!definition || !definition->declaring_type || !definition->declaring_type->module || !definition->declaring_type->module->assembly || !definition->is_generic_definition) {
        return IRON_ERROR(IRON_ERR_INVALID_GENERIC_ARGS, "Method is not a generic method definition");
    }

    type_context = definition->declaring_type->is_generic_instance ? definition->declaring_type->generic_args : definition->declaring_type->generic_params;
    type_context_count = definition->declaring_type->is_generic_instance ? definition->declaring_type->generic_arg_count : definition->declaring_type->generic_param_count;
    return validate_generic_argument_set(definition->declaring_type->module->assembly,
                                         definition->generic_params,
                                         definition->generic_param_count,
                                         args,
                                         arg_count,
                                         type_context,
                                         type_context_count,
                                         args,
                                         arg_count);
}

iron_result_t iron_generic_parameter_get_constraints(iron_runtime_type_t *parameter,
                                                      iron_allocator_t *allocator,
                                                      iron_runtime_type_t ***constraints,
                                                      iron_u32 *constraint_count)
{
    iron_assembly_t *assembly;
    iron_runtime_type_t **type_context;
    iron_runtime_type_t **method_context;
    iron_u32 type_context_count;
    iron_u32 method_context_count;
    iron_u32 metadata_count;
    iron_u32 metadata_index;
    iron_u32 result_index;

    if (!parameter || parameter->kind != IRON_KIND_GENERIC_PARAM || !parameter->module || !parameter->module->assembly || !constraints || !constraint_count) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Generic constraints require a generic parameter descriptor");
    }

    allocator = allocator ? allocator : parameter->module->assembly->allocator;
    assembly = parameter->module->assembly;
    type_context = NULL;
    type_context_count = 0;
    method_context = NULL;
    method_context_count = 0;
    if (parameter->declaring_method) {
        iron_runtime_type_t *declaring_type;

        declaring_type = parameter->declaring_method->declaring_type;
        if (declaring_type) {
            type_context = declaring_type->is_generic_instance ? declaring_type->generic_args : declaring_type->generic_params;
            type_context_count = declaring_type->is_generic_instance ? declaring_type->generic_arg_count : declaring_type->generic_param_count;
        }
        method_context = parameter->declaring_method->generic_params;
        method_context_count = parameter->declaring_method->generic_param_count;
    } else if (parameter->declaring_type) {
        type_context = parameter->declaring_type->generic_params;
        type_context_count = parameter->declaring_type->generic_param_count;
    }

    *constraints = NULL;
    *constraint_count = 0;
    metadata_count = iron_metadata_table_rows(&assembly->metadata, IRON_TABLE_GENERIC_PARAM_CONSTRAINT);
    for (metadata_index = 1; metadata_index <= metadata_count; metadata_index++) {
        iron_generic_param_constraint_row_t row;

        if (IRON_RESULT_OK(iron_metadata_read_row(&assembly->metadata, IRON_MAKE_TOKEN(IRON_TABLE_GENERIC_PARAM_CONSTRAINT, metadata_index), &row)) &&
            row.owner == IRON_TOKEN_INDEX(parameter->token)) {
            (*constraint_count)++;
        }
    }

    if (*constraint_count == 0) {
        return IRON_SUCCESS;
    }

    *constraints = (iron_runtime_type_t **)iron_alloc(allocator, (iron_size)*constraint_count * sizeof(iron_runtime_type_t *));
    if (!*constraints) {
        *constraint_count = 0;
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate generic parameter constraints");
    }

    result_index = 0;
    for (metadata_index = 1; metadata_index <= metadata_count; metadata_index++) {
        iron_generic_param_constraint_row_t row;
        iron_runtime_type_t *constraint;
        iron_token_t constraint_token;

        if (!IRON_RESULT_OK(iron_metadata_read_row(&assembly->metadata, IRON_MAKE_TOKEN(IRON_TABLE_GENERIC_PARAM_CONSTRAINT, metadata_index), &row)) ||
            row.owner != IRON_TOKEN_INDEX(parameter->token)) {
            continue;
        }

        constraint_token = iron_metadata_decode_coded(&assembly->metadata, IRON_CODED_TYPE_DEF_OR_REF, row.constraint);
        if (IRON_TOKEN_TABLE(constraint_token) == IRON_TABLE_TYPE_SPEC) {
            constraint = resolve_type_spec_context(assembly, constraint_token, type_context, type_context_count, method_context, method_context_count);
        } else {
            constraint = iron_type_resolve_token(assembly->module, constraint_token);
        }
        if (!constraint) {
            iron_free(allocator, *constraints, (iron_size)*constraint_count * sizeof(iron_runtime_type_t *));
            *constraints = NULL;
            *constraint_count = 0;
            return IRON_ERROR(IRON_ERR_INVALID_METADATA, "Cannot resolve a generic parameter constraint");
        }

        (*constraints)[result_index++] = constraint;
    }

    return IRON_SUCCESS;
}

iron_bool iron_type_contains_generic_parameters(const iron_runtime_type_t *type)
{
    iron_u32 argument_index;

    if (!type) {
        return IRON_FALSE;
    }
    if (type->kind == IRON_KIND_GENERIC_PARAM || type->is_generic_definition) {
        return IRON_TRUE;
    }
    if (type->element && iron_type_contains_generic_parameters(type->element)) {
        return IRON_TRUE;
    }

    for (argument_index = 0; argument_index < type->generic_arg_count; argument_index++) {
        if (iron_type_contains_generic_parameters(type->generic_args[argument_index])) {
            return IRON_TRUE;
        }
    }
    return IRON_FALSE;
}

static iron_bool instantiate_field_signature(iron_assembly_t *assembly,
                                             const iron_runtime_field_t *definition_field,
                                             iron_runtime_type_t **type_arguments,
                                             iron_u32 type_argument_count,
                                             iron_runtime_field_t *field)
{
    iron_field_row_t field_row;
    const iron_u8 *signature;
    iron_u32 signature_size;
    iron_sig_reader_t reader;
    iron_result_t result;

    result = iron_metadata_read_row(&assembly->metadata, definition_field->token, &field_row);
    if (!IRON_RESULT_OK(result)) {
        return IRON_FALSE;
    }

    result = iron_metadata_get_blob(&assembly->metadata, field_row.signature, &signature, &signature_size);
    if (!IRON_RESULT_OK(result) || signature_size < 2 || signature[0] != IRON_CALL_FIELD) {
        return IRON_FALSE;
    }

    iron_sig_init(&reader, signature, signature_size);
    reader.pos = 1;
    field->field_type = resolve_signature_type(assembly,
                                               &reader,
                                               type_arguments,
                                               type_argument_count,
                                               NULL,
                                               0,
                                               &field->element_type,
                                               &field->generic_param_index);
    if (!field->field_type) {
        return IRON_FALSE;
    }

    if (iron_type_is_managed_reference(field->field_type)) {
        field->element_type = IRON_TYPE_CLASS;
    } else if (field->field_type->element_type != IRON_TYPE_END) {
        field->element_type = field->field_type->element_type;
    } else {
        field->element_type = IRON_TYPE_VALUETYPE;
    }

    field->size = (iron_u32)iron_type_storage_size(field->field_type);
    return field->size != 0;
}

static iron_runtime_type_t *instantiate_base_type(iron_runtime_type_t *definition,
                                                  iron_runtime_type_t **type_arguments,
                                                  iron_u32 type_argument_count)
{
    iron_assembly_t *assembly;
    iron_type_def_row_t type_row;
    iron_token_t base_token;

    if (!definition->module || !definition->module->assembly || definition->token == 0) {
        return definition->base_type;
    }

    assembly = definition->module->assembly;
    if (!IRON_RESULT_OK(iron_metadata_read_row(&assembly->metadata, definition->token, &type_row)) || type_row.extends == 0) {
        return NULL;
    }

    base_token = iron_metadata_decode_coded(&assembly->metadata, IRON_CODED_TYPE_DEF_OR_REF, type_row.extends);
    if (IRON_TOKEN_TABLE(base_token) == IRON_TABLE_TYPE_SPEC) {
        return resolve_type_spec_arguments(assembly, base_token, type_arguments, type_argument_count);
    }

    return iron_type_resolve_token(definition->module, base_token);
}

static iron_bool instantiate_interfaces(iron_runtime_type_t *instance,
                                        iron_runtime_type_t *definition,
                                        iron_runtime_type_t **type_arguments,
                                        iron_u32 type_argument_count)
{
    iron_assembly_t *assembly;
    iron_u32 interface_rows;
    iron_u32 row_index;
    iron_u32 interface_index;

    if (!definition->module || !definition->module->assembly) {
        return definition->interface_count == 0;
    }

    assembly = definition->module->assembly;
    interface_rows = iron_metadata_table_rows(&assembly->metadata, IRON_TABLE_INTERFACE_IMPL);
    if (definition->interface_count == 0) {
        return IRON_TRUE;
    }

    instance->interfaces = (iron_runtime_type_t **)iron_alloc(assembly->allocator, definition->interface_count * sizeof(iron_runtime_type_t *));
    if (!instance->interfaces) {
        return IRON_FALSE;
    }
    memset(instance->interfaces, 0, definition->interface_count * sizeof(iron_runtime_type_t *));

    interface_index = 0;
    for (row_index = 1; row_index <= interface_rows; row_index++) {
        iron_interface_impl_row_t row;
        iron_runtime_type_t *interface_type;
        iron_token_t interface_token;

        if (!IRON_RESULT_OK(iron_metadata_read_row(&assembly->metadata, IRON_MAKE_TOKEN(IRON_TABLE_INTERFACE_IMPL, row_index), &row)) ||
            IRON_MAKE_TOKEN(IRON_TABLE_TYPE_DEF, row.class_) != definition->token) {
            continue;
        }

        interface_token = iron_metadata_decode_coded(&assembly->metadata, IRON_CODED_TYPE_DEF_OR_REF, row.interface_);
        if (IRON_TOKEN_TABLE(interface_token) == IRON_TABLE_TYPE_SPEC) {
            interface_type = resolve_type_spec_arguments(assembly, interface_token, type_arguments, type_argument_count);
        } else {
            interface_type = iron_type_resolve_token(definition->module, interface_token);
        }

        if (!interface_type || interface_index >= definition->interface_count) {
            return IRON_FALSE;
        }
        instance->interfaces[interface_index++] = interface_type;
    }

    instance->interface_count = interface_index;
    return interface_index == definition->interface_count;
}

static void configure_well_known_type(iron_runtime_type_t *type)
{
    iron_domain_t *domain;

    if (!type->namespace_ || strcmp(type->namespace_, "System") != 0 || !type->name) {
        return;
    }

    domain = type->module && type->module->assembly ? type->module->assembly->domain : NULL;

    if (strcmp(type->name, "Boolean") == 0) {
        type->kind = IRON_KIND_VALUETYPE;
        type->element_type = IRON_TYPE_BOOLEAN;
        type->instance_size = 1;
        type->alignment = 1;
        if (domain) {
            domain->type_boolean = type;
        }
    } else if (strcmp(type->name, "Char") == 0) {
        type->kind = IRON_KIND_VALUETYPE;
        type->element_type = IRON_TYPE_CHAR;
        type->instance_size = 2;
        type->alignment = 2;
        if (domain) {
            domain->type_char = type;
        }
    } else if (strcmp(type->name, "SByte") == 0) {
        type->kind = IRON_KIND_VALUETYPE;
        type->element_type = IRON_TYPE_I1;
        type->instance_size = 1;
        type->alignment = 1;
        if (domain) {
            domain->type_sbyte = type;
        }
    } else if (strcmp(type->name, "Byte") == 0) {
        type->kind = IRON_KIND_VALUETYPE;
        type->element_type = IRON_TYPE_U1;
        type->instance_size = 1;
        type->alignment = 1;
        if (domain) {
            domain->type_byte = type;
        }
    } else if (strcmp(type->name, "Int16") == 0) {
        type->kind = IRON_KIND_VALUETYPE;
        type->element_type = IRON_TYPE_I2;
        type->instance_size = 2;
        type->alignment = 2;
        if (domain) {
            domain->type_int16 = type;
        }
    } else if (strcmp(type->name, "UInt16") == 0) {
        type->kind = IRON_KIND_VALUETYPE;
        type->element_type = IRON_TYPE_U2;
        type->instance_size = 2;
        type->alignment = 2;
        if (domain) {
            domain->type_uint16 = type;
        }
    } else if (strcmp(type->name, "Int32") == 0) {
        type->kind = IRON_KIND_VALUETYPE;
        type->element_type = IRON_TYPE_I4;
        type->instance_size = 4;
        type->alignment = 4;
        if (domain) {
            domain->type_int32 = type;
        }
    } else if (strcmp(type->name, "UInt32") == 0) {
        type->kind = IRON_KIND_VALUETYPE;
        type->element_type = IRON_TYPE_U4;
        type->instance_size = 4;
        type->alignment = 4;
        if (domain) {
            domain->type_uint32 = type;
        }
    } else if (strcmp(type->name, "Int64") == 0) {
        type->kind = IRON_KIND_VALUETYPE;
        type->element_type = IRON_TYPE_I8;
        type->instance_size = 8;
        type->alignment = 8;
        if (domain) {
            domain->type_int64 = type;
        }
    } else if (strcmp(type->name, "UInt64") == 0) {
        type->kind = IRON_KIND_VALUETYPE;
        type->element_type = IRON_TYPE_U8;
        type->instance_size = 8;
        type->alignment = 8;
        if (domain) {
            domain->type_uint64 = type;
        }
    } else if (strcmp(type->name, "Single") == 0) {
        type->kind = IRON_KIND_VALUETYPE;
        type->element_type = IRON_TYPE_R4;
        type->instance_size = 4;
        type->alignment = 4;
        if (domain) {
            domain->type_single = type;
        }
    } else if (strcmp(type->name, "Double") == 0) {
        type->kind = IRON_KIND_VALUETYPE;
        type->element_type = IRON_TYPE_R8;
        type->instance_size = 8;
        type->alignment = 8;
        if (domain) {
            domain->type_double = type;
        }
    } else if (strcmp(type->name, "IntPtr") == 0) {
        type->kind = IRON_KIND_VALUETYPE;
        type->element_type = IRON_TYPE_I;
        type->instance_size = (iron_u32)sizeof(void *);
        type->alignment = (iron_u32)sizeof(void *);
        if (domain) {
            domain->type_intptr = type;
        }
    } else if (strcmp(type->name, "UIntPtr") == 0) {
        type->kind = IRON_KIND_VALUETYPE;
        type->element_type = IRON_TYPE_U;
        type->instance_size = (iron_u32)sizeof(void *);
        type->alignment = (iron_u32)sizeof(void *);
        if (domain) {
            domain->type_uintptr = type;
        }
    } else if (strcmp(type->name, "String") == 0) {
        type->element_type = IRON_TYPE_STRING;
        if (domain) {
            domain->type_string = type;
        }
    } else if (strcmp(type->name, "Object") == 0) {
        type->element_type = IRON_TYPE_OBJECT;
        if (domain) {
            domain->type_object = type;
        }
    }

    if (type->kind == IRON_KIND_VALUETYPE && type->element_type != IRON_TYPE_END) {
        type->layout_computed = IRON_TRUE;
    }

    if (!domain) {
        return;
    }

    if (strcmp(type->name, "ValueType") == 0) {
        domain->type_value_type = type;
    } else if (strcmp(type->name, "Enum") == 0) {
        domain->type_enum = type;
    } else if (strcmp(type->name, "Array") == 0) {
        domain->type_array = type;
    } else if (strcmp(type->name, "Delegate") == 0) {
        domain->type_delegate = type;
    } else if (strcmp(type->name, "MulticastDelegate") == 0) {
        domain->type_multicast_delegate = type;
    } else if (strcmp(type->name, "Exception") == 0) {
        domain->type_exception = type;
    } else if (strcmp(type->name, "Type") == 0) {
        domain->type_type = type;
    } else if (strcmp(type->name, "Void") == 0) {
        domain->type_void = type;
    }
}

static void configure_metadata_type_kind(iron_runtime_type_t *type)
{
    const char *base_name;

    if (!type || !type->base_type || !type->base_type->full_name || !type->full_name) {
        return;
    }

    base_name = type->base_type->full_name;
    if (strcmp(base_name, "System.ValueType") == 0 && strcmp(type->full_name, "System.Enum") != 0) {
        type->kind = IRON_KIND_VALUETYPE;
    } else if (strcmp(base_name, "System.Enum") == 0) {
        type->kind = IRON_KIND_ENUM;
    } else if ((strcmp(base_name, "System.Delegate") == 0 || strcmp(base_name, "System.MulticastDelegate") == 0) &&
               strcmp(type->full_name, "System.MulticastDelegate") != 0) {
        type->kind = IRON_KIND_DELEGATE;
    }
}

static void load_type_layout_metadata(iron_runtime_type_t *type)
{
    iron_assembly_t *assembly;
    iron_u32 type_row_index;
    iron_u32 row_index;
    iron_u32 row_count;

    if (!type || !type->module || !type->module->assembly || IRON_TOKEN_TABLE(type->token) != IRON_TABLE_TYPE_DEF) {
        return;
    }

    assembly = type->module->assembly;
    type_row_index = IRON_TOKEN_INDEX(type->token);
    row_count = iron_metadata_table_rows(&assembly->metadata, IRON_TABLE_CLASS_LAYOUT);
    for (row_index = 1; row_index <= row_count; row_index++) {
        iron_class_layout_row_t row;

        if (IRON_RESULT_OK(iron_metadata_read_row(&assembly->metadata, IRON_MAKE_TOKEN(IRON_TABLE_CLASS_LAYOUT, row_index), &row)) && row.parent == type_row_index) {
            type->packing_size = row.packing_size;
            type->metadata_size = row.class_size;
            break;
        }
    }

    if ((type->attrs & IRON_TYPE_LAYOUT_MASK) != IRON_TYPE_EXPLICIT_LAYOUT) {
        return;
    }

    row_count = iron_metadata_table_rows(&assembly->metadata, IRON_TABLE_FIELD_LAYOUT);
    for (row_index = 1; row_index <= row_count; row_index++) {
        iron_field_layout_row_t row;
        iron_runtime_field_t *field;

        if (!IRON_RESULT_OK(iron_metadata_read_row(&assembly->metadata, IRON_MAKE_TOKEN(IRON_TABLE_FIELD_LAYOUT, row_index), &row))) {
            continue;
        }

        field = iron_field_resolve_token(type->module, IRON_MAKE_TOKEN(IRON_TABLE_FIELD, row.field));
        if (field && field->declaring_type == type) {
            field->offset = row.offset;
        }
    }
}

static iron_result_t load_generic_parameters(iron_assembly_t *assembly,
                                             iron_token_t owner_token,
                                             iron_runtime_type_t ***out_parameters,
                                             iron_u32 *out_parameter_count)
{
    iron_u32 generic_param_rows;
    iron_u32 row_index;
    iron_u32 generic_param_count;
    iron_runtime_type_t **parameters;
    iron_u32 parameter_index;

    if (!assembly || !out_parameters || !out_parameter_count) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Invalid generic parameter metadata arguments");
    }

    *out_parameters = NULL;
    *out_parameter_count = 0;
    generic_param_rows = iron_metadata_table_rows(&assembly->metadata, IRON_TABLE_GENERIC_PARAM);
    generic_param_count = 0;
    for (row_index = 1; row_index <= generic_param_rows; row_index++) {
        iron_generic_param_row_t row;
        iron_token_t owner;

        if (!IRON_RESULT_OK(iron_metadata_read_row(&assembly->metadata, IRON_MAKE_TOKEN(IRON_TABLE_GENERIC_PARAM, row_index), &row))) {
            continue;
        }

        owner = iron_metadata_decode_coded(&assembly->metadata, IRON_CODED_TYPE_OR_METHOD_DEF, row.owner);
        if (owner == owner_token) {
            generic_param_count++;
        }
    }

    if (generic_param_count == 0) {
        return IRON_SUCCESS;
    }

    parameters = (iron_runtime_type_t **)iron_alloc(assembly->allocator, (iron_size)generic_param_count * sizeof(iron_runtime_type_t *));
    if (!parameters) {
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate generic parameter descriptors");
    }
    memset(parameters, 0, (iron_size)generic_param_count * sizeof(iron_runtime_type_t *));

    for (row_index = 1; row_index <= generic_param_rows; row_index++) {
        iron_generic_param_row_t row;
        iron_runtime_type_t *parameter;
        iron_token_t owner;

        if (!IRON_RESULT_OK(iron_metadata_read_row(&assembly->metadata, IRON_MAKE_TOKEN(IRON_TABLE_GENERIC_PARAM, row_index), &row))) {
            continue;
        }

        owner = iron_metadata_decode_coded(&assembly->metadata, IRON_CODED_TYPE_OR_METHOD_DEF, row.owner);
        if (owner != owner_token || row.number >= generic_param_count || parameters[row.number]) {
            continue;
        }

        parameter = (iron_runtime_type_t *)iron_alloc(assembly->allocator, sizeof(iron_runtime_type_t));
        if (!parameter) {
            free_generic_parameters(assembly->allocator, parameters, generic_param_count);
            return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate a generic parameter descriptor");
        }

        memset(parameter, 0, sizeof(*parameter));
        parameter->token = IRON_MAKE_TOKEN(IRON_TABLE_GENERIC_PARAM, row_index);
        parameter->module = assembly->module;
        parameter->name = iron_metadata_get_string(&assembly->metadata, row.name);
        parameter->full_name = parameter->name;
        parameter->kind = IRON_KIND_GENERIC_PARAM;
        parameter->element_type = IRON_TOKEN_TABLE(owner_token) == IRON_TABLE_METHOD_DEF ? IRON_TYPE_MVAR : IRON_TYPE_VAR;
        parameter->attrs = row.flags;
        parameter->generic_parameter_position = row.number;
        parameter->instance_size = (iron_u32)sizeof(void *);
        parameter->alignment = (iron_u32)sizeof(void *);
        parameter->layout_computed = IRON_TRUE;
        parameters[row.number] = parameter;
    }

    for (parameter_index = 0; parameter_index < generic_param_count; parameter_index++) {
        if (!parameters[parameter_index]) {
            free_generic_parameters(assembly->allocator, parameters, generic_param_count);
            return IRON_ERROR(IRON_ERR_INVALID_METADATA, "Generic parameter numbering is incomplete or duplicated");
        }
    }

    for (parameter_index = 0; parameter_index < generic_param_count; parameter_index++) {
        if (!register_type_descriptor(assembly->domain, parameters[parameter_index])) {
            free_generic_parameters(assembly->allocator, parameters, generic_param_count);
            return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to index a generic parameter descriptor");
        }
    }

    *out_parameters = parameters;
    *out_parameter_count = generic_param_count;
    return IRON_SUCCESS;
}

static iron_result_t load_type_generic_metadata(iron_runtime_type_t *type)
{
    iron_result_t result;

    if (!type || !type->module || !type->module->assembly) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Cannot load generic metadata for a type without an assembly");
    }

    result = load_generic_parameters(type->module->assembly, type->token, &type->generic_params, &type->generic_param_count);
    if (!IRON_RESULT_OK(result)) {
        return result;
    }

    {
        iron_u32 parameter_index;

        for (parameter_index = 0; parameter_index < type->generic_param_count; parameter_index++) {
            type->generic_params[parameter_index]->declaring_type = type;
        }
    }

    type->is_generic_definition = type->generic_param_count != 0;
    return IRON_SUCCESS;
}

static iron_bool load_type_interfaces(iron_runtime_type_t *type)
{
    iron_assembly_t *assembly;
    iron_u32 interface_rows;
    iron_u32 row_index;
    iron_u32 interface_count;
    iron_u32 interface_index;

    if (!type || !type->module || !type->module->assembly) {
        return IRON_FALSE;
    }

    assembly = type->module->assembly;
    interface_rows = iron_metadata_table_rows(&assembly->metadata, IRON_TABLE_INTERFACE_IMPL);
    interface_count = 0;
    for (row_index = 1; row_index <= interface_rows; row_index++) {
        iron_interface_impl_row_t row;

        if (IRON_RESULT_OK(iron_metadata_read_row(&assembly->metadata, IRON_MAKE_TOKEN(IRON_TABLE_INTERFACE_IMPL, row_index), &row)) &&
            IRON_MAKE_TOKEN(IRON_TABLE_TYPE_DEF, row.class_) == type->token) {
            interface_count++;
        }
    }

    if (interface_count == 0) {
        return IRON_TRUE;
    }

    type->interfaces = (iron_runtime_type_t **)iron_alloc(assembly->allocator, interface_count * sizeof(iron_runtime_type_t *));
    if (!type->interfaces) {
        return IRON_FALSE;
    }

    interface_index = 0;
    for (row_index = 1; row_index <= interface_rows; row_index++) {
        iron_interface_impl_row_t row;
        iron_token_t interface_token;
        iron_runtime_type_t *interface_type;

        if (!IRON_RESULT_OK(iron_metadata_read_row(&assembly->metadata, IRON_MAKE_TOKEN(IRON_TABLE_INTERFACE_IMPL, row_index), &row)) ||
            IRON_MAKE_TOKEN(IRON_TABLE_TYPE_DEF, row.class_) != type->token) {
            continue;
        }

        interface_token = iron_metadata_decode_coded(&assembly->metadata, IRON_CODED_TYPE_DEF_OR_REF, row.interface_);
        if (IRON_TOKEN_TABLE(interface_token) == IRON_TABLE_TYPE_SPEC) {
            interface_type = resolve_type_spec_arguments(assembly, interface_token, NULL, 0);
            if (!interface_type) {
                interface_type = resolve_type_spec_definition(assembly, interface_token);
            }
        } else {
            interface_type = iron_type_resolve_token(type->module, interface_token);
        }

        if (!interface_type) {
            iron_free(assembly->allocator, type->interfaces, interface_count * sizeof(iron_runtime_type_t *));
            type->interfaces = NULL;
            return IRON_FALSE;
        }

        type->interfaces[interface_index++] = interface_type;
    }

    type->interface_count = interface_index;
    return IRON_TRUE;
}

static iron_bool load_type_properties(iron_runtime_type_t *type)
{
    iron_assembly_t *assembly;
    iron_u32 property_map_count;
    iron_u32 property_map_index;
    iron_u32 property_start;
    iron_u32 property_end;
    iron_u32 property_index;

    if (!type || !type->module || !type->module->assembly || IRON_TOKEN_TABLE(type->token) != IRON_TABLE_TYPE_DEF) {
        return IRON_FALSE;
    }

    assembly = type->module->assembly;
    property_map_count = iron_metadata_table_rows(&assembly->metadata, IRON_TABLE_PROPERTY_MAP);
    property_start = 0;
    property_end = 0;
    for (property_map_index = 1; property_map_index <= property_map_count; property_map_index++) {
        iron_property_map_row_t map;

        if (!IRON_RESULT_OK(iron_metadata_read_row(&assembly->metadata, IRON_MAKE_TOKEN(IRON_TABLE_PROPERTY_MAP, property_map_index), &map)) ||
            map.parent != IRON_TOKEN_INDEX(type->token)) {
            continue;
        }

        property_start = map.property_list;
        if (property_map_index < property_map_count) {
            iron_property_map_row_t next_map;

            if (!IRON_RESULT_OK(iron_metadata_read_row(&assembly->metadata, IRON_MAKE_TOKEN(IRON_TABLE_PROPERTY_MAP, property_map_index + 1), &next_map))) {
                return IRON_FALSE;
            }
            property_end = next_map.property_list;
        } else {
            iron_u32 property_pointer_count;

            property_pointer_count = iron_metadata_table_rows(&assembly->metadata, IRON_TABLE_PROPERTY_PTR);
            property_end = (property_pointer_count != 0 ? property_pointer_count : iron_metadata_table_rows(&assembly->metadata, IRON_TABLE_PROPERTY)) + 1;
        }
        break;
    }

    if (property_start == 0 || property_end <= property_start) {
        return IRON_TRUE;
    }

    type->property_count = property_end - property_start;
    type->properties = (iron_runtime_property_t **)iron_alloc(assembly->allocator, (iron_size)type->property_count * sizeof(iron_runtime_property_t *));
    if (!type->properties) {
        type->property_count = 0;
        return IRON_FALSE;
    }
    memset(type->properties, 0, (iron_size)type->property_count * sizeof(iron_runtime_property_t *));

    for (property_index = 0; property_index < type->property_count; property_index++) {
        iron_u32 metadata_index;

        metadata_index = property_start + property_index;
        if (iron_metadata_table_rows(&assembly->metadata, IRON_TABLE_PROPERTY_PTR) != 0) {
            iron_property_ptr_row_t pointer_row;

            if (!IRON_RESULT_OK(iron_metadata_read_row(&assembly->metadata, IRON_MAKE_TOKEN(IRON_TABLE_PROPERTY_PTR, metadata_index), &pointer_row))) {
                iron_free(assembly->allocator, type->properties, (iron_size)type->property_count * sizeof(iron_runtime_property_t *));
                type->properties = NULL;
                type->property_count = 0;
                return IRON_FALSE;
            }
            metadata_index = pointer_row.property;
        }

        type->properties[property_index] = iron_property_resolve_token(type->module, IRON_MAKE_TOKEN(IRON_TABLE_PROPERTY, metadata_index));
        if (!type->properties[property_index]) {
            iron_free(assembly->allocator, type->properties, (iron_size)type->property_count * sizeof(iron_runtime_property_t *));
            type->properties = NULL;
            type->property_count = 0;
            return IRON_FALSE;
        }
        type->properties[property_index]->declaring_type = type;
    }

    return IRON_TRUE;
}

static iron_bool load_type_events(iron_runtime_type_t *type)
{
    iron_assembly_t *assembly;
    iron_u32 event_map_count;
    iron_u32 event_map_index;
    iron_u32 event_start;
    iron_u32 event_end;
    iron_u32 event_index;

    if (!type || !type->module || !type->module->assembly || IRON_TOKEN_TABLE(type->token) != IRON_TABLE_TYPE_DEF) {
        return IRON_FALSE;
    }

    assembly = type->module->assembly;
    event_map_count = iron_metadata_table_rows(&assembly->metadata, IRON_TABLE_EVENT_MAP);
    event_start = 0;
    event_end = 0;
    for (event_map_index = 1; event_map_index <= event_map_count; event_map_index++) {
        iron_event_map_row_t map;

        if (!IRON_RESULT_OK(iron_metadata_read_row(&assembly->metadata, IRON_MAKE_TOKEN(IRON_TABLE_EVENT_MAP, event_map_index), &map)) ||
            map.parent != IRON_TOKEN_INDEX(type->token)) {
            continue;
        }

        event_start = map.event_list;
        if (event_map_index < event_map_count) {
            iron_event_map_row_t next_map;

            if (!IRON_RESULT_OK(iron_metadata_read_row(&assembly->metadata, IRON_MAKE_TOKEN(IRON_TABLE_EVENT_MAP, event_map_index + 1), &next_map))) {
                return IRON_FALSE;
            }
            event_end = next_map.event_list;
        } else {
            iron_u32 event_pointer_count;

            event_pointer_count = iron_metadata_table_rows(&assembly->metadata, IRON_TABLE_EVENT_PTR);
            event_end = (event_pointer_count != 0 ? event_pointer_count : iron_metadata_table_rows(&assembly->metadata, IRON_TABLE_EVENT)) + 1;
        }
        break;
    }

    if (event_start == 0 || event_end <= event_start) {
        return IRON_TRUE;
    }

    type->event_count = event_end - event_start;
    type->events = (iron_runtime_event_t **)iron_alloc(assembly->allocator, (iron_size)type->event_count * sizeof(iron_runtime_event_t *));
    if (!type->events) {
        type->event_count = 0;
        return IRON_FALSE;
    }
    memset(type->events, 0, (iron_size)type->event_count * sizeof(iron_runtime_event_t *));

    for (event_index = 0; event_index < type->event_count; event_index++) {
        iron_u32 metadata_index;

        metadata_index = event_start + event_index;
        if (iron_metadata_table_rows(&assembly->metadata, IRON_TABLE_EVENT_PTR) != 0) {
            iron_event_ptr_row_t pointer_row;

            if (!IRON_RESULT_OK(iron_metadata_read_row(&assembly->metadata, IRON_MAKE_TOKEN(IRON_TABLE_EVENT_PTR, metadata_index), &pointer_row))) {
                iron_free(assembly->allocator, type->events, (iron_size)type->event_count * sizeof(iron_runtime_event_t *));
                type->events = NULL;
                type->event_count = 0;
                return IRON_FALSE;
            }
            metadata_index = pointer_row.event;
        }

        type->events[event_index] = iron_event_resolve_token(type->module, IRON_MAKE_TOKEN(IRON_TABLE_EVENT, metadata_index));
        if (!type->events[event_index]) {
            iron_free(assembly->allocator, type->events, (iron_size)type->event_count * sizeof(iron_runtime_event_t *));
            type->events = NULL;
            type->event_count = 0;
            return IRON_FALSE;
        }
        type->events[event_index]->declaring_type = type;
    }

    return IRON_TRUE;
}

static iron_bool load_type_declaring_type(iron_runtime_type_t *type)
{
    iron_assembly_t *assembly;
    iron_u32 row_count;
    iron_u32 row_index;

    if (!type || !type->module || !type->module->assembly || IRON_TOKEN_TABLE(type->token) != IRON_TABLE_TYPE_DEF) {
        return IRON_FALSE;
    }

    assembly = type->module->assembly;
    row_count = iron_metadata_table_rows(&assembly->metadata, IRON_TABLE_NESTED_CLASS);
    for (row_index = 1; row_index <= row_count; row_index++) {
        iron_nested_class_row_t row;

        if (!IRON_RESULT_OK(iron_metadata_read_row(&assembly->metadata, IRON_MAKE_TOKEN(IRON_TABLE_NESTED_CLASS, row_index), &row))) {
            return IRON_FALSE;
        }
        if (row.nested_class == IRON_TOKEN_INDEX(type->token)) {
            type->declaring_type = iron_type_resolve_token(type->module, IRON_MAKE_TOKEN(IRON_TABLE_TYPE_DEF, row.enclosing_class));
            return type->declaring_type != NULL;
        }
    }

    return IRON_TRUE;
}

static iron_bool load_type_nested_types(iron_runtime_type_t *type)
{
    iron_assembly_t *assembly;
    iron_u32 row_count;
    iron_u32 row_index;
    iron_u32 nested_count;
    iron_u32 nested_index;

    if (!type || !type->module || !type->module->assembly || IRON_TOKEN_TABLE(type->token) != IRON_TABLE_TYPE_DEF) {
        return IRON_FALSE;
    }

    assembly = type->module->assembly;
    row_count = iron_metadata_table_rows(&assembly->metadata, IRON_TABLE_NESTED_CLASS);
    nested_count = 0;
    for (row_index = 1; row_index <= row_count; row_index++) {
        iron_nested_class_row_t row;

        if (!IRON_RESULT_OK(iron_metadata_read_row(&assembly->metadata, IRON_MAKE_TOKEN(IRON_TABLE_NESTED_CLASS, row_index), &row))) {
            return IRON_FALSE;
        }
        if (row.enclosing_class == IRON_TOKEN_INDEX(type->token)) {
            nested_count++;
        }
    }

    if (nested_count == 0) {
        return IRON_TRUE;
    }

    type->nested_type_count = nested_count;
    type->nested_types = (iron_runtime_type_t **)iron_alloc(assembly->allocator, (iron_size)nested_count * sizeof(iron_runtime_type_t *));
    if (!type->nested_types) {
        type->nested_type_count = 0;
        return IRON_FALSE;
    }
    memset(type->nested_types, 0, (iron_size)nested_count * sizeof(iron_runtime_type_t *));

    nested_index = 0;
    for (row_index = 1; row_index <= row_count; row_index++) {
        iron_nested_class_row_t row;

        if (!IRON_RESULT_OK(iron_metadata_read_row(&assembly->metadata, IRON_MAKE_TOKEN(IRON_TABLE_NESTED_CLASS, row_index), &row))) {
            iron_free(assembly->allocator, type->nested_types, (iron_size)type->nested_type_count * sizeof(iron_runtime_type_t *));
            type->nested_types = NULL;
            type->nested_type_count = 0;
            return IRON_FALSE;
        }
        if (row.enclosing_class != IRON_TOKEN_INDEX(type->token)) {
            continue;
        }

        type->nested_types[nested_index] = iron_type_resolve_token(type->module, IRON_MAKE_TOKEN(IRON_TABLE_TYPE_DEF, row.nested_class));
        if (!type->nested_types[nested_index]) {
            iron_free(assembly->allocator, type->nested_types, (iron_size)type->nested_type_count * sizeof(iron_runtime_type_t *));
            type->nested_types = NULL;
            type->nested_type_count = 0;
            return IRON_FALSE;
        }
        nested_index++;
    }
    return IRON_TRUE;
}

iron_runtime_type_t *iron_type_resolve_token(iron_module_t *module,
                                             iron_token_t token)
{
    iron_u32 table_id;
    iron_u32 row_index;
    iron_assembly_t *assembly;

    if (!module || !module->assembly) return NULL;

    assembly = module->assembly;
    table_id = (token >> 24) & 0xFF;
    row_index = token & 0x00FFFFFF;

    if (row_index == 0) return NULL;

    if (table_id == IRON_TABLE_TYPE_DEF) {
        /* TypeDef - type defined in this assembly */
        iron_type_def_row_t type_def;
        iron_result_t res;
        iron_runtime_type_t *type;
        iron_u32 type_count;

        row_index--; /* Convert to 0-based */
        type_count = iron_metadata_table_rows(&assembly->metadata, IRON_TABLE_TYPE_DEF);
        if (row_index >= type_count) return NULL;

        /* Allocate types array if needed */
        if (!assembly->types) {
            iron_u32 descriptor_index;

            assembly->types = (iron_runtime_type_t *)iron_alloc(
                assembly->domain->allocator,
                type_count * sizeof(iron_runtime_type_t));
            if (!assembly->types) return NULL;
            memset(assembly->types, 0, type_count * sizeof(iron_runtime_type_t));
            assembly->type_count = type_count;

            for (descriptor_index = 0; descriptor_index < type_count; descriptor_index++) {
                if (!register_type_descriptor(assembly->domain, &assembly->types[descriptor_index])) {
                    while (descriptor_index != 0) {
                        descriptor_index--;
                        unregister_type_descriptor(assembly->domain, &assembly->types[descriptor_index]);
                    }
                    iron_free(assembly->domain->allocator, assembly->types, type_count * sizeof(iron_runtime_type_t));
                    assembly->types = NULL;
                    assembly->type_count = 0;
                    return NULL;
                }
            }

            assembly->module->types = (iron_runtime_type_t **)iron_alloc(assembly->domain->allocator, type_count * sizeof(iron_runtime_type_t *));
            if (!assembly->module->types) {
                for (descriptor_index = 0; descriptor_index < type_count; descriptor_index++) {
                    unregister_type_descriptor(assembly->domain, &assembly->types[descriptor_index]);
                }
                iron_free(assembly->domain->allocator, assembly->types, type_count * sizeof(iron_runtime_type_t));
                assembly->types = NULL;
                assembly->type_count = 0;
                return NULL;
            }

            memset(assembly->module->types, 0, type_count * sizeof(iron_runtime_type_t *));
            assembly->module->type_count = type_count;
        }

        type = &assembly->types[row_index];
        assembly->module->types[row_index] = type;

        /* Load type metadata if not already loaded */
        if (!type->name) {
            iron_u32 field_start, field_end;

            res = iron_metadata_read_row(&assembly->metadata, token, &type_def);
            if (!IRON_RESULT_OK(res)) return NULL;

            type->token = token;
            type->module = module;
            type->name = iron_metadata_get_string(&assembly->metadata, type_def.name);
            type->namespace_ = iron_metadata_get_string(&assembly->metadata, type_def.namespace_);
            type->attrs = type_def.flags;
            res = load_type_generic_metadata(type);
            if (!IRON_RESULT_OK(res)) {
                type->name = NULL;
                return NULL;
            }

            if (!load_type_declaring_type(type)) {
                type->name = NULL;
                return NULL;
            }

            if (type->declaring_type) {
                char full_name[512];

                if (!type->namespace_ || type->namespace_[0] == '\0') {
                    type->namespace_ = type->declaring_type->namespace_;
                }
                snprintf(full_name, sizeof(full_name), "%s+%s",
                         type->declaring_type->full_name ? type->declaring_type->full_name : type->declaring_type->name,
                         type->name);
                type->full_name = iron_intern_cstr(&assembly->domain->interner, full_name);
            } else if (type->namespace_ && type->namespace_[0] != '\0') {
                char full_name[512];

                snprintf(full_name, sizeof(full_name), "%s.%s", type->namespace_, type->name);
                type->full_name = iron_intern_cstr(&assembly->domain->interner, full_name);
            } else {
                type->full_name = type->name;
            }

            /* Determine type kind from flags */
            if (type->attrs & 0x00000020) { /* Interface */
                type->kind = IRON_KIND_INTERFACE;
            } else if (type->attrs & 0x00000100) { /* Abstract + Sealed = static class */
                type->kind = IRON_KIND_CLASS;
            } else {
                type->kind = IRON_KIND_CLASS;
            }

            if (type_def.extends != 0) {
                iron_token_t base_token;

                base_token = iron_metadata_decode_coded(&assembly->metadata, IRON_CODED_TYPE_DEF_OR_REF, type_def.extends);
                type->base_type = iron_type_resolve_token(module, base_token);
                IRON_DEBUG_META("Resolved base type: %s extends=0x%X token=0x%08X base=%s",
                                type->full_name ? type->full_name : type->name,
                                type_def.extends,
                                base_token,
                                type->base_type && type->base_type->full_name ? type->base_type->full_name : "<unresolved>");
            }

            configure_metadata_type_kind(type);

            /* Resolve fields and their metadata-defined layout inputs. */
            field_start = type_def.field_list;
            if (row_index + 1 < type_count) {
                iron_type_def_row_t next_type;
                iron_u32 next_token = (IRON_TABLE_TYPE_DEF << 24) | (row_index + 2);
                if (IRON_RESULT_OK(iron_metadata_read_row(&assembly->metadata, next_token, &next_type))) {
                    field_end = next_type.field_list;
                } else {
                    field_end = assembly->metadata.tables[IRON_TABLE_FIELD].row_count + 1;
                }
            } else {
                field_end = assembly->metadata.tables[IRON_TABLE_FIELD].row_count + 1;
            }

            type->field_count = (field_end > field_start) ? (field_end - field_start) : 0;

            if (type->field_count > 0) {
                iron_u32 field_index;

                type->fields = (iron_runtime_field_t **)iron_alloc(assembly->allocator, type->field_count * sizeof(iron_runtime_field_t *));
                if (!type->fields) {
                    type->name = NULL;
                    return NULL;
                }

                memset(type->fields, 0, type->field_count * sizeof(iron_runtime_field_t *));
                for (field_index = 0; field_index < type->field_count; field_index++) {
                    type->fields[field_index] = iron_field_resolve_token(module, IRON_MAKE_TOKEN(IRON_TABLE_FIELD, field_start + field_index));
                    if (!type->fields[field_index]) {
                        iron_free(assembly->allocator, type->fields, type->field_count * sizeof(iron_runtime_field_t *));
                        type->fields = NULL;
                        type->name = NULL;
                        return NULL;
                    }

                    type->fields[field_index]->declaring_type = type;
                }
            }

            type->alignment = 1;
            configure_well_known_type(type);
            load_type_layout_metadata(type);

            if (type->kind == IRON_KIND_ENUM && type->field_count != 0) {
                iron_u32 enum_field_index;

                for (enum_field_index = 0; enum_field_index < type->field_count; enum_field_index++) {
                    iron_runtime_field_t *enum_field;

                    enum_field = type->fields[enum_field_index];
                    if (enum_field && (enum_field->attrs & IRON_FIELD_STATIC) == 0 && enum_field->field_type) {
                        type->element_type = enum_field->field_type->element_type;
                        break;
                    }
                }
            }

            res = iron_type_compute_layout(type);
            if (!IRON_RESULT_OK(res)) {
                return NULL;
            }

            /* Load methods for this type */
            {
                iron_u32 method_start = type_def.method_list;
                iron_u32 method_end;
                iron_u32 method_count_for_type;
                
                if (row_index + 1 < type_count) {
                    iron_type_def_row_t next_type;
                    iron_u32 next_token = (IRON_TABLE_TYPE_DEF << 24) | (row_index + 2);
                    if (IRON_RESULT_OK(iron_metadata_read_row(&assembly->metadata, next_token, &next_type))) {
                        method_end = next_type.method_list;
                    } else {
                        method_end = assembly->metadata.tables[IRON_TABLE_METHOD_DEF].row_count + 1;
                    }
                } else {
                    method_end = assembly->metadata.tables[IRON_TABLE_METHOD_DEF].row_count + 1;
                }
                
                method_count_for_type = (method_end > method_start) ? (method_end - method_start) : 0;
                
                if (method_count_for_type > 0) {
                    iron_u32 m;
                    type->methods = (iron_runtime_method_t **)iron_alloc(
                        assembly->domain->allocator,
                        method_count_for_type * sizeof(iron_runtime_method_t *));
                    if (type->methods) {
                        type->method_count = method_count_for_type;
                        for (m = 0; m < method_count_for_type; m++) {
                            iron_u32 method_token = (IRON_TABLE_METHOD_DEF << 24) | (method_start + m);
                            type->methods[m] = iron_resolve_method_token(assembly, method_token);
                            if (type->methods[m]) {
                                type->methods[m]->declaring_type = type;
                                if (type->methods[m]->name && strcmp(type->methods[m]->name, ".cctor") == 0) {
                                    type->methods[m]->kind = IRON_METHOD_STATIC_CONSTRUCTOR;
                                    type->type_initializer = type->methods[m];
                                } else if (type->methods[m]->name && strcmp(type->methods[m]->name, ".ctor") == 0) {
                                    type->methods[m]->kind = IRON_METHOD_CONSTRUCTOR;
                                }
                            }
                        }
                    }
                }
            }

            if (!load_type_properties(type)) {
                return NULL;
            }

            if (!load_type_events(type)) {
                return NULL;
            }

            if (!load_type_interfaces(type)) {
                return NULL;
            }

            if (!load_type_nested_types(type)) {
                return NULL;
            }
            
        }

        return type;
    }
    else if (table_id == IRON_TABLE_TYPE_REF) {
        iron_type_ref_row_t type_ref;
        iron_result_t res;
        const char *type_name;
        const char *type_ns;
        iron_token_t resolution_scope;

        res = iron_metadata_read_row(&assembly->metadata, token, &type_ref);
        if (!IRON_RESULT_OK(res)) return NULL;

        type_name = iron_metadata_get_string(&assembly->metadata, type_ref.name);
        type_ns = iron_metadata_get_string(&assembly->metadata, type_ref.namespace_);
        resolution_scope = iron_metadata_decode_coded(&assembly->metadata, IRON_CODED_RESOLUTION_SCOPE, type_ref.resolution_scope);

        if (IRON_TOKEN_TABLE(resolution_scope) == IRON_TABLE_TYPE_REF) {
            iron_runtime_type_t *enclosing_type;
            iron_runtime_type_t *enclosing_definition;
            iron_assembly_t *definition_assembly;
            iron_u32 nested_row_count;
            iron_u32 nested_row_index;

            enclosing_type = iron_type_resolve_token(module, resolution_scope);
            enclosing_definition = enclosing_type && enclosing_type->generic_definition ? enclosing_type->generic_definition : enclosing_type;
            if (!enclosing_definition || !enclosing_definition->module || !enclosing_definition->module->assembly ||
                IRON_TOKEN_TABLE(enclosing_definition->token) != IRON_TABLE_TYPE_DEF) {
                return NULL;
            }

            definition_assembly = enclosing_definition->module->assembly;
            nested_row_count = iron_metadata_table_rows(&definition_assembly->metadata, IRON_TABLE_NESTED_CLASS);
            for (nested_row_index = 1; nested_row_index <= nested_row_count; nested_row_index++) {
                iron_nested_class_row_t nested_row;
                iron_runtime_type_t *nested_type;

                if (!IRON_RESULT_OK(iron_metadata_read_row(&definition_assembly->metadata, IRON_MAKE_TOKEN(IRON_TABLE_NESTED_CLASS, nested_row_index), &nested_row)) ||
                    nested_row.enclosing_class != IRON_TOKEN_INDEX(enclosing_definition->token)) {
                    continue;
                }

                nested_type = iron_type_resolve_token(definition_assembly->module, IRON_MAKE_TOKEN(IRON_TABLE_TYPE_DEF, nested_row.nested_class));
                if (nested_type && nested_type->name && type_name && strcmp(nested_type->name, type_name) == 0) {
                    return nested_type;
                }
            }

            return NULL;
        }

        /* Search in every loaded assembly, including the defining corlib. */
        {
            iron_u32 assembly_index;

            for (assembly_index = 0; assembly_index < assembly->domain->assembly_count; assembly_index++) {
                iron_runtime_type_t *resolved;

                resolved = iron_assembly_find_type(assembly->domain->assemblies[assembly_index], type_ns, type_name);
                if (resolved) {
                    return resolved;
                }
            }
        }

        return NULL;
    }

    return NULL;
}

iron_bool iron_type_is_managed_reference(const iron_runtime_type_t *type)
{
    if (!type) {
        return IRON_FALSE;
    }

    if (type->kind == IRON_KIND_CLASS || type->kind == IRON_KIND_INTERFACE || type->kind == IRON_KIND_DELEGATE || type->kind == IRON_KIND_ARRAY) {
        return IRON_TRUE;
    }

    return type->element_type == IRON_TYPE_CLASS || type->element_type == IRON_TYPE_OBJECT || type->element_type == IRON_TYPE_STRING ||
           type->element_type == IRON_TYPE_ARRAY || type->element_type == IRON_TYPE_SZARRAY;
}

static iron_u16 generic_parameter_flags(iron_runtime_type_t *definition, iron_u32 parameter_index)
{
    if (!definition || parameter_index >= definition->generic_param_count || !definition->generic_params || !definition->generic_params[parameter_index]) {
        return 0;
    }

    return (iron_u16)definition->generic_params[parameter_index]->attrs;
}

static iron_bool generic_instance_is_assignable_to(iron_runtime_type_t *source, iron_runtime_type_t *target)
{
    iron_runtime_type_t *definition;
    iron_u32 argument_index;

    if (!source || !target || !source->generic_definition || source->generic_definition != target->generic_definition ||
        source->generic_arg_count != target->generic_arg_count) {
        return IRON_FALSE;
    }

    definition = source->generic_definition;
    for (argument_index = 0; argument_index < source->generic_arg_count; argument_index++) {
        iron_u16 variance;

        variance = generic_parameter_flags(definition, argument_index) & 0x0003U;
        if (variance == 0) {
            if (source->generic_args[argument_index] != target->generic_args[argument_index]) {
                return IRON_FALSE;
            }
        } else if (variance == 1) {
            if (!iron_type_is_assignable_to(source->generic_args[argument_index], target->generic_args[argument_index])) {
                return IRON_FALSE;
            }
        } else if (variance == 2) {
            if (!iron_type_is_assignable_to(target->generic_args[argument_index], source->generic_args[argument_index])) {
                return IRON_FALSE;
            }
        } else {
            return IRON_FALSE;
        }
    }

    return IRON_TRUE;
}

static iron_bool array_type_is_assignable_to(iron_runtime_type_t *source, iron_runtime_type_t *target)
{
    iron_runtime_type_t *target_definition;
    const char *target_name;

    if (!source || !target || source->kind != IRON_KIND_ARRAY || !source->element) {
        return IRON_FALSE;
    }

    if (target->kind != IRON_KIND_ARRAY) {
        if (source->element_type != IRON_TYPE_SZARRAY || source->array_rank != 1 || !target->generic_definition ||
            target->generic_arg_count != 1 || !target->generic_args || !target->generic_args[0]) {
            return IRON_FALSE;
        }

        target_definition = target->generic_definition;
        target_name = target_definition->full_name;
        if (!target_name) {
            return IRON_FALSE;
        }

        if (strcmp(target_name, "System.Collections.Generic.IEnumerable`1") == 0 ||
            strcmp(target_name, "System.Collections.Generic.IReadOnlyCollection`1") == 0 ||
            strcmp(target_name, "System.Collections.Generic.IReadOnlyList`1") == 0) {
            if (source->element == target->generic_args[0]) {
                return IRON_TRUE;
            }

            return iron_type_is_managed_reference(source->element) && iron_type_is_managed_reference(target->generic_args[0]) &&
                   iron_type_is_assignable_to(source->element, target->generic_args[0]);
        }

        if (strcmp(target_name, "System.Collections.Generic.ICollection`1") == 0 || strcmp(target_name, "System.Collections.Generic.IList`1") == 0) {
            return source->element == target->generic_args[0];
        }

        return IRON_FALSE;
    }

    if (source->array_rank != target->array_rank || source->element_type != target->element_type || !target->element) {
        return IRON_FALSE;
    }

    if (source->element == target->element) {
        return IRON_TRUE;
    }

    return iron_type_is_managed_reference(source->element) && iron_type_is_managed_reference(target->element) && iron_type_is_assignable_to(source->element, target->element);
}

iron_runtime_type_t *iron_type_nullable_argument(const iron_runtime_type_t *type)
{
    if (!type || !type->generic_definition || !type->generic_definition->full_name ||
        type->generic_arg_count != 1 || !type->generic_args || strcmp(type->generic_definition->full_name, "System.Nullable`1") != 0) {
        return NULL;
    }

    return type->generic_args[0];
}

iron_bool iron_type_nullable_layout(iron_runtime_type_t *type, iron_u32 *has_value_offset, iron_u32 *value_offset)
{
    iron_runtime_type_t *argument;
    iron_u32 index;
    iron_bool found_flag;
    iron_bool found_value;
    iron_size value_size;

    argument = iron_type_nullable_argument(type);
    if (!argument || !has_value_offset || !value_offset) {
        return IRON_FALSE;
    }

    found_flag = IRON_FALSE;
    found_value = IRON_FALSE;
    value_size = iron_type_storage_size(argument);
    for (index = 0; index < type->field_count; index++) {
        iron_runtime_field_t *field;

        field = type->fields[index];
        if (!field || !field->name || (field->attrs & 0x0010) != 0) {
            continue;
        }
        if (strcmp(field->name, "_hasValue") == 0 && field->offset < type->instance_size) {
            *has_value_offset = field->offset;
            found_flag = IRON_TRUE;
        } else if (strcmp(field->name, "_value") == 0 && field->offset <= type->instance_size && value_size <= type->instance_size - field->offset) {
            *value_offset = field->offset;
            found_value = IRON_TRUE;
        }
    }

    return found_flag && found_value;
}

iron_bool iron_type_is_assignable_to(iron_runtime_type_t *source, iron_runtime_type_t *target)
{
    iron_runtime_type_t *current;

    if (!source || !target) {
        return IRON_FALSE;
    }

    if (source == target || source == iron_type_nullable_argument(target) || source->generic_definition == target || array_type_is_assignable_to(source, target)) {
        return IRON_TRUE;
    }

    if (generic_instance_is_assignable_to(source, target)) {
        return IRON_TRUE;
    }

    for (current = source->base_type; current; current = current->base_type) {
        if (current == target || current->generic_definition == target || generic_instance_is_assignable_to(current, target) ||
            (current->full_name && target->full_name && strcmp(current->full_name, target->full_name) == 0)) {
            return IRON_TRUE;
        }
    }

    for (current = source; current; current = current->base_type) {
        iron_u32 interface_index;

        for (interface_index = 0; interface_index < current->interface_count; interface_index++) {
            iron_runtime_type_t *interface_type;

            interface_type = current->interfaces[interface_index];
            if (interface_type && (interface_type == target || interface_type->generic_definition == target || generic_instance_is_assignable_to(interface_type, target) ||
                                   (interface_type->full_name && target->full_name && strcmp(interface_type->full_name, target->full_name) == 0) ||
                                   iron_type_is_assignable_to(interface_type, target))) {
                return IRON_TRUE;
            }
        }
    }

    return IRON_FALSE;
}

iron_size iron_type_storage_size(const iron_runtime_type_t *type)
{
    if (!type) {
        return sizeof(void *);
    }

    if (iron_type_is_managed_reference(type) || type->kind == IRON_KIND_POINTER || type->kind == IRON_KIND_BYREF || type->kind == IRON_KIND_FNPTR ||
        type->element_type == IRON_TYPE_I || type->element_type == IRON_TYPE_U || type->element_type == IRON_TYPE_PTR || type->element_type == IRON_TYPE_FNPTR) {
        return sizeof(void *);
    }

    return type->instance_size;
}

static iron_runtime_type_t *make_array_type(iron_domain_t *domain,
                                            iron_runtime_type_t *element,
                                            iron_u32 rank,
                                            iron_bool is_vector)
{
    iron_runtime_type_t *array_type;
    char full_name[512];
    const char *interned_name;
    
    if (!domain || !element || rank == 0 || rank > 32 || (is_vector && rank != 1)) {
        return NULL;
    }
    
    /* Build array type name (e.g., "System.Int32[]", "System.Int32[*]", or "System.Int32[,]"). */
    if (is_vector) {
        snprintf(full_name, sizeof(full_name), "%s[]", 
                 element->full_name ? element->full_name : element->name);
    } else {
        char brackets[64];
        iron_u32 i;

        brackets[0] = '[';
        if (rank == 1) {
            brackets[1] = '*';
            i = 2;
        } else {
            for (i = 1; i < rank; i++) {
                brackets[i] = ',';
            }
        }
        brackets[i] = ']';
        brackets[i + 1] = '\0';
        snprintf(full_name, sizeof(full_name), "%s%s",
                 element->full_name ? element->full_name : element->name, brackets);
    }
    
    /* Check if already cached */
    interned_name = full_name;
    if (iron_hashmap_get(&domain->type_cache, &interned_name, &array_type)) {
        return array_type;
    }
    
    /* Allocate new array type */
    array_type = (iron_runtime_type_t *)iron_alloc(domain->allocator, 
                                                    sizeof(iron_runtime_type_t));
    if (!array_type) {
        return NULL;
    }
    
    memset(array_type, 0, sizeof(iron_runtime_type_t));
    array_type->kind = IRON_KIND_ARRAY;
    array_type->element = element;
    array_type->array_rank = rank;
    array_type->element_type = is_vector ? IRON_TYPE_SZARRAY : IRON_TYPE_ARRAY;
    array_type->name = iron_intern_cstr(&domain->interner, full_name);
    array_type->full_name = array_type->name;
    array_type->base_type = domain->type_array;
    array_type->instance_size = sizeof(iron_u32) + sizeof(void*); /* length + data ptr */
    array_type->alignment = sizeof(void*);
    
    /* Cache the type */
    interned_name = array_type->name;
    if (!interned_name || !iron_hashmap_set(&domain->type_cache, &interned_name, &array_type)) {
        iron_free(domain->allocator, array_type, sizeof(iron_runtime_type_t));
        return NULL;
    }
    if (!register_type_descriptor(domain, array_type)) {
        iron_hashmap_remove(&domain->type_cache, &interned_name);
        iron_free(domain->allocator, array_type, sizeof(iron_runtime_type_t));
        return NULL;
    }
    
    return array_type;
}

iron_runtime_type_t *iron_type_make_array(iron_domain_t *domain,
                                          iron_runtime_type_t *element,
                                          iron_u32 rank)
{
    return make_array_type(domain, element, rank, rank == 1);
}

iron_runtime_type_t *iron_type_make_mdarray(iron_domain_t *domain,
                                            iron_runtime_type_t *element,
                                            iron_u32 rank)
{
    return make_array_type(domain, element, rank, IRON_FALSE);
}

iron_runtime_type_t *iron_type_make_pointer(iron_domain_t *domain,
                                            iron_runtime_type_t *element)
{
    iron_runtime_type_t *ptr_type;
    char full_name[512];
    const char *interned_name;
    
    if (!domain || !element) return NULL;
    
    /* Build pointer type name (e.g., "System.Int32*") */
    snprintf(full_name, sizeof(full_name), "%s*",
             element->full_name ? element->full_name : element->name);
    
    /* Check if already cached */
    interned_name = full_name;
    if (iron_hashmap_get(&domain->type_cache, &interned_name, &ptr_type)) {
        return ptr_type;
    }
    
    /* Allocate new pointer type */
    ptr_type = (iron_runtime_type_t *)iron_alloc(domain->allocator,
                                                  sizeof(iron_runtime_type_t));
    if (!ptr_type) return NULL;
    
    memset(ptr_type, 0, sizeof(iron_runtime_type_t));
    ptr_type->kind = IRON_KIND_POINTER;
    ptr_type->element = element;
    ptr_type->element_type = IRON_TYPE_PTR;
    ptr_type->name = iron_intern_cstr(&domain->interner, full_name);
    ptr_type->full_name = ptr_type->name;
    ptr_type->instance_size = sizeof(void*);
    ptr_type->alignment = sizeof(void*);
    
    /* Cache the type */
    interned_name = ptr_type->name;
    if (!interned_name || !iron_hashmap_set(&domain->type_cache, &interned_name, &ptr_type)) {
        iron_free(domain->allocator, ptr_type, sizeof(iron_runtime_type_t));
        return NULL;
    }
    if (!register_type_descriptor(domain, ptr_type)) {
        iron_hashmap_remove(&domain->type_cache, &interned_name);
        iron_free(domain->allocator, ptr_type, sizeof(iron_runtime_type_t));
        return NULL;
    }
    
    return ptr_type;
}

iron_runtime_type_t *iron_type_make_byref(iron_domain_t *domain,
                                          iron_runtime_type_t *element)
{
    iron_runtime_type_t *byref_type;
    char full_name[512];
    const char *interned_name;
    
    if (!domain || !element) return NULL;
    
    /* Build byref type name (e.g., "System.Int32&") */
    snprintf(full_name, sizeof(full_name), "%s&",
             element->full_name ? element->full_name : element->name);
    
    /* Check if already cached */
    interned_name = full_name;
    if (iron_hashmap_get(&domain->type_cache, &interned_name, &byref_type)) {
        return byref_type;
    }
    
    /* Allocate new byref type */
    byref_type = (iron_runtime_type_t *)iron_alloc(domain->allocator,
                                                    sizeof(iron_runtime_type_t));
    if (!byref_type) return NULL;
    
    memset(byref_type, 0, sizeof(iron_runtime_type_t));
    byref_type->kind = IRON_KIND_BYREF;
    byref_type->element = element;
    byref_type->element_type = IRON_TYPE_BYREF;
    byref_type->name = iron_intern_cstr(&domain->interner, full_name);
    byref_type->full_name = byref_type->name;
    byref_type->instance_size = sizeof(void*);
    byref_type->alignment = sizeof(void*);
    
    /* Cache the type */
    interned_name = byref_type->name;
    if (!interned_name || !iron_hashmap_set(&domain->type_cache, &interned_name, &byref_type)) {
        iron_free(domain->allocator, byref_type, sizeof(iron_runtime_type_t));
        return NULL;
    }
    if (!register_type_descriptor(domain, byref_type)) {
        iron_hashmap_remove(&domain->type_cache, &interned_name);
        iron_free(domain->allocator, byref_type, sizeof(iron_runtime_type_t));
        return NULL;
    }
    
    return byref_type;
}

static iron_runtime_method_t *find_instantiated_method(iron_runtime_type_t *instance, const iron_runtime_method_t *definition_method)
{
    iron_u32 method_index;

    if (!instance || !definition_method) {
        return NULL;
    }

    for (method_index = 0; method_index < instance->method_count; method_index++) {
        iron_runtime_method_t *method;

        method = instance->methods[method_index];
        if (method && method->token == definition_method->token) {
            return method;
        }
    }

    return NULL;
}

iron_runtime_type_t *iron_type_make_generic(iron_domain_t *domain,
                                            iron_runtime_type_t *definition,
                                            iron_runtime_type_t **args,
                                            iron_u32 arg_count)
{
    iron_runtime_type_t *inst_type;
    char full_name[1024];
    const char *interned_name;
    iron_u32 i;
    int pos;
    
    if (!domain || !definition || !args || arg_count == 0 || !IRON_RESULT_OK(iron_type_validate_generic_arguments(definition, args, arg_count))) {
        return NULL;
    }

    for (i = 0; i < arg_count; i++) {
        if (!args[i] || (!args[i]->full_name && !args[i]->name)) {
            return NULL;
        }
    }
    
    /* Build generic type name (e.g., "List`1<System.Int32>") */
    pos = snprintf(full_name, sizeof(full_name), "%s<",
                   definition->full_name ? definition->full_name : definition->name);
    
    for (i = 0; i < arg_count && pos < (int)sizeof(full_name) - 2; i++) {
        if (i > 0) {
            full_name[pos++] = ',';
        }
        pos += snprintf(full_name + pos, sizeof(full_name) - pos, "%s",
                       args[i]->full_name ? args[i]->full_name : args[i]->name);
    }
    if (pos < (int)sizeof(full_name) - 1) {
        full_name[pos++] = '>';
        full_name[pos] = '\0';
    } else {
        return NULL;
    }

    interned_name = full_name;
    if (iron_hashmap_get(&domain->type_cache, &interned_name, &inst_type)) {
        return inst_type;
    }
    
    /* Allocate new generic instantiation */
    inst_type = (iron_runtime_type_t *)iron_alloc(domain->allocator,
                                                   sizeof(iron_runtime_type_t));
    if (!inst_type) return NULL;
    
    memset(inst_type, 0, sizeof(iron_runtime_type_t));
    inst_type->token = definition->token;
    inst_type->module = definition->module;
    inst_type->namespace_ = definition->namespace_;
    inst_type->attrs = definition->attrs;
    inst_type->kind = definition->kind;
    inst_type->element_type = definition->element_type;
    inst_type->packing_size = definition->packing_size;
    inst_type->generic_definition = definition;
    inst_type->is_generic_instance = IRON_TRUE;
    inst_type->name = iron_intern_cstr(&domain->interner, full_name);
    inst_type->full_name = inst_type->name;
    inst_type->element = definition->element;
    inst_type->declaring_type = definition->declaring_type;
    
    /* Copy generic arguments */
    inst_type->generic_args = (iron_runtime_type_t **)iron_alloc(
        domain->allocator, arg_count * sizeof(iron_runtime_type_t*));
    if (!inst_type->generic_args) {
        iron_free(domain->allocator, inst_type, sizeof(iron_runtime_type_t));
        return NULL;
    }

    for (i = 0; i < arg_count; i++) {
        inst_type->generic_args[i] = args[i];
    }
    inst_type->generic_arg_count = arg_count;

    /* Publish the identity before cloning members so recursive signatures resolve to the same instance. */
    interned_name = inst_type->name;
    if (!interned_name || !iron_hashmap_set(&domain->type_cache, &interned_name, &inst_type)) {
        free_constructed_type(domain, inst_type);
        return NULL;
    }
    if (!register_type_descriptor(domain, inst_type)) {
        iron_hashmap_remove(&domain->type_cache, &interned_name);
        free_constructed_type(domain, inst_type);
        return NULL;
    }

    inst_type->base_type = instantiate_base_type(definition, args, arg_count);
    if (definition->base_type && !inst_type->base_type) {
        goto instantiation_failed;
    }

    if (definition->field_count != 0) {
        inst_type->fields = (iron_runtime_field_t **)iron_alloc(domain->allocator, definition->field_count * sizeof(iron_runtime_field_t *));
        if (!inst_type->fields) {
            goto instantiation_failed;
        }

        memset(inst_type->fields, 0, definition->field_count * sizeof(iron_runtime_field_t *));
        inst_type->field_count = definition->field_count;
        for (i = 0; i < definition->field_count; i++) {
            iron_runtime_field_t *field;

            field = (iron_runtime_field_t *)iron_alloc(domain->allocator, sizeof(iron_runtime_field_t));
            if (!field) {
                goto instantiation_failed;
            }

            memcpy(field, definition->fields[i], sizeof(iron_runtime_field_t));
            field->declaring_type = inst_type;
            if (!instantiate_field_signature(definition->module->assembly, definition->fields[i], args, arg_count, field)) {
                iron_free(domain->allocator, field, sizeof(iron_runtime_field_t));
                goto instantiation_failed;
            }

            inst_type->fields[i] = field;
        }
    }

    if (!instantiate_interfaces(inst_type, definition, args, arg_count)) {
        goto instantiation_failed;
    }

    if (definition->method_count != 0) {
        inst_type->methods = (iron_runtime_method_t **)iron_alloc(domain->allocator, definition->method_count * sizeof(iron_runtime_method_t *));
        if (!inst_type->methods) {
            goto instantiation_failed;
        }

        memset(inst_type->methods, 0, definition->method_count * sizeof(iron_runtime_method_t *));
        inst_type->method_count = definition->method_count;
        for (i = 0; i < definition->method_count; i++) {
            iron_runtime_method_t *method;

            method = (iron_runtime_method_t *)iron_alloc(domain->allocator, sizeof(iron_runtime_method_t));
            if (!method) {
                goto instantiation_failed;
            }

            memcpy(method, definition->methods[i], sizeof(iron_runtime_method_t));
            method->declaring_type = inst_type;
            method->return_type = NULL;
            method->params = NULL;
            method->signature_loading = IRON_FALSE;
            method->signature_loaded = IRON_FALSE;
            method->locals = NULL;
            method->local_count = 0;
            method->generic_params = NULL;
            method->generic_param_count = 0;
            if (!IRON_RESULT_OK(load_generic_parameters(definition->module->assembly, method->token, &method->generic_params, &method->generic_param_count))) {
                if (method->generic_params) {
                    free_generic_parameters(domain->allocator, method->generic_params, method->generic_param_count);
                }
                iron_free(domain->allocator, method, sizeof(iron_runtime_method_t));
                goto instantiation_failed;
            }
            {
                iron_u32 parameter_index;

                for (parameter_index = 0; parameter_index < method->generic_param_count; parameter_index++) {
                    method->generic_params[parameter_index]->declaring_method = method;
                }
            }
            method->is_generic_definition = method->generic_param_count != 0;
            inst_type->methods[i] = method;
            if (method->kind == IRON_METHOD_STATIC_CONSTRUCTOR || (method->name && strcmp(method->name, ".cctor") == 0)) {
                inst_type->type_initializer = method;
            }
        }
    }

    if (definition->property_count != 0) {
        inst_type->properties = (iron_runtime_property_t **)iron_alloc(domain->allocator, (iron_size)definition->property_count * sizeof(iron_runtime_property_t *));
        if (!inst_type->properties) {
            goto instantiation_failed;
        }

        memset(inst_type->properties, 0, (iron_size)definition->property_count * sizeof(iron_runtime_property_t *));
        inst_type->property_count = definition->property_count;
        for (i = 0; i < definition->property_count; i++) {
            iron_runtime_property_t *property;

            property = (iron_runtime_property_t *)iron_alloc(domain->allocator, sizeof(iron_runtime_property_t));
            if (!property) {
                goto instantiation_failed;
            }

            memcpy(property, definition->properties[i], sizeof(*property));
            property->declaring_type = inst_type;
            property->property_type = NULL;
            property->signature_loading = IRON_FALSE;
            property->signature_loaded = IRON_FALSE;
            property->index_params = NULL;
            property->index_param_count = 0;
            property->getter = find_instantiated_method(inst_type, definition->properties[i]->getter);
            property->setter = find_instantiated_method(inst_type, definition->properties[i]->setter);
            inst_type->properties[i] = property;
        }
    }

    if (definition->event_count != 0) {
        inst_type->events = (iron_runtime_event_t **)iron_alloc(domain->allocator, (iron_size)definition->event_count * sizeof(iron_runtime_event_t *));
        if (!inst_type->events) {
            goto instantiation_failed;
        }

        memset(inst_type->events, 0, (iron_size)definition->event_count * sizeof(iron_runtime_event_t *));
        inst_type->event_count = definition->event_count;
        for (i = 0; i < definition->event_count; i++) {
            iron_runtime_event_t *event;

            event = (iron_runtime_event_t *)iron_alloc(domain->allocator, sizeof(iron_runtime_event_t));
            if (!event) {
                goto instantiation_failed;
            }

            memcpy(event, definition->events[i], sizeof(*event));
            event->declaring_type = inst_type;
            event->event_type = resolve_event_type(definition->module->assembly, inst_type, event->event_type_token);
            event->add_method = find_instantiated_method(inst_type, definition->events[i]->add_method);
            event->remove_method = find_instantiated_method(inst_type, definition->events[i]->remove_method);
            event->raise_method = find_instantiated_method(inst_type, definition->events[i]->raise_method);
            if (!event->event_type) {
                iron_free(domain->allocator, event, sizeof(*event));
                goto instantiation_failed;
            }
            inst_type->events[i] = event;
        }
    }

    if (definition->nested_type_count != 0) {
        inst_type->nested_types = (iron_runtime_type_t **)iron_alloc(domain->allocator, (iron_size)definition->nested_type_count * sizeof(iron_runtime_type_t *));
        if (!inst_type->nested_types) {
            goto instantiation_failed;
        }
        memcpy(inst_type->nested_types, definition->nested_types, (iron_size)definition->nested_type_count * sizeof(iron_runtime_type_t *));
        inst_type->nested_type_count = definition->nested_type_count;
    }

    if (!IRON_RESULT_OK(iron_type_compute_layout(inst_type))) {
        goto instantiation_failed;
    }

    return inst_type;

instantiation_failed:
    interned_name = inst_type->name;
    iron_hashmap_remove(&domain->type_cache, &interned_name);
    free_constructed_type(domain, inst_type);
    return NULL;
}

iron_runtime_method_t *iron_type_find_method(iron_runtime_type_t *type,
                                              const char *name)
{
    iron_u32 i;
    iron_runtime_type_t *current;
    
    if (!type || !name) return NULL;
    
    /* Search in type hierarchy (current type first, then base types) */
    for (current = type; current != NULL; current = current->base_type) {
        /* Search in methods array */
        if (current->methods) {
            for (i = 0; i < current->method_count; i++) {
                if (current->methods[i] && current->methods[i]->name &&
                    strcmp(current->methods[i]->name, name) == 0) {
                    return current->methods[i];
                }
            }
        }
    }
    
    return NULL;
}

iron_runtime_field_t *iron_type_find_instance_field(iron_runtime_type_t *type, const char *name)
{
    iron_runtime_type_t *current;

    if (!type || !name) {
        return NULL;
    }

    for (current = type; current; current = current->base_type) {
        iron_u32 field_index;

        for (field_index = 0; field_index < current->field_count; field_index++) {
            iron_runtime_field_t *field;

            field = current->fields[field_index];
            if (field && (field->attrs & 0x0010) == 0 && field->name && strcmp(field->name, name) == 0) {
                return field;
            }
        }
    }

    return NULL;
}

static iron_result_t compute_static_field_layout(iron_runtime_type_t *type)
{
    iron_u32 static_offset;
    iron_u32 static_max_align;
    iron_u32 effective_packing;
    iron_u32 i;
    iron_u32 *old_layout;
    void *old_data;
    iron_u32 old_data_size;
    iron_allocator_t *allocator;
    iron_bool layout_changed;

    allocator = type->module && type->module->assembly ? type->module->assembly->allocator : NULL;
    old_data = type->static_data;
    old_data_size = type->static_data_size;
    old_layout = NULL;
    if (old_data && type->field_count != 0) {
        old_layout = (iron_u32 *)iron_alloc(allocator, (iron_size)type->field_count * 2U * sizeof(iron_u32));
        if (!old_layout) {
            return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to preserve the previous static field layout");
        }

        for (i = 0; i < type->field_count; i++) {
            old_layout[i * 2U] = type->fields[i] ? type->fields[i]->offset : 0;
            old_layout[i * 2U + 1U] = type->fields[i] ? type->fields[i]->size : 0;
        }
    }

    static_offset = 0;
    static_max_align = 1;
    effective_packing = type->packing_size != 0 ? type->packing_size : 8;

    for (i = 0; i < type->field_count; i++) {
        iron_runtime_field_t *field;
        iron_u32 field_size;
        iron_u32 field_align;

        field = type->fields[i];
        if (!field || (field->attrs & IRON_FIELD_STATIC) == 0 || (field->attrs & IRON_FIELD_LITERAL) != 0) {
            continue;
        }

        if (field->field_type && field->field_type != type &&
            (field->field_type->kind == IRON_KIND_VALUETYPE || field->field_type->kind == IRON_KIND_ENUM)) {
            iron_result_t field_layout_result;

            field_layout_result = iron_type_compute_layout(field->field_type);
            if (!IRON_RESULT_OK(field_layout_result)) {
                if (old_layout) {
                    iron_free(allocator, old_layout, (iron_size)type->field_count * 2U * sizeof(iron_u32));
                }
                return field_layout_result;
            }
        }

        field_size = field->field_type ? (iron_u32)iron_type_storage_size(field->field_type) : field->size;
        if (field_size == 0) {
            field_size = (iron_u32)sizeof(void *);
        }
        if (field->field_type && (field->field_type->kind == IRON_KIND_VALUETYPE || field->field_type->kind == IRON_KIND_ENUM)) {
            field_align = field->field_type->alignment;
        } else {
            field_align = field_size;
        }
        if (field_align == 0) {
            field_align = (iron_u32)sizeof(void *);
        }
        if (field_align > effective_packing) {
            field_align = effective_packing;
        }
        if (static_offset > UINT32_MAX - (field_align - 1)) {
            if (old_layout) {
                iron_free(allocator, old_layout, (iron_size)type->field_count * 2U * sizeof(iron_u32));
            }
            return IRON_ERROR(IRON_ERR_BUFFER_OVERFLOW, "Static field alignment exceeds the supported storage size");
        }

        static_offset = (static_offset + field_align - 1) & ~(field_align - 1);
        if (static_offset > UINT32_MAX - field_size) {
            if (old_layout) {
                iron_free(allocator, old_layout, (iron_size)type->field_count * 2U * sizeof(iron_u32));
            }
            return IRON_ERROR(IRON_ERR_BUFFER_OVERFLOW, "Static fields exceed the supported storage size");
        }

        field->offset = static_offset;
        field->size = field_size;
        static_offset += field_size;
        if (field_align > static_max_align) {
            static_max_align = field_align;
        }
    }

    if (static_offset != 0) {
        if (static_offset > UINT32_MAX - (static_max_align - 1)) {
            if (old_layout) {
                iron_free(allocator, old_layout, (iron_size)type->field_count * 2U * sizeof(iron_u32));
            }
            return IRON_ERROR(IRON_ERR_BUFFER_OVERFLOW, "Static field storage alignment exceeds the supported size");
        }

        static_offset = (static_offset + static_max_align - 1) & ~(static_max_align - 1);
        if (!allocator) {
            if (old_layout) {
                iron_free(allocator, old_layout, (iron_size)type->field_count * 2U * sizeof(iron_u32));
            }
            return IRON_ERROR(IRON_ERR_INVALID_STATE, "Type with static fields has no allocator");
        }

        layout_changed = old_data_size != static_offset;
        if (!layout_changed && old_layout) {
            for (i = 0; i < type->field_count; i++) {
                iron_runtime_field_t *field;

                field = type->fields[i];
                if (field && (field->attrs & IRON_FIELD_STATIC) != 0 && (field->attrs & IRON_FIELD_LITERAL) == 0 &&
                    (old_layout[i * 2U] != field->offset || old_layout[i * 2U + 1U] != field->size)) {
                    layout_changed = IRON_TRUE;
                    break;
                }
            }
        }

        if (old_data && !layout_changed) {
            iron_free(allocator, old_layout, (iron_size)type->field_count * 2U * sizeof(iron_u32));
            return IRON_SUCCESS;
        }

        type->static_data = iron_alloc(allocator, static_offset);
        if (!type->static_data) {
            type->static_data = old_data;
            if (old_layout) {
                iron_free(allocator, old_layout, (iron_size)type->field_count * 2U * sizeof(iron_u32));
            }
            return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate static field storage");
        }

        memset(type->static_data, 0, static_offset);
        type->static_data_size = static_offset;
        if (old_data && old_layout) {
            for (i = 0; i < type->field_count; i++) {
                iron_runtime_field_t *field;
                iron_u32 old_offset;
                iron_u32 old_size;
                iron_u32 copy_size;

                field = type->fields[i];
                if (!field || (field->attrs & IRON_FIELD_STATIC) == 0 || (field->attrs & IRON_FIELD_LITERAL) != 0) {
                    continue;
                }

                old_offset = old_layout[i * 2U];
                old_size = old_layout[i * 2U + 1U];
                copy_size = old_size < field->size ? old_size : field->size;
                if (old_offset <= old_data_size && copy_size <= old_data_size - old_offset &&
                    field->offset <= static_offset && copy_size <= static_offset - field->offset) {
                    memcpy((iron_u8 *)type->static_data + field->offset, (iron_u8 *)old_data + old_offset, copy_size);
                }
            }

            iron_free(allocator, old_data, old_data_size);
        }
    } else if (old_data) {
        iron_free(allocator, old_data, old_data_size);
        type->static_data = NULL;
        type->static_data_size = 0;
    }

    if (old_layout) {
        iron_free(allocator, old_layout, (iron_size)type->field_count * 2U * sizeof(iron_u32));
    }

    return IRON_SUCCESS;
}

iron_result_t iron_type_compute_layout(iron_runtime_type_t *type)
{
    iron_u32 offset = 0;
    iron_u32 max_align = 1;
    iron_u32 effective_packing;
    iron_bool explicit_layout;
    iron_u32 i;
    
    if (!type) return IRON_ERROR(IRON_ERR_NULL_POINTER, "Null type");

    if ((type->kind == IRON_KIND_VALUETYPE || type->kind == IRON_KIND_ENUM) &&
        type->element_type != IRON_TYPE_END && type->element_type != IRON_TYPE_VALUETYPE && type->element_type != IRON_TYPE_GENERICINST) {
        iron_size primitive_size;

        primitive_size = iron_element_type_size(type->element_type);
        if (primitive_size == 0 || primitive_size > UINT32_MAX) {
            return IRON_ERROR(IRON_ERR_INVALID_TYPE, "Primitive type has an invalid storage size");
        }

        type->instance_size = (iron_u32)primitive_size;
        type->alignment = (iron_u32)primitive_size;
        type->layout_computed = IRON_TRUE;
        return compute_static_field_layout(type);
    }
    
    explicit_layout = (type->attrs & IRON_TYPE_LAYOUT_MASK) == IRON_TYPE_EXPLICIT_LAYOUT;
    effective_packing = type->packing_size != 0 ? type->packing_size : 8;

    /* Value types do not embed the managed System.ValueType/Enum base object. */
    if (type->base_type && type->kind != IRON_KIND_VALUETYPE && type->kind != IRON_KIND_ENUM) {
        iron_result_t res;

        res = iron_type_compute_layout(type->base_type);
        if (!IRON_RESULT_OK(res)) return res;

        offset = type->base_type->instance_size;
        max_align = type->base_type->alignment;
    }
    
    /* Compute field offsets */
    for (i = 0; i < type->field_count; i++) {
        iron_runtime_field_t *field = type->fields[i];
        iron_u32 field_size;
        iron_u32 field_align;
        
        if (!field) continue;

        if (field->field_type && field->field_type != type &&
            (field->field_type->kind == IRON_KIND_VALUETYPE || field->field_type->kind == IRON_KIND_ENUM)) {
            iron_result_t field_layout_result;

            field_layout_result = iron_type_compute_layout(field->field_type);
            if (!IRON_RESULT_OK(field_layout_result)) {
                return field_layout_result;
            }
        }

        field_size = field->field_type ? (iron_u32)iron_type_storage_size(field->field_type) : field->size;
        if (field->field_type && (field->field_type->kind == IRON_KIND_VALUETYPE || field->field_type->kind == IRON_KIND_ENUM)) {
            field_align = field->field_type->alignment;
        } else {
            field_align = field_size;
        }

        if (field_size == 0) field_size = sizeof(void*);
        if (field_align == 0) field_align = sizeof(void*);

        if (field_align > effective_packing) {
            field_align = effective_packing;
        }

        if ((field->attrs & IRON_FIELD_STATIC) != 0) {
            continue;
        }
        
        if (explicit_layout) {
            if (field->offset > UINT32_MAX - field_size) {
                return IRON_ERROR(IRON_ERR_BUFFER_OVERFLOW, "Explicit field layout exceeds the supported type size");
            }
            if (offset < field->offset + field_size) {
                offset = field->offset + field_size;
            }
        } else {
            offset = (offset + field_align - 1) & ~(field_align - 1);
            field->offset = offset;
            field->size = field_size;
            offset += field_size;
        }
        
        if (field_align > max_align) {
            max_align = field_align;
        }
    }
    
    /* Final alignment */
    if (offset > UINT32_MAX - (max_align - 1)) {
        return IRON_ERROR(IRON_ERR_BUFFER_OVERFLOW, "Type layout exceeds the supported instance size");
    }
    type->instance_size = (offset + max_align - 1) & ~(max_align - 1);
    if (type->metadata_size > type->instance_size) {
        type->instance_size = type->metadata_size;
    }
    if (type->instance_size == 0) {
        type->instance_size = (type->kind == IRON_KIND_VALUETYPE || type->kind == IRON_KIND_ENUM) ? 1 : (iron_u32)sizeof(void *);
    }
    type->alignment = max_align;

    type->layout_computed = IRON_TRUE;
    return compute_static_field_layout(type);
}

iron_result_t iron_type_init_static(iron_runtime_type_t *type, iron_exec_context_t *ctx)
{
    iron_stack_value_t result;
    iron_result_t execution_result;

    if (!type || !ctx) {
        return IRON_ERROR(IRON_ERR_NULL_POINTER, "Invalid type initialization parameters");
    }

    if (type->static_initialized || type->static_initializing) {
        return IRON_SUCCESS;
    }

    type->static_initializing = IRON_TRUE;
    if (type->type_initializer) {
        memset(&result, 0, sizeof(result));
        result.type = IRON_VAL_VOID;
        execution_result = iron_exec_method(ctx, type->type_initializer, NULL, 0, &result);
        if (!IRON_RESULT_OK(execution_result)) {
            type->static_initializing = IRON_FALSE;
            return execution_result;
        }
    }

    type->static_initialized = IRON_TRUE;
    type->static_initializing = IRON_FALSE;
    return IRON_SUCCESS;
}

iron_runtime_method_t *iron_method_resolve_token(iron_module_t *module,
                                                 iron_token_t token)
{
    iron_assembly_t *assembly;
    
    if (!module || !module->assembly) return NULL;
    
    assembly = module->assembly;
    
    /* Use the existing iron_resolve_method_token which handles both MethodDef and MemberRef */
    return iron_resolve_method_token(assembly, token);
}

iron_runtime_method_t *iron_method_make_generic(iron_domain_t *domain,
                                                iron_runtime_method_t *definition,
                                                iron_runtime_type_t **args,
                                                iron_u32 arg_count)
{
    struct iron_generic_method_inst cache_key;
    iron_runtime_method_t *inst_method;
    iron_u32 i;

    if (!domain || !definition || !args || arg_count == 0 || !IRON_RESULT_OK(iron_method_validate_generic_arguments(definition, args, arg_count))) {
        return NULL;
    }

    cache_key.definition = definition;
    cache_key.args = args;
    cache_key.arg_count = arg_count;
    if (iron_hashmap_get(&domain->generic_method_cache, &cache_key, &inst_method)) {
        return inst_method;
    }

    inst_method = (iron_runtime_method_t *)iron_alloc(domain->allocator, sizeof(iron_runtime_method_t));
    if (!inst_method) {
        return NULL;
    }

    memcpy(inst_method, definition, sizeof(iron_runtime_method_t));
    inst_method->generic_definition = definition;
    inst_method->is_generic_instance = IRON_TRUE;
    inst_method->is_generic_definition = IRON_FALSE;
    inst_method->return_type = NULL;
    inst_method->params = NULL;
    inst_method->signature_loading = IRON_FALSE;
    inst_method->signature_loaded = IRON_FALSE;
    inst_method->locals = NULL;
    inst_method->local_count = 0;
    inst_method->generic_params = NULL;
    inst_method->generic_param_count = 0;

    inst_method->generic_args = (iron_runtime_type_t **)iron_alloc(domain->allocator, (iron_size)arg_count * sizeof(iron_runtime_type_t *));
    if (!inst_method->generic_args) {
        iron_free(domain->allocator, inst_method, sizeof(iron_runtime_method_t));
        return NULL;
    }

    for (i = 0; i < arg_count; i++) {
        inst_method->generic_args[i] = args[i];
    }
    inst_method->generic_arg_count = arg_count;

    cache_key.args = inst_method->generic_args;
    if (!iron_hashmap_set(&domain->generic_method_cache, &cache_key, &inst_method)) {
        iron_free(domain->allocator, inst_method->generic_args, (iron_size)arg_count * sizeof(iron_runtime_type_t *));
        iron_free(domain->allocator, inst_method, sizeof(iron_runtime_method_t));
        return NULL;
    }

    return inst_method;
}

iron_result_t iron_method_load_signature(iron_runtime_method_t *method)
{
    iron_assembly_t *assembly;
    iron_method_def_row_t row;
    iron_result_t result;

    if (!method) {
        return IRON_ERROR(IRON_ERR_NULL_POINTER, "Null method");
    }
    if (method->signature_loaded) {
        return IRON_SUCCESS;
    }
    if (method->signature_loading) {
        return IRON_ERROR(IRON_ERR_INVALID_STATE, "Recursive method signature resolution");
    }
    if (!method->declaring_type || !method->declaring_type->module || !method->declaring_type->module->assembly || IRON_TOKEN_TABLE(method->token) != IRON_TABLE_METHOD_DEF) {
        return IRON_ERROR(IRON_ERR_INVALID_STATE, "Method has no MethodDef signature context");
    }

    assembly = method->declaring_type->module->assembly;
    result = iron_metadata_read_row(&assembly->metadata, method->token, &row);
    if (!IRON_RESULT_OK(result)) {
        return result;
    }

    method->signature_loading = IRON_TRUE;
    result = load_method_signature(assembly, method, &row);
    method->signature_loading = IRON_FALSE;
    if (!IRON_RESULT_OK(result)) {
        method->return_type = NULL;
        return result;
    }

    method->signature_loaded = IRON_TRUE;
    return IRON_SUCCESS;
}

iron_result_t iron_method_load_body(iron_runtime_method_t *method)
{
    iron_assembly_t *assembly;
    iron_method_def_row_t row;
    iron_result_t res;
    
    if (!method) {
        return IRON_ERROR(IRON_ERR_NULL_POINTER, "Null method");
    }
    
    /* Already loaded? */
    if (method->body) {
        return IRON_SUCCESS;
    }
    
    /* Internal calls don't have IL bodies */
    if (method->is_internal_call) {
        return IRON_SUCCESS;
    }
    
    /* Find assembly from declaring type */
    if (!method->declaring_type || !method->declaring_type->module ||
        !method->declaring_type->module->assembly) {
        return IRON_ERROR(IRON_ERR_INVALID_STATE, "Method has no assembly context");
    }
    
    assembly = method->declaring_type->module->assembly;
    
    /* Read method row to get RVA */
    res = iron_metadata_read_row(&assembly->metadata, method->token, &row);
    if (!IRON_RESULT_OK(res)) {
        return res;
    }
    
    if (row.rva == 0) {
        return IRON_ERROR(IRON_ERR_NOT_FOUND, "Method has no IL body");
    }
    
    /* Parse method body */
    method->body = (iron_method_body_t *)iron_alloc(
        assembly->domain->allocator, sizeof(iron_method_body_t));
    if (!method->body) {
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate method body");
    }
    
    res = iron_parse_method_body(&assembly->image, row.rva, 
                                  method->body, assembly->domain->allocator);
    if (!IRON_RESULT_OK(res)) {
        iron_free(assembly->domain->allocator, method->body, sizeof(iron_method_body_t));
        method->body = NULL;
        return res;
    }
    
    if (method->is_generic_definition || (method->declaring_type && iron_type_contains_generic_parameters(method->declaring_type))) {
        return IRON_SUCCESS;
    }

    return load_method_local_signature(assembly, method);
}

iron_runtime_field_t *iron_field_resolve_token(iron_module_t *module,
                                               iron_token_t token)
{
    iron_u32 table_id;
    iron_u32 row_index;
    iron_assembly_t *assembly;
    
    if (!module || !module->assembly) return NULL;
    
    assembly = module->assembly;
    table_id = (token >> 24) & 0xFF;
    row_index = token & 0x00FFFFFF;
    
    if (row_index == 0) return NULL;
    
    if (table_id == IRON_TABLE_FIELD) {
        iron_field_row_t field_row;
        iron_result_t res;
        iron_runtime_field_t *field;

        if (!assembly->fields) {
            assembly->field_count = iron_metadata_table_rows(&assembly->metadata, IRON_TABLE_FIELD);
            assembly->fields = (iron_runtime_field_t *)iron_alloc(assembly->allocator, assembly->field_count * sizeof(iron_runtime_field_t));
            if (!assembly->fields) {
                assembly->field_count = 0;
                return NULL;
            }

            memset(assembly->fields, 0, assembly->field_count * sizeof(iron_runtime_field_t));
        }

        if (row_index > assembly->field_count) {
            return NULL;
        }

        field = &assembly->fields[row_index - 1];
        if (field->name) {
            return field;
        }

        res = iron_metadata_read_row(&assembly->metadata, token, &field_row);
        if (!IRON_RESULT_OK(res)) {
            return NULL;
        }

        field->token = token;
        field->name = iron_metadata_get_string(&assembly->metadata, field_row.name);
        field->attrs = field_row.flags;
        load_field_signature(assembly, field, field_row.signature);
        load_field_constant(assembly, field, row_index);
        load_field_rva_data(assembly, field, row_index);

        {
            iron_u32 type_count;
            iron_u32 type_index;

            type_count = iron_metadata_table_rows(&assembly->metadata, IRON_TABLE_TYPE_DEF);
            for (type_index = 1; type_index <= type_count; type_index++) {
                iron_type_def_row_t type_row;
                iron_u32 field_start;
                iron_u32 field_end;

                if (!IRON_RESULT_OK(iron_metadata_read_row(&assembly->metadata, IRON_MAKE_TOKEN(IRON_TABLE_TYPE_DEF, type_index), &type_row))) {
                    continue;
                }

                field_start = type_row.field_list;
                if (type_index < type_count) {
                    iron_type_def_row_t next_type_row;

                    if (IRON_RESULT_OK(iron_metadata_read_row(&assembly->metadata, IRON_MAKE_TOKEN(IRON_TABLE_TYPE_DEF, type_index + 1), &next_type_row))) {
                        field_end = next_type_row.field_list;
                    } else {
                        field_end = assembly->field_count + 1;
                    }
                } else {
                    field_end = assembly->field_count + 1;
                }

                if (row_index >= field_start && row_index < field_end) {
                    field->declaring_type = iron_type_resolve_token(module, IRON_MAKE_TOKEN(IRON_TABLE_TYPE_DEF, type_index));
                    break;
                }
            }
        }

        return field;
    } else if (table_id == IRON_TABLE_MEMBER_REF) {
        iron_member_ref_row_t member_ref;
        iron_result_t res;
        const char *field_name;
        iron_u32 class_token;
        iron_u32 class_table;

        res = iron_metadata_read_row(&assembly->metadata, token, &member_ref);
        if (!IRON_RESULT_OK(res)) return NULL;

        field_name = iron_metadata_get_string(&assembly->metadata, member_ref.name);
        class_token = iron_metadata_decode_coded(&assembly->metadata,
                                                  IRON_CODED_MEMBER_REF_PARENT,
                                                  member_ref.class_);
        class_table = (class_token >> 24) & 0xFF;

        if (class_table == IRON_TABLE_TYPE_REF || class_table == IRON_TABLE_TYPE_DEF) {
            iron_runtime_type_t *type = iron_type_resolve_token(module, class_token);
            iron_runtime_type_t *current;

            for (current = type; current; current = current->base_type) {
                iron_u32 field_index;

                for (field_index = 0; field_index < current->field_count; field_index++) {
                    iron_runtime_field_t *candidate;

                    candidate = current->fields[field_index];
                    if (candidate && candidate->name && strcmp(candidate->name, field_name) == 0) {
                        return candidate;
                    }
                }
            }
        } else if (class_table == IRON_TABLE_TYPE_SPEC) {
            iron_runtime_type_t *type;
            iron_u32 field_index;

            type = resolve_type_spec_definition(assembly, class_token);
            if (type) {
                for (field_index = 0; field_index < type->field_count; field_index++) {
                    iron_runtime_field_t *candidate;

                    candidate = type->fields[field_index];
                    if (candidate && candidate->name && strcmp(candidate->name, field_name) == 0) {
                        return candidate;
                    }
                }
            }
        }
    }
    
    return NULL;
}

static iron_u32 property_list_metadata_index(iron_assembly_t *assembly, iron_u32 list_index)
{
    if (iron_metadata_table_rows(&assembly->metadata, IRON_TABLE_PROPERTY_PTR) != 0) {
        iron_property_ptr_row_t pointer_row;

        if (!IRON_RESULT_OK(iron_metadata_read_row(&assembly->metadata, IRON_MAKE_TOKEN(IRON_TABLE_PROPERTY_PTR, list_index), &pointer_row))) {
            return 0;
        }
        return pointer_row.property;
    }

    return list_index;
}

iron_runtime_property_t *iron_property_resolve_token(iron_module_t *module, iron_token_t token)
{
    iron_assembly_t *assembly;
    iron_runtime_property_t *property;
    iron_property_row_t row;
    iron_u32 row_index;
    iron_u32 row_count;
    iron_u32 map_count;
    iron_u32 map_index;
    iron_result_t result;

    if (!module || !module->assembly || IRON_TOKEN_TABLE(token) != IRON_TABLE_PROPERTY || IRON_TOKEN_INDEX(token) == 0) {
        return NULL;
    }

    assembly = module->assembly;
    row_index = IRON_TOKEN_INDEX(token);
    row_count = iron_metadata_table_rows(&assembly->metadata, IRON_TABLE_PROPERTY);
    if (row_index > row_count) {
        return NULL;
    }

    if (!assembly->properties) {
        assembly->properties = (iron_runtime_property_t *)iron_alloc(assembly->allocator, (iron_size)row_count * sizeof(iron_runtime_property_t));
        if (!assembly->properties) {
            return NULL;
        }
        memset(assembly->properties, 0, (iron_size)row_count * sizeof(iron_runtime_property_t));
        assembly->property_count = row_count;
    }

    property = &assembly->properties[row_index - 1];
    if (property->name) {
        return property;
    }

    result = iron_metadata_read_row(&assembly->metadata, token, &row);
    if (!IRON_RESULT_OK(result)) {
        return NULL;
    }
    property->token = token;
    property->name = iron_metadata_get_string(&assembly->metadata, row.name);
    property->attrs = row.flags;

    map_count = iron_metadata_table_rows(&assembly->metadata, IRON_TABLE_PROPERTY_MAP);
    for (map_index = 1; map_index <= map_count; map_index++) {
        iron_property_map_row_t map;
        iron_u32 list_start;
        iron_u32 list_end;
        iron_u32 list_index;

        if (!IRON_RESULT_OK(iron_metadata_read_row(&assembly->metadata, IRON_MAKE_TOKEN(IRON_TABLE_PROPERTY_MAP, map_index), &map))) {
            continue;
        }
        list_start = map.property_list;
        if (map_index < map_count) {
            iron_property_map_row_t next_map;

            if (!IRON_RESULT_OK(iron_metadata_read_row(&assembly->metadata, IRON_MAKE_TOKEN(IRON_TABLE_PROPERTY_MAP, map_index + 1), &next_map))) {
                continue;
            }
            list_end = next_map.property_list;
        } else {
            iron_u32 pointer_count;

            pointer_count = iron_metadata_table_rows(&assembly->metadata, IRON_TABLE_PROPERTY_PTR);
            list_end = (pointer_count != 0 ? pointer_count : row_count) + 1;
        }

        for (list_index = list_start; list_index < list_end; list_index++) {
            if (property_list_metadata_index(assembly, list_index) == row_index) {
                property->declaring_type = iron_type_resolve_token(module, IRON_MAKE_TOKEN(IRON_TABLE_TYPE_DEF, map.parent));
                break;
            }
        }
        if (property->declaring_type) {
            break;
        }
    }
    if (!property->declaring_type) {
        property->name = NULL;
        return NULL;
    }

    {
        iron_u32 semantics_count;
        iron_u32 semantics_index;

        semantics_count = iron_metadata_table_rows(&assembly->metadata, IRON_TABLE_METHOD_SEMANTICS);
        for (semantics_index = 1; semantics_index <= semantics_count; semantics_index++) {
            iron_method_semantics_row_t semantics;
            iron_token_t association;
            iron_runtime_method_t *accessor;

            if (!IRON_RESULT_OK(iron_metadata_read_row(&assembly->metadata, IRON_MAKE_TOKEN(IRON_TABLE_METHOD_SEMANTICS, semantics_index), &semantics))) {
                continue;
            }
            association = iron_metadata_decode_coded(&assembly->metadata, IRON_CODED_HAS_SEMANTICS, semantics.association);
            if (association != token) {
                continue;
            }

            accessor = iron_method_resolve_token(module, IRON_MAKE_TOKEN(IRON_TABLE_METHOD_DEF, semantics.method));
            if (!accessor) {
                property->name = NULL;
                return NULL;
            }
            if ((semantics.semantics & 0x0002) != 0) {
                property->getter = accessor;
                accessor->kind = IRON_METHOD_PROPERTY_GET;
            }
            if ((semantics.semantics & 0x0001) != 0) {
                property->setter = accessor;
                accessor->kind = IRON_METHOD_PROPERTY_SET;
            }
        }
    }

    return property;
}

iron_runtime_method_t *iron_type_find_method_implementation(iron_runtime_type_t *type, const iron_runtime_method_t *contract)
{
    iron_assembly_t *assembly;
    iron_u32 row_count;
    iron_u32 row_index;
    const iron_runtime_method_t *contract_definition;

    if (!type || !type->module || !type->module->assembly || !contract || !contract->declaring_type) {
        return NULL;
    }

    assembly = type->module->assembly;
    contract_definition = contract->is_generic_instance ? contract->generic_definition : contract;
    row_count = iron_metadata_table_rows(&assembly->metadata, IRON_TABLE_METHOD_IMPL);

    for (row_index = 1; row_index <= row_count; row_index++) {
        iron_method_impl_row_t row;
        iron_token_t declaration_token;
        iron_token_t body_token;
        iron_runtime_method_t *declaration;
        iron_runtime_type_t *owner;
        iron_runtime_method_t *body;
        iron_u32 method_index;

        if (!IRON_RESULT_OK(iron_metadata_read_row(&assembly->metadata, IRON_MAKE_TOKEN(IRON_TABLE_METHOD_IMPL, row_index), &row)) ||
            row.class_ != IRON_TOKEN_INDEX(type->token)) {
            continue;
        }

        declaration_token = iron_metadata_decode_coded(&assembly->metadata, IRON_CODED_METHOD_DEF_OR_REF, row.method_declaration);
        declaration = iron_resolve_method_token(assembly, declaration_token);
        if (!declaration || !contract_definition || declaration->token != contract_definition->token ||
            declaration->declaring_type->module != contract_definition->declaring_type->module) {
            continue;
        }

        owner = declaration->declaring_type;
        if (IRON_TOKEN_TABLE(declaration_token) == IRON_TABLE_MEMBER_REF) {
            iron_member_ref_row_t member_ref;
            iron_token_t parent_token;

            if (!IRON_RESULT_OK(iron_metadata_read_row(&assembly->metadata, declaration_token, &member_ref))) {
                continue;
            }

            parent_token = iron_metadata_decode_coded(&assembly->metadata, IRON_CODED_MEMBER_REF_PARENT, member_ref.class_);
            if (IRON_TOKEN_TABLE(parent_token) == IRON_TABLE_TYPE_SPEC) {
                iron_runtime_type_t **arguments;
                iron_u32 argument_count;

                arguments = type->is_generic_instance ? type->generic_args : type->generic_params;
                argument_count = type->is_generic_instance ? type->generic_arg_count : type->generic_param_count;
                owner = resolve_type_spec_arguments(assembly, parent_token, arguments, argument_count);
            }
        }

        if (owner != contract->declaring_type) {
            continue;
        }

        body_token = iron_metadata_decode_coded(&assembly->metadata, IRON_CODED_METHOD_DEF_OR_REF, row.method_body);
        body = iron_resolve_method_token(assembly, body_token);
        if (!body) {
            continue;
        }

        for (method_index = 0; method_index < type->method_count; method_index++) {
            iron_runtime_method_t *candidate;

            candidate = type->methods[method_index];
            if (candidate && candidate->token == body->token && candidate->declaring_type->module == body->declaring_type->module) {
                if (contract->is_generic_instance && candidate->is_generic_definition) {
                    return iron_method_make_generic(assembly->domain, candidate, contract->generic_args, contract->generic_arg_count);
                }

                return candidate;
            }
        }
    }

    return NULL;
}

static iron_u32 event_list_metadata_index(iron_assembly_t *assembly, iron_u32 list_index)
{
    if (iron_metadata_table_rows(&assembly->metadata, IRON_TABLE_EVENT_PTR) != 0) {
        iron_event_ptr_row_t pointer_row;

        if (!IRON_RESULT_OK(iron_metadata_read_row(&assembly->metadata, IRON_MAKE_TOKEN(IRON_TABLE_EVENT_PTR, list_index), &pointer_row))) {
            return 0;
        }
        return pointer_row.event;
    }

    return list_index;
}

static iron_runtime_type_t *resolve_event_type(iron_assembly_t *assembly,
                                               iron_runtime_type_t *declaring_type,
                                               iron_token_t type_token)
{
    iron_runtime_type_t **type_arguments;
    iron_u32 type_argument_count;

    if (IRON_TOKEN_TABLE(type_token) != IRON_TABLE_TYPE_SPEC) {
        return iron_type_resolve_token(assembly->module, type_token);
    }

    type_arguments = declaring_type->is_generic_instance ? declaring_type->generic_args : declaring_type->generic_params;
    type_argument_count = declaring_type->is_generic_instance ? declaring_type->generic_arg_count : declaring_type->generic_param_count;
    return resolve_type_spec_arguments(assembly, type_token, type_arguments, type_argument_count);
}

iron_runtime_event_t *iron_event_resolve_token(iron_module_t *module, iron_token_t token)
{
    iron_assembly_t *assembly;
    iron_runtime_event_t *event;
    iron_event_row_t row;
    iron_u32 row_index;
    iron_u32 row_count;
    iron_u32 map_count;
    iron_u32 map_index;
    iron_result_t result;

    if (!module || !module->assembly || IRON_TOKEN_TABLE(token) != IRON_TABLE_EVENT || IRON_TOKEN_INDEX(token) == 0) {
        return NULL;
    }

    assembly = module->assembly;
    row_index = IRON_TOKEN_INDEX(token);
    row_count = iron_metadata_table_rows(&assembly->metadata, IRON_TABLE_EVENT);
    if (row_index > row_count) {
        return NULL;
    }

    if (!assembly->events) {
        assembly->events = (iron_runtime_event_t *)iron_alloc(assembly->allocator, (iron_size)row_count * sizeof(iron_runtime_event_t));
        if (!assembly->events) {
            return NULL;
        }
        memset(assembly->events, 0, (iron_size)row_count * sizeof(iron_runtime_event_t));
        assembly->event_count = row_count;
    }

    event = &assembly->events[row_index - 1];
    if (event->name) {
        return event;
    }

    result = iron_metadata_read_row(&assembly->metadata, token, &row);
    if (!IRON_RESULT_OK(result)) {
        return NULL;
    }
    event->token = token;
    event->name = iron_metadata_get_string(&assembly->metadata, row.name);
    event->attrs = row.event_flags;

    map_count = iron_metadata_table_rows(&assembly->metadata, IRON_TABLE_EVENT_MAP);
    for (map_index = 1; map_index <= map_count; map_index++) {
        iron_event_map_row_t map;
        iron_u32 list_start;
        iron_u32 list_end;
        iron_u32 list_index;

        if (!IRON_RESULT_OK(iron_metadata_read_row(&assembly->metadata, IRON_MAKE_TOKEN(IRON_TABLE_EVENT_MAP, map_index), &map))) {
            continue;
        }
        list_start = map.event_list;
        if (map_index < map_count) {
            iron_event_map_row_t next_map;

            if (!IRON_RESULT_OK(iron_metadata_read_row(&assembly->metadata, IRON_MAKE_TOKEN(IRON_TABLE_EVENT_MAP, map_index + 1), &next_map))) {
                continue;
            }
            list_end = next_map.event_list;
        } else {
            iron_u32 pointer_count;

            pointer_count = iron_metadata_table_rows(&assembly->metadata, IRON_TABLE_EVENT_PTR);
            list_end = (pointer_count != 0 ? pointer_count : row_count) + 1;
        }

        for (list_index = list_start; list_index < list_end; list_index++) {
            if (event_list_metadata_index(assembly, list_index) == row_index) {
                event->declaring_type = iron_type_resolve_token(module, IRON_MAKE_TOKEN(IRON_TABLE_TYPE_DEF, map.parent));
                break;
            }
        }
        if (event->declaring_type) {
            break;
        }
    }
    if (!event->declaring_type) {
        event->name = NULL;
        return NULL;
    }

    event->event_type_token = iron_metadata_decode_coded(&assembly->metadata, IRON_CODED_TYPE_DEF_OR_REF, row.event_type);
    event->event_type = resolve_event_type(assembly, event->declaring_type, event->event_type_token);
    if (!event->event_type) {
        event->name = NULL;
        return NULL;
    }

    {
        iron_u32 semantics_count;
        iron_u32 semantics_index;

        semantics_count = iron_metadata_table_rows(&assembly->metadata, IRON_TABLE_METHOD_SEMANTICS);
        for (semantics_index = 1; semantics_index <= semantics_count; semantics_index++) {
            iron_method_semantics_row_t semantics;
            iron_token_t association;
            iron_runtime_method_t *accessor;

            if (!IRON_RESULT_OK(iron_metadata_read_row(&assembly->metadata, IRON_MAKE_TOKEN(IRON_TABLE_METHOD_SEMANTICS, semantics_index), &semantics))) {
                continue;
            }
            association = iron_metadata_decode_coded(&assembly->metadata, IRON_CODED_HAS_SEMANTICS, semantics.association);
            if (association != token) {
                continue;
            }

            accessor = iron_method_resolve_token(module, IRON_MAKE_TOKEN(IRON_TABLE_METHOD_DEF, semantics.method));
            if (!accessor) {
                event->name = NULL;
                return NULL;
            }
            if ((semantics.semantics & 0x0008) != 0) {
                event->add_method = accessor;
                accessor->kind = IRON_METHOD_EVENT_ADD;
            }
            if ((semantics.semantics & 0x0010) != 0) {
                event->remove_method = accessor;
                accessor->kind = IRON_METHOD_EVENT_REMOVE;
            }
            if ((semantics.semantics & 0x0020) != 0) {
                event->raise_method = accessor;
                accessor->kind = IRON_METHOD_EVENT_RAISE;
            }
        }
    }

    return event;
}

void *iron_field_get_address(iron_runtime_field_t *field, void *instance)
{
    if (!field) return NULL;
    
    if (field->attrs & 0x0010) { /* Static */
        if (field->declaring_type && field->declaring_type->static_data) {
            return (char *)field->declaring_type->static_data + field->offset;
        }
        return NULL;
    }
    
    if (!instance) return NULL;
    return (char *)instance + field->offset;
}

/* ============================================================================
 * Method Execution
 * ============================================================================ */

iron_result_t iron_exec_method(iron_exec_context_t *ctx,
                               iron_runtime_method_t *method,
                               iron_stack_value_t *args,
                               iron_u32 arg_count,
                               iron_stack_value_t *result)
{
    iron_internal_call_fn icall;
    
    if (!ctx || !method) {
        return IRON_ERROR(IRON_ERR_NULL_POINTER, "Invalid context or method");
    }
    
    if (result) {
        memset(result, 0, sizeof(*result));
        result->type = IRON_VAL_VOID;
    }
    
    /* Check if this is an internal call */
    if (method->is_internal_call) {
        icall = iron_lookup_internal_call(ctx, method);
        if (icall) {
            return icall(ctx, args, arg_count, result);
        }
        /* Try to find by name pattern */
        return IRON_ERROR(IRON_ERR_NOT_FOUND, "Internal call not registered");
    }
    
    /* Check if method has IL body */
    if (!method->body) {
        /* Try to load method body from assembly */
        if (method->token != 0) {
            iron_assembly_t *assembly = NULL;
            iron_method_def_row_t row;
            iron_result_t res;
            
            /* Find assembly from declaring type's module or search loaded assemblies */
            if (method->declaring_type && method->declaring_type->module &&
                method->declaring_type->module->assembly) {
                assembly = method->declaring_type->module->assembly;
            } else if (ctx->domain && ctx->domain->assembly_count > 0) {
                /* Try to find assembly that contains this method */
                iron_u32 i;
                for (i = 0; i < ctx->domain->assembly_count; i++) {
                    if (ctx->domain->assemblies[i]->methods == NULL) continue;
                    /* Check if method belongs to this assembly */
                    if (method >= ctx->domain->assemblies[i]->methods &&
                        method < ctx->domain->assemblies[i]->methods + 
                                 ctx->domain->assemblies[i]->method_count) {
                        assembly = ctx->domain->assemblies[i];
                        break;
                    }
                }
            }
            
            if (assembly) {
                res = iron_metadata_read_row(&assembly->metadata, method->token, &row);
                if (IRON_RESULT_OK(res) && row.rva != 0) {
                    /* Allocate and parse method body using existing function */
                    method->body = (iron_method_body_t *)iron_alloc(
                        ctx->allocator, sizeof(iron_method_body_t));
                    if (method->body) {
                        res = iron_parse_method_body(&assembly->image, row.rva,
                                                     method->body, ctx->allocator);
                        if (!IRON_RESULT_OK(res)) {
                            iron_free(ctx->allocator, method->body, sizeof(iron_method_body_t));
                            method->body = NULL;
                        }
                    }
                }
            }
        }
    }
    
    if (!method->body || !method->body->code) {
        return IRON_ERROR(IRON_ERR_NOT_IMPLEMENTED, "Method has no IL body");
    }
    
    /* Execute IL code using interpreter */
    {
        iron_thread_context_t *thread = iron_exec_get_current_thread(ctx);
        iron_stack_frame_t frame;
        iron_interp_result_t interp_result;
        iron_u32 instruction_ip;
        iron_u32 i;
        iron_u32 stack_base; /* Save stack size before method execution */
        iron_bool cleanup_execution_acquired;
        
        if (!thread) {
            return IRON_ERROR(IRON_ERR_INVALID_STATE, "No thread context");
        }
        
        /* Save current stack size - this is the "base" for this method call */
        stack_base = thread->eval_stack.size;
        
        /* Set up stack frame */
        memset(&frame, 0, sizeof(frame));
        frame.method = method;
        frame.code = method->body->code;
        frame.code_size = method->body->code_size;
        frame.ip = 0;
        frame.stack_base = stack_base;
        frame.prev = thread->current_frame;
        frame.arena_mark = iron_arena_save(&thread->frame_arena);
        
        /* Load and allocate strongly typed local slots. */
        {
            iron_assembly_t *assembly;
            iron_result_t local_result;

            assembly = method->declaring_type && method->declaring_type->module ? method->declaring_type->module->assembly : NULL;
            if (method->body->local_var_sig_token != 0 && !method->locals) {
                local_result = load_method_local_signature(assembly, method);
                if (!IRON_RESULT_OK(local_result)) {
                    return local_result;
                }
            }

            if (method->local_count != 0) {
                frame.locals = (iron_stack_value_t *)iron_alloc(ctx->allocator, (iron_size)method->local_count * sizeof(iron_stack_value_t));
                if (!frame.locals) {
                    return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate method local values");
                }
                memset(frame.locals, 0, (iron_size)method->local_count * sizeof(iron_stack_value_t));
                frame.local_count = method->local_count;
            }
        }
        
        /* Copy arguments */
        if (arg_count > 0 && args) {
            frame.args = (iron_stack_value_t *)iron_alloc(
                ctx->allocator, arg_count * sizeof(iron_stack_value_t));
            if (!frame.args) {
                if (frame.locals) {
                    iron_free(ctx->allocator, frame.locals, (iron_size)frame.local_count * sizeof(iron_stack_value_t));
                }
                return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate method argument values");
            }
            for (i = 0; i < arg_count; i++) {
                frame.args[i] = args[i];
            }
            frame.arg_count = arg_count;
        }
        
        /* Push frame */
        thread->current_frame = &frame;

        for (i = 0; i < frame.local_count; i++) {
            if (!method->locals || !method->locals[i] || !iron_stack_value_init_default(ctx, &frame.locals[i], method->locals[i])) {
                IRON_ERROR_EXEC("Failed to initialize local %u (%s) in method %s", i,
                                method->locals && method->locals[i] && method->locals[i]->full_name ? method->locals[i]->full_name : "<unknown>",
                                method->name ? method->name : "<unknown>");
                thread->current_frame = frame.prev;
                if (frame.locals) {
                    iron_free(ctx->allocator, frame.locals, (iron_size)frame.local_count * sizeof(iron_stack_value_t));
                }
                if (frame.args) {
                    iron_free(ctx->allocator, frame.args, (iron_size)frame.arg_count * sizeof(iron_stack_value_t));
                }
                return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to initialize method local values");
            }
        }
        
        /* Execute instructions until return or an exception that has no handler
         * in this frame. Each iteration retains the throwing instruction offset,
         * because call instructions save their continuation before invoking the
         * target and the continuation may be outside the protected region. */
        for (;;) {
            iron_bool execution_acquired;

            instruction_ip = frame.ip;
            execution_acquired = iron_exec_enter_execution(ctx);
            interp_result = iron_exec_instruction(thread);
            if (interp_result == IRON_INTERP_OK || interp_result == IRON_INTERP_BRANCH) {
                iron_exec_leave_execution(ctx, execution_acquired);
                continue;
            }

            if (interp_result == IRON_INTERP_EXCEPTION && thread->exception_state.current_exception) {
                iron_stack_frame_t *handler_frame;
                iron_u32 handler_index;
                iron_u32 continuation_ip;
                iron_u32 search_offset;

                if ((frame.flags & IRON_FRAME_FILTER) != 0 && frame.filter_exception) {
                    thread->exception_state.current_exception = frame.filter_exception;
                    frame.filter_exception = NULL;
                    frame.flags &= ~IRON_FRAME_FILTER;
                    frame.flags |= IRON_FRAME_FILTER_REJECTED;
                    thread->eval_stack.size = stack_base;
                }

                continuation_ip = frame.ip;
                search_offset = (frame.flags & IRON_FRAME_FILTER_REJECTED) != 0 ? frame.exception_search_offset : instruction_ip;
                frame.ip = search_offset;
                handler_frame = NULL;
                handler_index = 0;
                if (iron_find_exception_handler(thread, thread->exception_state.current_exception, &handler_frame, &handler_index) && handler_frame == &frame) {
                    iron_exception_clause_t *clause;
                    iron_u32 clause_kind;
                    iron_stack_value_t exception_value;

                    clause = &method->body->exceptions[handler_index];
                    clause_kind = clause->flags & (IRON_EX_CLAUSE_FILTER | IRON_EX_CLAUSE_FINALLY | IRON_EX_CLAUSE_FAULT);
                    thread->eval_stack.size = stack_base;
                    iron_exec_discard_finally(thread, clause_kind == IRON_EX_CLAUSE_FILTER ? clause->u.filter_offset : clause->handler_offset);
                    frame.exception_handler_index = handler_index;
                    if (clause_kind == IRON_EX_CLAUSE_FILTER) {
                        memset(&exception_value, 0, sizeof(exception_value));
                        exception_value.type = IRON_VAL_OBJ;
                        exception_value.value.obj = thread->exception_state.current_exception;
                        iron_stack_push(&thread->eval_stack, exception_value);
                        frame.exception_search_offset = search_offset;
                        frame.exception_filter_try_length = clause->try_length;
                        frame.filter_exception = thread->exception_state.current_exception;
                        frame.flags &= ~IRON_FRAME_FILTER_REJECTED;
                        frame.flags |= IRON_FRAME_FILTER;
                        frame.ip = clause->u.filter_offset;
                    } else if (clause_kind == IRON_EX_CLAUSE_FINALLY || clause_kind == IRON_EX_CLAUSE_FAULT) {
                        frame.filter_exception = NULL;
                        frame.flags &= ~IRON_FRAME_FILTER_REJECTED;
                        if (!iron_exec_enter_finally(thread, handler_index, 0, 0, thread->exception_state.current_exception)) {
                            interp_result = IRON_INTERP_ERROR;
                            iron_exec_leave_execution(ctx, execution_acquired);
                            break;
                        }
                    } else {
                        memset(&exception_value, 0, sizeof(exception_value));
                        exception_value.type = IRON_VAL_OBJ;
                        exception_value.value.obj = thread->exception_state.current_exception;
                        iron_stack_push(&thread->eval_stack, exception_value);
                        frame.filter_exception = NULL;
                        frame.flags &= ~IRON_FRAME_FILTER_REJECTED;
                        frame.flags |= IRON_FRAME_EXCEPTION;
                        frame.ip = clause->handler_offset;
                    }
                    interp_result = IRON_INTERP_OK;
                    iron_exec_leave_execution(ctx, execution_acquired);
                    continue;
                }
                frame.ip = continuation_ip;
            }

            iron_exec_leave_execution(ctx, execution_acquired);
            break;
        }
        
        /* Keep continuation removal and frame-root cleanup atomic with respect to GC. */
        cleanup_execution_acquired = iron_exec_enter_execution(ctx);
        iron_exec_discard_finally(thread, UINT32_MAX);
        thread->current_frame = frame.prev;
        
        /* Get return value from stack if any (only values pushed by this method) */
        if (result && thread->eval_stack.size > stack_base) {
            *result = iron_stack_pop(&thread->eval_stack);
            /* result->type is already set by the pop operation */
        }
        /* If no value was returned, result->type remains 0xFF (void) */
        
        /* Restore stack to base - remove any leftover values from this method */
        thread->eval_stack.size = stack_base;
        
        /* Clean up */
        iron_arena_restore(&thread->frame_arena, frame.arena_mark);
        if (frame.locals) {
            iron_free(ctx->allocator, frame.locals, 
                      frame.local_count * sizeof(iron_stack_value_t));
        }
        if (frame.args) {
            iron_free(ctx->allocator, frame.args,
                      frame.arg_count * sizeof(iron_stack_value_t));
        }

        iron_exec_leave_execution(ctx, cleanup_execution_acquired);
        
        if (interp_result == IRON_INTERP_RETURN) {
            return IRON_SUCCESS;
        } else if (interp_result == IRON_INTERP_EXCEPTION) {
            return IRON_ERROR(IRON_ERR_EXCEPTION, "Unhandled exception");
        } else {
            return IRON_ERROR(IRON_ERR_EXECUTION, "Execution error");
        }
    }
}
