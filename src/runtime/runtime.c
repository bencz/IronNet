/*
 * IronNet CLR Interpreter
 * runtime.c - Runtime type system implementation
 */

#include "iron/runtime.h"
#include "iron/exec.h"
#include "iron/pe.h"
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
    /* TODO: Implement dynamic search path list */
    /* For now, just store in config if possible */
    (void)domain;
    (void)path;
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
    
    if (!domain || !full_name) return NULL;
    
    /* Check cache first */
    if (iron_hashmap_get(&domain->type_cache, &full_name, &type)) {
        return type;
    }
    
    /* Search in all assemblies */
    /* TODO: Implement type search */
    
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
    if (table_id != 0x06) {
        /* TODO: Handle MemberRef (0x0A) for cross-assembly references */
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
        /* Read the MethodDef row */
        result = iron_metadata_read_row(&assembly->metadata, token, &row);
        if (!IRON_RESULT_OK(result)) {
            return NULL;
        }
        
        method->name = iron_metadata_get_string(&assembly->metadata, row.name);
        method->token = token;
        method->attrs = row.flags;
        method->impl_attrs = row.impl_flags;
        
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
            /* TODO: Parse method body from RVA */
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
    /* TODO: Implement full type resolution */
    (void)module;
    (void)token;
    return NULL;
}

iron_runtime_type_t *iron_type_make_array(iron_domain_t *domain,
                                          iron_runtime_type_t *element,
                                          iron_u32 rank)
{
    /* TODO: Implement array type creation */
    (void)domain;
    (void)element;
    (void)rank;
    return NULL;
}

iron_runtime_type_t *iron_type_make_pointer(iron_domain_t *domain,
                                            iron_runtime_type_t *element)
{
    /* TODO: Implement pointer type creation */
    (void)domain;
    (void)element;
    return NULL;
}

iron_runtime_type_t *iron_type_make_byref(iron_domain_t *domain,
                                          iron_runtime_type_t *element)
{
    /* TODO: Implement byref type creation */
    (void)domain;
    (void)element;
    return NULL;
}

iron_runtime_type_t *iron_type_make_generic(iron_domain_t *domain,
                                            iron_runtime_type_t *definition,
                                            iron_runtime_type_t **args,
                                            iron_u32 arg_count)
{
    /* TODO: Implement generic instantiation */
    (void)domain;
    (void)definition;
    (void)args;
    (void)arg_count;
    return NULL;
}

iron_result_t iron_type_compute_layout(iron_runtime_type_t *type)
{
    /* TODO: Implement type layout computation */
    (void)type;
    return (iron_result_t)IRON_SUCCESS;
}

iron_runtime_method_t *iron_method_resolve_token(iron_module_t *module,
                                                 iron_token_t token)
{
    /* TODO: Implement method resolution */
    (void)module;
    (void)token;
    return NULL;
}

iron_runtime_method_t *iron_method_make_generic(iron_domain_t *domain,
                                                iron_runtime_method_t *definition,
                                                iron_runtime_type_t **args,
                                                iron_u32 arg_count)
{
    /* TODO: Implement generic method instantiation */
    (void)domain;
    (void)definition;
    (void)args;
    (void)arg_count;
    return NULL;
}

iron_result_t iron_method_load_body(iron_runtime_method_t *method)
{
    /* TODO: Implement method body loading */
    (void)method;
    return (iron_result_t)IRON_SUCCESS;
}

iron_runtime_field_t *iron_field_resolve_token(iron_module_t *module,
                                               iron_token_t token)
{
    /* TODO: Implement field resolution */
    (void)module;
    (void)token;
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
            iron_u32 rva;
            
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
        
        if (!thread) {
            return IRON_ERROR(IRON_ERR_INVALID_STATE, "No thread context");
        }
        
        /* Set up stack frame */
        memset(&frame, 0, sizeof(frame));
        frame.method = method;
        frame.code = method->body->code;
        frame.code_size = method->body->code_size;
        frame.ip = 0;
        frame.prev = thread->current_frame;
        
        /* Allocate locals if method has local variables */
        /* TODO: Parse local_var_sig_token to get actual local count */
        if (method->local_count > 0) {
            frame.locals = (iron_stack_value_t *)iron_alloc(
                ctx->allocator, 
                method->local_count * sizeof(iron_stack_value_t));
            if (frame.locals) {
                memset(frame.locals, 0, 
                       method->local_count * sizeof(iron_stack_value_t));
            }
            frame.local_count = method->local_count;
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
        
        /* Get return value from stack if any */
        if (result && thread->eval_stack.size > 0) {
            *result = iron_stack_pop(&thread->eval_stack);
        }
        
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
