/*
 * IronNet CLR Interpreter
 * runtime.c - Runtime type system implementation
 */

#include "iron/runtime.h"
#include "iron/exec.h"
#include "iron/pe.h"
#include "iron/debug.h"
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * Domain Implementation
 * ============================================================================ */

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
        domain->name = config->name ? 
            iron_intern_cstr(&domain->interner, config->name) : "DefaultDomain";
        domain->base_path = config->base_path ?
            iron_intern_cstr(&domain->interner, config->base_path) : ".";
        domain->assembly_paths = config->assembly_paths;
        domain->assembly_path_count = config->assembly_path_count;
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
    
    /* Initialize generic instantiation cache */
    iron_hashmap_init(&domain->generic_inst_cache, alloc,
                      sizeof(iron_generic_inst_t), sizeof(iron_runtime_type_t*),
                      NULL, NULL);
    
    *out_domain = domain;
    return (iron_result_t)IRON_SUCCESS;
}

void iron_domain_destroy(iron_domain_t *domain)
{
    iron_u32 i;
    
    if (!domain) return;
    
    /* Free assemblies */
    for (i = 0; i < domain->assembly_count; i++) {
        iron_assembly_free(domain->assemblies[i]);
    }
    if (domain->assemblies) {
        iron_free(domain->allocator, domain->assemblies,
                  domain->assembly_capacity * sizeof(iron_assembly_t*));
    }
    
    /* Free caches */
    iron_hashmap_destroy(&domain->type_cache);
    iron_hashmap_destroy(&domain->generic_inst_cache);
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
    
    /* Add to domain's assembly list */
    if (domain->assembly_count >= domain->assembly_capacity) {
        iron_u32 new_cap = domain->assembly_capacity ? domain->assembly_capacity * 2 : 8;
        iron_assembly_t **new_list = (iron_assembly_t **)iron_realloc(
            domain->allocator, domain->assemblies,
            domain->assembly_capacity * sizeof(iron_assembly_t*),
            new_cap * sizeof(iron_assembly_t*));
        if (!new_list) {
            iron_assembly_free(assembly);
            return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to grow assembly list");
        }
        domain->assemblies = new_list;
        domain->assembly_capacity = new_cap;
    }
    
    domain->assemblies[domain->assembly_count++] = assembly;
    *out_assembly = assembly;
    
    return (iron_result_t)IRON_SUCCESS;
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

/* ============================================================================
 * Assembly Implementation
 * ============================================================================ */

iron_result_t iron_assembly_load(iron_assembly_t **out_assembly,
                                 iron_domain_t *domain,
                                 const char *path)
{
    iron_result_t result;
    iron_assembly_t *assembly;
    iron_assembly_row_t asm_row;
    
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
    
    /* Initialize metadata reader */
    result = iron_metadata_init(&assembly->metadata, &assembly->image, domain->allocator);
    if (!IRON_RESULT_OK(result)) {
        iron_pe_free(&assembly->image);
        iron_free(domain->allocator, assembly, sizeof(iron_assembly_t));
        return result;
    }
    
    /* Read assembly info */
    if (iron_metadata_table_rows(&assembly->metadata, IRON_TABLE_ASSEMBLY) > 0) {
        result = iron_metadata_read_row(&assembly->metadata,
                                        IRON_MAKE_TOKEN(IRON_TABLE_ASSEMBLY, 1),
                                        &asm_row);
        if (IRON_RESULT_OK(result)) {
            assembly->name = iron_metadata_get_string(&assembly->metadata, asm_row.name);
            assembly->culture = iron_metadata_get_string(&assembly->metadata, asm_row.culture);
            assembly->major_version = asm_row.major_version;
            assembly->minor_version = asm_row.minor_version;
            assembly->build_number = asm_row.build_number;
            assembly->revision_number = asm_row.revision_number;
            assembly->flags = asm_row.flags;
        }
    }
    
    /* Create module */
    assembly->module = (iron_module_t *)iron_alloc(domain->allocator, sizeof(iron_module_t));
    if (assembly->module) {
        iron_module_row_t mod_row;
        
        memset(assembly->module, 0, sizeof(iron_module_t));
        assembly->module->assembly = assembly;
        
        if (iron_metadata_table_rows(&assembly->metadata, IRON_TABLE_MODULE) > 0) {
            result = iron_metadata_read_row(&assembly->metadata,
                                            IRON_MAKE_TOKEN(IRON_TABLE_MODULE, 1),
                                            &mod_row);
            if (IRON_RESULT_OK(result)) {
                assembly->module->name = iron_metadata_get_string(&assembly->metadata, mod_row.name);
                
                const iron_u8 *mvid = iron_metadata_get_guid(&assembly->metadata, mod_row.mvid);
                if (mvid) {
                    memcpy(assembly->module->mvid, mvid, 16);
                }
            }
        }
    }
    
    *out_assembly = assembly;
    return (iron_result_t)IRON_SUCCESS;
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
    
    /* Initialize metadata reader */
    result = iron_metadata_init(&assembly->metadata, &assembly->image, domain->allocator);
    if (!IRON_RESULT_OK(result)) {
        iron_pe_free(&assembly->image);
        iron_free(domain->allocator, assembly, sizeof(iron_assembly_t));
        return result;
    }
    
    *out_assembly = assembly;
    return (iron_result_t)IRON_SUCCESS;
}

void iron_assembly_free(iron_assembly_t *assembly)
{
    if (!assembly) return;
    
    if (assembly->module) {
        /* Free types */
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
    
    if (!assembly || !assembly->module || !name) return NULL;
    
    for (i = 0; i < assembly->module->type_count; i++) {
        iron_runtime_type_t *type = assembly->module->types[i];
        if (!type) continue;
        
        if (type->name && strcmp(type->name, name) == 0) {
            if (!namespace_ || !type->namespace_ ||
                strcmp(type->namespace_, namespace_) == 0) {
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
            /* Reference to type in another assembly - search in loaded assemblies */
            iron_type_ref_row_t type_ref;
            res = iron_metadata_read_row(&assembly->metadata, class_token, &type_ref);
            if (IRON_RESULT_OK(res)) {
                const char *type_name = iron_metadata_get_string(&assembly->metadata, type_ref.name);
                const char *type_ns = iron_metadata_get_string(&assembly->metadata, type_ref.namespace_);
                iron_u32 i;
                
                /* Search in all loaded assemblies */
                for (i = 0; i < assembly->domain->assembly_count; i++) {
                    iron_assembly_t *ref_asm = assembly->domain->assemblies[i];
                    iron_runtime_type_t *type;
                    iron_u32 j;
                    
                    if (!ref_asm) continue;
                    
                    type = iron_assembly_find_type(ref_asm, type_ns, type_name);
                    if (type && type->methods) {
                        /* Search for method by name */
                        for (j = 0; j < type->method_count; j++) {
                            if (type->methods[j] && type->methods[j]->name &&
                                strcmp(type->methods[j]->name, method_name) == 0) {
                                return type->methods[j];
                            }
                        }
                    }
                }
            }
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
                    if (meth && meth->name && strcmp(meth->name, method_name) == 0) {
                        return meth;
                    }
                }
            }
        }
        else if (class_table == IRON_TABLE_TYPE_SPEC) {
            /* Reference to generic type instantiation (TypeSpec) */
            /* TypeSpec contains a blob signature describing the generic type */
            iron_type_spec_row_t type_spec;
            IRON_DEBUG_META("MemberRef resolving TypeSpec: class_token=0x%08X method=%s", 
                           class_token, method_name);
            res = iron_metadata_read_row(&assembly->metadata, class_token, &type_spec);
            IRON_DEBUG_META("TypeSpec read_row result: %s sig_idx=0x%X", 
                           IRON_RESULT_OK(res) ? "OK" : "FAIL",
                           IRON_RESULT_OK(res) ? type_spec.signature : 0);
            if (IRON_RESULT_OK(res)) {
                const iron_u8 *sig_data;
                iron_u32 sig_size;
                
                if (IRON_RESULT_OK(iron_metadata_get_blob(&assembly->metadata, type_spec.signature, &sig_data, &sig_size))) {
                    /* TypeSpec signature format:
                     * GENERICINST (0x15) followed by:
                     * - CLASS (0x12) or VALUETYPE (0x11)
                     * - TypeDefOrRef coded index
                     * - GenArgCount
                     * - Type arguments...
                     */
                    IRON_DEBUG_META("TypeSpec sig: size=%u bytes=[%02X %02X %02X %02X]",
                                   sig_size,
                                   sig_size > 0 ? sig_data[0] : 0,
                                   sig_size > 1 ? sig_data[1] : 0,
                                   sig_size > 2 ? sig_data[2] : 0,
                                   sig_size > 3 ? sig_data[3] : 0);
                    if (sig_size >= 3 && sig_data[0] == 0x15) { /* GENERICINST */
                        iron_u8 class_or_value = sig_data[1];
                        iron_u32 type_token_coded;
                        iron_u32 idx = 2;
                        iron_u32 generic_type_token;
                        
                        (void)class_or_value; /* Unused for now */
                        
                        /* Read compressed TypeDefOrRef token */
                        type_token_coded = sig_data[idx++];
                        if (type_token_coded & 0x80) {
                            type_token_coded = ((type_token_coded & 0x3F) << 8) | sig_data[idx++];
                        }
                        
                        /* Decode TypeDefOrRef coded index */
                        generic_type_token = iron_metadata_decode_coded(&assembly->metadata,
                                                                        IRON_CODED_TYPE_DEF_OR_REF,
                                                                        type_token_coded);
                        IRON_DEBUG_META("TypeSpec: type_token_coded=0x%X generic_type_token=0x%08X",
                                       type_token_coded, generic_type_token);
                        
                        /* Resolve the generic type definition and find the method */
                        {
                            iron_u32 gen_table = (generic_type_token >> 24) & 0xFF;
                            if (gen_table == IRON_TABLE_TYPE_DEF) {
                                iron_type_def_row_t gen_type_def;
                                if (IRON_RESULT_OK(iron_metadata_read_row(&assembly->metadata, generic_type_token, &gen_type_def))) {
                                    iron_u32 method_start = gen_type_def.method_list;
                                    iron_u32 method_end;
                                    iron_u32 type_count = iron_metadata_table_rows(&assembly->metadata, IRON_TABLE_TYPE_DEF);
                                    iron_u32 type_row = (generic_type_token & 0x00FFFFFF);
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
                                        iron_token_t meth_token = IRON_MAKE_TOKEN(IRON_TABLE_METHOD_DEF, m);
                                        iron_runtime_method_t *meth = iron_resolve_method_token(assembly, meth_token);
                                        if (meth && meth->name && strcmp(meth->name, method_name) == 0) {
                                            return meth;
                                        }
                                    }
                                }
                            }
                            else if (gen_table == IRON_TABLE_TYPE_REF) {
                                /* TypeRef - reference to type in another assembly (e.g., corlib) */
                                iron_type_ref_row_t type_ref;
                                if (IRON_RESULT_OK(iron_metadata_read_row(&assembly->metadata, generic_type_token, &type_ref))) {
                                    const char *type_name = iron_metadata_get_string(&assembly->metadata, type_ref.name);
                                    const char *type_ns = iron_metadata_get_string(&assembly->metadata, type_ref.namespace_);
                                    
                                    /* Search in domain's loaded assemblies for this type */
                                    if (assembly->domain) {
                                        iron_u32 a;
                                        for (a = 0; a < assembly->domain->assembly_count; a++) {
                                            iron_assembly_t *ref_asm = assembly->domain->assemblies[a];
                                            if (!ref_asm) continue;
                                            
                                            /* Search for type in this assembly */
                                            iron_u32 t;
                                            iron_u32 ref_type_count = iron_metadata_table_rows(&ref_asm->metadata, IRON_TABLE_TYPE_DEF);
                                            for (t = 1; t <= ref_type_count; t++) {
                                                iron_type_def_row_t ref_type_def;
                                                iron_token_t ref_type_token = IRON_MAKE_TOKEN(IRON_TABLE_TYPE_DEF, t);
                                                if (IRON_RESULT_OK(iron_metadata_read_row(&ref_asm->metadata, ref_type_token, &ref_type_def))) {
                                                    const char *ref_name = iron_metadata_get_string(&ref_asm->metadata, ref_type_def.name);
                                                    const char *ref_ns = iron_metadata_get_string(&ref_asm->metadata, ref_type_def.namespace_);
                                                    
                                                    if (ref_name && type_name && strcmp(ref_name, type_name) == 0 &&
                                                        ((ref_ns == NULL && type_ns == NULL) ||
                                                         (ref_ns && type_ns && strcmp(ref_ns, type_ns) == 0))) {
                                                        /* Found the type - now find the method */
                                                        iron_u32 method_start = ref_type_def.method_list;
                                                        iron_u32 method_end;
                                                        iron_u32 m;
                                                        
                                                        if (t < ref_type_count) {
                                                            iron_type_def_row_t next_type;
                                                            iron_token_t next_token = IRON_MAKE_TOKEN(IRON_TABLE_TYPE_DEF, t + 1);
                                                            if (IRON_RESULT_OK(iron_metadata_read_row(&ref_asm->metadata, next_token, &next_type))) {
                                                                method_end = next_type.method_list;
                                                            } else {
                                                                method_end = iron_metadata_table_rows(&ref_asm->metadata, IRON_TABLE_METHOD_DEF) + 1;
                                                            }
                                                        } else {
                                                            method_end = iron_metadata_table_rows(&ref_asm->metadata, IRON_TABLE_METHOD_DEF) + 1;
                                                        }
                                                        
                                                        for (m = method_start; m < method_end; m++) {
                                                            iron_token_t meth_token = IRON_MAKE_TOKEN(IRON_TABLE_METHOD_DEF, m);
                                                            iron_runtime_method_t *meth = iron_resolve_method_token(ref_asm, meth_token);
                                                            if (meth && meth->name && strcmp(meth->name, method_name) == 0) {
                                                                return meth;
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
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
        
        /* Get signature blob and parse param count */
        if (row.signature > 0) {
            const iron_u8 *sig_data;
            iron_u32 sig_size;
            result = iron_metadata_get_blob(&assembly->metadata, row.signature, 
                                            &sig_data, &sig_size);
            if (IRON_RESULT_OK(result) && sig_size > 1) {
                /* Parse param count - skip calling convention, read compressed int */
                method->param_count = sig_data[1];
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
                } else {
                    /* Parse local variable signature if present */
                    if (method->body->local_var_sig_token != 0) {
                        iron_u32 sig_table = (method->body->local_var_sig_token >> 24) & 0xFF;
                        if (sig_table == IRON_TABLE_STANDALONE_SIG) {
                            iron_standalone_sig_row_t sig_row;
                            body_res = iron_metadata_read_row(&assembly->metadata, 
                                                               method->body->local_var_sig_token, &sig_row);
                            if (IRON_RESULT_OK(body_res)) {
                                const iron_u8 *sig_data;
                                iron_u32 sig_size;
                                body_res = iron_metadata_get_blob(&assembly->metadata, 
                                                                   sig_row.signature, &sig_data, &sig_size);
                                if (IRON_RESULT_OK(body_res) && sig_size >= 2) {
                                    /* LOCAL_SIG = 0x07, followed by count */
                                    if (sig_data[0] == 0x07) {
                                        method->local_count = sig_data[1];
                                    }
                                }
                            }
                        }
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
 * Type Resolution (Stubs)
 * ============================================================================ */

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
            assembly->types = (iron_runtime_type_t *)iron_alloc(
                assembly->domain->allocator,
                type_count * sizeof(iron_runtime_type_t));
            if (!assembly->types) return NULL;
            memset(assembly->types, 0, type_count * sizeof(iron_runtime_type_t));
            assembly->type_count = type_count;
        }
        
        type = &assembly->types[row_index];
        
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
            
            /* Determine type kind from flags */
            if (type->attrs & 0x00000020) { /* Interface */
                type->kind = IRON_KIND_INTERFACE;
            } else if (type->attrs & 0x00000100) { /* Abstract + Sealed = static class */
                type->kind = IRON_KIND_CLASS;
            } else {
                type->kind = IRON_KIND_CLASS;
            }
            
            /* Calculate field count from metadata */
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
            
            /* Calculate instance size: 8 bytes per field, minimum 8 bytes */
            type->instance_size = type->field_count * 8;
            if (type->instance_size < 8) {
                type->instance_size = 8;
            }
            type->alignment = 8;
            
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
                            }
                        }
                    }
                }
            }
            
        }
        
        return type;
    }
    else if (table_id == IRON_TABLE_TYPE_REF) {
        /* TypeRef - reference to type in another assembly */
        iron_type_ref_row_t type_ref;
        iron_result_t res;
        const char *type_name;
        const char *type_ns;
        
        res = iron_metadata_read_row(&assembly->metadata, token, &type_ref);
        if (!IRON_RESULT_OK(res)) return NULL;
        
        type_name = iron_metadata_get_string(&assembly->metadata, type_ref.name);
        type_ns = iron_metadata_get_string(&assembly->metadata, type_ref.namespace_);
        
        /* Search in domain's loaded assemblies */
        return iron_assembly_find_type(assembly, type_ns, type_name);
    }
    
    return NULL;
}

iron_runtime_type_t *iron_type_make_array(iron_domain_t *domain,
                                          iron_runtime_type_t *element,
                                          iron_u32 rank)
{
    iron_runtime_type_t *array_type;
    char full_name[512];
    const char *interned_name;
    
    if (!domain || !element) return NULL;
    
    /* Build array type name (e.g., "System.Int32[]" or "System.Int32[,]") */
    if (rank == 1) {
        snprintf(full_name, sizeof(full_name), "%s[]", 
                 element->full_name ? element->full_name : element->name);
    } else {
        char brackets[64];
        iron_u32 i;
        brackets[0] = '[';
        for (i = 1; i < rank && i < 32; i++) {
            brackets[i] = ',';
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
    if (!array_type) return NULL;
    
    memset(array_type, 0, sizeof(iron_runtime_type_t));
    array_type->kind = IRON_KIND_ARRAY;
    array_type->element = element;
    array_type->element_type = IRON_TYPE_SZARRAY;
    array_type->name = iron_intern_cstr(&domain->interner, full_name);
    array_type->full_name = array_type->name;
    array_type->base_type = domain->type_array;
    array_type->instance_size = sizeof(iron_u32) + sizeof(void*); /* length + data ptr */
    array_type->alignment = sizeof(void*);
    
    /* Cache the type */
    interned_name = array_type->name;
    iron_hashmap_set(&domain->type_cache, &interned_name, &array_type);
    
    return array_type;
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
    iron_hashmap_set(&domain->type_cache, &interned_name, &ptr_type);
    
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
    iron_hashmap_set(&domain->type_cache, &interned_name, &byref_type);
    
    return byref_type;
}

iron_runtime_type_t *iron_type_make_generic(iron_domain_t *domain,
                                            iron_runtime_type_t *definition,
                                            iron_runtime_type_t **args,
                                            iron_u32 arg_count)
{
    iron_runtime_type_t *inst_type;
    iron_generic_inst_t inst_key;
    char full_name[1024];
    const char *interned_name;
    iron_u32 i;
    int pos;
    
    if (!domain || !definition || !args || arg_count == 0) return NULL;
    
    /* Check generic instantiation cache */
    inst_key.args = args;
    inst_key.arg_count = arg_count;
    inst_key.hash = iron_generic_inst_hash(&inst_key);
    
    if (iron_hashmap_get(&domain->generic_inst_cache, &inst_key, &inst_type)) {
        return inst_type;
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
    }
    
    /* Allocate new generic instantiation */
    inst_type = (iron_runtime_type_t *)iron_alloc(domain->allocator,
                                                   sizeof(iron_runtime_type_t));
    if (!inst_type) return NULL;
    
    memset(inst_type, 0, sizeof(iron_runtime_type_t));
    inst_type->kind = IRON_KIND_GENERIC_INST;
    inst_type->generic_definition = definition;
    inst_type->is_generic_instance = IRON_TRUE;
    inst_type->name = iron_intern_cstr(&domain->interner, full_name);
    inst_type->full_name = inst_type->name;
    inst_type->base_type = definition->base_type;
    inst_type->instance_size = definition->instance_size;
    inst_type->alignment = definition->alignment;
    
    /* Copy generic arguments */
    inst_type->generic_args = (iron_runtime_type_t **)iron_alloc(
        domain->allocator, arg_count * sizeof(iron_runtime_type_t*));
    if (inst_type->generic_args) {
        for (i = 0; i < arg_count; i++) {
            inst_type->generic_args[i] = args[i];
        }
        inst_type->generic_arg_count = arg_count;
    }
    
    /* Cache the instantiation */
    interned_name = inst_type->name;
    iron_hashmap_set(&domain->type_cache, &interned_name, &inst_type);
    
    return inst_type;
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

iron_result_t iron_type_compute_layout(iron_runtime_type_t *type)
{
    iron_u32 offset = 0;
    iron_u32 max_align = 1;
    iron_u32 i;
    
    if (!type) return IRON_ERROR(IRON_ERR_NULL_POINTER, "Null type");
    
    if (type->layout_computed) {
        return (iron_result_t)IRON_SUCCESS;
    }
    
    /* Compute base type layout first */
    if (type->base_type && !type->base_type->layout_computed) {
        iron_result_t res = iron_type_compute_layout(type->base_type);
        if (!IRON_RESULT_OK(res)) return res;
        offset = type->base_type->instance_size;
        max_align = type->base_type->alignment;
    }
    
    /* Compute field offsets */
    for (i = 0; i < type->field_count; i++) {
        iron_runtime_field_t *field = type->fields[i];
        iron_u32 field_size;
        iron_u32 field_align;
        
        if (!field || !field->field_type) continue;
        if (field->attrs & 0x0010) continue; /* Skip static fields */
        
        /* Get field size and alignment */
        field_size = field->field_type->instance_size;
        field_align = field->field_type->alignment;
        
        if (field_size == 0) field_size = sizeof(void*);
        if (field_align == 0) field_align = sizeof(void*);
        
        /* Align offset */
        offset = (offset + field_align - 1) & ~(field_align - 1);
        field->offset = offset;
        field->size = field_size;
        offset += field_size;
        
        if (field_align > max_align) {
            max_align = field_align;
        }
    }
    
    /* Final alignment */
    type->instance_size = (offset + max_align - 1) & ~(max_align - 1);
    if (type->instance_size == 0) {
        type->instance_size = sizeof(void*); /* Minimum size */
    }
    type->alignment = max_align;
    type->layout_computed = IRON_TRUE;
    
    return (iron_result_t)IRON_SUCCESS;
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
    iron_runtime_method_t *inst_method;
    char full_name[1024];
    iron_u32 i;
    int pos;
    
    if (!domain || !definition || !args || arg_count == 0) return NULL;
    
    /* Allocate new generic method instantiation */
    inst_method = (iron_runtime_method_t *)iron_alloc(domain->allocator,
                                                       sizeof(iron_runtime_method_t));
    if (!inst_method) return NULL;
    
    /* Copy base method info */
    memcpy(inst_method, definition, sizeof(iron_runtime_method_t));
    
    /* Build generic method name (e.g., "Method<System.Int32>") */
    pos = snprintf(full_name, sizeof(full_name), "%s<",
                   definition->name ? definition->name : "?");
    
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
    }
    
    inst_method->name = iron_intern_cstr(&domain->interner, full_name);
    inst_method->generic_definition = definition;
    inst_method->is_generic_instance = IRON_TRUE;
    inst_method->is_generic_definition = IRON_FALSE;
    
    /* Copy generic arguments */
    inst_method->generic_args = (iron_runtime_type_t **)iron_alloc(
        domain->allocator, arg_count * sizeof(iron_runtime_type_t*));
    if (inst_method->generic_args) {
        for (i = 0; i < arg_count; i++) {
            inst_method->generic_args[i] = args[i];
        }
        inst_method->generic_arg_count = arg_count;
    }
    
    return inst_method;
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
        return (iron_result_t)IRON_SUCCESS;
    }
    
    /* Internal calls don't have IL bodies */
    if (method->is_internal_call) {
        return (iron_result_t)IRON_SUCCESS;
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
    
    /* Parse local variable signature if present */
    if (method->body->local_var_sig_token != 0) {
        iron_u32 sig_table = (method->body->local_var_sig_token >> 24) & 0xFF;
        if (sig_table == IRON_TABLE_STANDALONE_SIG) {
            iron_standalone_sig_row_t sig_row;
            res = iron_metadata_read_row(&assembly->metadata, 
                                          method->body->local_var_sig_token, &sig_row);
            if (IRON_RESULT_OK(res)) {
                const iron_u8 *sig_data;
                iron_u32 sig_size;
                res = iron_metadata_get_blob(&assembly->metadata, 
                                              sig_row.signature, &sig_data, &sig_size);
                if (IRON_RESULT_OK(res) && sig_size >= 2 && sig_data[0] == 0x07) {
                    method->local_count = sig_data[1];
                }
            }
        }
    }
    
    return (iron_result_t)IRON_SUCCESS;
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
        /* Field defined in this assembly */
        iron_field_row_t field_row;
        iron_result_t res;
        iron_runtime_field_t *field;
        
        res = iron_metadata_read_row(&assembly->metadata, token, &field_row);
        if (!IRON_RESULT_OK(res)) return NULL;
        
        /* Allocate field structure */
        field = (iron_runtime_field_t *)iron_alloc(assembly->domain->allocator,
                                                    sizeof(iron_runtime_field_t));
        if (!field) return NULL;
        
        memset(field, 0, sizeof(iron_runtime_field_t));
        field->token = token;
        field->name = iron_metadata_get_string(&assembly->metadata, field_row.name);
        field->attrs = field_row.flags;
        
        return field;
    }
    else if (table_id == IRON_TABLE_MEMBER_REF) {
        /* Field reference (MemberRef) - need to resolve to actual field */
        iron_member_ref_row_t member_ref;
        iron_result_t res;
        const char *field_name;
        iron_u32 class_token;
        iron_u32 class_table;
        
        res = iron_metadata_read_row(&assembly->metadata, token, &member_ref);
        if (!IRON_RESULT_OK(res)) return NULL;
        
        field_name = iron_metadata_get_string(&assembly->metadata, member_ref.name);
        
        /* Decode the class */
        class_token = iron_metadata_decode_coded(&assembly->metadata,
                                                  IRON_CODED_MEMBER_REF_PARENT,
                                                  member_ref.class_);
        class_table = (class_token >> 24) & 0xFF;
        
        if (class_table == IRON_TABLE_TYPE_REF || class_table == IRON_TABLE_TYPE_DEF) {
            iron_runtime_type_t *type = iron_type_resolve_token(module, class_token);
            if (type && type->fields) {
                iron_u32 i;
                for (i = 0; i < type->field_count; i++) {
                    if (type->fields[i] && type->fields[i]->name &&
                        strcmp(type->fields[i]->name, field_name) == 0) {
                        return type->fields[i];
                    }
                }
            }
        }
    }
    
    return NULL;
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
        result->type = 0xFF; /* Mark as void/no return initially */
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
        iron_thread_context_t *thread = ctx->main_thread;
        iron_stack_frame_t frame;
        iron_interp_result_t interp_result;
        iron_u32 i;
        iron_u32 stack_base; /* Save stack size before method execution */
        
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
        frame.prev = thread->current_frame;
        
        /* Allocate locals if method has local variables */
        /* Get local count from method body's local_var_sig_token */
        {
            iron_u32 local_count = method->local_count;
            
            IRON_DEBUG_EXEC("Method %s: local_count=%u, local_var_sig_token=0x%08X",
                           method->name ? method->name : "?",
                           method->local_count,
                           method->body->local_var_sig_token);
            
            /* If method->local_count is 0, try to get from body's local_var_sig_token */
            if (local_count == 0 && method->body->local_var_sig_token != 0) {
                /* Parse StandAloneSig to get local variable count */
                /* The token format is 0x11XXXXXX where 0x11 is StandAloneSig table */
                iron_assembly_t *assembly = NULL;
                
                if (method->declaring_type && method->declaring_type->module &&
                    method->declaring_type->module->assembly) {
                    assembly = method->declaring_type->module->assembly;
                } else if (ctx->domain && ctx->domain->assembly_count > 0) {
                    assembly = ctx->domain->assemblies[ctx->domain->assembly_count - 1];
                }
                
                if (assembly) {
                    const iron_u8 *sig_data = NULL;
                    iron_u32 sig_size = 0;
                    iron_standalone_sig_row_t sig_row;
                    iron_result_t sig_res;
                    
                    IRON_DEBUG_EXEC("Parsing local_var_sig_token from assembly %s", 
                                   assembly->name ? assembly->name : "?");
                    
                    sig_res = iron_metadata_read_row(&assembly->metadata, 
                                                      method->body->local_var_sig_token, 
                                                      &sig_row);
                    if (IRON_RESULT_OK(sig_res)) {
                        IRON_DEBUG_EXEC("StandAloneSig row: signature=0x%X", sig_row.signature);
                        sig_res = iron_metadata_get_blob(&assembly->metadata, 
                                                          sig_row.signature, 
                                                          &sig_data, &sig_size);
                        if (IRON_RESULT_OK(sig_res) && sig_data && sig_size >= 2) {
                            IRON_DEBUG_EXEC("Blob: size=%u, first bytes: 0x%02X 0x%02X", 
                                           sig_size, sig_data[0], sig_data[1]);
                            /* LocalVarSig format: 0x07 count type1 type2 ... */
                            if (sig_data[0] == 0x07) {
                                local_count = sig_data[1];
                                IRON_DEBUG_EXEC("Method has %u local variables (from sig)", local_count);
                            }
                        } else {
                            IRON_DEBUG_EXEC("Failed to get blob or blob too small");
                        }
                    } else {
                        IRON_DEBUG_EXEC("Failed to read StandAloneSig row: %s", sig_res.message);
                    }
                } else {
                    IRON_DEBUG_EXEC("No assembly found for local var parsing");
                }
            }
            
            if (local_count > 0) {
                frame.locals = (iron_stack_value_t *)iron_alloc(
                    ctx->allocator, 
                    local_count * sizeof(iron_stack_value_t));
                if (frame.locals) {
                    memset(frame.locals, 0, 
                           local_count * sizeof(iron_stack_value_t));
                }
                frame.local_count = local_count;
                IRON_DEBUG_EXEC("Allocated %u locals for method %s", 
                               local_count, method->name ? method->name : "?");
            }
        }
        
        /* Copy arguments */
        if (arg_count > 0 && args) {
            frame.args = (iron_stack_value_t *)iron_alloc(
                ctx->allocator, arg_count * sizeof(iron_stack_value_t));
            if (frame.args) {
                for (i = 0; i < arg_count; i++) {
                    frame.args[i] = args[i];
                }
            }
            frame.arg_count = arg_count;
        }
        
        /* Push frame */
        thread->current_frame = &frame;
        
        /* Execute instructions until return or error */
        do {
            interp_result = iron_exec_instruction(thread);
        } while (interp_result == IRON_INTERP_OK || 
                 interp_result == IRON_INTERP_BRANCH);
        
        /* Pop frame */
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
        if (frame.locals) {
            iron_free(ctx->allocator, frame.locals, 
                      frame.local_count * sizeof(iron_stack_value_t));
        }
        if (frame.args) {
            iron_free(ctx->allocator, frame.args,
                      frame.arg_count * sizeof(iron_stack_value_t));
        }
        
        if (interp_result == IRON_INTERP_RETURN) {
            return (iron_result_t)IRON_SUCCESS;
        } else if (interp_result == IRON_INTERP_EXCEPTION) {
            return IRON_ERROR(IRON_ERR_EXCEPTION, "Unhandled exception");
        } else {
            return IRON_ERROR(IRON_ERR_EXECUTION, "Execution error");
        }
    }
}
