/*
 * IronNet CLR Interpreter
 * exec.c - IL Execution engine implementation
 */

#include "iron/exec.h"
#include "iron/opcodes.h"
#include "iron/metadata.h"
#include "iron/runtime.h"
#include "iron/gc.h"
#include "iron/types.h"
#include "iron/debug.h"
#include "iron/thread.h"
#include <limits.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

/* Forward declarations for helper functions */
static iron_result_t resolve_method_token(iron_thread_context_t *thread,
                                          iron_u32 token,
                                          iron_runtime_method_t **out_method,
                                          iron_assembly_t **out_assembly);
static iron_result_t call_method(iron_thread_context_t *thread,
                                 iron_runtime_method_t *method,
                                 iron_assembly_t *assembly,
                                 iron_bool is_newobj);
static iron_u32 get_method_param_count_from_sig(const iron_u8 *sig_data, iron_u32 sig_size);
static iron_runtime_field_t *resolve_runtime_field(iron_thread_context_t *thread, iron_assembly_t *assembly, iron_u32 field_token);
static iron_runtime_type_t *resolve_type_spec(iron_thread_context_t *thread, iron_assembly_t *assembly, iron_token_t token);
static void build_signature_string_with_metadata(const iron_metadata_t *meta,
                                                 const iron_u8 *sig_data,
                                                 iron_u32 sig_size,
                                                 char *out_sig,
                                                 iron_u32 out_size);
static iron_result_t invoke_array_get(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result);
static iron_result_t invoke_array_set(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result);
static iron_result_t invoke_array_address(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result);
static iron_result_t invoke_array_constructor(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result);
static iron_result_t convert_managed_call_error(iron_thread_context_t *thread, iron_result_t result);

static iron_assembly_t *get_frame_assembly(const iron_stack_frame_t *frame)
{
    if (!frame || !frame->method || !frame->method->declaring_type || !frame->method->declaring_type->module) {
        return NULL;
    }

    return frame->method->declaring_type->module->assembly;
}

static const char *get_array_interface_bridge_name(const iron_runtime_method_t *contract)
{
    const iron_runtime_type_t *interface_definition;
    const char *interface_name;

    if (!contract || !contract->name || !contract->declaring_type || contract->declaring_type->generic_arg_count != 1 ||
        !contract->declaring_type->generic_args || !contract->declaring_type->generic_args[0]) {
        return NULL;
    }

    interface_definition = contract->declaring_type->generic_definition;
    interface_name = interface_definition ? interface_definition->full_name : NULL;
    if (!interface_name) {
        return NULL;
    }

    if (strcmp(interface_name, "System.Collections.Generic.ICollection`1") == 0) {
        if (strcmp(contract->name, "get_Count") == 0) {
            return "InternalGetCount";
        }
        if (strcmp(contract->name, "get_IsReadOnly") == 0) {
            return "InternalGetIsReadOnly";
        }
        if (strcmp(contract->name, "Add") == 0) {
            return "InternalAdd";
        }
        if (strcmp(contract->name, "Clear") == 0) {
            return "InternalClear";
        }
        if (strcmp(contract->name, "Contains") == 0) {
            return "InternalContains";
        }
        if (strcmp(contract->name, "CopyTo") == 0) {
            return "InternalCopyTo";
        }
        if (strcmp(contract->name, "Remove") == 0) {
            return "InternalRemove";
        }
    }

    if (strcmp(interface_name, "System.Collections.Generic.IList`1") == 0) {
        if (strcmp(contract->name, "get_Item") == 0) {
            return "InternalGetItem";
        }
        if (strcmp(contract->name, "set_Item") == 0) {
            return "InternalSetItem";
        }
        if (strcmp(contract->name, "IndexOf") == 0) {
            return "InternalIndexOf";
        }
        if (strcmp(contract->name, "Insert") == 0) {
            return "InternalInsert";
        }
        if (strcmp(contract->name, "RemoveAt") == 0) {
            return "InternalRemoveAt";
        }
    }

    return NULL;
}

static iron_runtime_method_t *find_array_interface_implementation(iron_runtime_type_t *array_type, const iron_runtime_method_t *contract)
{
    const char *bridge_name;
    iron_runtime_type_t *current;
    iron_runtime_type_t *generic_argument;

    if (!array_type || array_type->kind != IRON_KIND_ARRAY || array_type->element_type != IRON_TYPE_SZARRAY || array_type->array_rank != 1) {
        return NULL;
    }

    bridge_name = get_array_interface_bridge_name(contract);
    if (!bridge_name) {
        return NULL;
    }

    generic_argument = contract->declaring_type->generic_args[0];
    for (current = array_type; current; current = current->base_type) {
        iron_u32 method_index;

        for (method_index = 0; method_index < current->method_count; method_index++) {
            iron_runtime_method_t *candidate;
            iron_runtime_method_t *bound_candidate;
            iron_domain_t *domain;
            iron_u32 parameter_index;
            iron_bool signature_matches;

            candidate = current->methods[method_index];
            if (!candidate || !candidate->name || strcmp(candidate->name, bridge_name) != 0 || !candidate->is_generic_definition ||
                candidate->generic_param_count != 1 || candidate->param_count != contract->param_count) {
                continue;
            }

            domain = candidate->declaring_type && candidate->declaring_type->module && candidate->declaring_type->module->assembly
                ? candidate->declaring_type->module->assembly->domain
                : NULL;
            bound_candidate = iron_method_make_generic(domain, candidate, &generic_argument, 1);
            if (!bound_candidate || !IRON_RESULT_OK(iron_method_load_signature(bound_candidate)) ||
                !IRON_RESULT_OK(iron_method_load_signature((iron_runtime_method_t *)contract))) {
                continue;
            }

            signature_matches = bound_candidate->return_type == contract->return_type;
            for (parameter_index = 0; signature_matches && parameter_index < contract->param_count; parameter_index++) {
                signature_matches = bound_candidate->params[parameter_index]->param_type == contract->params[parameter_index]->param_type;
            }
            if (signature_matches) {
                return bound_candidate;
            }
        }
    }

    return NULL;
}

static iron_runtime_method_t *find_virtual_implementation(iron_runtime_type_t *type, const iron_runtime_method_t *contract)
{
    iron_runtime_type_t *current;
    iron_runtime_method_t *array_implementation;
    iron_u32 method_index;

    if (!type || !contract || !contract->name) {
        return NULL;
    }

    array_implementation = find_array_interface_implementation(type, contract);
    if (array_implementation) {
        return array_implementation;
    }

    for (current = type; current; current = current->base_type) {
        for (method_index = 0; method_index < current->method_count; method_index++) {
            iron_runtime_method_t *candidate;
            iron_runtime_method_t *bound_candidate;

            candidate = current->methods[method_index];
            if (!candidate || !candidate->name) {
                continue;
            }

            iron_bool exact_name;
            iron_bool explicit_name;
            iron_size candidate_name_length;
            iron_size contract_name_length;

            exact_name = strcmp(candidate->name, contract->name) == 0;
            candidate_name_length = strlen(candidate->name);
            contract_name_length = strlen(contract->name);
            explicit_name = candidate_name_length > contract_name_length &&
                            candidate->name[candidate_name_length - contract_name_length - 1U] == '.' &&
                            strcmp(candidate->name + candidate_name_length - contract_name_length, contract->name) == 0;

            if ((!exact_name && !explicit_name) || candidate->param_count != contract->param_count) {
                continue;
            }

            bound_candidate = candidate;
            if (contract->is_generic_instance && candidate->is_generic_definition && contract->generic_args && contract->generic_arg_count != 0) {
                iron_domain_t *domain;

                domain = candidate->declaring_type && candidate->declaring_type->module && candidate->declaring_type->module->assembly
                    ? candidate->declaring_type->module->assembly->domain
                    : NULL;
                bound_candidate = iron_method_make_generic(domain, candidate, contract->generic_args, contract->generic_arg_count);
                if (!bound_candidate) {
                    continue;
                }
            }

            if (IRON_RESULT_OK(iron_method_load_signature(bound_candidate)) && IRON_RESULT_OK(iron_method_load_signature((iron_runtime_method_t *)contract))) {
                iron_u32 parameter_index;
                iron_bool signature_matches;

                signature_matches = IRON_TRUE;
                for (parameter_index = 0; parameter_index < contract->param_count; parameter_index++) {
                    if (bound_candidate->params[parameter_index]->param_type != contract->params[parameter_index]->param_type) {
                        signature_matches = IRON_FALSE;
                        break;
                    }
                }
                if (!signature_matches) {
                    continue;
                }

                /* Explicit implementations of generic and non-generic interfaces commonly have the same short name and parameter list.
                 * Their return type is therefore required to identify the correct MethodImpl body. */
                if (explicit_name && bound_candidate->return_type != contract->return_type) {
                    continue;
                }
            }

            if ((candidate->attrs & IRON_METHOD_STATIC) != 0) {
                continue;
            }

            if ((candidate->attrs & IRON_METHOD_ABSTRACT) != 0) {
                continue;
            }

            return bound_candidate;
        }
    }

    return NULL;
}

iron_runtime_method_t *iron_exec_resolve_virtual(iron_runtime_type_t *obj_type, iron_runtime_method_t *method)
{
    iron_runtime_method_t *implementation;

    implementation = find_virtual_implementation(obj_type, method);
    return implementation ? implementation : method;
}

iron_runtime_method_t *iron_exec_resolve_interface(iron_runtime_type_t *obj_type,
                                                   iron_runtime_type_t *interface_type,
                                                   iron_runtime_method_t *method)
{
    if (!obj_type || !interface_type || !method || !iron_type_is_assignable_to(obj_type, interface_type)) {
        return NULL;
    }

    return find_virtual_implementation(obj_type, method);
}

static iron_bool type_is_delegate(const iron_runtime_type_t *type)
{
    const iron_runtime_type_t *current;

    for (current = type; current; current = current->base_type) {
        if (current->full_name && (strcmp(current->full_name, "System.Delegate") == 0 || strcmp(current->full_name, "System.MulticastDelegate") == 0)) {
            return IRON_TRUE;
        }
    }

    return IRON_FALSE;
}

static iron_runtime_method_t *find_bound_method(iron_runtime_type_t *type, const iron_runtime_method_t *definition)
{
    iron_u32 method_index;

    if (!type || !definition) {
        return NULL;
    }

    for (method_index = 0; method_index < type->method_count; method_index++) {
        iron_runtime_method_t *method;

        method = type->methods[method_index];
        if (method && method->token == definition->token) {
            return method;
        }
    }

    return NULL;
}

static iron_result_t initialize_delegate(iron_runtime_type_t *delegate_type, iron_stack_value_t *args, iron_u32 arg_count)
{
    iron_gc_header_t *delegate_header;
    iron_runtime_field_t *target_field;
    iron_runtime_field_t *method_field;
    void *delegate_object;
    void *target;
    void *method_pointer;

    if (!delegate_type || !args || arg_count < 3) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Delegate constructor requires this, target, and method pointer");
    }

    delegate_object = args[0].value.obj;
    target = args[1].value.obj;
    method_pointer = args[2].value.ptr;
    if (!delegate_object || !method_pointer) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Delegate constructor received an invalid method pointer");
    }

    target_field = iron_type_find_instance_field(delegate_type, "_target");
    method_field = iron_type_find_instance_field(delegate_type, "_methodPtr");
    if (!target_field || !method_field) {
        return IRON_ERROR(IRON_ERR_INVALID_TYPE, "Delegate layout does not contain the runtime target fields");
    }

    delegate_header = IRON_GC_HEADER(delegate_object);
    if ((iron_size)target_field->offset > delegate_header->size || sizeof(target) > delegate_header->size - target_field->offset ||
        (iron_size)method_field->offset > delegate_header->size || sizeof(method_pointer) > delegate_header->size - method_field->offset) {
        IRON_ERROR_EXEC("Delegate layout exceeds allocated storage: runtime=%s constructor=%s size=%zu target=%s+%u method=%s+%u",
                        delegate_header->type && delegate_header->type->full_name ? delegate_header->type->full_name : "<unknown type>",
                        delegate_type->full_name ? delegate_type->full_name : "<unknown type>",
                        delegate_header->size,
                        target_field->declaring_type && target_field->declaring_type->full_name ? target_field->declaring_type->full_name : "<unknown type>",
                        target_field->offset,
                        method_field->declaring_type && method_field->declaring_type->full_name ? method_field->declaring_type->full_name : "<unknown type>",
                        method_field->offset);
        {
            iron_runtime_type_t *current_type;

            for (current_type = delegate_header->type; current_type; current_type = current_type->base_type) {
                IRON_ERROR_EXEC("Delegate hierarchy layout: %s size=%u alignment=%u computed=%u",
                                current_type->full_name ? current_type->full_name : "<unknown type>",
                                current_type->instance_size,
                                current_type->alignment,
                                current_type->layout_computed);
                {
                    iron_u32 field_index;

                    for (field_index = 0; field_index < current_type->field_count; field_index++) {
                        iron_runtime_field_t *field;

                        field = current_type->fields[field_index];
                        IRON_ERROR_EXEC("Delegate hierarchy field: %s attrs=0x%04X offset=%u size=%u",
                                        field && field->name ? field->name : "<unknown field>",
                                        field ? field->attrs : 0,
                                        field ? field->offset : 0,
                                        field ? field->size : 0);
                    }
                }
            }
        }
        return IRON_ERROR(IRON_ERR_INVALID_TYPE, "Delegate runtime layout exceeds its allocated object storage");
    }

    iron_gc_write_barrier(delegate_object, (void **)((iron_u8 *)delegate_object + target_field->offset), target);
    memcpy((iron_u8 *)delegate_object + method_field->offset, &method_pointer, sizeof(method_pointer));
    return IRON_SUCCESS;
}

static iron_result_t invoke_delegate(iron_exec_context_t *ctx, iron_runtime_type_t *delegate_type, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_field_t *invocation_list_field;
    iron_runtime_field_t *target_field;
    iron_runtime_field_t *method_field;
    iron_runtime_method_t *target_method;
    iron_stack_value_t *target_args;
    iron_u32 invoke_arg_count;
    iron_u32 target_arg_count;
    void *delegate_object;
    void *target;
    void *method_pointer;
    iron_bool prepend_target;
    iron_result_t execution_result;

    if (!ctx || !delegate_type || !args || arg_count == 0 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Invalid delegate invocation parameters");
    }

    delegate_object = args[0].value.obj;
    if (!delegate_object) {
        return IRON_ERROR(IRON_ERR_NULL_REFERENCE, "Cannot invoke a null delegate");
    }

    invocation_list_field = iron_type_find_instance_field(delegate_type, "_invocationList");
    if (invocation_list_field) {
        void *invocation_list;

        memcpy(&invocation_list, (iron_u8 *)delegate_object + invocation_list_field->offset, sizeof(invocation_list));
        if (invocation_list) {
            void **delegates;
            iron_u32 delegate_count;
            iron_u32 delegate_index;

            delegates = (void **)iron_array_get_data(invocation_list);
            delegate_count = iron_array_get_length(invocation_list);
            for (delegate_index = 0; delegate_index < delegate_count; delegate_index++) {
                iron_stack_value_t *multicast_args;
                iron_u32 argument_index;

                multicast_args = (iron_stack_value_t *)iron_alloc(ctx->allocator, arg_count * sizeof(iron_stack_value_t));
                if (!multicast_args) {
                    return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate multicast delegate arguments");
                }

                multicast_args[0].type = IRON_VAL_OBJ;
                multicast_args[0].value.obj = delegates[delegate_index];
                for (argument_index = 1; argument_index < arg_count; argument_index++) {
                    multicast_args[argument_index] = args[argument_index];
                }

                execution_result = invoke_delegate(ctx, IRON_GC_HEADER(delegates[delegate_index])->type, multicast_args, arg_count, result);
                iron_free(ctx->allocator, multicast_args, arg_count * sizeof(iron_stack_value_t));
                if (!IRON_RESULT_OK(execution_result)) {
                    return execution_result;
                }
            }

            return IRON_SUCCESS;
        }
    }

    target_field = iron_type_find_instance_field(delegate_type, "_target");
    method_field = iron_type_find_instance_field(delegate_type, "_methodPtr");
    if (!target_field || !method_field) {
        return IRON_ERROR(IRON_ERR_INVALID_TYPE, "Delegate layout does not contain the runtime target fields");
    }

    memcpy(&target, (iron_u8 *)delegate_object + target_field->offset, sizeof(target));
    memcpy(&method_pointer, (iron_u8 *)delegate_object + method_field->offset, sizeof(method_pointer));
    target_method = (iron_runtime_method_t *)method_pointer;
    if (!target_method) {
        return IRON_ERROR(IRON_ERR_INVALID_PROGRAM, "Delegate has no target method");
    }

    invoke_arg_count = arg_count - 1;
    prepend_target = target != NULL && (((target_method->attrs & 0x0010) == 0) || target_method->param_count == invoke_arg_count + 1);
    target_arg_count = invoke_arg_count + (prepend_target ? 1 : 0);
    target_args = NULL;
    if (target_arg_count != 0) {
        iron_u32 source_index;
        iron_u32 destination_index;

        target_args = (iron_stack_value_t *)iron_alloc(ctx->allocator, target_arg_count * sizeof(iron_stack_value_t));
        if (!target_args) {
            return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate delegate target arguments");
        }

        destination_index = 0;
        if (prepend_target) {
            memset(&target_args[0], 0, sizeof(target_args[0]));
            target_args[0].type = IRON_VAL_OBJ;
            target_args[0].value.obj = target;
            destination_index = 1;
        }

        for (source_index = 1; source_index < arg_count; source_index++) {
            target_args[destination_index++] = args[source_index];
        }
    }

    execution_result = iron_exec_method(ctx, target_method, target_args, target_arg_count, result);
    if (target_args) {
        iron_free(ctx->allocator, target_args, target_arg_count * sizeof(iron_stack_value_t));
    }

    return execution_result;
}

iron_result_t iron_exec_invoke_delegate(iron_thread_context_t *thread,
                                        void *delegate_object,
                                        void *parameter,
                                        iron_bool has_parameter,
                                        iron_stack_value_t *result)
{
    iron_stack_value_t arguments[2];
    iron_u32 argument_count;

    if (!thread || !thread->exec_ctx || !delegate_object || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Invalid managed delegate invocation");
    }

    memset(arguments, 0, sizeof(arguments));
    arguments[0].type = IRON_VAL_OBJ;
    arguments[0].value.obj = delegate_object;
    argument_count = 1;

    if (has_parameter) {
        arguments[1].type = IRON_VAL_OBJ;
        arguments[1].value.obj = parameter;
        argument_count++;
    }

    return invoke_delegate(thread->exec_ctx, IRON_GC_HEADER(delegate_object)->type, arguments, argument_count, result);
}

static void update_method_assembly(iron_runtime_method_t *method, iron_assembly_t **assembly)
{
    if (!method || !assembly || !method->declaring_type || !method->declaring_type->module) {
        return;
    }

    *assembly = method->declaring_type->module->assembly;
}

static void *box_constrained_value(iron_exec_context_t *ctx, iron_runtime_type_t *type, const iron_stack_value_t *value)
{
    const void *source;
    void *boxed;
    iron_size value_size;

    if (!ctx || !type || !value) {
        return NULL;
    }

    value_size = type->instance_size;
    if (value_size == 0) {
        return NULL;
    }

    if (value->type == IRON_VAL_PTR) {
        source = value->value.ptr;
    } else if (value->type == IRON_VAL_BYREF) {
        source = value->value.byref.ptr;
    } else if (value->type == IRON_VAL_VALUETYPE) {
        source = value->value.obj;
    } else {
        source = &value->value;
    }

    if (!source) {
        return NULL;
    }

    boxed = iron_gc_alloc_object(&ctx->gc, type, value_size);
    if (!boxed) {
        return NULL;
    }

    memcpy(boxed, source, value_size);
    return boxed;
}

typedef enum iron_compare_relation {
    IRON_COMPARE_LESS,
    IRON_COMPARE_LESS_OR_EQUAL,
    IRON_COMPARE_GREATER,
    IRON_COMPARE_GREATER_OR_EQUAL
} iron_compare_relation_t;

static iron_bool stack_value_is_float(const iron_stack_value_t *value)
{
    return value->type == IRON_VAL_F32 || value->type == IRON_VAL_F64;
}

static iron_f64 stack_value_to_f64(const iron_stack_value_t *value)
{
    if (value->type == IRON_VAL_F32) {
        return (iron_f64)value->value.f32;
    }

    return value->value.f64;
}

static iron_i64 stack_value_to_i64(const iron_stack_value_t *value)
{
    if (value->type == IRON_VAL_I32) {
        return (iron_i64)value->value.i32;
    }

    if (value->type == IRON_VAL_PTR || value->type == IRON_VAL_METHOD_PTR || value->type == IRON_VAL_OBJ) {
        return (iron_i64)(iron_size)value->value.ptr;
    }

    if (value->type == IRON_VAL_BYREF) {
        return (iron_i64)(iron_size)value->value.byref.ptr;
    }

    return value->value.i64;
}

static iron_u64 stack_value_to_u64(const iron_stack_value_t *value)
{
    if (value->type == IRON_VAL_I32) {
        return (iron_u64)(iron_u32)value->value.i32;
    }

    if (value->type == IRON_VAL_PTR || value->type == IRON_VAL_METHOD_PTR || value->type == IRON_VAL_OBJ) {
        return (iron_u64)(iron_size)value->value.ptr;
    }

    if (value->type == IRON_VAL_BYREF) {
        return (iron_u64)(iron_size)value->value.byref.ptr;
    }

    return (iron_u64)value->value.i64;
}

static iron_bool stack_value_is_zero(const iron_stack_value_t *value)
{
    switch (value->type) {
        case IRON_VAL_I32:
            return value->value.i32 == 0;
        case IRON_VAL_I64:
            return value->value.i64 == 0;
        case IRON_VAL_F32:
            return value->value.f32 == 0.0f;
        case IRON_VAL_F64:
            return value->value.f64 == 0.0;
        case IRON_VAL_PTR:
        case IRON_VAL_METHOD_PTR:
        case IRON_VAL_OBJ:
            return value->value.ptr == NULL;
        case IRON_VAL_BYREF:
            return value->value.byref.ptr == NULL;
        default:
            return IRON_FALSE;
    }
}

static iron_bool stack_values_equal(const iron_stack_value_t *left, const iron_stack_value_t *right)
{
    if (stack_value_is_float(left) || stack_value_is_float(right)) {
        iron_f64 left_value;
        iron_f64 right_value;

        left_value = stack_value_to_f64(left);
        right_value = stack_value_to_f64(right);
        return left_value == right_value;
    }

    return stack_value_to_u64(left) == stack_value_to_u64(right);
}

static iron_bool stack_values_compare(const iron_stack_value_t *left,
                                      const iron_stack_value_t *right,
                                      iron_compare_relation_t relation,
                                      iron_bool unsigned_or_unordered)
{
    if (stack_value_is_float(left) || stack_value_is_float(right)) {
        iron_f64 left_value;
        iron_f64 right_value;
        iron_bool unordered;

        left_value = stack_value_to_f64(left);
        right_value = stack_value_to_f64(right);
        unordered = left_value != left_value || right_value != right_value;
        if (unordered) {
            return unsigned_or_unordered;
        }

        switch (relation) {
            case IRON_COMPARE_LESS:
                return left_value < right_value;
            case IRON_COMPARE_LESS_OR_EQUAL:
                return left_value <= right_value;
            case IRON_COMPARE_GREATER:
                return left_value > right_value;
            case IRON_COMPARE_GREATER_OR_EQUAL:
                return left_value >= right_value;
        }
    }

    if (unsigned_or_unordered) {
        iron_u64 left_value;
        iron_u64 right_value;

        left_value = stack_value_to_u64(left);
        right_value = stack_value_to_u64(right);
        switch (relation) {
            case IRON_COMPARE_LESS:
                return left_value < right_value;
            case IRON_COMPARE_LESS_OR_EQUAL:
                return left_value <= right_value;
            case IRON_COMPARE_GREATER:
                return left_value > right_value;
            case IRON_COMPARE_GREATER_OR_EQUAL:
                return left_value >= right_value;
        }
    } else {
        iron_i64 left_value;
        iron_i64 right_value;

        left_value = stack_value_to_i64(left);
        right_value = stack_value_to_i64(right);
        switch (relation) {
            case IRON_COMPARE_LESS:
                return left_value < right_value;
            case IRON_COMPARE_LESS_OR_EQUAL:
                return left_value <= right_value;
            case IRON_COMPARE_GREATER:
                return left_value > right_value;
            case IRON_COMPARE_GREATER_OR_EQUAL:
                return left_value >= right_value;
        }
    }

    return IRON_FALSE;
}

static void stack_push_integer_bits(iron_eval_stack_t *stack, iron_u64 bits, iron_u32 width)
{
    if (width <= 32) {
        iron_u32 narrowed;
        iron_i32 value;

        narrowed = (iron_u32)bits;
        memcpy(&value, &narrowed, sizeof(value));
        iron_stack_push_i32(stack, value);
    } else {
        iron_i64 value;

        memcpy(&value, &bits, sizeof(value));
        iron_stack_push_i64(stack, value);
    }
}

static iron_u32 arithmetic_shift_right_u32(iron_i32 value, iron_u32 shift)
{
    iron_u32 bits;

    bits = (iron_u32)value;
    if (shift == 0) {
        return bits;
    }

    bits >>= shift;
    if (value < 0) {
        bits |= UINT32_MAX << (32U - shift);
    }

    return bits;
}

static iron_u64 arithmetic_shift_right_u64(iron_i64 value, iron_u32 shift)
{
    iron_u64 bits;

    bits = (iron_u64)value;
    if (shift == 0) {
        return bits;
    }

    bits >>= shift;
    if (value < 0) {
        bits |= UINT64_MAX << (64U - shift);
    }

    return bits;
}

static iron_bool checked_convert_integer(iron_eval_stack_t *stack,
                                         const iron_stack_value_t *source,
                                         iron_u32 target_width,
                                         iron_bool target_unsigned,
                                         iron_bool source_unsigned)
{
    iron_u64 unsigned_max;
    iron_u64 signed_max;
    iron_i64 signed_min;
    iron_u64 converted;

    if (!stack || !source || target_width == 0 || target_width > 64) {
        return IRON_FALSE;
    }

    unsigned_max = target_width == 64 ? UINT64_MAX : (((iron_u64)1 << target_width) - 1);
    signed_max = target_width == 64 ? (iron_u64)INT64_MAX : (((iron_u64)1 << (target_width - 1)) - 1);
    signed_min = target_width == 64 ? INT64_MIN : -(iron_i64)((iron_u64)1 << (target_width - 1));

    if (stack_value_is_float(source)) {
        long double floating_value;
        long double truncated_value;
        long double upper_exclusive;

        floating_value = (long double)stack_value_to_f64(source);
        if (!isfinite((double)floating_value)) {
            return IRON_FALSE;
        }

        truncated_value = truncl(floating_value);
        if (target_unsigned) {
            upper_exclusive = ldexpl(1.0L, (int)target_width);
            if (truncated_value < 0.0L || truncated_value >= upper_exclusive) {
                return IRON_FALSE;
            }
            converted = (iron_u64)truncated_value;
        } else {
            upper_exclusive = ldexpl(1.0L, (int)(target_width - 1));
            if (truncated_value < -upper_exclusive || truncated_value >= upper_exclusive) {
                return IRON_FALSE;
            }
            converted = (iron_u64)(iron_i64)truncated_value;
        }
    } else if (source_unsigned) {
        converted = stack_value_to_u64(source);
        if (converted > (target_unsigned ? unsigned_max : signed_max)) {
            return IRON_FALSE;
        }
    } else {
        iron_i64 signed_value;

        signed_value = stack_value_to_i64(source);
        if (target_unsigned) {
            if (signed_value < 0 || (iron_u64)signed_value > unsigned_max) {
                return IRON_FALSE;
            }
        } else if (signed_value < signed_min || (signed_value >= 0 && (iron_u64)signed_value > signed_max)) {
            return IRON_FALSE;
        }
        converted = (iron_u64)signed_value;
    }

    stack_push_integer_bits(stack, converted, target_width);
    return IRON_TRUE;
}

static iron_bool checked_signed_add(iron_i64 left, iron_i64 right, iron_i64 minimum, iron_i64 maximum, iron_i64 *result)
{
    if ((right > 0 && left > maximum - right) || (right < 0 && left < minimum - right)) {
        return IRON_FALSE;
    }

    *result = left + right;
    return IRON_TRUE;
}

static iron_bool checked_signed_subtract(iron_i64 left, iron_i64 right, iron_i64 minimum, iron_i64 maximum, iron_i64 *result)
{
    if ((right < 0 && left > maximum + right) || (right > 0 && left < minimum + right)) {
        return IRON_FALSE;
    }

    *result = left - right;
    return IRON_TRUE;
}

static iron_bool checked_signed_multiply(iron_i64 left, iron_i64 right, iron_i64 minimum, iron_i64 maximum, iron_i64 *result)
{
    if ((left > 0 && ((right > 0 && left > maximum / right) || (right < 0 && right < minimum / left))) ||
        (left < 0 && ((right > 0 && left < minimum / right) || (right < 0 && left < maximum / right)))) {
        return IRON_FALSE;
    }

    *result = left * right;
    return IRON_TRUE;
}

static iron_u32 stack_integer_width(const iron_stack_value_t *left, const iron_stack_value_t *right)
{
    if ((left && left->type == IRON_VAL_I64) || (right && right->type == IRON_VAL_I64)) {
        return 64;
    }
    if ((left && (left->type == IRON_VAL_PTR || left->type == IRON_VAL_BYREF)) ||
        (right && (right->type == IRON_VAL_PTR || right->type == IRON_VAL_BYREF))) {
        return (iron_u32)(sizeof(void *) * CHAR_BIT);
    }

    return 32;
}

typedef struct iron_internal_method_cache_entry {
    iron_assembly_t *assembly;
    iron_token_t token;
    iron_runtime_type_t *declaring_type;
    iron_runtime_method_t method;
    struct iron_internal_method_cache_entry *next;
} iron_internal_method_cache_entry_t;

typedef struct iron_vararg_handle {
    iron_stack_value_t *arguments;
    iron_u32 count;
} iron_vararg_handle_t;

static iron_runtime_method_t *cache_internal_method(iron_exec_context_t *ctx,
                                                     iron_assembly_t *assembly,
                                                     iron_token_t token,
                                                     const char *name,
                                                     iron_internal_call_fn internal_call,
                                                     iron_u32 param_count,
                                                     iron_bool is_static,
                                                     iron_runtime_type_t *declaring_type)
{
    iron_internal_method_cache_entry_t *entry;

    entry = (iron_internal_method_cache_entry_t *)ctx->resolved_internal_methods;
    while (entry) {
        if (entry->assembly == assembly && entry->token == token && entry->declaring_type == declaring_type) {
            return &entry->method;
        }

        entry = entry->next;
    }

    entry = (iron_internal_method_cache_entry_t *)iron_alloc(ctx->allocator, sizeof(iron_internal_method_cache_entry_t));
    if (!entry) {
        return NULL;
    }

    memset(entry, 0, sizeof(iron_internal_method_cache_entry_t));
    entry->assembly = assembly;
    entry->token = token;
    entry->declaring_type = declaring_type;
    entry->method.name = name;
    entry->method.token = token;
    entry->method.declaring_type = declaring_type;
    entry->method.is_internal_call = IRON_TRUE;
    entry->method.internal_call = (iron_method_invoke_fn)(void *)internal_call;
    entry->method.param_count = param_count;
    if (is_static) {
        entry->method.attrs = 0x0010;
    }

    entry->next = (iron_internal_method_cache_entry_t *)ctx->resolved_internal_methods;
    ctx->resolved_internal_methods = entry;
    return &entry->method;
}

static iron_runtime_method_t *cache_array_method(iron_exec_context_t *ctx,
                                                 iron_assembly_t *assembly,
                                                 iron_token_t token,
                                                 const char *name,
                                                 iron_runtime_type_t *array_type,
                                                 iron_u32 param_count)
{
    iron_internal_call_fn internal_call;
    iron_runtime_method_t *method;

    if (!ctx || !assembly || !name || !array_type || array_type->kind != IRON_KIND_ARRAY) {
        return NULL;
    }

    internal_call = NULL;
    if (strcmp(name, ".ctor") == 0 && (param_count == array_type->array_rank || param_count == array_type->array_rank * 2U)) {
        internal_call = invoke_array_constructor;
    } else if (strcmp(name, "Get") == 0 && param_count == array_type->array_rank) {
        internal_call = invoke_array_get;
    } else if (strcmp(name, "Set") == 0 && param_count == array_type->array_rank + 1U) {
        internal_call = invoke_array_set;
    } else if (strcmp(name, "Address") == 0 && param_count == array_type->array_rank) {
        internal_call = invoke_array_address;
    }

    if (!internal_call) {
        return NULL;
    }

    method = cache_internal_method(ctx, assembly, token, name, internal_call, param_count, IRON_FALSE, array_type);
    if (!method) {
        return NULL;
    }

    if (strcmp(name, ".ctor") == 0) {
        method->kind = IRON_METHOD_CONSTRUCTOR;
    }
    return method;
}

/* ============================================================================
 * Evaluation Stack Implementation
 * ============================================================================ */

void iron_stack_init(iron_eval_stack_t *stack, iron_allocator_t *alloc,
                     iron_u32 capacity)
{
    stack->data = (iron_stack_value_t *)iron_alloc(alloc,
        capacity * sizeof(iron_stack_value_t));
    stack->size = 0;
    stack->capacity = capacity;
}

void iron_stack_destroy(iron_eval_stack_t *stack, iron_allocator_t *alloc)
{
    if (stack->data) {
        iron_free(alloc, stack->data, stack->capacity * sizeof(iron_stack_value_t));
        stack->data = NULL;
    }
    stack->size = 0;
    stack->capacity = 0;
}

void iron_stack_push(iron_eval_stack_t *stack, iron_stack_value_t value)
{
    IRON_ASSERT(stack->size < stack->capacity);
    stack->data[stack->size++] = value;
}

iron_stack_value_t iron_stack_pop(iron_eval_stack_t *stack)
{
    IRON_ASSERT(stack->size > 0);
    return stack->data[--stack->size];
}

iron_stack_value_t *iron_stack_peek(iron_eval_stack_t *stack, iron_u32 depth)
{
    IRON_ASSERT(stack->size > depth);
    return &stack->data[stack->size - 1 - depth];
}

void iron_stack_dup(iron_eval_stack_t *stack)
{
    IRON_ASSERT(stack->size > 0 && stack->size < stack->capacity);
    stack->data[stack->size] = stack->data[stack->size - 1];
    stack->size++;
}

void iron_stack_clear(iron_eval_stack_t *stack)
{
    stack->size = 0;
}

void iron_stack_push_i32(iron_eval_stack_t *stack, iron_i32 value)
{
    iron_stack_value_t sv;
    memset(&sv, 0, sizeof(sv));
    sv.type = IRON_VAL_I32;
    sv.value.i32 = value;
    iron_stack_push(stack, sv);
}

void iron_stack_push_i64(iron_eval_stack_t *stack, iron_i64 value)
{
    iron_stack_value_t sv;
    memset(&sv, 0, sizeof(sv));
    sv.type = IRON_VAL_I64;
    sv.value.i64 = value;
    iron_stack_push(stack, sv);
}

void iron_stack_push_f32(iron_eval_stack_t *stack, iron_f32 value)
{
    iron_stack_value_t sv;
    memset(&sv, 0, sizeof(sv));
    sv.type = IRON_VAL_F32;
    sv.value.f32 = value;
    iron_stack_push(stack, sv);
}

void iron_stack_push_f64(iron_eval_stack_t *stack, iron_f64 value)
{
    iron_stack_value_t sv;
    memset(&sv, 0, sizeof(sv));
    sv.type = IRON_VAL_F64;
    sv.value.f64 = value;
    iron_stack_push(stack, sv);
}

void iron_stack_push_ptr(iron_eval_stack_t *stack, void *value)
{
    iron_stack_value_t sv;
    memset(&sv, 0, sizeof(sv));
    sv.type = IRON_VAL_PTR;
    sv.value.ptr = value;
    iron_stack_push(stack, sv);
}

static void iron_stack_push_slot_byref(iron_eval_stack_t *stack, iron_stack_value_t *slot)
{
    iron_stack_value_t value;

    memset(&value, 0, sizeof(value));
    value.type = IRON_VAL_BYREF;
    value.value.byref.ptr = slot && slot->type == IRON_VAL_VALUETYPE ? slot->value.obj : (slot ? (void *)&slot->value : NULL);
    value.value.byref.stack_slot = slot;
    iron_stack_push(stack, value);
}

static void iron_stack_push_method_ptr(iron_eval_stack_t *stack, iron_runtime_method_t *method)
{
    iron_stack_value_t value;

    memset(&value, 0, sizeof(value));
    value.type = IRON_VAL_METHOD_PTR;
    value.value.ptr = method;
    iron_stack_push(stack, value);
}

void iron_stack_push_obj(iron_eval_stack_t *stack, void *obj)
{
    iron_stack_value_t sv;
    memset(&sv, 0, sizeof(sv));
    sv.type = IRON_VAL_OBJ;
    sv.value.obj = obj;
    iron_stack_push(stack, sv);
}

void iron_stack_push_null(iron_eval_stack_t *stack)
{
    iron_stack_value_t sv;
    memset(&sv, 0, sizeof(sv));
    sv.type = IRON_VAL_OBJ;
    sv.value.obj = NULL;
    iron_stack_push(stack, sv);
}

iron_i32 iron_stack_pop_i32(iron_eval_stack_t *stack)
{
    iron_stack_value_t sv = iron_stack_pop(stack);
    switch (sv.type) {
        case IRON_VAL_I32: return sv.value.i32;
        case IRON_VAL_I64: return (iron_i32)sv.value.i64;
        case IRON_VAL_PTR: return (iron_i32)(iron_size)sv.value.ptr;
        default: return 0;
    }
}

iron_i64 iron_stack_pop_i64(iron_eval_stack_t *stack)
{
    iron_stack_value_t sv = iron_stack_pop(stack);
    switch (sv.type) {
        case IRON_VAL_I32: return (iron_i64)sv.value.i32;
        case IRON_VAL_I64: return sv.value.i64;
        case IRON_VAL_PTR: return (iron_i64)(iron_size)sv.value.ptr;
        default: return 0;
    }
}

iron_f32 iron_stack_pop_f32(iron_eval_stack_t *stack)
{
    iron_stack_value_t sv = iron_stack_pop(stack);
    return (sv.type == IRON_VAL_F32) ? sv.value.f32 : (iron_f32)sv.value.f64;
}

iron_f64 iron_stack_pop_f64(iron_eval_stack_t *stack)
{
    iron_stack_value_t sv = iron_stack_pop(stack);
    return (sv.type == IRON_VAL_F64) ? sv.value.f64 : (iron_f64)sv.value.f32;
}

void *iron_stack_pop_ptr(iron_eval_stack_t *stack)
{
    iron_stack_value_t sv = iron_stack_pop(stack);
    return sv.value.ptr;
}

void *iron_stack_pop_obj(iron_eval_stack_t *stack)
{
    iron_stack_value_t sv = iron_stack_pop(stack);
    return sv.value.obj;
}

/* ============================================================================
 * Execution Context
 * ============================================================================ */

static iron_result_t initialize_execution_synchronization(iron_exec_context_t *ctx)
{
    iron_tls_key_t *thread_tls;
    iron_mutex_t *thread_lock;
    iron_rmutex_t *execution_lock;
    iron_result_t result;

    thread_tls = (iron_tls_key_t *)iron_alloc(ctx->allocator, sizeof(*thread_tls));
    thread_lock = (iron_mutex_t *)iron_alloc(ctx->allocator, sizeof(*thread_lock));
    execution_lock = (iron_rmutex_t *)iron_alloc(ctx->allocator, sizeof(*execution_lock));
    if (!thread_tls || !thread_lock || !execution_lock) {
        if (thread_tls) {
            iron_free(ctx->allocator, thread_tls, sizeof(*thread_tls));
        }
        if (thread_lock) {
            iron_free(ctx->allocator, thread_lock, sizeof(*thread_lock));
        }
        if (execution_lock) {
            iron_free(ctx->allocator, execution_lock, sizeof(*execution_lock));
        }
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate execution synchronization state");
    }

    memset(thread_tls, 0, sizeof(*thread_tls));
    memset(thread_lock, 0, sizeof(*thread_lock));
    memset(execution_lock, 0, sizeof(*execution_lock));

    result = iron_tls_create(thread_tls);
    if (!IRON_RESULT_OK(result)) {
        goto synchronization_failed;
    }

    result = iron_mutex_init(thread_lock);
    if (!IRON_RESULT_OK(result)) {
        iron_tls_destroy(thread_tls);
        goto synchronization_failed;
    }

    result = iron_rmutex_init(execution_lock);
    if (!IRON_RESULT_OK(result)) {
        iron_mutex_destroy(thread_lock);
        iron_tls_destroy(thread_tls);
        goto synchronization_failed;
    }

    ctx->thread_tls = thread_tls;
    ctx->thread_lock = thread_lock;
    ctx->execution_lock = execution_lock;
    return IRON_SUCCESS;

synchronization_failed:
    iron_free(ctx->allocator, execution_lock, sizeof(*execution_lock));
    iron_free(ctx->allocator, thread_lock, sizeof(*thread_lock));
    iron_free(ctx->allocator, thread_tls, sizeof(*thread_tls));
    return result;
}

static void destroy_execution_synchronization(iron_exec_context_t *ctx)
{
    if (ctx->execution_lock) {
        iron_rmutex_destroy((iron_rmutex_t *)ctx->execution_lock);
        iron_free(ctx->allocator, ctx->execution_lock, sizeof(iron_rmutex_t));
        ctx->execution_lock = NULL;
    }
    if (ctx->thread_lock) {
        iron_mutex_destroy((iron_mutex_t *)ctx->thread_lock);
        iron_free(ctx->allocator, ctx->thread_lock, sizeof(iron_mutex_t));
        ctx->thread_lock = NULL;
    }
    if (ctx->thread_tls) {
        iron_tls_destroy((iron_tls_key_t *)ctx->thread_tls);
        iron_free(ctx->allocator, ctx->thread_tls, sizeof(iron_tls_key_t));
        ctx->thread_tls = NULL;
    }
}

iron_result_t iron_exec_create(iron_exec_context_t **out_ctx, iron_domain_t *domain)
{
    iron_exec_context_t *ctx;
    iron_thread_context_t *main_thread;
    iron_allocator_t *alloc;

    if (!out_ctx || !domain) {
        return IRON_ERROR(IRON_ERR_NULL_POINTER, "Invalid parameters");
    }

    alloc = domain->allocator;
    ctx = (iron_exec_context_t *)iron_alloc(alloc, sizeof(iron_exec_context_t));
    if (!ctx) {
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate exec context");
    }

    memset(ctx, 0, sizeof(iron_exec_context_t));
    ctx->allocator = alloc;
    ctx->domain = domain;
    ctx->next_thread_id = 2;

    /* Initialize internal call table */
    iron_hashmap_init(&ctx->internal_calls, alloc,
                      sizeof(const char*), sizeof(iron_internal_call_fn),
                      iron_hash_string, iron_string_eq);

    {
        iron_result_t synchronization_result;

        synchronization_result = initialize_execution_synchronization(ctx);
        if (!IRON_RESULT_OK(synchronization_result)) {
            iron_hashmap_destroy(&ctx->internal_calls);
            iron_free(alloc, ctx, sizeof(iron_exec_context_t));
            return synchronization_result;
        }
    }

    /* Create main thread context */
    main_thread = (iron_thread_context_t *)iron_alloc(alloc, sizeof(iron_thread_context_t));
    if (!main_thread) {
        destroy_execution_synchronization(ctx);
        iron_hashmap_destroy(&ctx->internal_calls);
        iron_free(alloc, ctx, sizeof(iron_exec_context_t));
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate main thread");
    }

    memset(main_thread, 0, sizeof(iron_thread_context_t));
    main_thread->exec_ctx = ctx;
    main_thread->allocator = alloc;
    main_thread->thread_id = 1;
    main_thread->name = "Main";
    main_thread->state = IRON_THREAD_RUNNING;
    main_thread->max_stack_depth = 1024;
    iron_arena_init(&main_thread->frame_arena, alloc, 4096);

    /* Initialize evaluation stack */
    iron_stack_init(&main_thread->eval_stack, alloc, 256);

    ctx->main_thread = main_thread;
    ctx->current_thread = main_thread;
    iron_tls_set((iron_tls_key_t *)ctx->thread_tls, main_thread);

    /* Initialize GC */
    {
        iron_result_t gc_res = iron_gc_init(&ctx->gc, ctx, NULL);
        if (!IRON_RESULT_OK(gc_res)) {
            iron_stack_destroy(&main_thread->eval_stack, alloc);
            main_thread->frame_arena.base.vt->destroy(&main_thread->frame_arena.base);
            iron_free(alloc, main_thread, sizeof(iron_thread_context_t));
            destroy_execution_synchronization(ctx);
            iron_hashmap_destroy(&ctx->internal_calls);
            iron_free(alloc, ctx, sizeof(iron_exec_context_t));
            return gc_res;
        }
    }

    *out_ctx = ctx;
    return IRON_SUCCESS;
}

iron_thread_context_t *iron_exec_get_current_thread(iron_exec_context_t *ctx)
{
    iron_thread_context_t *thread;

    if (!ctx || !ctx->thread_tls) {
        return NULL;
    }

    thread = (iron_thread_context_t *)iron_tls_get((iron_tls_key_t *)ctx->thread_tls);
    return thread ? thread : ctx->main_thread;
}

iron_thread_context_t *iron_exec_find_managed_thread(iron_exec_context_t *ctx, void *managed_thread)
{
    iron_thread_context_t *found;
    iron_u32 index;

    if (!ctx || !managed_thread || !ctx->thread_lock) {
        return NULL;
    }

    found = NULL;
    iron_mutex_lock((iron_mutex_t *)ctx->thread_lock);
    if (ctx->main_thread && ctx->main_thread->managed_thread == managed_thread) {
        found = ctx->main_thread;
    }
    for (index = 0; !found && index < ctx->thread_count; index++) {
        if (ctx->threads[index] && ctx->threads[index]->managed_thread == managed_thread) {
            found = ctx->threads[index];
        }
    }
    iron_mutex_unlock((iron_mutex_t *)ctx->thread_lock);
    return found;
}

iron_bool iron_exec_enter_execution(iron_exec_context_t *ctx)
{
    iron_thread_context_t *thread;

    if (!ctx || !ctx->execution_lock) {
        return IRON_FALSE;
    }

    thread = iron_exec_get_current_thread(ctx);
    if (!thread || thread->owns_execution_lock) {
        return IRON_FALSE;
    }

    iron_rmutex_lock((iron_rmutex_t *)ctx->execution_lock);
    thread->owns_execution_lock = IRON_TRUE;
    return IRON_TRUE;
}

void iron_exec_leave_execution(iron_exec_context_t *ctx, iron_bool acquired)
{
    iron_thread_context_t *thread;

    if (!ctx || !ctx->execution_lock || !acquired) {
        return;
    }

    thread = iron_exec_get_current_thread(ctx);
    if (thread && thread->owns_execution_lock) {
        thread->owns_execution_lock = IRON_FALSE;
        iron_rmutex_unlock((iron_rmutex_t *)ctx->execution_lock);
    }
}

iron_bool iron_exec_suspend_execution(iron_exec_context_t *ctx)
{
    iron_thread_context_t *thread;

    if (!ctx || !ctx->execution_lock) {
        return IRON_FALSE;
    }

    thread = iron_exec_get_current_thread(ctx);
    if (!thread || !thread->owns_execution_lock) {
        return IRON_FALSE;
    }

    thread->owns_execution_lock = IRON_FALSE;
    iron_rmutex_unlock((iron_rmutex_t *)ctx->execution_lock);
    return IRON_TRUE;
}

void iron_exec_resume_execution(iron_exec_context_t *ctx, iron_bool suspended)
{
    iron_thread_context_t *thread;

    if (!ctx || !ctx->execution_lock || !suspended) {
        return;
    }

    thread = iron_exec_get_current_thread(ctx);
    if (thread && !thread->owns_execution_lock) {
        iron_rmutex_lock((iron_rmutex_t *)ctx->execution_lock);
        thread->owns_execution_lock = IRON_TRUE;
    }
}

static iron_result_t add_thread_context(iron_exec_context_t *ctx, iron_thread_context_t *thread)
{
    iron_thread_context_t **threads;
    iron_u32 capacity;

    iron_mutex_lock((iron_mutex_t *)ctx->thread_lock);
    if (ctx->thread_count == ctx->thread_capacity) {
        capacity = ctx->thread_capacity ? ctx->thread_capacity * 2 : 4;
        threads = (iron_thread_context_t **)iron_realloc(ctx->allocator,
                                                         ctx->threads,
                                                         (iron_size)ctx->thread_capacity * sizeof(*threads),
                                                         (iron_size)capacity * sizeof(*threads));
        if (!threads) {
            iron_mutex_unlock((iron_mutex_t *)ctx->thread_lock);
            return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to grow the managed thread list");
        }

        ctx->threads = threads;
        ctx->thread_capacity = capacity;
    }

    ctx->threads[ctx->thread_count++] = thread;
    iron_mutex_unlock((iron_mutex_t *)ctx->thread_lock);
    return IRON_SUCCESS;
}

static void remove_thread_context(iron_exec_context_t *ctx, iron_thread_context_t *thread)
{
    iron_u32 index;

    if (!ctx || !ctx->thread_lock) {
        return;
    }

    iron_mutex_lock((iron_mutex_t *)ctx->thread_lock);
    for (index = 0; index < ctx->thread_count; index++) {
        if (ctx->threads[index] == thread) {
            ctx->thread_count--;
            ctx->threads[index] = ctx->threads[ctx->thread_count];
            break;
        }
    }
    iron_mutex_unlock((iron_mutex_t *)ctx->thread_lock);
}

static iron_result_t allocate_thread_context(iron_thread_context_t **out_thread, iron_exec_context_t *ctx)
{
    iron_thread_context_t *thread;
    iron_result_t result;

    if (!out_thread || !ctx) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Invalid managed thread context arguments");
    }

    thread = (iron_thread_context_t *)iron_alloc(ctx->allocator, sizeof(*thread));
    if (!thread) {
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate a managed thread context");
    }

    memset(thread, 0, sizeof(*thread));
    thread->exec_ctx = ctx;
    thread->allocator = ctx->allocator;
    thread->thread_id = ctx->next_thread_id++;
    thread->state = IRON_THREAD_CREATED;
    thread->max_stack_depth = 1024;
    thread->execution_result = IRON_SUCCESS;
    iron_arena_init(&thread->frame_arena, ctx->allocator, 4096);
    iron_stack_init(&thread->eval_stack, ctx->allocator, 256);

    result = add_thread_context(ctx, thread);
    if (!IRON_RESULT_OK(result)) {
        iron_stack_destroy(&thread->eval_stack, ctx->allocator);
        thread->frame_arena.base.vt->destroy(&thread->frame_arena.base);
        iron_free(ctx->allocator, thread, sizeof(*thread));
        return result;
    }

    *out_thread = thread;
    return IRON_SUCCESS;
}

iron_result_t iron_thread_create(iron_thread_context_t **out_thread,
                                 iron_exec_context_t *ctx,
                                 iron_runtime_method_t *start_method,
                                 void *parameter)
{
    iron_thread_context_t *thread;
    iron_result_t result;

    if (!start_method) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "A managed thread requires a start method");
    }

    result = allocate_thread_context(&thread, ctx);
    if (!IRON_RESULT_OK(result)) {
        return result;
    }

    thread->start_method = start_method;
    thread->start_parameter = parameter;
    thread->start_has_parameter = start_method->param_count != 0;
    *out_thread = thread;
    return IRON_SUCCESS;
}

iron_result_t iron_thread_create_delegate(iron_thread_context_t **out_thread,
                                          iron_exec_context_t *ctx,
                                          void *managed_thread,
                                          void *start_delegate,
                                          void *parameter,
                                          iron_bool has_parameter)
{
    iron_thread_context_t *thread;
    iron_result_t result;

    if (!managed_thread || !start_delegate) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "A managed thread requires an object and a start delegate");
    }

    result = allocate_thread_context(&thread, ctx);
    if (!IRON_RESULT_OK(result)) {
        return result;
    }

    thread->managed_thread = managed_thread;
    thread->start_delegate = start_delegate;
    thread->start_parameter = parameter;
    thread->start_has_parameter = has_parameter;
    *out_thread = thread;
    return IRON_SUCCESS;
}

static void *managed_thread_entry(void *argument)
{
    iron_thread_context_t *thread;
    iron_stack_value_t result_value;

    thread = (iron_thread_context_t *)argument;
    iron_tls_set((iron_tls_key_t *)thread->exec_ctx->thread_tls, thread);
    iron_atomic_store_i32((volatile iron_i32 *)&thread->state, IRON_THREAD_RUNNING);
    memset(&result_value, 0, sizeof(result_value));
    result_value.type = IRON_VAL_VOID;

    if (thread->start_delegate) {
        thread->execution_result = iron_exec_invoke_delegate(thread,
                                                              thread->start_delegate,
                                                              thread->start_parameter,
                                                              thread->start_has_parameter,
                                                              &result_value);
    } else if (thread->start_method) {
        iron_stack_value_t method_argument;

        memset(&method_argument, 0, sizeof(method_argument));
        method_argument.type = IRON_VAL_OBJ;
        method_argument.value.obj = thread->start_parameter;
        thread->execution_result = iron_exec_method(thread->exec_ctx,
                                                     thread->start_method,
                                                     thread->start_has_parameter ? &method_argument : NULL,
                                                     thread->start_has_parameter ? 1 : 0,
                                                     &result_value);
    }

    iron_atomic_store_i32((volatile iron_i32 *)&thread->state, IRON_THREAD_STOPPED);
    iron_tls_set((iron_tls_key_t *)thread->exec_ctx->thread_tls, NULL);
    return thread;
}

iron_result_t iron_thread_ctx_start(iron_thread_context_t *thread)
{
    iron_thread_t *native_thread;
    iron_result_t result;

    if (!thread || !thread->exec_ctx) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Invalid managed thread context");
    }
    if (iron_atomic_load_i32((const volatile iron_i32 *)&thread->state) != IRON_THREAD_CREATED || thread->native_thread) {
        return IRON_ERROR(IRON_ERR_INVALID_STATE, "A managed thread can only be started once");
    }

    native_thread = (iron_thread_t *)iron_alloc(thread->allocator, sizeof(*native_thread));
    if (!native_thread) {
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate the native thread handle");
    }

    memset(native_thread, 0, sizeof(*native_thread));
    iron_atomic_store_i32((volatile iron_i32 *)&thread->state, IRON_THREAD_RUNNING);
    result = iron_thread_create_simple(native_thread, managed_thread_entry, thread);
    if (!IRON_RESULT_OK(result)) {
        iron_atomic_store_i32((volatile iron_i32 *)&thread->state, IRON_THREAD_CREATED);
        iron_free(thread->allocator, native_thread, sizeof(*native_thread));
        return result;
    }

    thread->native_thread = native_thread;
    return IRON_SUCCESS;
}

iron_result_t iron_thread_ctx_join(iron_thread_context_t *thread, iron_u32 timeout_ms)
{
    iron_bool execution_suspended;
    iron_u64 deadline;
    iron_result_t result;

    if (!thread || !thread->native_thread) {
        return IRON_ERROR(IRON_ERR_INVALID_STATE, "The managed thread has not been started");
    }
    if (thread == iron_exec_get_current_thread(thread->exec_ctx)) {
        return IRON_ERROR(IRON_ERR_DEADLOCK, "A managed thread cannot join itself");
    }
    if (thread->native_joined) {
        return IRON_SUCCESS;
    }

    deadline = timeout_ms == UINT32_MAX ? UINT64_MAX : iron_platform_monotonic_milliseconds() + timeout_ms;
    execution_suspended = iron_exec_suspend_execution(thread->exec_ctx);
    while (iron_atomic_load_i32((const volatile iron_i32 *)&thread->state) != IRON_THREAD_STOPPED) {
        if (deadline != UINT64_MAX && iron_platform_monotonic_milliseconds() >= deadline) {
            iron_exec_resume_execution(thread->exec_ctx, execution_suspended);
            return IRON_ERROR(IRON_ERR_TIMEOUT, "The managed thread join operation timed out");
        }

        iron_thread_sleep(1);
    }

    result = iron_thread_join((iron_thread_t *)thread->native_thread, NULL);
    if (IRON_RESULT_OK(result)) {
        thread->native_joined = IRON_TRUE;
    }
    iron_exec_resume_execution(thread->exec_ctx, execution_suspended);
    return result;
}

void iron_thread_destroy(iron_thread_context_t *thread)
{
    iron_exec_context_t *ctx;

    if (!thread || thread == thread->exec_ctx->main_thread) {
        return;
    }

    ctx = thread->exec_ctx;
    if (thread->native_thread && !thread->native_joined) {
        (void)iron_thread_ctx_join(thread, UINT32_MAX);
    }
    if (thread->native_thread) {
        iron_free(thread->allocator, thread->native_thread, sizeof(iron_thread_t));
    }

    remove_thread_context(ctx, thread);
    iron_stack_destroy(&thread->eval_stack, thread->allocator);
    thread->frame_arena.base.vt->destroy(&thread->frame_arena.base);
    iron_free(thread->allocator, thread, sizeof(*thread));
}

void iron_exec_destroy(iron_exec_context_t *ctx)
{
    iron_internal_method_cache_entry_t *entry;
    iron_internal_method_cache_entry_t *next;

    if (!ctx) {
        return;
    }

    while (ctx->thread_count != 0) {
        iron_thread_destroy(ctx->threads[ctx->thread_count - 1]);
    }
    if (ctx->threads) {
        iron_free(ctx->allocator, ctx->threads, (iron_size)ctx->thread_capacity * sizeof(*ctx->threads));
        ctx->threads = NULL;
    }

    entry = (iron_internal_method_cache_entry_t *)ctx->resolved_internal_methods;
    while (entry) {
        next = entry->next;
        iron_free(ctx->allocator, entry, sizeof(iron_internal_method_cache_entry_t));
        entry = next;
    }

    iron_gc_shutdown(&ctx->gc);
    if (ctx->main_thread) {
        iron_stack_destroy(&ctx->main_thread->eval_stack, ctx->allocator);
        ctx->main_thread->frame_arena.base.vt->destroy(&ctx->main_thread->frame_arena.base);
        iron_free(ctx->allocator, ctx->main_thread, sizeof(iron_thread_context_t));
    }

    destroy_execution_synchronization(ctx);
    iron_hashmap_destroy(&ctx->internal_calls);
    iron_free(ctx->allocator, ctx, sizeof(iron_exec_context_t));
}

/* ============================================================================
 * Internal Call Registration
 * ============================================================================ */

iron_result_t iron_register_internal_call(iron_exec_context_t *ctx,
                                          const char *type_name,
                                          const char *method_name,
                                          const char *signature,
                                          iron_internal_call_fn fn)
{
    char key[512];
    const char *key_ptr;

    if (!ctx || !type_name || !method_name || !fn) {
        return IRON_ERROR(IRON_ERR_NULL_POINTER, "Invalid parameters");
    }

    /* Build key: "TypeName::MethodName(Signature)" */
    snprintf(key, sizeof(key), "%s::%s(%s)", type_name, method_name,
             signature ? signature : "");

    key_ptr = iron_intern_cstr(&ctx->domain->interner, key);
    if (!key_ptr || !iron_hashmap_set(&ctx->internal_calls, &key_ptr, &fn)) {
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to register internal call");
    }

    return IRON_SUCCESS;
}

iron_result_t iron_register_internal_calls(iron_exec_context_t *ctx,
                                           const iron_internal_call_t *calls,
                                           iron_u32 count)
{
    iron_u32 i;
    iron_result_t result;

    for (i = 0; i < count; i++) {
        result = iron_register_internal_call(ctx, calls[i].type_name,
                                             calls[i].method_name,
                                             calls[i].signature,
                                             calls[i].fn);
        if (!IRON_RESULT_OK(result)) {
            return result;
        }
    }

    return IRON_SUCCESS;
}

iron_internal_call_fn iron_lookup_internal_call(iron_exec_context_t *ctx,
                                                iron_runtime_method_t *method)
{
    char key[512];
    const char *key_ptr;
    iron_internal_call_fn fn = NULL;
    const char *type_name;
    iron_assembly_t *assembly;

    if (!ctx || !method) return NULL;

    /* Get type name - prefer full_name, fallback to namespace.name */
    type_name = "";
    if (method->declaring_type) {
        if (method->declaring_type->full_name) {
            type_name = method->declaring_type->full_name;
        } else if (method->declaring_type->namespace_ && method->declaring_type->name) {
            char type_buf[256];
            snprintf(type_buf, sizeof(type_buf), "%s.%s",
                     method->declaring_type->namespace_, method->declaring_type->name);
            type_name = type_buf;
        } else if (method->declaring_type->name) {
            type_name = method->declaring_type->name;
        }
    }

    /* Build key from method info - try with empty signature first */
    snprintf(key, sizeof(key), "%s::%s()", type_name, method->name ? method->name : "");
    IRON_TRACE_EXEC("Internal call lookup: %s", key);

    key_ptr = key;
    if (iron_hashmap_get(&ctx->internal_calls, &key_ptr, &fn)) {
        return fn;
    }

    assembly = method->declaring_type && method->declaring_type->module ? method->declaring_type->module->assembly : NULL;
    if (assembly && IRON_TOKEN_TABLE(method->token) == IRON_TABLE_METHOD_DEF) {
        iron_method_def_row_t method_row;
        iron_result_t metadata_result;

        metadata_result = iron_metadata_read_row(&assembly->metadata, method->token, &method_row);
        if (IRON_RESULT_OK(metadata_result) && method_row.signature != 0) {
            const iron_u8 *signature_data;
            iron_u32 signature_size;

            metadata_result = iron_metadata_get_blob(&assembly->metadata, method_row.signature, &signature_data, &signature_size);
            if (IRON_RESULT_OK(metadata_result)) {
                char signature[384];

                build_signature_string_with_metadata(&assembly->metadata, signature_data, signature_size, signature, sizeof(signature));
                snprintf(key, sizeof(key), "%s::%s(%s)", type_name, method->name ? method->name : "", signature);
                IRON_TRACE_EXEC("Internal call lookup: %s", key);
                key_ptr = key;
                if (iron_hashmap_get(&ctx->internal_calls, &key_ptr, &fn)) {
                    return fn;
                }
            }
        }
    }

    return NULL;
}

/* ============================================================================
 * Field Offset Resolution
 * ============================================================================ */

static iron_runtime_field_t *resolve_runtime_field(iron_thread_context_t *thread, iron_assembly_t *assembly, iron_u32 field_token)
{
    iron_member_ref_row_t member_ref;
    iron_token_t parent_token;

    if (!assembly || !assembly->module) {
        return NULL;
    }

    if (thread && IRON_TOKEN_TABLE(field_token) == IRON_TABLE_MEMBER_REF &&
        IRON_RESULT_OK(iron_metadata_read_row(&assembly->metadata, field_token, &member_ref))) {
        parent_token = iron_metadata_decode_coded(&assembly->metadata, IRON_CODED_MEMBER_REF_PARENT, member_ref.class_);
        if (IRON_TOKEN_TABLE(parent_token) == IRON_TABLE_TYPE_SPEC) {
            iron_runtime_type_t *declaring_type;
            const char *field_name;

            declaring_type = resolve_type_spec(thread, assembly, parent_token);
            field_name = iron_metadata_get_string(&assembly->metadata, member_ref.name);
            while (declaring_type) {
                iron_u32 field_index;

                for (field_index = 0; field_index < declaring_type->field_count; field_index++) {
                    iron_runtime_field_t *field;

                    field = declaring_type->fields[field_index];
                    if (field && field->name && field_name && strcmp(field->name, field_name) == 0) {
                        return field;
                    }
                }
                declaring_type = declaring_type->base_type;
            }
        }
    }

    return iron_field_resolve_token(assembly->module, field_token);
}

static iron_runtime_field_t *bind_runtime_field(void *object, iron_value_type_t object_value_type, iron_runtime_field_t *definition_field)
{
    iron_runtime_type_t *runtime_type;
    iron_runtime_type_t *current_type;
    iron_u32 field_index;

    if (!object || object_value_type != IRON_VAL_OBJ || !definition_field) {
        return definition_field;
    }

    runtime_type = IRON_GC_HEADER(object)->type;
    for (current_type = runtime_type; current_type; current_type = current_type->base_type) {
        for (field_index = 0; field_index < current_type->field_count; field_index++) {
            iron_runtime_field_t *candidate;

            candidate = current_type->fields[field_index];
            if (candidate && candidate->token == definition_field->token && candidate->declaring_type && definition_field->declaring_type &&
                candidate->declaring_type->module == definition_field->declaring_type->module) {
                return candidate;
            }
        }
    }

    return definition_field;
}

static iron_bool instance_field_fits_object(void *object, iron_value_type_t object_value_type, const iron_runtime_field_t *field)
{
    iron_gc_header_t *header;
    iron_size field_size;

    if (!object || !field || !field->field_type) {
        return IRON_FALSE;
    }
    if (object_value_type != IRON_VAL_OBJ) {
        return IRON_TRUE;
    }

    header = IRON_GC_HEADER(object);
    field_size = iron_type_storage_size(field->field_type);
    if ((iron_size)field->offset <= header->size && field_size <= header->size - field->offset) {
        return IRON_TRUE;
    }

    IRON_ERROR_EXEC("Instance field storage is outside the object: runtime=%s field=%s::%s offset=%u field_size=%zu object_size=%zu",
                    header->type && header->type->full_name ? header->type->full_name : "<unknown type>",
                    field->declaring_type && field->declaring_type->full_name ? field->declaring_type->full_name : "<unknown type>",
                    field->name ? field->name : "<unknown field>",
                    field->offset,
                    field_size,
                    header->size);
    return IRON_FALSE;
}

static void read_storage_element(iron_stack_value_t *result, const void *address, iron_element_type_t element_type)
{
    memset(result, 0, sizeof(*result));

    switch (element_type) {
        case IRON_TYPE_BOOLEAN:
        case IRON_TYPE_U1: {
            iron_u8 value;

            memcpy(&value, address, sizeof(value));
            result->type = IRON_VAL_I32;
            result->value.i32 = (iron_i32)value;
            break;
        }
        case IRON_TYPE_I1: {
            iron_i8 value;

            memcpy(&value, address, sizeof(value));
            result->type = IRON_VAL_I32;
            result->value.i32 = (iron_i32)value;
            break;
        }
        case IRON_TYPE_CHAR:
        case IRON_TYPE_U2: {
            iron_u16 value;

            memcpy(&value, address, sizeof(value));
            result->type = IRON_VAL_I32;
            result->value.i32 = (iron_i32)value;
            break;
        }
        case IRON_TYPE_I2: {
            iron_i16 value;

            memcpy(&value, address, sizeof(value));
            result->type = IRON_VAL_I32;
            result->value.i32 = (iron_i32)value;
            break;
        }
        case IRON_TYPE_I4:
        case IRON_TYPE_U4: {
            result->type = IRON_VAL_I32;
            memcpy(&result->value.i32, address, sizeof(result->value.i32));
            break;
        }
        case IRON_TYPE_I8:
        case IRON_TYPE_U8: {
            result->type = IRON_VAL_I64;
            memcpy(&result->value.i64, address, sizeof(result->value.i64));
            break;
        }
        case IRON_TYPE_R4: {
            result->type = IRON_VAL_F32;
            memcpy(&result->value.f32, address, sizeof(result->value.f32));
            break;
        }
        case IRON_TYPE_R8: {
            result->type = IRON_VAL_F64;
            memcpy(&result->value.f64, address, sizeof(result->value.f64));
            break;
        }
        case IRON_TYPE_I:
        case IRON_TYPE_U:
        case IRON_TYPE_PTR:
        case IRON_TYPE_FNPTR: {
            result->type = IRON_VAL_PTR;
            memcpy(&result->value.ptr, address, sizeof(result->value.ptr));
            break;
        }
        default: {
            result->type = IRON_VAL_OBJ;
            memcpy(&result->value.obj, address, sizeof(result->value.obj));
            break;
        }
    }
}

static void push_field_value(iron_eval_stack_t *stack, const void *address, iron_element_type_t element_type)
{
    iron_stack_value_t value;

    read_storage_element(&value, address, element_type);
    iron_stack_push(stack, value);
}

static void *stack_value_address(const iron_stack_value_t *value)
{
    if (!value) {
        return NULL;
    }
    if (value->type == IRON_VAL_BYREF) {
        return value->value.byref.ptr;
    }
    if (value->type == IRON_VAL_I32) {
        return (void *)(uintptr_t)(iron_u32)value->value.i32;
    }
    if (value->type == IRON_VAL_I64) {
        return (void *)(uintptr_t)(iron_u64)value->value.i64;
    }

    return value->value.ptr;
}

static iron_stack_value_t *stack_value_addressed_slot(const iron_stack_value_t *value)
{
    if (!value || value->type != IRON_VAL_BYREF) {
        return NULL;
    }

    return (iron_stack_value_t *)value->value.byref.stack_slot;
}

static iron_value_type_t stack_value_type_for_element(iron_element_type_t element_type)
{
    switch (element_type) {
        case IRON_TYPE_BOOLEAN:
        case IRON_TYPE_CHAR:
        case IRON_TYPE_I1:
        case IRON_TYPE_U1:
        case IRON_TYPE_I2:
        case IRON_TYPE_U2:
        case IRON_TYPE_I4:
        case IRON_TYPE_U4:
            return IRON_VAL_I32;
        case IRON_TYPE_I8:
        case IRON_TYPE_U8:
            return IRON_VAL_I64;
        case IRON_TYPE_R4:
            return IRON_VAL_F32;
        case IRON_TYPE_R8:
            return IRON_VAL_F64;
        case IRON_TYPE_I:
        case IRON_TYPE_U:
        case IRON_TYPE_PTR:
        case IRON_TYPE_FNPTR:
            return IRON_VAL_PTR;
        case IRON_TYPE_BYREF:
            return IRON_VAL_BYREF;
        case IRON_TYPE_VALUETYPE:
            return IRON_VAL_VALUETYPE;
        default:
            return IRON_VAL_OBJ;
    }
}

iron_runtime_type_t *iron_managed_reference_get_type(iron_domain_t *domain, void *object)
{
    if (!object) {
        return NULL;
    }
    if (domain && iron_domain_is_type_descriptor(domain, object)) {
        return domain->type_type;
    }

    return iron_gc_get_type(object);
}

iron_bool iron_managed_reference_is_assignable(iron_domain_t *domain, void *object, iron_runtime_type_t *target_type)
{
    iron_runtime_type_t *object_type;

    if (!object) {
        return IRON_TRUE;
    }
    if (!target_type) {
        return IRON_FALSE;
    }

    object_type = iron_managed_reference_get_type(domain, object);
    if (object_type && iron_type_is_assignable_to(object_type, target_type)) {
        return IRON_TRUE;
    }

    /* System.Array.GetEnumerator is the storage-efficient non-generic enumerator used by the CoreLib Array base class. When an SZ array is consumed
     * through IEnumerable<T>, the VM adapts its boxed Current value to T. Preserve type safety here by accepting the adapter only when its backing
     * array element is compatible with the requested IEnumerator<T>. */
    if (object_type && object_type->full_name && strcmp(object_type->full_name, "System.Array+ArrayEnumerator") == 0 &&
        target_type->generic_definition && target_type->generic_definition->full_name &&
        strcmp(target_type->generic_definition->full_name, "System.Collections.Generic.IEnumerator`1") == 0 &&
        target_type->generic_arg_count == 1 && target_type->generic_args && target_type->generic_args[0]) {
        iron_runtime_type_t *current_type;

        for (current_type = object_type; current_type; current_type = current_type->base_type) {
            iron_u32 field_index;

            for (field_index = 0; field_index < current_type->field_count; field_index++) {
                iron_runtime_field_t *field;

                field = current_type->fields[field_index];
                if (field && field->name && strcmp(field->name, "_array") == 0 &&
                    (iron_size)field->offset <= IRON_GC_HEADER(object)->size && sizeof(void *) <= IRON_GC_HEADER(object)->size - field->offset) {
                    void *array_object;
                    iron_runtime_type_t *array_type;
                    iron_runtime_type_t *requested_element;

                    memcpy(&array_object, (iron_u8 *)object + field->offset, sizeof(array_object));
                    array_type = iron_managed_reference_get_type(domain, array_object);
                    requested_element = target_type->generic_args[0];
                    if (!array_type || array_type->kind != IRON_KIND_ARRAY || !array_type->element) {
                        return IRON_FALSE;
                    }
                    if (array_type->element == requested_element) {
                        return IRON_TRUE;
                    }

                    return iron_type_is_managed_reference(array_type->element) && iron_type_is_managed_reference(requested_element) &&
                           iron_type_is_assignable_to(array_type->element, requested_element);
                }
            }
        }
    }

    return IRON_FALSE;
}

static void store_stack_slot_element(iron_stack_value_t *slot, iron_element_type_t element_type, const iron_stack_value_t *value)
{
    iron_value_type_t value_type;

    if (!slot || !value) {
        return;
    }

    value_type = stack_value_type_for_element(element_type);
    memset(&slot->value, 0, sizeof(slot->value));
    slot->type = value_type;

    switch (value_type) {
        case IRON_VAL_I32:
            if (element_type == IRON_TYPE_I1) {
                slot->value.i32 = (iron_i32)(iron_i8)value->value.i32;
            } else if (element_type == IRON_TYPE_BOOLEAN || element_type == IRON_TYPE_U1) {
                slot->value.i32 = (iron_i32)(iron_u8)value->value.i32;
            } else if (element_type == IRON_TYPE_I2) {
                slot->value.i32 = (iron_i32)(iron_i16)value->value.i32;
            } else if (element_type == IRON_TYPE_CHAR || element_type == IRON_TYPE_U2) {
                slot->value.i32 = (iron_i32)(iron_u16)value->value.i32;
            } else {
                slot->value.i32 = value->value.i32;
            }
            break;
        case IRON_VAL_I64:
            slot->value.i64 = value->type == IRON_VAL_I32 ? (iron_i64)value->value.i32 : value->value.i64;
            break;
        case IRON_VAL_F32:
            slot->value.f32 = value->type == IRON_VAL_F64 ? (iron_f32)value->value.f64 : value->value.f32;
            break;
        case IRON_VAL_F64:
            slot->value.f64 = value->type == IRON_VAL_F32 ? (iron_f64)value->value.f32 : value->value.f64;
            break;
        case IRON_VAL_PTR:
            slot->value.ptr = value->value.ptr;
            break;
        case IRON_VAL_BYREF:
            slot->value.byref = value->value.byref;
            break;
        default:
            slot->value.obj = value->value.obj;
            break;
    }
}

static void store_field_value(void *object, void *address, iron_element_type_t element_type, const iron_stack_value_t *value)
{
    switch (element_type) {
        case IRON_TYPE_BOOLEAN:
        case IRON_TYPE_I1:
        case IRON_TYPE_U1: {
            iron_u8 converted = (iron_u8)value->value.i32;
            memcpy(address, &converted, sizeof(converted));
            break;
        }
        case IRON_TYPE_CHAR:
        case IRON_TYPE_I2:
        case IRON_TYPE_U2: {
            iron_u16 converted = (iron_u16)value->value.i32;
            memcpy(address, &converted, sizeof(converted));
            break;
        }
        case IRON_TYPE_I4:
        case IRON_TYPE_U4:
            memcpy(address, &value->value.i32, sizeof(value->value.i32));
            break;
        case IRON_TYPE_I8:
        case IRON_TYPE_U8: {
            iron_i64 converted = value->type == IRON_VAL_I32 ? (iron_i64)value->value.i32 : value->value.i64;
            memcpy(address, &converted, sizeof(converted));
            break;
        }
        case IRON_TYPE_R4: {
            iron_f32 converted = value->type == IRON_VAL_F64 ? (iron_f32)value->value.f64 : value->value.f32;
            memcpy(address, &converted, sizeof(converted));
            break;
        }
        case IRON_TYPE_R8: {
            iron_f64 converted = value->type == IRON_VAL_F32 ? (iron_f64)value->value.f32 : value->value.f64;
            memcpy(address, &converted, sizeof(converted));
            break;
        }
        case IRON_TYPE_I:
        case IRON_TYPE_U:
        case IRON_TYPE_PTR:
        case IRON_TYPE_FNPTR:
            memcpy(address, &value->value.ptr, sizeof(value->value.ptr));
            break;
        default:
            iron_gc_write_barrier(object, (void **)address, value->value.obj);
            break;
    }
}

static iron_bool push_storage_value(iron_exec_context_t *ctx,
                                    iron_eval_stack_t *stack,
                                    const void *storage,
                                    iron_runtime_type_t *type)
{
    if (!ctx || !stack || !storage || !type) {
        return IRON_FALSE;
    }

    if (iron_type_is_managed_reference(type)) {
        void *object;

        memcpy(&object, storage, sizeof(object));
        iron_stack_push_obj(stack, object);
        return IRON_TRUE;
    }

    if (type->element_type != IRON_TYPE_END && type->element_type != IRON_TYPE_VALUETYPE) {
        push_field_value(stack, storage, type->element_type);
        return IRON_TRUE;
    }

    {
        void *value_copy;
        iron_size storage_size;

        storage_size = iron_type_storage_size(type);
        value_copy = iron_gc_alloc_object(&ctx->gc, type, storage_size);
        if (!value_copy) {
            return IRON_FALSE;
        }
        memcpy(value_copy, storage, storage_size);
        {
            iron_stack_value_t value;

            memset(&value, 0, sizeof(value));
            value.type = IRON_VAL_VALUETYPE;
            value.value.obj = value_copy;
            iron_stack_push(stack, value);
        }
    }

    return IRON_TRUE;
}

static iron_result_t adapt_interface_dispatch_return(iron_exec_context_t *ctx,
                                                     iron_eval_stack_t *stack,
                                                     iron_runtime_method_t *contract,
                                                     iron_runtime_method_t *implementation)
{
    iron_runtime_type_t *expected_type;
    iron_runtime_type_t *actual_type;
    iron_stack_value_t boxed_value;
    iron_stack_value_t adapted_value;
    iron_runtime_type_t *boxed_type;
    iron_u32 boxed_index;

    if (!ctx || !stack || !contract || !implementation || contract == implementation) {
        return IRON_SUCCESS;
    }
    if (!IRON_RESULT_OK(iron_method_load_signature(contract)) || !IRON_RESULT_OK(iron_method_load_signature(implementation))) {
        return IRON_ERROR(IRON_ERR_INVALID_SIGNATURE, "Cannot load an interface dispatch signature");
    }

    expected_type = contract->return_type;
    actual_type = implementation->return_type;
    if (!expected_type || !actual_type || expected_type == actual_type || iron_type_is_managed_reference(expected_type)) {
        return IRON_SUCCESS;
    }
    if (!iron_type_is_managed_reference(actual_type) || stack->size == 0) {
        return IRON_SUCCESS;
    }

    boxed_index = stack->size - 1U;
    boxed_value = stack->data[boxed_index];
    if ((boxed_value.type != IRON_VAL_OBJ && boxed_value.type != IRON_VAL_VALUETYPE) || !boxed_value.value.obj) {
        return IRON_ERROR(IRON_ERR_INVALID_CAST, "An interface adapter expected a boxed value");
    }

    boxed_type = iron_managed_reference_get_type(ctx->domain, boxed_value.value.obj);
    if (!boxed_type || !iron_type_is_assignable_to(boxed_type, expected_type)) {
        return IRON_ERROR(IRON_ERR_INVALID_CAST, "An interface adapter received an incompatible boxed value");
    }

    /* Keep the boxed object on the evaluation stack while push_storage_value performs any allocation.
     * This makes the source a GC root even when allocation triggers a collection. */
    if (!push_storage_value(ctx, stack, boxed_value.value.obj, expected_type)) {
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Cannot materialize an interface return value");
    }

    adapted_value = stack->data[stack->size - 1U];
    stack->data[boxed_index] = adapted_value;
    stack->size--;
    return IRON_SUCCESS;
}

static iron_bool store_storage_value(void *owner,
                                     void *storage,
                                     iron_runtime_type_t *type,
                                     const iron_stack_value_t *value)
{
    if (!storage || !type || !value) {
        return IRON_FALSE;
    }

    if (iron_type_is_managed_reference(type)) {
        void *object;

        object = value->value.obj;
        if (value->type != IRON_VAL_OBJ || !iron_managed_reference_is_assignable(type->module && type->module->assembly ? type->module->assembly->domain : NULL, object, type)) {
            return IRON_FALSE;
        }
        iron_gc_write_barrier(owner, (void **)storage, object);
        return IRON_TRUE;
    }

    if (type->element_type != IRON_TYPE_END && type->element_type != IRON_TYPE_VALUETYPE) {
        if ((value->type == IRON_VAL_OBJ || value->type == IRON_VAL_VALUETYPE) && value->value.obj) {
            iron_runtime_type_t *value_type;
            iron_size storage_size;

            value_type = IRON_GC_HEADER(value->value.obj)->type;
            storage_size = iron_type_storage_size(type);
            if (value_type != type || storage_size > value_type->instance_size) {
                return IRON_FALSE;
            }

            memcpy(storage, value->value.obj, storage_size);
            return IRON_TRUE;
        }
        if (value->type == IRON_VAL_BYREF && value->value.byref.ptr) {
            memcpy(storage, value->value.byref.ptr, iron_type_storage_size(type));
            return IRON_TRUE;
        }

        store_field_value(owner, storage, type->element_type, value);
        return IRON_TRUE;
    }

    {
        const void *source;
        iron_size storage_size;

        storage_size = iron_type_storage_size(type);
        if ((value->type == IRON_VAL_OBJ || value->type == IRON_VAL_VALUETYPE) && !value->value.obj) {
            source = NULL;
        } else if ((value->type == IRON_VAL_OBJ || value->type == IRON_VAL_VALUETYPE) && value->value.obj && IRON_GC_HEADER(value->value.obj)->type == type) {
            source = value->value.obj;
        } else if (value->type == IRON_VAL_PTR || value->type == IRON_VAL_BYREF) {
            source = stack_value_address(value);
        } else if (storage_size <= sizeof(value->value)) {
            source = &value->value;
        } else {
            return IRON_FALSE;
        }

        if (!source) {
            return IRON_FALSE;
        }
        memcpy(storage, source, storage_size);
    }

    return IRON_TRUE;
}

iron_bool iron_stack_value_load_from_storage(iron_exec_context_t *ctx,
                                             iron_stack_value_t *result,
                                             const void *storage,
                                             iron_runtime_type_t *type)
{
    if (!ctx || !result || !storage || !type) {
        return IRON_FALSE;
    }

    if (iron_type_is_managed_reference(type)) {
        memset(result, 0, sizeof(*result));
        result->type = IRON_VAL_OBJ;
        memcpy(&result->value.obj, storage, sizeof(result->value.obj));
        return IRON_TRUE;
    }

    if (type->element_type != IRON_TYPE_END && type->element_type != IRON_TYPE_VALUETYPE) {
        read_storage_element(result, storage, type->element_type);
        return IRON_TRUE;
    }

    result->value.obj = iron_gc_box(ctx, type, storage);
    if (!result->value.obj) {
        return IRON_FALSE;
    }
    result->type = IRON_VAL_VALUETYPE;
    return IRON_TRUE;
}

iron_bool iron_stack_value_store_to_storage(void *owner,
                                            void *storage,
                                            iron_runtime_type_t *type,
                                            const iron_stack_value_t *value)
{
    return store_storage_value(owner, storage, type, value);
}

static iron_result_t get_array_intrinsic_storage(iron_stack_value_t *args,
                                                 iron_u32 arg_count,
                                                 void **array,
                                                 iron_runtime_type_t **element_type,
                                                 void **storage)
{
    iron_i32 indices[32];
    iron_u32 rank;
    iron_u32 dimension;
    iron_size offset;

    if (!args || arg_count < 2 || !array || !element_type || !storage) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Invalid array intrinsic arguments");
    }

    *array = args[0].value.obj;
    if (!*array) {
        return IRON_ERROR(IRON_ERR_NULL_REFERENCE, "NullReferenceException");
    }

    rank = iron_array_get_rank(*array);
    if (rank == 0 || rank > 32 || arg_count != rank + 1U) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Array intrinsic index count does not match the array rank");
    }

    for (dimension = 0; dimension < rank; dimension++) {
        if (args[dimension + 1U].type != IRON_VAL_I32) {
            return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Array indices must be Int32 values");
        }
        indices[dimension] = args[dimension + 1U].value.i32;
    }

    if (!iron_array_get_element_offset(*array, indices, rank, &offset)) {
        return IRON_ERROR(IRON_ERR_INDEX_OUT_OF_RANGE, "IndexOutOfRangeException");
    }

    *element_type = iron_array_get_element_type(*array);
    if (!*element_type) {
        return IRON_ERROR(IRON_ERR_INVALID_TYPE, "Array has no element type");
    }

    *storage = (iron_u8 *)iron_array_get_data(*array) + offset;
    return IRON_SUCCESS;
}

static iron_result_t invoke_array_get(iron_exec_context_t *ctx,
                                      iron_stack_value_t *args,
                                      iron_u32 arg_count,
                                      iron_stack_value_t *result)
{
    void *array;
    void *storage;
    iron_runtime_type_t *element_type;
    iron_result_t resolution;

    if (!ctx || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Invalid array Get invocation");
    }

    resolution = get_array_intrinsic_storage(args, arg_count, &array, &element_type, &storage);
    if (!IRON_RESULT_OK(resolution)) {
        return resolution;
    }
    (void)array;

    if (!iron_stack_value_load_from_storage(ctx, result, storage, element_type)) {
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to load a multidimensional array element");
    }
    return IRON_SUCCESS;
}

static iron_result_t invoke_array_set(iron_exec_context_t *ctx,
                                      iron_stack_value_t *args,
                                      iron_u32 arg_count,
                                      iron_stack_value_t *result)
{
    void *array;
    void *storage;
    iron_runtime_type_t *element_type;
    iron_result_t resolution;

    (void)ctx;
    if (!args || arg_count < 3) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Invalid array Set invocation");
    }

    resolution = get_array_intrinsic_storage(args, arg_count - 1U, &array, &element_type, &storage);
    if (!IRON_RESULT_OK(resolution)) {
        return resolution;
    }

    if (!iron_stack_value_store_to_storage(array, storage, element_type, &args[arg_count - 1U])) {
        return IRON_ERROR(IRON_ERR_INVALID_CAST, "ArrayTypeMismatchException");
    }

    if (result) {
        result->type = IRON_VAL_VOID;
    }
    return IRON_SUCCESS;
}

static iron_result_t invoke_array_address(iron_exec_context_t *ctx,
                                          iron_stack_value_t *args,
                                          iron_u32 arg_count,
                                          iron_stack_value_t *result)
{
    void *array;
    void *storage;
    iron_runtime_type_t *element_type;
    iron_result_t resolution;

    (void)ctx;
    if (!result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Invalid array Address invocation");
    }

    resolution = get_array_intrinsic_storage(args, arg_count, &array, &element_type, &storage);
    if (!IRON_RESULT_OK(resolution)) {
        return resolution;
    }
    (void)array;
    (void)element_type;

    memset(result, 0, sizeof(*result));
    result->type = IRON_VAL_PTR;
    result->value.ptr = storage;
    return IRON_SUCCESS;
}

static iron_result_t invoke_array_constructor(iron_exec_context_t *ctx,
                                              iron_stack_value_t *args,
                                              iron_u32 arg_count,
                                              iron_stack_value_t *result)
{
    (void)ctx;

    if (!args || arg_count < 2 || !args[0].value.obj) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Invalid multidimensional array constructor invocation");
    }

    if (result) {
        result->type = IRON_VAL_VOID;
    }
    return IRON_SUCCESS;
}

void *iron_stack_value_box(iron_exec_context_t *ctx,
                           iron_runtime_type_t *type,
                           const iron_stack_value_t *value)
{
    void *boxed;

    if (!ctx || !type || !value || type->element_type == IRON_TYPE_VOID) {
        return NULL;
    }

    if (iron_type_is_managed_reference(type)) {
        if (value->type != IRON_VAL_OBJ || !iron_managed_reference_is_assignable(ctx->domain, value->value.obj, type)) {
            return NULL;
        }
        return value->value.obj;
    }

    if ((value->type == IRON_VAL_OBJ || value->type == IRON_VAL_VALUETYPE) && value->value.obj && IRON_GC_HEADER(value->value.obj)->type == type) {
        return value->value.obj;
    }

    boxed = iron_gc_alloc_object(&ctx->gc, type, iron_type_storage_size(type));
    if (!boxed) {
        return NULL;
    }
    if (!store_storage_value(boxed, boxed, type, value)) {
        return NULL;
    }

    return boxed;
}

static iron_bool store_stack_slot_value(iron_exec_context_t *ctx,
                                        iron_stack_value_t *slot,
                                        iron_runtime_type_t *type,
                                        const iron_stack_value_t *value)
{
    if (!ctx || !slot || !type || !value) {
        return IRON_FALSE;
    }

    if (iron_type_is_managed_reference(type)) {
        if (value->type != IRON_VAL_OBJ || !iron_managed_reference_is_assignable(ctx->domain, value->value.obj, type)) {
            return IRON_FALSE;
        }
        memset(&slot->value, 0, sizeof(slot->value));
        slot->type = IRON_VAL_OBJ;
        slot->value.obj = value->value.obj;
        return IRON_TRUE;
    }

    if (type->element_type != IRON_TYPE_END && type->element_type != IRON_TYPE_VALUETYPE) {
        if ((value->type == IRON_VAL_OBJ || value->type == IRON_VAL_VALUETYPE) && value->value.obj) {
            iron_runtime_type_t *value_type;
            iron_stack_value_t direct_value;

            value_type = IRON_GC_HEADER(value->value.obj)->type;
            if (value_type != type) {
                return IRON_FALSE;
            }

            read_storage_element(&direct_value, value->value.obj, type->element_type);
            store_stack_slot_element(slot, type->element_type, &direct_value);
            return IRON_TRUE;
        }
        if (value->type == IRON_VAL_BYREF && value->value.byref.ptr) {
            iron_stack_value_t direct_value;

            read_storage_element(&direct_value, value->value.byref.ptr, type->element_type);
            store_stack_slot_element(slot, type->element_type, &direct_value);
            return IRON_TRUE;
        }

        store_stack_slot_element(slot, type->element_type, value);
        return IRON_TRUE;
    }

    {
        const void *source;
        void *value_copy;
        iron_size storage_size;

        storage_size = iron_type_storage_size(type);
        if ((value->type == IRON_VAL_OBJ || value->type == IRON_VAL_VALUETYPE) && !value->value.obj) {
            source = NULL;
        } else if ((value->type == IRON_VAL_OBJ || value->type == IRON_VAL_VALUETYPE) && value->value.obj && IRON_GC_HEADER(value->value.obj)->type == type) {
            source = value->value.obj;
        } else if (value->type == IRON_VAL_PTR || value->type == IRON_VAL_BYREF) {
            source = stack_value_address(value);
        } else if (storage_size <= sizeof(value->value)) {
            source = &value->value;
        } else {
            return IRON_FALSE;
        }

        value_copy = iron_gc_alloc_object(&ctx->gc, type, storage_size);
        if (!value_copy) {
            return IRON_FALSE;
        }
        if (source) {
            memcpy(value_copy, source, storage_size);
        } else {
            memset(value_copy, 0, storage_size);
        }
        memset(&slot->value, 0, sizeof(slot->value));
        slot->type = IRON_VAL_VALUETYPE;
        slot->value.obj = value_copy;
    }

    return IRON_TRUE;
}

iron_bool iron_stack_value_init_default(iron_exec_context_t *ctx, iron_stack_value_t *value, iron_runtime_type_t *type)
{
    iron_stack_value_t zero_value;

    if (!ctx || !value || !type) {
        return IRON_FALSE;
    }

    memset(&zero_value, 0, sizeof(zero_value));
    if (type->kind == IRON_KIND_VALUETYPE || type->kind == IRON_KIND_ENUM) {
        zero_value.type = IRON_VAL_VALUETYPE;
    } else {
        zero_value.type = stack_value_type_for_element(type->element_type);
    }
    return store_stack_slot_value(ctx, value, type, &zero_value);
}

static iron_bool push_address_value(iron_exec_context_t *ctx,
                                    iron_eval_stack_t *stack,
                                    const iron_stack_value_t *address_value,
                                    iron_runtime_type_t *type)
{
    iron_stack_value_t *slot;

    slot = stack_value_addressed_slot(address_value);
    if (!slot) {
        return push_storage_value(ctx, stack, stack_value_address(address_value), type);
    }

    if (!iron_type_is_managed_reference(type) && type->element_type == IRON_TYPE_END &&
        (type->kind == IRON_KIND_VALUETYPE || type->kind == IRON_KIND_ENUM)) {
        iron_stack_value_t copy;

        memset(&copy, 0, sizeof(copy));
        if (!store_stack_slot_value(ctx, &copy, type, slot)) {
            return IRON_FALSE;
        }
        iron_stack_push(stack, copy);
    } else {
        iron_stack_push(stack, *slot);
    }

    return IRON_TRUE;
}

static iron_bool store_address_value(iron_exec_context_t *ctx,
                                     const iron_stack_value_t *address_value,
                                     iron_runtime_type_t *type,
                                     const iron_stack_value_t *value)
{
    iron_stack_value_t *slot;

    slot = stack_value_addressed_slot(address_value);
    if (slot) {
        return store_stack_slot_value(ctx, slot, type, value);
    }

    return store_storage_value(NULL, stack_value_address(address_value), type, value);
}

static iron_runtime_type_t *resolve_primitive_type(iron_domain_t *domain, iron_element_type_t element_type)
{
    const char *name;

    name = iron_element_type_managed_name(element_type);
    if (!name) {
        return NULL;
    }

    return iron_domain_find_type(domain, name);
}

static iron_runtime_type_t *resolve_type_signature(iron_thread_context_t *thread, iron_assembly_t *assembly, iron_sig_reader_t *reader)
{
    iron_element_type_t element_type;
    iron_result_t result;

    result = iron_sig_read_element_type(reader, &element_type);
    if (!IRON_RESULT_OK(result)) {
        return NULL;
    }

    {
        iron_runtime_type_t *primitive;

        primitive = resolve_primitive_type(thread->exec_ctx->domain, element_type);
        if (primitive) {
            return primitive;
        }
    }

    if (element_type == IRON_TYPE_CLASS || element_type == IRON_TYPE_VALUETYPE) {
        iron_token_t type_token;

        result = iron_sig_read_type_def_or_ref(reader, &type_token);
        return IRON_RESULT_OK(result) ? iron_type_resolve_token(assembly->module, type_token) : NULL;
    }

    if (element_type == IRON_TYPE_VAR || element_type == IRON_TYPE_MVAR) {
        iron_u32 generic_index;
        iron_runtime_method_t *method;

        result = iron_sig_read_compressed_u32(reader, &generic_index);
        if (!IRON_RESULT_OK(result) || !thread->current_frame) {
            return NULL;
        }

        method = thread->current_frame->method;
        if (element_type == IRON_TYPE_MVAR) {
            return generic_index < method->generic_arg_count ? method->generic_args[generic_index] : NULL;
        }

        if (method->declaring_type && generic_index < method->declaring_type->generic_arg_count) {
            return method->declaring_type->generic_args[generic_index];
        }

        return NULL;
    }

    if (element_type == IRON_TYPE_SZARRAY) {
        iron_runtime_type_t *array_element;

        array_element = resolve_type_signature(thread, assembly, reader);
        return array_element ? iron_type_make_array(assembly->domain, array_element, 1) : NULL;
    }

    if (element_type == IRON_TYPE_ARRAY) {
        iron_runtime_type_t *array_element;
        iron_u32 rank;
        iron_u32 size_count;
        iron_u32 lower_bound_count;
        iron_u32 index;

        array_element = resolve_type_signature(thread, assembly, reader);
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

        pointed_type = resolve_type_signature(thread, assembly, reader);
        if (!pointed_type) {
            return NULL;
        }

        return element_type == IRON_TYPE_PTR ? iron_type_make_pointer(assembly->domain, pointed_type) : iron_type_make_byref(assembly->domain, pointed_type);
    }

    if (element_type == IRON_TYPE_GENERICINST) {
        iron_element_type_t class_or_value_type;
        iron_token_t definition_token;
        iron_runtime_type_t *definition;
        iron_runtime_type_t **generic_args;
        iron_runtime_type_t *instance;
        iron_u32 generic_arg_count;
        iron_u32 generic_index;

        result = iron_sig_read_element_type(reader, &class_or_value_type);
        if (!IRON_RESULT_OK(result) || (class_or_value_type != IRON_TYPE_CLASS && class_or_value_type != IRON_TYPE_VALUETYPE)) {
            return NULL;
        }

        result = iron_sig_read_type_def_or_ref(reader, &definition_token);
        if (!IRON_RESULT_OK(result)) {
            return NULL;
        }

        definition = iron_type_resolve_token(assembly->module, definition_token);
        result = iron_sig_read_compressed_u32(reader, &generic_arg_count);
        if (!definition || !IRON_RESULT_OK(result) || generic_arg_count == 0) {
            return NULL;
        }

        generic_args = (iron_runtime_type_t **)iron_alloc(thread->exec_ctx->allocator, generic_arg_count * sizeof(iron_runtime_type_t *));
        if (!generic_args) {
            return NULL;
        }

        for (generic_index = 0; generic_index < generic_arg_count; generic_index++) {
            generic_args[generic_index] = resolve_type_signature(thread, assembly, reader);
            if (!generic_args[generic_index]) {
                iron_free(thread->exec_ctx->allocator, generic_args, generic_arg_count * sizeof(iron_runtime_type_t *));
                return NULL;
            }
        }

        instance = iron_type_make_generic(assembly->domain, definition, generic_args, generic_arg_count);
        iron_free(thread->exec_ctx->allocator, generic_args, generic_arg_count * sizeof(iron_runtime_type_t *));
        return instance;
    }

    return NULL;
}

static iron_runtime_type_t *resolve_type_spec(iron_thread_context_t *thread, iron_assembly_t *assembly, iron_token_t token)
{
    iron_type_spec_row_t type_spec;
    const iron_u8 *signature;
    iron_u32 signature_size;
    iron_sig_reader_t reader;
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
    return resolve_type_signature(thread, assembly, &reader);
}

static iron_runtime_type_t *resolve_runtime_type_token(iron_thread_context_t *thread, iron_token_t token)
{
    iron_assembly_t *assembly;

    if (!thread || !thread->current_frame) {
        return NULL;
    }

    assembly = get_frame_assembly(thread->current_frame);
    if (!assembly) {
        return NULL;
    }
    if (IRON_TOKEN_TABLE(token) == IRON_TABLE_TYPE_SPEC) {
        return resolve_type_spec(thread, assembly, token);
    }
    return iron_type_resolve_token(assembly->module, token);
}

static iron_bool checked_instruction_target(iron_u32 base, iron_i32 displacement, iron_u32 code_size, iron_u32 *target)
{
    iron_i64 calculated;

    calculated = (iron_i64)base + (iron_i64)displacement;
    if (calculated < 0 || calculated >= (iron_i64)code_size) {
        return IRON_FALSE;
    }

    *target = (iron_u32)calculated;
    return IRON_TRUE;
}

static iron_bool stack_value_to_native_size(const iron_stack_value_t *value, iron_size *size)
{
    iron_u64 converted;

    if (!value || !size) {
        return IRON_FALSE;
    }

    if (value->type == IRON_VAL_I32) {
        converted = (iron_u64)(iron_u32)value->value.i32;
    } else if (value->type == IRON_VAL_I64) {
        converted = (iron_u64)value->value.i64;
    } else if (value->type == IRON_VAL_PTR) {
        converted = (iron_u64)(iron_size)value->value.ptr;
    } else {
        return IRON_FALSE;
    }

    if (converted > (iron_u64)(iron_size)-1) {
        return IRON_FALSE;
    }

    *size = (iron_size)converted;
    return IRON_TRUE;
}

/* ============================================================================
 * IL Interpreter - Single Instruction Execution
 * ============================================================================ */

static iron_bool find_next_leave_finally(const iron_method_body_t *body,
                                         iron_u32 leave_offset,
                                         iron_u32 target_offset,
                                         iron_u32 completed_try_length,
                                         iron_u32 *handler_index)
{
    iron_u32 best_index;
    iron_u32 best_try_length;
    iron_u32 index;

    if (!body || !handler_index) {
        return IRON_FALSE;
    }

    best_index = UINT32_MAX;
    best_try_length = UINT32_MAX;
    for (index = 0; index < body->exception_count; index++) {
        const iron_exception_clause_t *clause;
        iron_u32 clause_kind;
        iron_u32 try_end;

        clause = &body->exceptions[index];
        clause_kind = clause->flags & (IRON_EX_CLAUSE_FILTER | IRON_EX_CLAUSE_FINALLY | IRON_EX_CLAUSE_FAULT);
        if (clause_kind != IRON_EX_CLAUSE_FINALLY || clause->try_length <= completed_try_length ||
            clause->try_length > UINT32_MAX - clause->try_offset) {
            continue;
        }

        try_end = clause->try_offset + clause->try_length;
        if (leave_offset >= clause->try_offset && leave_offset < try_end &&
            (target_offset < clause->try_offset || target_offset >= try_end) && clause->try_length < best_try_length) {
            best_index = index;
            best_try_length = clause->try_length;
        }
    }

    if (best_index == UINT32_MAX) {
        return IRON_FALSE;
    }

    *handler_index = best_index;
    return IRON_TRUE;
}

iron_interp_result_t iron_exec_instruction(iron_thread_context_t *thread)
{
    iron_stack_frame_t *frame;
    iron_eval_stack_t *stack;
    const iron_u8 *code;
    iron_u32 ip;
    iron_opcode_t opcode;
    iron_u32 opcode_size;
    iron_i32 i32_val;
    iron_stack_value_t val1, val2;
    const iron_opcode_info_t *opcode_info;

    if (!thread || !thread->current_frame) {
        IRON_ERROR_EXEC("No thread or current frame");
        return IRON_INTERP_ERROR;
    }

    frame = thread->current_frame;
    stack = &thread->eval_stack;
    code = frame->code;
    ip = frame->ip;
    
    if (ip >= frame->code_size) {
        IRON_ERROR_EXEC("IP %u out of bounds (code_size=%u)", ip, frame->code_size);
        return IRON_INTERP_ERROR;
    }
    
    /* Decode opcode */
    opcode = iron_opcode_decode(code + ip, &opcode_size);
    ip += opcode_size;
    
    /* Trace opcode execution */
    opcode_info = iron_opcode_info(opcode);
    IRON_TRACE_EXEC("IP=%04X opcode=0x%04X %-12s stack=%u locals=%u args=%u", 
                    frame->ip, opcode, 
                    opcode_info ? opcode_info->name : "???",
                    stack->size,
                    frame->local_count,
                    frame->arg_count);
    
    /* Debug: show stack top values */
    if (stack->size > 0) {
        IRON_TRACE_EXEC("  Stack[0]: type=%d val=0x%llX", 
                        stack->data[stack->size-1].type,
                        (unsigned long long)stack->data[stack->size-1].value.i64);
    }
    if (stack->size > 1) {
        IRON_TRACE_EXEC("  Stack[1]: type=%d val=0x%llX", 
                        stack->data[stack->size-2].type,
                        (unsigned long long)stack->data[stack->size-2].value.i64);
    }
    
    switch (opcode) {
        case IRON_CEE_NOP:
            break;

        case IRON_CEE_BREAK:
            frame->ip = ip;
            return IRON_INTERP_BREAK;

        /* Load argument instructions */
        case IRON_CEE_LDARG_0:
            iron_stack_push(stack, frame->args[0]);
            break;
        case IRON_CEE_LDARG_1:
            iron_stack_push(stack, frame->args[1]);
            break;
        case IRON_CEE_LDARG_2:
            iron_stack_push(stack, frame->args[2]);
            break;
        case IRON_CEE_LDARG_3:
            iron_stack_push(stack, frame->args[3]);
            break;
        case IRON_CEE_LDARG_S:
            iron_stack_push(stack, frame->args[code[ip++]]);
            break;
        case IRON_CEE_LDARGA_S:
            {
                iron_u8 arg_idx;

                arg_idx = code[ip++];
                iron_stack_push_slot_byref(stack, &frame->args[arg_idx]);
            }
            break;
        case IRON_CEE_STARG_S:
            frame->args[code[ip++]] = iron_stack_pop(stack);
            break;
        case IRON_CEE_LDARG:
            iron_stack_push(stack, frame->args[iron_read_u16_le(code + ip)]);
            ip += 2;
            break;
        case IRON_CEE_LDARGA:
            {
                iron_u16 arg_idx;

                arg_idx = iron_read_u16_le(code + ip);
                ip += 2;
                iron_stack_push_slot_byref(stack, &frame->args[arg_idx]);
            }
            break;
        case IRON_CEE_STARG:
            {
                iron_u16 arg_idx;

                arg_idx = iron_read_u16_le(code + ip);
                ip += 2;
                frame->args[arg_idx] = iron_stack_pop(stack);
            }
            break;

        /* Load local instructions */
        case IRON_CEE_LDLOC_0:
            iron_stack_push(stack, frame->locals[0]);
            break;
        case IRON_CEE_LDLOC_1:
            iron_stack_push(stack, frame->locals[1]);
            break;
        case IRON_CEE_LDLOC_2:
            iron_stack_push(stack, frame->locals[2]);
            break;
        case IRON_CEE_LDLOC_3:
            iron_stack_push(stack, frame->locals[3]);
            break;
        case IRON_CEE_LDLOC_S:
            iron_stack_push(stack, frame->locals[code[ip++]]);
            break;
        case IRON_CEE_LDLOC:
            iron_stack_push(stack, frame->locals[iron_read_u16_le(code + ip)]);
            ip += 2;
            break;
        case IRON_CEE_LDLOCA_S:
            iron_stack_push_slot_byref(stack, &frame->locals[code[ip++]]);
            break;
        case IRON_CEE_LDLOCA:
            iron_stack_push_slot_byref(stack, &frame->locals[iron_read_u16_le(code + ip)]);
            ip += 2;
            break;

        /* Store local instructions */
        case IRON_CEE_STLOC_0:
            frame->locals[0] = iron_stack_pop(stack);
            break;
        case IRON_CEE_STLOC_1:
            frame->locals[1] = iron_stack_pop(stack);
            break;
        case IRON_CEE_STLOC_2:
            frame->locals[2] = iron_stack_pop(stack);
            break;
        case IRON_CEE_STLOC_3:
            frame->locals[3] = iron_stack_pop(stack);
            break;
        case IRON_CEE_STLOC_S:
            frame->locals[code[ip++]] = iron_stack_pop(stack);
            break;
        case IRON_CEE_STLOC:
            frame->locals[iron_read_u16_le(code + ip)] = iron_stack_pop(stack);
            ip += 2;
            break;

        case IRON_CEE_LOCALLOC:
            {
                iron_stack_value_t size_value;
                iron_size allocation_size;
                void *memory;

                size_value = iron_stack_pop(stack);
                if (!stack_value_to_native_size(&size_value, &allocation_size)) {
                    IRON_ERROR_EXEC("localloc received an invalid native unsigned size");
                    frame->ip = ip;
                    return IRON_INTERP_ERROR;
                }

                memory = iron_alloc(iron_arena_allocator(&thread->frame_arena), allocation_size == 0 ? 1 : allocation_size);
                if (!memory) {
                    iron_throw_overflow(thread);
                    frame->ip = ip;
                    return IRON_INTERP_EXCEPTION;
                }

                if (allocation_size != 0) {
                    memset(memory, 0, allocation_size);
                }
                iron_stack_push_ptr(stack, memory);
            }
            break;

        /* Load constant instructions */
        case IRON_CEE_LDNULL:
            iron_stack_push_null(stack);
            break;
        case IRON_CEE_LDC_I4_M1:
            iron_stack_push_i32(stack, -1);
            break;
        case IRON_CEE_LDC_I4_0:
            iron_stack_push_i32(stack, 0);
            break;
        case IRON_CEE_LDC_I4_1:
            iron_stack_push_i32(stack, 1);
            break;
        case IRON_CEE_LDC_I4_2:
            iron_stack_push_i32(stack, 2);
            break;
        case IRON_CEE_LDC_I4_3:
            iron_stack_push_i32(stack, 3);
            break;
        case IRON_CEE_LDC_I4_4:
            iron_stack_push_i32(stack, 4);
            break;
        case IRON_CEE_LDC_I4_5:
            iron_stack_push_i32(stack, 5);
            break;
        case IRON_CEE_LDC_I4_6:
            iron_stack_push_i32(stack, 6);
            break;
        case IRON_CEE_LDC_I4_7:
            iron_stack_push_i32(stack, 7);
            break;
        case IRON_CEE_LDC_I4_8:
            iron_stack_push_i32(stack, 8);
            break;
        case IRON_CEE_LDC_I4_S:
            iron_stack_push_i32(stack, (iron_i8)code[ip++]);
            break;
        case IRON_CEE_LDC_I4:
            iron_stack_push_i32(stack, iron_read_i32_le(code + ip));
            ip += 4;
            break;
        case IRON_CEE_LDC_I8:
            iron_stack_push_i64(stack, iron_read_i64_le(code + ip));
            ip += 8;
            break;
        case IRON_CEE_LDC_R4:
            iron_stack_push_f32(stack, iron_read_f32_le(code + ip));
            ip += 4;
            break;
        case IRON_CEE_LDC_R8:
            iron_stack_push_f64(stack, iron_read_f64_le(code + ip));
            ip += 8;
            break;

        /* Stack manipulation */
        case IRON_CEE_DUP:
            iron_stack_dup(stack);
            break;
        case IRON_CEE_POP:
            iron_stack_pop(stack);
            break;

        /* Indirect memory access */
        case IRON_CEE_LDIND_I1:
        case IRON_CEE_LDIND_U1:
        case IRON_CEE_LDIND_I2:
        case IRON_CEE_LDIND_U2:
        case IRON_CEE_LDIND_I4:
        case IRON_CEE_LDIND_U4:
        case IRON_CEE_LDIND_I8:
        case IRON_CEE_LDIND_I:
        case IRON_CEE_LDIND_R4:
        case IRON_CEE_LDIND_R8:
        case IRON_CEE_LDIND_REF:
            {
                void *address;
                iron_element_type_t element_type;

                val1 = iron_stack_pop(stack);
                address = stack_value_address(&val1);
                if (!address) {
                    iron_throw_null_reference(thread);
                    frame->ip = ip;
                    return IRON_INTERP_EXCEPTION;
                }

                switch (opcode) {
                    case IRON_CEE_LDIND_I1: element_type = IRON_TYPE_I1; break;
                    case IRON_CEE_LDIND_U1: element_type = IRON_TYPE_U1; break;
                    case IRON_CEE_LDIND_I2: element_type = IRON_TYPE_I2; break;
                    case IRON_CEE_LDIND_U2: element_type = IRON_TYPE_U2; break;
                    case IRON_CEE_LDIND_I4: element_type = IRON_TYPE_I4; break;
                    case IRON_CEE_LDIND_U4: element_type = IRON_TYPE_U4; break;
                    case IRON_CEE_LDIND_I8: element_type = IRON_TYPE_I8; break;
                    case IRON_CEE_LDIND_I: element_type = IRON_TYPE_I; break;
                    case IRON_CEE_LDIND_R4: element_type = IRON_TYPE_R4; break;
                    case IRON_CEE_LDIND_R8: element_type = IRON_TYPE_R8; break;
                    default: element_type = IRON_TYPE_OBJECT; break;
                }

                push_field_value(stack, address, element_type);
            }
            break;

        case IRON_CEE_STIND_REF:
        case IRON_CEE_STIND_I1:
        case IRON_CEE_STIND_I2:
        case IRON_CEE_STIND_I4:
        case IRON_CEE_STIND_I8:
        case IRON_CEE_STIND_R4:
        case IRON_CEE_STIND_R8:
        case IRON_CEE_STIND_I:
            {
                void *address;
                iron_element_type_t element_type;

                val2 = iron_stack_pop(stack);
                val1 = iron_stack_pop(stack);
                address = stack_value_address(&val1);
                if (!address) {
                    iron_throw_null_reference(thread);
                    frame->ip = ip;
                    return IRON_INTERP_EXCEPTION;
                }

                switch (opcode) {
                    case IRON_CEE_STIND_I1: element_type = IRON_TYPE_I1; break;
                    case IRON_CEE_STIND_I2: element_type = IRON_TYPE_I2; break;
                    case IRON_CEE_STIND_I4: element_type = IRON_TYPE_I4; break;
                    case IRON_CEE_STIND_I8: element_type = IRON_TYPE_I8; break;
                    case IRON_CEE_STIND_R4: element_type = IRON_TYPE_R4; break;
                    case IRON_CEE_STIND_R8: element_type = IRON_TYPE_R8; break;
                    case IRON_CEE_STIND_I: element_type = IRON_TYPE_I; break;
                    default: element_type = IRON_TYPE_OBJECT; break;
                }

                {
                    iron_stack_value_t *slot;

                    slot = stack_value_addressed_slot(&val1);
                    if (slot) {
                        store_stack_slot_element(slot, element_type, &val2);
                    } else {
                        store_field_value(NULL, address, element_type, &val2);
                    }
                }
            }
            break;

        case IRON_CEE_CPOBJ:
            {
                iron_token_t type_token;
                iron_runtime_type_t *type;
                iron_stack_value_t source_address;
                iron_stack_value_t destination_address;
                iron_stack_value_t *source_value;

                type_token = iron_read_u32_le(code + ip);
                ip += 4;
                source_address = iron_stack_pop(stack);
                destination_address = iron_stack_pop(stack);
                if (!stack_value_address(&source_address) || !stack_value_address(&destination_address)) {
                    iron_throw_null_reference(thread);
                    frame->ip = ip;
                    return IRON_INTERP_EXCEPTION;
                }

                type = resolve_runtime_type_token(thread, type_token);
                if (!type || !push_address_value(thread->exec_ctx, stack, &source_address, type)) {
                    IRON_ERROR_EXEC("Cannot load cpobj source for type token 0x%08X", type_token);
                    frame->ip = ip;
                    return IRON_INTERP_ERROR;
                }

                source_value = iron_stack_peek(stack, 0);
                if (!source_value || !store_address_value(thread->exec_ctx, &destination_address, type, source_value)) {
                    iron_stack_pop(stack);
                    IRON_ERROR_EXEC("Cannot store cpobj destination for type token 0x%08X", type_token);
                    frame->ip = ip;
                    return IRON_INTERP_ERROR;
                }
                iron_stack_pop(stack);
            }
            break;

        case IRON_CEE_LDOBJ:
            {
                iron_token_t type_token;
                iron_runtime_type_t *type;
                iron_stack_value_t address_value;

                type_token = iron_read_u32_le(code + ip);
                ip += 4;
                address_value = iron_stack_pop(stack);
                if (!stack_value_address(&address_value)) {
                    iron_throw_null_reference(thread);
                    frame->ip = ip;
                    return IRON_INTERP_EXCEPTION;
                }

                type = resolve_runtime_type_token(thread, type_token);
                if (!type || !push_address_value(thread->exec_ctx, stack, &address_value, type)) {
                    IRON_ERROR_EXEC("Cannot load object for type token 0x%08X", type_token);
                    frame->ip = ip;
                    return IRON_INTERP_ERROR;
                }
            }
            break;

        case IRON_CEE_STOBJ:
            {
                iron_token_t type_token;
                iron_runtime_type_t *type;
                iron_stack_value_t value;
                iron_stack_value_t address_value;

                type_token = iron_read_u32_le(code + ip);
                ip += 4;
                value = iron_stack_pop(stack);
                address_value = iron_stack_pop(stack);
                if (!stack_value_address(&address_value)) {
                    iron_throw_null_reference(thread);
                    frame->ip = ip;
                    return IRON_INTERP_EXCEPTION;
                }

                type = resolve_runtime_type_token(thread, type_token);
                if (!type || !store_address_value(thread->exec_ctx, &address_value, type, &value)) {
                    IRON_ERROR_EXEC("Cannot store object for type token 0x%08X", type_token);
                    frame->ip = ip;
                    return IRON_INTERP_ERROR;
                }
            }
            break;

        case IRON_CEE_INITOBJ:
            {
                iron_token_t type_token;
                iron_runtime_type_t *type;
                iron_stack_value_t address_value;
                iron_size storage_size;
                void *address;

                type_token = iron_read_u32_le(code + ip);
                ip += 4;
                address_value = iron_stack_pop(stack);
                address = stack_value_address(&address_value);
                if (!address) {
                    iron_throw_null_reference(thread);
                    frame->ip = ip;
                    return IRON_INTERP_EXCEPTION;
                }

                type = resolve_runtime_type_token(thread, type_token);

                if (!type) {
                    IRON_ERROR_EXEC("Cannot resolve initobj type token 0x%08X", type_token);
                    frame->ip = ip;
                    return IRON_INTERP_ERROR;
                }

                storage_size = iron_type_storage_size(type);

                if (storage_size == 0) {
                    IRON_ERROR_EXEC("Cannot initialize a type with zero storage size");
                    frame->ip = ip;
                    return IRON_INTERP_ERROR;
                }

                {
                    iron_stack_value_t *slot;

                    slot = stack_value_addressed_slot(&address_value);
                    if (slot) {
                        iron_stack_value_t zero_value;

                        memset(&zero_value, 0, sizeof(zero_value));
                        zero_value.type = stack_value_type_for_element(type->element_type);
                        if (!store_stack_slot_value(thread->exec_ctx, slot, type, &zero_value)) {
                            IRON_ERROR_EXEC("Cannot initialize stack storage for type token 0x%08X", type_token);
                            frame->ip = ip;
                            return IRON_INTERP_ERROR;
                        }
                    } else {
                        memset(address, 0, storage_size);
                    }
                }
            }
            break;

        case IRON_CEE_CPBLK:
        case IRON_CEE_INITBLK:
            {
                iron_stack_value_t size_value;
                iron_stack_value_t source_or_value;
                iron_stack_value_t destination_value;
                iron_size block_size;
                void *destination;

                size_value = iron_stack_pop(stack);
                source_or_value = iron_stack_pop(stack);
                destination_value = iron_stack_pop(stack);
                destination = stack_value_address(&destination_value);
                if (!stack_value_to_native_size(&size_value, &block_size)) {
                    IRON_ERROR_EXEC("Block instruction received an invalid native unsigned size");
                    frame->ip = ip;
                    return IRON_INTERP_ERROR;
                }

                if (block_size != 0 && !destination) {
                    iron_throw_null_reference(thread);
                    frame->ip = ip;
                    return IRON_INTERP_EXCEPTION;
                }

                if (frame->volatile_prefix) {
                    iron_atomic_fence_seq_cst();
                }

                if (opcode == IRON_CEE_CPBLK) {
                    void *source;

                    source = stack_value_address(&source_or_value);
                    if (block_size != 0 && !source) {
                        iron_throw_null_reference(thread);
                        frame->ip = ip;
                        return IRON_INTERP_EXCEPTION;
                    }
                    if (block_size != 0) {
                        memmove(destination, source, block_size);
                    }
                } else if (block_size != 0) {
                    memset(destination, (iron_u8)stack_value_to_u64(&source_or_value), block_size);
                }

                if (frame->volatile_prefix) {
                    iron_atomic_fence_seq_cst();
                }
            }
            break;

        case IRON_CEE_SIZEOF:
            {
                iron_token_t type_token;
                iron_runtime_type_t *type;
                iron_size storage_size;

                type_token = iron_read_u32_le(code + ip);
                ip += 4;
                type = resolve_runtime_type_token(thread, type_token);
                if (!type) {
                    IRON_ERROR_EXEC("Cannot determine sizeof type token 0x%08X", type_token);
                    frame->ip = ip;
                    return IRON_INTERP_ERROR;
                }

                storage_size = iron_type_storage_size(type);
                if (storage_size > INT32_MAX) {
                    IRON_ERROR_EXEC("Type token 0x%08X is too large for sizeof", type_token);
                    frame->ip = ip;
                    return IRON_INTERP_ERROR;
                }

                iron_stack_push_i32(stack, (iron_i32)storage_size);
            }
            break;

        case IRON_CEE_BOX:
        case IRON_CEE_UNBOX:
        case IRON_CEE_UNBOX_ANY:
        case IRON_CEE_CASTCLASS:
        case IRON_CEE_ISINST:
            {
                iron_token_t type_token;
                iron_runtime_type_t *type;
                iron_stack_value_t value;
                void *object;

                type_token = iron_read_u32_le(code + ip);
                ip += 4;
                type = resolve_runtime_type_token(thread, type_token);

                if (!type) {
                    IRON_ERROR_EXEC("Cannot resolve type token 0x%08X", type_token);
                    frame->ip = ip;
                    return IRON_INTERP_ERROR;
                }

                value = iron_stack_pop(stack);
                object = value.value.obj;

                if (opcode == IRON_CEE_BOX) {
                    if (iron_type_is_managed_reference(type)) {
                        if (value.type != IRON_VAL_OBJ || !iron_managed_reference_is_assignable(thread->exec_ctx->domain, object, type)) {
                            iron_throw_invalid_cast(thread);
                            frame->ip = ip;
                            return IRON_INTERP_EXCEPTION;
                        }

                        iron_stack_push_obj(stack, object);
                    } else if ((value.type == IRON_VAL_OBJ || value.type == IRON_VAL_VALUETYPE) && object && IRON_GC_HEADER(object)->type == type) {
                        iron_stack_push_obj(stack, object);
                    } else {
                        const void *source;

                        source = value.type == IRON_VAL_PTR || value.type == IRON_VAL_BYREF ? stack_value_address(&value) :
                                 (value.type == IRON_VAL_VALUETYPE ? value.value.obj : (const void *)&value.value);
                        object = iron_gc_box(thread->exec_ctx, type, source);
                        if (!object) {
                            frame->ip = ip;
                            return IRON_INTERP_ERROR;
                        }

                        iron_stack_push_obj(stack, object);
                    }
                } else if (opcode == IRON_CEE_UNBOX_ANY && iron_type_is_managed_reference(type)) {
                    if (!iron_managed_reference_is_assignable(thread->exec_ctx->domain, object, type)) {
                        iron_throw_invalid_cast(thread);
                        frame->ip = ip;
                        return IRON_INTERP_EXCEPTION;
                    }

                    iron_stack_push_obj(stack, object);
                } else if (opcode == IRON_CEE_UNBOX || opcode == IRON_CEE_UNBOX_ANY) {
                    if (!object) {
                        iron_throw_null_reference(thread);
                        frame->ip = ip;
                        return IRON_INTERP_EXCEPTION;
                    }

                    if (!iron_type_is_assignable_to(IRON_GC_HEADER(object)->type, type)) {
                        iron_throw_invalid_cast(thread);
                        frame->ip = ip;
                        return IRON_INTERP_EXCEPTION;
                    }

                    if (opcode == IRON_CEE_UNBOX) {
                        iron_stack_push_ptr(stack, object);
                    } else if (type->kind == IRON_KIND_VALUETYPE || type->kind == IRON_KIND_ENUM) {
                        if (!push_storage_value(thread->exec_ctx, stack, object, type)) {
                            frame->ip = ip;
                            return IRON_INTERP_ERROR;
                        }
                    } else {
                        iron_stack_push_obj(stack, object);
                    }
                } else {
                    iron_runtime_type_t *object_type;
                    iron_bool compatible;

                    object_type = iron_managed_reference_get_type(thread->exec_ctx->domain, object);
                    compatible = !object || (object_type && iron_type_is_assignable_to(object_type, type));
                    if (opcode == IRON_CEE_ISINST) {
                        iron_stack_push_obj(stack, compatible ? object : NULL);
                    } else if (!compatible) {
                        iron_throw_invalid_cast(thread);
                        frame->ip = ip;
                        return IRON_INTERP_EXCEPTION;
                    } else {
                        iron_stack_push_obj(stack, object);
                    }
                }
            }
            break;

        /* String loading */
        case IRON_CEE_LDSTR:
            {
                iron_u32 str_token = iron_read_u32_le(code + ip);
                iron_exec_context_t *exec_ctx = thread->exec_ctx;
                iron_assembly_t *asm_ = NULL;
                const iron_u16 *str_data;
                iron_u32 str_len;
                iron_u32 str_index;
                iron_result_t res;

                ip += 4;
                str_index = str_token & 0x00FFFFFF;

                /* Get assembly context */
                if (frame->method && frame->method->declaring_type &&
                    frame->method->declaring_type->module &&
                    frame->method->declaring_type->module->assembly) {
                    asm_ = frame->method->declaring_type->module->assembly;
                } else if (exec_ctx && exec_ctx->domain && exec_ctx->domain->assembly_count > 0) {
                    asm_ = exec_ctx->domain->assemblies[exec_ctx->domain->assembly_count - 1];
                }

                if (asm_) {
                    res = iron_metadata_get_user_string(&asm_->metadata, str_index, &str_data, &str_len);
                    if (IRON_RESULT_OK(res)) {
                        void *str_obj;

                        str_obj = iron_gc_alloc_string(exec_ctx, NULL, str_len);
                        if (str_obj) {
                            iron_u16 *destination;
                            iron_u32 character_index;

                            destination = (iron_u16 *)((iron_u8 *)str_obj + sizeof(iron_u32));
                            for (character_index = 0; character_index < str_len; character_index++) {
                                destination[character_index] = iron_read_u16_le((const iron_u8 *)str_data + (iron_size)character_index * sizeof(iron_u16));
                            }
                            iron_stack_push_obj(stack, str_obj);
                        } else {
                            iron_stack_push_null(stack);
                        }
                    } else {
                        iron_stack_push_null(stack);
                    }
                } else {
                    iron_stack_push_null(stack);
                }
            }
            break;

        /* Method calls */
        case IRON_CEE_ARGLIST:
            {
                iron_vararg_handle_t *handle;
                iron_u32 fixed_argument_count;

                fixed_argument_count = frame->method->param_count;
                if ((frame->method->attrs & IRON_METHOD_STATIC) == 0) {
                    fixed_argument_count++;
                }
                if (fixed_argument_count > frame->arg_count) {
                    IRON_ERROR_EXEC("Method argument layout is inconsistent with arglist");
                    frame->ip = ip;
                    return IRON_INTERP_ERROR;
                }

                handle = (iron_vararg_handle_t *)iron_alloc(iron_arena_allocator(&thread->frame_arena), sizeof(iron_vararg_handle_t));
                if (!handle) {
                    iron_throw_overflow(thread);
                    frame->ip = ip;
                    return IRON_INTERP_EXCEPTION;
                }

                handle->arguments = frame->args + fixed_argument_count;
                handle->count = frame->arg_count - fixed_argument_count;
                iron_stack_push_ptr(stack, handle);
            }
            break;

        case IRON_CEE_LDFTN:
        case IRON_CEE_LDVIRTFTN:
            {
                iron_u32 method_token;
                iron_runtime_method_t *target_method;
                iron_assembly_t *target_assembly;
                iron_result_t resolve_result;

                method_token = iron_read_u32_le(code + ip);
                ip += 4;
                target_method = NULL;
                target_assembly = NULL;
                resolve_result = resolve_method_token(thread, method_token, &target_method, &target_assembly);
                if (!IRON_RESULT_OK(resolve_result) || !target_method) {
                    IRON_ERROR_EXEC("Cannot resolve delegate target token 0x%08X", method_token);
                    frame->ip = ip;
                    return IRON_INTERP_ERROR;
                }

                if (opcode == IRON_CEE_LDVIRTFTN) {
                    iron_stack_value_t object_value;
                    iron_runtime_type_t *runtime_type;
                    iron_runtime_method_t *implementation;
                    void *object;

                    object_value = iron_stack_pop(stack);
                    object = object_value.value.obj;
                    if (!object) {
                        iron_throw_null_reference(thread);
                        frame->ip = ip;
                        return IRON_INTERP_EXCEPTION;
                    }

                    runtime_type = iron_managed_reference_get_type(thread->exec_ctx->domain, object);
                    if (!runtime_type) {
                        iron_throw_invalid_cast(thread);
                        frame->ip = ip;
                        return IRON_INTERP_EXCEPTION;
                    }
                    implementation = find_virtual_implementation(runtime_type, target_method);
                    if (implementation) {
                        target_method = implementation;
                    }
                }

                iron_stack_push_method_ptr(stack, target_method);
            }
            break;

        case IRON_CEE_JMP:
            {
                iron_token_t method_token;
                iron_runtime_method_t *target_method;
                iron_assembly_t *target_assembly;
                iron_stack_value_t result;
                iron_result_t execution_result;
                iron_u32 expected_argument_count;

                method_token = iron_read_u32_le(code + ip);
                ip += 4;
                target_method = NULL;
                target_assembly = NULL;
                if (!IRON_RESULT_OK(resolve_method_token(thread, method_token, &target_method, &target_assembly)) || !target_method) {
                    IRON_ERROR_EXEC("Cannot resolve jmp target token 0x%08X", method_token);
                    frame->ip = ip;
                    return IRON_INTERP_ERROR;
                }

                expected_argument_count = target_method->param_count;
                if ((target_method->attrs & IRON_METHOD_STATIC) == 0) {
                    expected_argument_count++;
                }
                if (expected_argument_count != frame->arg_count || stack->size != frame->stack_base) {
                    IRON_ERROR_EXEC("jmp target signature or evaluation stack is incompatible with the current method");
                    frame->ip = ip;
                    return IRON_INTERP_ERROR;
                }

                memset(&result, 0, sizeof(result));
                execution_result = iron_exec_method(thread->exec_ctx, target_method, frame->args, frame->arg_count, &result);
                if (!IRON_RESULT_OK(execution_result)) {
                    frame->ip = ip;
                    return execution_result.error == IRON_ERR_EXCEPTION ? IRON_INTERP_EXCEPTION : IRON_INTERP_ERROR;
                }
                if (result.type != IRON_VAL_VOID) {
                    iron_stack_push(stack, result);
                }
                frame->ip = ip;
                return IRON_INTERP_RETURN;
            }

        case IRON_CEE_CALLI:
            {
                iron_token_t signature_token;
                iron_assembly_t *assembly;
                iron_standalone_sig_row_t signature_row;
                const iron_u8 *signature_data;
                iron_u32 signature_size;
                iron_u32 parameter_count;
                iron_stack_value_t function_pointer;
                iron_runtime_method_t *target_method;
                iron_result_t call_result;

                signature_token = iron_read_u32_le(code + ip);
                ip += 4;
                assembly = get_frame_assembly(frame);
                signature_data = NULL;
                signature_size = 0;
                if (!assembly || IRON_TOKEN_TABLE(signature_token) != IRON_TABLE_STANDALONE_SIG ||
                    !IRON_RESULT_OK(iron_metadata_read_row(&assembly->metadata, signature_token, &signature_row)) ||
                    !IRON_RESULT_OK(iron_metadata_get_blob(&assembly->metadata, signature_row.signature, &signature_data, &signature_size))) {
                    IRON_ERROR_EXEC("Cannot resolve calli signature token 0x%08X", signature_token);
                    frame->ip = ip;
                    return IRON_INTERP_ERROR;
                }

                parameter_count = get_method_param_count_from_sig(signature_data, signature_size);
                function_pointer = iron_stack_pop(stack);
                if (function_pointer.type != IRON_VAL_METHOD_PTR || !function_pointer.value.ptr) {
                    IRON_ERROR_EXEC("calli requires a managed function pointer produced by ldftn or ldvirtftn");
                    frame->ip = ip;
                    return IRON_INTERP_ERROR;
                }

                target_method = (iron_runtime_method_t *)function_pointer.value.ptr;
                if (parameter_count != target_method->param_count) {
                    IRON_ERROR_EXEC("calli signature has %u parameters but the target has %u", parameter_count, target_method->param_count);
                    frame->ip = ip;
                    return IRON_INTERP_ERROR;
                }

                frame->ip = ip;
                call_result = convert_managed_call_error(
                    thread,
                    call_method(thread, target_method, target_method->declaring_type && target_method->declaring_type->module ? target_method->declaring_type->module->assembly : assembly, IRON_FALSE));
                if (!IRON_RESULT_OK(call_result)) {
                    return call_result.error == IRON_ERR_EXCEPTION ? IRON_INTERP_EXCEPTION : IRON_INTERP_ERROR;
                }
            }
            break;

        case IRON_CEE_LDTOKEN:
            {
                iron_token_t token;
                iron_u32 table;
                iron_assembly_t *assembly;
                void *handle;

                token = iron_read_u32_le(code + ip);
                ip += 4;
                table = IRON_TOKEN_TABLE(token);
                assembly = get_frame_assembly(frame);
                handle = NULL;

                if (assembly && (table == IRON_TABLE_TYPE_DEF || table == IRON_TABLE_TYPE_REF)) {
                    handle = iron_type_resolve_token(assembly->module, token);
                } else if (assembly && table == IRON_TABLE_TYPE_SPEC) {
                    handle = resolve_type_spec(thread, assembly, token);
                } else if (assembly && (table == IRON_TABLE_FIELD || table == IRON_TABLE_MEMBER_REF)) {
                    handle = resolve_runtime_field(thread, assembly, token);
                } else if (table == IRON_TABLE_METHOD_DEF || table == IRON_TABLE_METHOD_SPEC || table == IRON_TABLE_MEMBER_REF) {
                    iron_runtime_method_t *method;
                    iron_assembly_t *method_assembly;

                    method = NULL;
                    method_assembly = NULL;
                    if (IRON_RESULT_OK(resolve_method_token(thread, token, &method, &method_assembly))) {
                        handle = method;
                    }
                }

                if (!handle) {
                    IRON_ERROR_EXEC("Cannot resolve runtime handle token 0x%08X", token);
                    frame->ip = ip;
                    return IRON_INTERP_ERROR;
                }

                iron_stack_push_ptr(stack, handle);
            }
            break;

        case IRON_CEE_MKREFANY:
            {
                iron_token_t type_token;
                iron_runtime_type_t *type;
                iron_stack_value_t address;
                iron_stack_value_t typed_reference;

                type_token = iron_read_u32_le(code + ip);
                ip += 4;
                type = resolve_runtime_type_token(thread, type_token);
                address = iron_stack_pop(stack);
                if (!type || !stack_value_address(&address)) {
                    IRON_ERROR_EXEC("Cannot create typed reference for token 0x%08X", type_token);
                    frame->ip = ip;
                    return IRON_INTERP_ERROR;
                }

                memset(&typed_reference, 0, sizeof(typed_reference));
                typed_reference.type = IRON_VAL_TYPEDREF;
                typed_reference.value.typedref.ptr = stack_value_address(&address);
                typed_reference.value.typedref.type = type;
                iron_stack_push(stack, typed_reference);
            }
            break;

        case IRON_CEE_REFANYVAL:
            {
                iron_token_t type_token;
                iron_runtime_type_t *type;
                iron_stack_value_t typed_reference;

                type_token = iron_read_u32_le(code + ip);
                ip += 4;
                type = resolve_runtime_type_token(thread, type_token);
                typed_reference = iron_stack_pop(stack);
                if (!type || typed_reference.type != IRON_VAL_TYPEDREF || typed_reference.value.typedref.type != type) {
                    iron_throw_invalid_cast(thread);
                    frame->ip = ip;
                    return IRON_INTERP_EXCEPTION;
                }

                memset(&val1, 0, sizeof(val1));
                val1.type = IRON_VAL_BYREF;
                val1.value.byref.ptr = typed_reference.value.typedref.ptr;
                iron_stack_push(stack, val1);
            }
            break;

        case IRON_CEE_REFANYTYPE:
            val1 = iron_stack_pop(stack);
            if (val1.type != IRON_VAL_TYPEDREF || !val1.value.typedref.type) {
                IRON_ERROR_EXEC("refanytype requires a typed reference");
                frame->ip = ip;
                return IRON_INTERP_ERROR;
            }
            iron_stack_push_ptr(stack, val1.value.typedref.type);
            break;

        case IRON_CEE_CALL:
            {
                iron_u32 method_token = iron_read_u32_le(code + ip);
                iron_runtime_method_t *target_method = NULL;
                iron_assembly_t *target_assembly = NULL;
                iron_result_t call_res;
                
                ip += 4;
                frame->ip = ip; /* Save IP before call */
                
                call_res = resolve_method_token(thread, method_token, 
                                                &target_method, &target_assembly);
                if (IRON_RESULT_OK(call_res) && target_method) {
                    call_res = convert_managed_call_error(thread, call_method(thread, target_method, target_assembly, IRON_FALSE));
                    if (!IRON_RESULT_OK(call_res)) {
                        return call_res.error == IRON_ERR_EXCEPTION ? IRON_INTERP_EXCEPTION : IRON_INTERP_ERROR;
                    }
                } else {
                    IRON_ERROR_EXEC("Cannot resolve call target token 0x%08X", method_token);
                    return IRON_INTERP_ERROR;
                }
            }
            break;
            
        case IRON_CEE_CONSTRAINED:
            {
                iron_token_t type_token;

                type_token = iron_read_u32_le(code + ip);
                ip += 4;
                frame->constrained_type = resolve_runtime_type_token(thread, type_token);
                if (!frame->constrained_type) {
                    IRON_ERROR_EXEC("Cannot resolve constrained type token 0x%08X", type_token);
                    frame->ip = ip;
                    return IRON_INTERP_ERROR;
                }

                frame->flags |= IRON_FRAME_CONSTRAINED;
            }
            break;

        case IRON_CEE_UNALIGNED:
            frame->unaligned_prefix = code[ip++];
            if (frame->unaligned_prefix != 1 && frame->unaligned_prefix != 2 && frame->unaligned_prefix != 4) {
                IRON_ERROR_EXEC("unaligned. prefix must specify 1, 2, or 4");
                frame->ip = ip;
                return IRON_INTERP_ERROR;
            }
            break;

        case IRON_CEE_VOLATILE:
            frame->volatile_prefix = IRON_TRUE;
            break;

        case IRON_CEE_TAIL:
            frame->tail_prefix = IRON_TRUE;
            frame->flags |= IRON_FRAME_TAIL_CALL;
            break;

        case IRON_CEE_NO:
            /* The verifier consumes these check-suppression flags. The runtime keeps
             * its memory-safety checks enabled for unverifiable input. */
            ip++;
            break;
            
        case IRON_CEE_READONLY:
            frame->readonly_prefix = IRON_TRUE;
            break;
            
        case IRON_CEE_CALLVIRT:
            {
                iron_u32 method_token = iron_read_u32_le(code + ip);
                iron_runtime_method_t *target_method = NULL;
                iron_assembly_t *target_assembly = NULL;
                iron_result_t call_res;
                
                ip += 4;
                frame->ip = ip; /* Save IP before call */
                
                call_res = resolve_method_token(thread, method_token, 
                                                &target_method, &target_assembly);
                if (IRON_RESULT_OK(call_res) && target_method) {
                    iron_u32 arg_count = target_method->param_count;
                    iron_bool requires_virtual_dispatch;
                    iron_runtime_method_t *contract_method;
                    iron_runtime_method_t *implementation;

                    contract_method = target_method;
                    implementation = NULL;

                    requires_virtual_dispatch = (target_method->attrs & IRON_METHOD_VIRTUAL) != 0 ||
                                                (target_method->declaring_type && target_method->declaring_type->kind == IRON_KIND_INTERFACE);

                    if (stack->size > arg_count) {
                        iron_stack_value_t this_val = stack->data[stack->size - arg_count - 1];
                        iron_runtime_type_t *runtime_type = NULL;
                        if ((frame->flags & IRON_FRAME_CONSTRAINED) != 0) {
                            runtime_type = frame->constrained_type;

                            if (runtime_type->kind != IRON_KIND_VALUETYPE && runtime_type->kind != IRON_KIND_ENUM) {
                                void *managed_address;
                                void *this_obj;

                                managed_address = stack_value_address(&this_val);
                                if (!managed_address) {
                                    iron_throw_null_reference(thread);
                                    frame->flags &= ~IRON_FRAME_CONSTRAINED;
                                    frame->constrained_type = NULL;
                                    frame->ip = ip;
                                    return IRON_INTERP_EXCEPTION;
                                }

                                memcpy(&this_obj, managed_address, sizeof(this_obj));
                                if (!this_obj) {
                                    iron_throw_null_reference(thread);
                                    frame->flags &= ~IRON_FRAME_CONSTRAINED;
                                    frame->constrained_type = NULL;
                                    frame->ip = ip;
                                    return IRON_INTERP_EXCEPTION;
                                }

                                memset(&stack->data[stack->size - arg_count - 1], 0, sizeof(iron_stack_value_t));
                                stack->data[stack->size - arg_count - 1].type = IRON_VAL_OBJ;
                                stack->data[stack->size - arg_count - 1].value.obj = this_obj;
                                this_val = stack->data[stack->size - arg_count - 1];
                            }
                        } else {
                            void *this_obj;

                            this_obj = this_val.type == IRON_VAL_OBJ ? this_val.value.obj : this_val.value.ptr;
                            if (!this_obj) {
                                iron_throw_null_reference(thread);
                                frame->flags &= ~IRON_FRAME_CONSTRAINED;
                                frame->constrained_type = NULL;
                                frame->ip = ip;
                                return IRON_INTERP_EXCEPTION;
                            }

                            if (requires_virtual_dispatch) {
                                runtime_type = iron_managed_reference_get_type(thread->exec_ctx->domain, this_obj);
                                if (!runtime_type) {
                                    iron_throw_invalid_cast(thread);
                                    frame->flags &= ~IRON_FRAME_CONSTRAINED;
                                    frame->constrained_type = NULL;
                                    frame->ip = ip;
                                    return IRON_INTERP_EXCEPTION;
                                }
                            }
                        }

                        implementation = requires_virtual_dispatch && runtime_type ? find_virtual_implementation(runtime_type, target_method) : NULL;
                        if (implementation) {
                            target_method = implementation;
                            update_method_assembly(target_method, &target_assembly);
                        }

                        if (requires_virtual_dispatch && (frame->flags & IRON_FRAME_CONSTRAINED) != 0 && runtime_type &&
                            (runtime_type->kind == IRON_KIND_VALUETYPE || runtime_type->kind == IRON_KIND_ENUM) &&
                            (!implementation || target_method->declaring_type != runtime_type)) {
                            void *boxed;

                            boxed = box_constrained_value(thread->exec_ctx, runtime_type, &this_val);
                            if (!boxed) {
                                frame->flags &= ~IRON_FRAME_CONSTRAINED;
                                frame->constrained_type = NULL;
                                return IRON_INTERP_ERROR;
                            }

                            memset(&stack->data[stack->size - arg_count - 1], 0, sizeof(iron_stack_value_t));
                            stack->data[stack->size - arg_count - 1].type = IRON_VAL_OBJ;
                            stack->data[stack->size - arg_count - 1].value.obj = boxed;
                        }
                    } else {
                        frame->flags &= ~IRON_FRAME_CONSTRAINED;
                        frame->constrained_type = NULL;
                        return IRON_INTERP_ERROR;
                    }

                    frame->flags &= ~IRON_FRAME_CONSTRAINED;
                    frame->constrained_type = NULL;
                    
                    call_res = convert_managed_call_error(thread, call_method(thread, target_method, target_assembly, IRON_FALSE));
                    if (!IRON_RESULT_OK(call_res)) {
                        return call_res.error == IRON_ERR_EXCEPTION ? IRON_INTERP_EXCEPTION : IRON_INTERP_ERROR;
                    }

                    call_res = adapt_interface_dispatch_return(thread->exec_ctx, stack, contract_method, target_method);
                    if (!IRON_RESULT_OK(call_res)) {
                        IRON_DEBUG_EXEC("Interface return adaptation failed for %s::%s implemented by %s::%s: %s",
                                        contract_method->declaring_type && contract_method->declaring_type->full_name ? contract_method->declaring_type->full_name : "<unknown type>",
                                        contract_method->name ? contract_method->name : "<unknown method>",
                                        target_method->declaring_type && target_method->declaring_type->full_name ? target_method->declaring_type->full_name : "<unknown type>",
                                        target_method->name ? target_method->name : "<unknown method>",
                                        call_res.message ? call_res.message : "no diagnostic message");
                        call_res = convert_managed_call_error(thread, call_res);
                        return call_res.error == IRON_ERR_EXCEPTION ? IRON_INTERP_EXCEPTION : IRON_INTERP_ERROR;
                    }
                } else {
                    IRON_ERROR_EXEC("Cannot resolve callvirt target token 0x%08X", method_token);
                    return IRON_INTERP_ERROR;
                }
            }
            break;
            
        /* Object creation */
        case IRON_CEE_NEWOBJ:
            {
                iron_u32 ctor_token = iron_read_u32_le(code + ip);
                iron_runtime_method_t *ctor_method = NULL;
                iron_assembly_t *ctor_assembly = NULL;
                iron_result_t newobj_res;
                
                ip += 4;
                frame->ip = ip;
                
                newobj_res = resolve_method_token(thread, ctor_token,
                                                  &ctor_method, &ctor_assembly);
                if (IRON_RESULT_OK(newobj_res) && ctor_method) {
                    IRON_TRACE_EXEC("newobj: method=%s param_count=%u stack_before=%u",
                                   ctor_method->name ? ctor_method->name : "?",
                                   ctor_method->param_count,
                                   stack->size);
                    newobj_res = convert_managed_call_error(thread, call_method(thread, ctor_method, ctor_assembly, IRON_TRUE));
                    IRON_TRACE_EXEC("newobj: stack_after=%u", stack->size);
                    if (!IRON_RESULT_OK(newobj_res)) {
                        return newobj_res.error == IRON_ERR_EXCEPTION ? IRON_INTERP_EXCEPTION : IRON_INTERP_ERROR;
                    }
                } else {
                    IRON_ERROR_EXEC("Cannot resolve constructor token 0x%08X", ctor_token);
                    return IRON_INTERP_ERROR;
                }
            }
            break;

        /* Arithmetic operations */
        case IRON_CEE_CKFINITE:
            val1 = iron_stack_pop(stack);
            if (!stack_value_is_float(&val1) || !isfinite(stack_value_to_f64(&val1))) {
                iron_throw_arithmetic(thread);
                frame->ip = ip;
                return IRON_INTERP_EXCEPTION;
            }
            iron_stack_push(stack, val1);
            break;

        case IRON_CEE_ADD_OVF:
        case IRON_CEE_ADD_OVF_UN:
        case IRON_CEE_SUB_OVF:
        case IRON_CEE_SUB_OVF_UN:
        case IRON_CEE_MUL_OVF:
        case IRON_CEE_MUL_OVF_UN:
            {
                iron_u32 width;
                iron_bool unsigned_operation;
                iron_bool succeeded;
                iron_u64 result_bits;

                val2 = iron_stack_pop(stack);
                val1 = iron_stack_pop(stack);
                width = stack_integer_width(&val1, &val2);
                unsigned_operation = opcode == IRON_CEE_ADD_OVF_UN || opcode == IRON_CEE_SUB_OVF_UN || opcode == IRON_CEE_MUL_OVF_UN;
                succeeded = IRON_FALSE;
                result_bits = 0;

                if (unsigned_operation) {
                    iron_u64 left;
                    iron_u64 right;
                    iron_u64 maximum;

                    left = stack_value_to_u64(&val1);
                    right = stack_value_to_u64(&val2);
                    maximum = width == 64 ? UINT64_MAX : UINT32_MAX;
                    if (opcode == IRON_CEE_ADD_OVF_UN) {
                        succeeded = left <= maximum && right <= maximum - left;
                        if (succeeded) {
                            result_bits = left + right;
                        }
                    } else if (opcode == IRON_CEE_SUB_OVF_UN) {
                        succeeded = left <= maximum && right <= left;
                        if (succeeded) {
                            result_bits = left - right;
                        }
                    } else {
                        succeeded = left <= maximum && right <= maximum && (right == 0 || left <= maximum / right);
                        if (succeeded) {
                            result_bits = left * right;
                        }
                    }
                } else {
                    iron_i64 left;
                    iron_i64 right;
                    iron_i64 minimum;
                    iron_i64 maximum;
                    iron_i64 result_value;

                    left = stack_value_to_i64(&val1);
                    right = stack_value_to_i64(&val2);
                    minimum = width == 64 ? INT64_MIN : INT32_MIN;
                    maximum = width == 64 ? INT64_MAX : INT32_MAX;
                    if (opcode == IRON_CEE_ADD_OVF) {
                        succeeded = checked_signed_add(left, right, minimum, maximum, &result_value);
                    } else if (opcode == IRON_CEE_SUB_OVF) {
                        succeeded = checked_signed_subtract(left, right, minimum, maximum, &result_value);
                    } else {
                        succeeded = checked_signed_multiply(left, right, minimum, maximum, &result_value);
                    }
                    if (succeeded) {
                        result_bits = (iron_u64)result_value;
                    }
                }

                if (!succeeded) {
                    iron_throw_overflow(thread);
                    frame->ip = ip;
                    return IRON_INTERP_EXCEPTION;
                }
                stack_push_integer_bits(stack, result_bits, width);
            }
            break;

        case IRON_CEE_ADD:
            val2 = iron_stack_pop(stack);
            val1 = iron_stack_pop(stack);
            if (val1.type == IRON_VAL_PTR || val1.type == IRON_VAL_BYREF || val2.type == IRON_VAL_PTR || val2.type == IRON_VAL_BYREF) {
                iron_stack_value_t *address_value;
                iron_stack_value_t *offset_value;
                iron_stack_value_t result;
                iron_size address;
                iron_i64 offset;

                address_value = val1.type == IRON_VAL_PTR || val1.type == IRON_VAL_BYREF ? &val1 : &val2;
                offset_value = address_value == &val1 ? &val2 : &val1;
                address = (iron_size)stack_value_address(address_value);
                offset = stack_value_to_i64(offset_value);
                memset(&result, 0, sizeof(result));
                result.type = address_value->type;
                if (result.type == IRON_VAL_BYREF) {
                    result.value.byref.ptr = (void *)(address + (iron_size)offset);
                    result.value.byref.stack_slot = offset == 0 ? address_value->value.byref.stack_slot : NULL;
                } else {
                    result.value.ptr = (void *)(address + (iron_size)offset);
                }
                iron_stack_push(stack, result);
            } else if (val1.type == IRON_VAL_I32 && val2.type == IRON_VAL_I32) {
                stack_push_integer_bits(stack, (iron_u32)val1.value.i32 + (iron_u32)val2.value.i32, 32);
            } else if (val1.type == IRON_VAL_I64 || val2.type == IRON_VAL_I64) {
                stack_push_integer_bits(stack, (iron_u64)stack_value_to_i64(&val1) + (iron_u64)stack_value_to_i64(&val2), 64);
            } else if (val1.type == IRON_VAL_F64 || val2.type == IRON_VAL_F64) {
                iron_stack_push_f64(stack, 
                    (val1.type == IRON_VAL_F64 ? val1.value.f64 : val1.value.f32) +
                    (val2.type == IRON_VAL_F64 ? val2.value.f64 : val2.value.f32));
            } else {
                iron_stack_push_f32(stack, val1.value.f32 + val2.value.f32);
            }
            break;
            
        case IRON_CEE_SUB:
            val2 = iron_stack_pop(stack);
            val1 = iron_stack_pop(stack);
            if ((val1.type == IRON_VAL_PTR || val1.type == IRON_VAL_BYREF) && (val2.type == IRON_VAL_PTR || val2.type == IRON_VAL_BYREF)) {
                iron_size left_address;
                iron_size right_address;

                left_address = (iron_size)stack_value_address(&val1);
                right_address = (iron_size)stack_value_address(&val2);
                stack_push_integer_bits(stack, (iron_u64)(left_address - right_address), (iron_u32)(sizeof(void *) * CHAR_BIT));
            } else if (val1.type == IRON_VAL_PTR || val1.type == IRON_VAL_BYREF) {
                iron_stack_value_t result;
                iron_size address;
                iron_i64 offset;

                address = (iron_size)stack_value_address(&val1);
                offset = stack_value_to_i64(&val2);
                memset(&result, 0, sizeof(result));
                result.type = val1.type;
                if (result.type == IRON_VAL_BYREF) {
                    result.value.byref.ptr = (void *)(address - (iron_size)offset);
                    result.value.byref.stack_slot = offset == 0 ? val1.value.byref.stack_slot : NULL;
                } else {
                    result.value.ptr = (void *)(address - (iron_size)offset);
                }
                iron_stack_push(stack, result);
            } else if (val1.type == IRON_VAL_I32 && val2.type == IRON_VAL_I32) {
                stack_push_integer_bits(stack, (iron_u32)val1.value.i32 - (iron_u32)val2.value.i32, 32);
            } else if (val1.type == IRON_VAL_I64 || val2.type == IRON_VAL_I64) {
                stack_push_integer_bits(stack, (iron_u64)stack_value_to_i64(&val1) - (iron_u64)stack_value_to_i64(&val2), 64);
            } else if (val1.type == IRON_VAL_F64 || val2.type == IRON_VAL_F64) {
                iron_stack_push_f64(stack,
                    (val1.type == IRON_VAL_F64 ? val1.value.f64 : (iron_f64)val1.value.f32) -
                    (val2.type == IRON_VAL_F64 ? val2.value.f64 : (iron_f64)val2.value.f32));
            } else {
                iron_stack_push_f32(stack, val1.value.f32 - val2.value.f32);
            }
            break;
            
        case IRON_CEE_MUL:
            val2 = iron_stack_pop(stack);
            val1 = iron_stack_pop(stack);
            if (val1.type == IRON_VAL_I32 && val2.type == IRON_VAL_I32) {
                stack_push_integer_bits(stack, (iron_u32)val1.value.i32 * (iron_u32)val2.value.i32, 32);
            } else if (val1.type == IRON_VAL_I64 || val2.type == IRON_VAL_I64) {
                stack_push_integer_bits(stack, (iron_u64)stack_value_to_i64(&val1) * (iron_u64)stack_value_to_i64(&val2), 64);
            } else if (val1.type == IRON_VAL_F64 || val2.type == IRON_VAL_F64) {
                iron_stack_push_f64(stack,
                    (val1.type == IRON_VAL_F64 ? val1.value.f64 : (iron_f64)val1.value.f32) *
                    (val2.type == IRON_VAL_F64 ? val2.value.f64 : (iron_f64)val2.value.f32));
            } else {
                iron_stack_push_f32(stack, val1.value.f32 * val2.value.f32);
            }
            break;

        case IRON_CEE_DIV:
            val2 = iron_stack_pop(stack);
            val1 = iron_stack_pop(stack);
            if (val1.type == IRON_VAL_I32 && val2.type == IRON_VAL_I32) {
                if (val2.value.i32 == 0) {
                    iron_throw_divide_by_zero(thread);
                    frame->ip = ip;
                    return IRON_INTERP_EXCEPTION;
                }
                if (val1.value.i32 == INT32_MIN && val2.value.i32 == -1) {
                    iron_throw_overflow(thread);
                    frame->ip = ip;
                    return IRON_INTERP_EXCEPTION;
                }
                iron_stack_push_i32(stack, val1.value.i32 / val2.value.i32);
            } else if (val1.type == IRON_VAL_I64 || val2.type == IRON_VAL_I64) {
                iron_i64 dividend;
                iron_i64 divisor;

                dividend = val1.type == IRON_VAL_I64 ? val1.value.i64 : (iron_i64)val1.value.i32;
                divisor = val2.type == IRON_VAL_I64 ? val2.value.i64 : (iron_i64)val2.value.i32;
                if (divisor == 0) {
                    iron_throw_divide_by_zero(thread);
                    frame->ip = ip;
                    return IRON_INTERP_EXCEPTION;
                }
                if (dividend == INT64_MIN && divisor == -1) {
                    iron_throw_overflow(thread);
                    frame->ip = ip;
                    return IRON_INTERP_EXCEPTION;
                }
                iron_stack_push_i64(stack, dividend / divisor);
            } else if (val1.type == IRON_VAL_F64 || val2.type == IRON_VAL_F64) {
                iron_f64 dividend;
                iron_f64 divisor;

                dividend = val1.type == IRON_VAL_F64 ? val1.value.f64 : (iron_f64)val1.value.f32;
                divisor = val2.type == IRON_VAL_F64 ? val2.value.f64 : (iron_f64)val2.value.f32;
                iron_stack_push_f64(stack, dividend / divisor);
            } else {
                iron_stack_push_f32(stack, val1.value.f32 / val2.value.f32);
            }
            break;

        case IRON_CEE_DIV_UN:
            val2 = iron_stack_pop(stack);
            val1 = iron_stack_pop(stack);
            if (val1.type == IRON_VAL_I64 || val2.type == IRON_VAL_I64) {
                iron_u64 dividend;
                iron_u64 divisor;

                dividend = val1.type == IRON_VAL_I64 ? (iron_u64)val1.value.i64 : (iron_u64)(iron_u32)val1.value.i32;
                divisor = val2.type == IRON_VAL_I64 ? (iron_u64)val2.value.i64 : (iron_u64)(iron_u32)val2.value.i32;
                if (divisor == 0) {
                    iron_throw_divide_by_zero(thread);
                    frame->ip = ip;
                    return IRON_INTERP_EXCEPTION;
                }
                iron_stack_push_i64(stack, (iron_i64)(dividend / divisor));
            } else {
                iron_u32 dividend;
                iron_u32 divisor;

                dividend = (iron_u32)val1.value.i32;
                divisor = (iron_u32)val2.value.i32;
                if (divisor == 0) {
                    iron_throw_divide_by_zero(thread);
                    frame->ip = ip;
                    return IRON_INTERP_EXCEPTION;
                }
                iron_stack_push_i32(stack, (iron_i32)(dividend / divisor));
            }
            break;
            
        case IRON_CEE_REM:
            val2 = iron_stack_pop(stack);
            val1 = iron_stack_pop(stack);
            if (val1.type == IRON_VAL_I64 || val2.type == IRON_VAL_I64) {
                iron_i64 a = (val1.type == IRON_VAL_I64 ? val1.value.i64 : (iron_i64)val1.value.i32);
                iron_i64 b = (val2.type == IRON_VAL_I64 ? val2.value.i64 : (iron_i64)val2.value.i32);
                if (b == 0) {
                    iron_throw_divide_by_zero(thread);
                    frame->ip = ip;
                    return IRON_INTERP_EXCEPTION;
                }
                if (a == INT64_MIN && b == -1) {
                    iron_stack_push_i64(stack, 0);
                } else {
                    iron_stack_push_i64(stack, a % b);
                }
            } else if (val1.type == IRON_VAL_F64 || val2.type == IRON_VAL_F64) {
                iron_f64 a = (val1.type == IRON_VAL_F64 ? val1.value.f64 : (iron_f64)val1.value.f32);
                iron_f64 b = (val2.type == IRON_VAL_F64 ? val2.value.f64 : (iron_f64)val2.value.f32);
                iron_stack_push_f64(stack, fmod(a, b));
            } else if (val1.type == IRON_VAL_F32 || val2.type == IRON_VAL_F32) {
                iron_stack_push_f32(stack, fmodf(val1.value.f32, val2.value.f32));
            } else {
                if (val2.value.i32 == 0) {
                    iron_throw_divide_by_zero(thread);
                    frame->ip = ip;
                    return IRON_INTERP_EXCEPTION;
                }
                if (val1.value.i32 == INT32_MIN && val2.value.i32 == -1) {
                    iron_stack_push_i32(stack, 0);
                } else {
                    iron_stack_push_i32(stack, val1.value.i32 % val2.value.i32);
                }
            }
            break;

        case IRON_CEE_REM_UN:
            val2 = iron_stack_pop(stack);
            val1 = iron_stack_pop(stack);
            if (val1.type == IRON_VAL_I64 || val2.type == IRON_VAL_I64) {
                iron_u64 dividend;
                iron_u64 divisor;

                dividend = val1.type == IRON_VAL_I64 ? (iron_u64)val1.value.i64 : (iron_u64)(iron_u32)val1.value.i32;
                divisor = val2.type == IRON_VAL_I64 ? (iron_u64)val2.value.i64 : (iron_u64)(iron_u32)val2.value.i32;
                if (divisor == 0) {
                    iron_throw_divide_by_zero(thread);
                    frame->ip = ip;
                    return IRON_INTERP_EXCEPTION;
                }
                iron_stack_push_i64(stack, (iron_i64)(dividend % divisor));
            } else {
                iron_u32 dividend;
                iron_u32 divisor;

                dividend = (iron_u32)val1.value.i32;
                divisor = (iron_u32)val2.value.i32;
                if (divisor == 0) {
                    iron_throw_divide_by_zero(thread);
                    frame->ip = ip;
                    return IRON_INTERP_EXCEPTION;
                }
                iron_stack_push_i32(stack, (iron_i32)(dividend % divisor));
            }
            break;
            
        /* Bitwise operations */
        case IRON_CEE_AND:
            val2 = iron_stack_pop(stack);
            val1 = iron_stack_pop(stack);
            if (val1.type == IRON_VAL_I64 || val2.type == IRON_VAL_I64) {
                iron_stack_push_i64(stack,
                    (val1.type == IRON_VAL_I64 ? val1.value.i64 : (iron_i64)val1.value.i32) &
                    (val2.type == IRON_VAL_I64 ? val2.value.i64 : (iron_i64)val2.value.i32));
            } else {
                iron_stack_push_i32(stack, val1.value.i32 & val2.value.i32);
            }
            break;

        case IRON_CEE_OR:
            val2 = iron_stack_pop(stack);
            val1 = iron_stack_pop(stack);
            if (val1.type == IRON_VAL_I64 || val2.type == IRON_VAL_I64) {
                iron_stack_push_i64(stack,
                    (val1.type == IRON_VAL_I64 ? val1.value.i64 : (iron_i64)val1.value.i32) |
                    (val2.type == IRON_VAL_I64 ? val2.value.i64 : (iron_i64)val2.value.i32));
            } else {
                iron_stack_push_i32(stack, val1.value.i32 | val2.value.i32);
            }
            break;

        case IRON_CEE_XOR:
            val2 = iron_stack_pop(stack);
            val1 = iron_stack_pop(stack);
            if (val1.type == IRON_VAL_I64 || val2.type == IRON_VAL_I64) {
                iron_stack_push_i64(stack,
                    (val1.type == IRON_VAL_I64 ? val1.value.i64 : (iron_i64)val1.value.i32) ^
                    (val2.type == IRON_VAL_I64 ? val2.value.i64 : (iron_i64)val2.value.i32));
            } else {
                iron_stack_push_i32(stack, val1.value.i32 ^ val2.value.i32);
            }
            break;

        case IRON_CEE_SHL:
            val2 = iron_stack_pop(stack);
            val1 = iron_stack_pop(stack);
            if (val1.type == IRON_VAL_I64) {
                stack_push_integer_bits(stack, (iron_u64)val1.value.i64 << (val2.value.i32 & 0x3F), 64);
            } else {
                stack_push_integer_bits(stack, (iron_u32)val1.value.i32 << (val2.value.i32 & 0x1F), 32);
            }
            break;

        case IRON_CEE_SHR:
            val2 = iron_stack_pop(stack);
            val1 = iron_stack_pop(stack);
            if (val1.type == IRON_VAL_I64) {
                stack_push_integer_bits(stack, arithmetic_shift_right_u64(val1.value.i64, (iron_u32)val2.value.i32 & 0x3FU), 64);
            } else {
                stack_push_integer_bits(stack, arithmetic_shift_right_u32(val1.value.i32, (iron_u32)val2.value.i32 & 0x1FU), 32);
            }
            break;

        case IRON_CEE_SHR_UN:
            val2 = iron_stack_pop(stack);
            val1 = iron_stack_pop(stack);
            if (val1.type == IRON_VAL_I64) {
                iron_stack_push_i64(stack, (iron_i64)((iron_u64)val1.value.i64 >> (val2.value.i32 & 0x3F)));
            } else {
                iron_stack_push_i32(stack, (iron_i32)((iron_u32)val1.value.i32 >> (val2.value.i32 & 0x1F)));
            }
            break;

        case IRON_CEE_NEG:
            val1 = iron_stack_pop(stack);
            if (val1.type == IRON_VAL_I32) {
                stack_push_integer_bits(stack, 0U - (iron_u32)val1.value.i32, 32);
            } else if (val1.type == IRON_VAL_I64) {
                stack_push_integer_bits(stack, 0ULL - (iron_u64)val1.value.i64, 64);
            } else if (val1.type == IRON_VAL_F32) {
                iron_stack_push_f32(stack, -val1.value.f32);
            } else {
                iron_stack_push_f64(stack, -val1.value.f64);
            }
            break;

        case IRON_CEE_NOT:
            val1 = iron_stack_pop(stack);
            if (val1.type == IRON_VAL_I64) {
                stack_push_integer_bits(stack, ~(iron_u64)val1.value.i64, 64);
            } else {
                stack_push_integer_bits(stack, ~(iron_u32)val1.value.i32, 32);
            }
            break;
            
        /* Comparison operations */
        case IRON_CEE_CEQ:
            val2 = iron_stack_pop(stack);
            val1 = iron_stack_pop(stack);
            iron_stack_push_i32(stack, stack_values_equal(&val1, &val2) ? 1 : 0);
            break;
            
        case IRON_CEE_CGT:
            val2 = iron_stack_pop(stack);
            val1 = iron_stack_pop(stack);
            iron_stack_push_i32(stack, stack_values_compare(&val1, &val2, IRON_COMPARE_GREATER, IRON_FALSE) ? 1 : 0);
            break;
            
        case IRON_CEE_CLT:
            val2 = iron_stack_pop(stack);
            val1 = iron_stack_pop(stack);
            iron_stack_push_i32(stack, stack_values_compare(&val1, &val2, IRON_COMPARE_LESS, IRON_FALSE) ? 1 : 0);
            break;
            
        case IRON_CEE_CGT_UN:
            val2 = iron_stack_pop(stack);
            val1 = iron_stack_pop(stack);
            iron_stack_push_i32(stack, stack_values_compare(&val1, &val2, IRON_COMPARE_GREATER, IRON_TRUE) ? 1 : 0);
            break;
            
        case IRON_CEE_CLT_UN:
            val2 = iron_stack_pop(stack);
            val1 = iron_stack_pop(stack);
            iron_stack_push_i32(stack, stack_values_compare(&val1, &val2, IRON_COMPARE_LESS, IRON_TRUE) ? 1 : 0);
            break;
            
        /* Branch instructions */
        case IRON_CEE_BR:
            i32_val = iron_read_i32_le(code + ip);
            ip += 4 + i32_val;
            break;
            
        case IRON_CEE_BR_S:
            i32_val = (iron_i8)code[ip++];
            ip += i32_val;
            break;
            
        case IRON_CEE_BRFALSE:
        case IRON_CEE_BRTRUE:
            i32_val = iron_read_i32_le(code + ip);
            ip += 4;
            val1 = iron_stack_pop(stack);
            if ((opcode == IRON_CEE_BRFALSE && stack_value_is_zero(&val1)) ||
                (opcode == IRON_CEE_BRTRUE && !stack_value_is_zero(&val1))) {
                ip += i32_val;
            }
            break;
            
        case IRON_CEE_BRFALSE_S:
        case IRON_CEE_BRTRUE_S:
            i32_val = (iron_i8)code[ip++];
            val1 = iron_stack_pop(stack);
            if ((opcode == IRON_CEE_BRFALSE_S && stack_value_is_zero(&val1)) ||
                (opcode == IRON_CEE_BRTRUE_S && !stack_value_is_zero(&val1))) {
                ip += i32_val;
            }
            break;
            
        case IRON_CEE_BEQ:
        case IRON_CEE_BEQ_S:
            if (opcode == IRON_CEE_BEQ) {
                i32_val = iron_read_i32_le(code + ip);
                ip += 4;
            } else {
                i32_val = (iron_i8)code[ip++];
            }
            val2 = iron_stack_pop(stack);
            val1 = iron_stack_pop(stack);
            if (stack_values_equal(&val1, &val2)) {
                ip += i32_val;
            }
            break;
            
        case IRON_CEE_BNE_UN:
        case IRON_CEE_BNE_UN_S:
            if (opcode == IRON_CEE_BNE_UN) {
                i32_val = iron_read_i32_le(code + ip);
                ip += 4;
            } else {
                i32_val = (iron_i8)code[ip++];
            }
            val2 = iron_stack_pop(stack);
            val1 = iron_stack_pop(stack);
            if (!stack_values_equal(&val1, &val2)) {
                ip += i32_val;
            }
            break;
            
        case IRON_CEE_BGE:
        case IRON_CEE_BGE_S:
            if (opcode == IRON_CEE_BGE) {
                i32_val = iron_read_i32_le(code + ip);
                ip += 4;
            } else {
                i32_val = (iron_i8)code[ip++];
            }
            val2 = iron_stack_pop(stack);
            val1 = iron_stack_pop(stack);
            if (stack_values_compare(&val1, &val2, IRON_COMPARE_GREATER_OR_EQUAL, IRON_FALSE)) {
                ip += i32_val;
            }
            break;
            
        case IRON_CEE_BGT:
        case IRON_CEE_BGT_S:
            if (opcode == IRON_CEE_BGT) {
                i32_val = iron_read_i32_le(code + ip);
                ip += 4;
            } else {
                i32_val = (iron_i8)code[ip++];
            }
            val2 = iron_stack_pop(stack);
            val1 = iron_stack_pop(stack);
            if (stack_values_compare(&val1, &val2, IRON_COMPARE_GREATER, IRON_FALSE)) {
                ip += i32_val;
            }
            break;
            
        case IRON_CEE_BLE:
        case IRON_CEE_BLE_S:
            if (opcode == IRON_CEE_BLE) {
                i32_val = iron_read_i32_le(code + ip);
                ip += 4;
            } else {
                i32_val = (iron_i8)code[ip++];
            }
            val2 = iron_stack_pop(stack);
            val1 = iron_stack_pop(stack);
            if (stack_values_compare(&val1, &val2, IRON_COMPARE_LESS_OR_EQUAL, IRON_FALSE)) {
                ip += i32_val;
            }
            break;
            
        case IRON_CEE_BLT:
        case IRON_CEE_BLT_S:
            if (opcode == IRON_CEE_BLT) {
                i32_val = iron_read_i32_le(code + ip);
                ip += 4;
            } else {
                i32_val = (iron_i8)code[ip++];
            }
            val2 = iron_stack_pop(stack);
            val1 = iron_stack_pop(stack);
            if (stack_values_compare(&val1, &val2, IRON_COMPARE_LESS, IRON_FALSE)) {
                ip += i32_val;
            }
            break;
            
        case IRON_CEE_BGE_UN:
        case IRON_CEE_BGE_UN_S:
            if (opcode == IRON_CEE_BGE_UN) {
                i32_val = iron_read_i32_le(code + ip);
                ip += 4;
            } else {
                i32_val = (iron_i8)code[ip++];
            }
            val2 = iron_stack_pop(stack);
            val1 = iron_stack_pop(stack);
            if (stack_values_compare(&val1, &val2, IRON_COMPARE_GREATER_OR_EQUAL, IRON_TRUE)) {
                ip += i32_val;
            }
            break;
            
        case IRON_CEE_BGT_UN:
        case IRON_CEE_BGT_UN_S:
            if (opcode == IRON_CEE_BGT_UN) {
                i32_val = iron_read_i32_le(code + ip);
                ip += 4;
            } else {
                i32_val = (iron_i8)code[ip++];
            }
            val2 = iron_stack_pop(stack);
            val1 = iron_stack_pop(stack);
            if (stack_values_compare(&val1, &val2, IRON_COMPARE_GREATER, IRON_TRUE)) {
                ip += i32_val;
            }
            break;
            
        case IRON_CEE_BLE_UN:
        case IRON_CEE_BLE_UN_S:
            if (opcode == IRON_CEE_BLE_UN) {
                i32_val = iron_read_i32_le(code + ip);
                ip += 4;
            } else {
                i32_val = (iron_i8)code[ip++];
            }
            val2 = iron_stack_pop(stack);
            val1 = iron_stack_pop(stack);
            if (stack_values_compare(&val1, &val2, IRON_COMPARE_LESS_OR_EQUAL, IRON_TRUE)) {
                ip += i32_val;
            }
            break;
            
        case IRON_CEE_BLT_UN:
        case IRON_CEE_BLT_UN_S:
            if (opcode == IRON_CEE_BLT_UN) {
                i32_val = iron_read_i32_le(code + ip);
                ip += 4;
            } else {
                i32_val = (iron_i8)code[ip++];
            }
            val2 = iron_stack_pop(stack);
            val1 = iron_stack_pop(stack);
            if (stack_values_compare(&val1, &val2, IRON_COMPARE_LESS, IRON_TRUE)) {
                ip += i32_val;
            }
            break;

        case IRON_CEE_SWITCH:
            {
                iron_u32 count;
                iron_u32 table_size;
                iron_u32 base;
                iron_i32 index;

                if (ip > frame->code_size || frame->code_size - ip < 4) {
                    IRON_ERROR_EXEC("Truncated switch instruction");
                    frame->ip = ip;
                    return IRON_INTERP_ERROR;
                }

                count = iron_read_u32_le(code + ip);
                ip += 4;
                if (count > (frame->code_size - ip) / 4) {
                    IRON_ERROR_EXEC("Switch table exceeds the method body");
                    frame->ip = ip;
                    return IRON_INTERP_ERROR;
                }

                table_size = count * 4;
                base = ip + table_size;
                index = iron_stack_pop_i32(stack);
                if (index >= 0 && (iron_u32)index < count) {
                    iron_u32 target;
                    iron_i32 displacement;

                    displacement = iron_read_i32_le(code + ip + (iron_u32)index * 4);
                    if (!checked_instruction_target(base, displacement, frame->code_size, &target)) {
                        IRON_ERROR_EXEC("Switch target is outside the method body");
                        frame->ip = ip;
                        return IRON_INTERP_ERROR;
                    }
                    ip = target;
                } else {
                    ip = base;
                }
            }
            break;
            
        /* Return */
        case IRON_CEE_RET:
            frame->ip = ip;
            return IRON_INTERP_RETURN;
            
        /* Conversion instructions */
        case IRON_CEE_CONV_OVF_I1:
        case IRON_CEE_CONV_OVF_I1_UN:
        case IRON_CEE_CONV_OVF_U1:
        case IRON_CEE_CONV_OVF_U1_UN:
        case IRON_CEE_CONV_OVF_I2:
        case IRON_CEE_CONV_OVF_I2_UN:
        case IRON_CEE_CONV_OVF_U2:
        case IRON_CEE_CONV_OVF_U2_UN:
        case IRON_CEE_CONV_OVF_I4:
        case IRON_CEE_CONV_OVF_I4_UN:
        case IRON_CEE_CONV_OVF_U4:
        case IRON_CEE_CONV_OVF_U4_UN:
        case IRON_CEE_CONV_OVF_I8:
        case IRON_CEE_CONV_OVF_I8_UN:
        case IRON_CEE_CONV_OVF_U8:
        case IRON_CEE_CONV_OVF_U8_UN:
        case IRON_CEE_CONV_OVF_I:
        case IRON_CEE_CONV_OVF_I_UN:
        case IRON_CEE_CONV_OVF_U:
        case IRON_CEE_CONV_OVF_U_UN:
            {
                iron_u32 target_width;
                iron_bool target_unsigned;
                iron_bool source_unsigned;

                val1 = iron_stack_pop(stack);
                target_width = (iron_u32)(sizeof(void *) * CHAR_BIT);
                target_unsigned = IRON_FALSE;

                switch (opcode) {
                    case IRON_CEE_CONV_OVF_I1:
                    case IRON_CEE_CONV_OVF_I1_UN:
                        target_width = 8;
                        break;
                    case IRON_CEE_CONV_OVF_U1:
                    case IRON_CEE_CONV_OVF_U1_UN:
                        target_width = 8;
                        target_unsigned = IRON_TRUE;
                        break;
                    case IRON_CEE_CONV_OVF_I2:
                    case IRON_CEE_CONV_OVF_I2_UN:
                        target_width = 16;
                        break;
                    case IRON_CEE_CONV_OVF_U2:
                    case IRON_CEE_CONV_OVF_U2_UN:
                        target_width = 16;
                        target_unsigned = IRON_TRUE;
                        break;
                    case IRON_CEE_CONV_OVF_I4:
                    case IRON_CEE_CONV_OVF_I4_UN:
                        target_width = 32;
                        break;
                    case IRON_CEE_CONV_OVF_U4:
                    case IRON_CEE_CONV_OVF_U4_UN:
                        target_width = 32;
                        target_unsigned = IRON_TRUE;
                        break;
                    case IRON_CEE_CONV_OVF_I8:
                    case IRON_CEE_CONV_OVF_I8_UN:
                        target_width = 64;
                        break;
                    case IRON_CEE_CONV_OVF_U8:
                    case IRON_CEE_CONV_OVF_U8_UN:
                        target_width = 64;
                        target_unsigned = IRON_TRUE;
                        break;
                    case IRON_CEE_CONV_OVF_U:
                    case IRON_CEE_CONV_OVF_U_UN:
                        target_unsigned = IRON_TRUE;
                        break;
                    default:
                        break;
                }

                source_unsigned = opcode == IRON_CEE_CONV_OVF_I1_UN || opcode == IRON_CEE_CONV_OVF_U1_UN ||
                                  opcode == IRON_CEE_CONV_OVF_I2_UN || opcode == IRON_CEE_CONV_OVF_U2_UN ||
                                  opcode == IRON_CEE_CONV_OVF_I4_UN || opcode == IRON_CEE_CONV_OVF_U4_UN ||
                                  opcode == IRON_CEE_CONV_OVF_I8_UN || opcode == IRON_CEE_CONV_OVF_U8_UN ||
                                  opcode == IRON_CEE_CONV_OVF_I_UN || opcode == IRON_CEE_CONV_OVF_U_UN;
                if (!checked_convert_integer(stack, &val1, target_width, target_unsigned, source_unsigned)) {
                    iron_throw_overflow(thread);
                    frame->ip = ip;
                    return IRON_INTERP_EXCEPTION;
                }
            }
            break;

        case IRON_CEE_CONV_I1:
            val1 = iron_stack_pop(stack);
            if (val1.type == IRON_VAL_F64) {
                iron_stack_push_i32(stack, (iron_i32)(iron_i8)(iron_i64)val1.value.f64);
            } else if (val1.type == IRON_VAL_F32) {
                iron_stack_push_i32(stack, (iron_i32)(iron_i8)(iron_i32)val1.value.f32);
            } else {
                iron_stack_push_i32(stack, (iron_i32)(iron_i8)stack_value_to_i64(&val1));
            }
            break;
        case IRON_CEE_CONV_I2:
            val1 = iron_stack_pop(stack);
            if (val1.type == IRON_VAL_F64) {
                iron_stack_push_i32(stack, (iron_i32)(iron_i16)(iron_i64)val1.value.f64);
            } else if (val1.type == IRON_VAL_F32) {
                iron_stack_push_i32(stack, (iron_i32)(iron_i16)(iron_i32)val1.value.f32);
            } else {
                iron_stack_push_i32(stack, (iron_i32)(iron_i16)stack_value_to_i64(&val1));
            }
            break;
        case IRON_CEE_CONV_I4:
            val1 = iron_stack_pop(stack);
            if (val1.type == IRON_VAL_F64) {
                iron_stack_push_i32(stack, (iron_i32)val1.value.f64);
            } else if (val1.type == IRON_VAL_F32) {
                iron_stack_push_i32(stack, (iron_i32)val1.value.f32);
            } else {
                iron_stack_push_i32(stack, (iron_i32)stack_value_to_i64(&val1));
            }
            break;
        case IRON_CEE_CONV_I8:
            val1 = iron_stack_pop(stack);
            if (val1.type == IRON_VAL_F64) {
                iron_stack_push_i64(stack, (iron_i64)val1.value.f64);
            } else if (val1.type == IRON_VAL_F32) {
                iron_stack_push_i64(stack, (iron_i64)val1.value.f32);
            } else {
                iron_stack_push_i64(stack, stack_value_to_i64(&val1));
            }
            break;
        case IRON_CEE_CONV_U1:
            val1 = iron_stack_pop(stack);
            if (val1.type == IRON_VAL_F64) {
                iron_stack_push_i32(stack, (iron_i32)(iron_u8)(iron_u64)val1.value.f64);
            } else if (val1.type == IRON_VAL_F32) {
                iron_stack_push_i32(stack, (iron_i32)(iron_u8)(iron_u32)val1.value.f32);
            } else {
                iron_stack_push_i32(stack, (iron_i32)(iron_u8)stack_value_to_u64(&val1));
            }
            break;
        case IRON_CEE_CONV_U2:
            val1 = iron_stack_pop(stack);
            if (val1.type == IRON_VAL_F64) {
                iron_stack_push_i32(stack, (iron_i32)(iron_u16)(iron_u64)val1.value.f64);
            } else if (val1.type == IRON_VAL_F32) {
                iron_stack_push_i32(stack, (iron_i32)(iron_u16)(iron_u32)val1.value.f32);
            } else {
                iron_stack_push_i32(stack, (iron_i32)(iron_u16)stack_value_to_u64(&val1));
            }
            break;
        case IRON_CEE_CONV_U4:
            val1 = iron_stack_pop(stack);
            if (val1.type == IRON_VAL_F64) {
                iron_stack_push_i32(stack, (iron_i32)(iron_u32)val1.value.f64);
            } else if (val1.type == IRON_VAL_F32) {
                iron_stack_push_i32(stack, (iron_i32)(iron_u32)val1.value.f32);
            } else {
                iron_stack_push_i32(stack, (iron_i32)(iron_u32)stack_value_to_u64(&val1));
            }
            break;
        case IRON_CEE_CONV_U8:
            val1 = iron_stack_pop(stack);
            if (val1.type == IRON_VAL_F64) {
                iron_stack_push_i64(stack, (iron_i64)(iron_u64)val1.value.f64);
            } else if (val1.type == IRON_VAL_F32) {
                iron_stack_push_i64(stack, (iron_i64)(iron_u64)val1.value.f32);
            } else {
                iron_stack_push_i64(stack, (iron_i64)stack_value_to_u64(&val1));
            }
            break;
        case IRON_CEE_CONV_R4:
            val1 = iron_stack_pop(stack);
            if (val1.type == IRON_VAL_I64) {
                iron_stack_push_f32(stack, (iron_f32)val1.value.i64);
            } else if (val1.type == IRON_VAL_F64) {
                iron_stack_push_f32(stack, (iron_f32)val1.value.f64);
            } else if (val1.type == IRON_VAL_F32) {
                iron_stack_push_f32(stack, val1.value.f32);
            } else {
                iron_stack_push_f32(stack, (iron_f32)val1.value.i32);
            }
            break;
        case IRON_CEE_CONV_R8:
            val1 = iron_stack_pop(stack);
            if (val1.type == IRON_VAL_I64) {
                iron_stack_push_f64(stack, (iron_f64)val1.value.i64);
            } else if (val1.type == IRON_VAL_F64) {
                iron_stack_push_f64(stack, val1.value.f64);
            } else if (val1.type == IRON_VAL_F32) {
                iron_stack_push_f64(stack, (iron_f64)val1.value.f32);
            } else {
                iron_stack_push_f64(stack, (iron_f64)val1.value.i32);
            }
            break;
        case IRON_CEE_CONV_R_UN:
            val1 = iron_stack_pop(stack);
            iron_stack_push_f64(stack, (iron_f64)stack_value_to_u64(&val1));
            break;
        case IRON_CEE_CONV_I:
            val1 = iron_stack_pop(stack);
            if (val1.type == IRON_VAL_F64) {
                stack_push_integer_bits(stack, (iron_u64)(iron_i64)val1.value.f64, (iron_u32)(sizeof(void *) * CHAR_BIT));
            } else if (val1.type == IRON_VAL_F32) {
                stack_push_integer_bits(stack, (iron_u64)(iron_i64)val1.value.f32, (iron_u32)(sizeof(void *) * CHAR_BIT));
            } else {
                stack_push_integer_bits(stack, (iron_u64)stack_value_to_i64(&val1), (iron_u32)(sizeof(void *) * CHAR_BIT));
            }
            break;
        case IRON_CEE_CONV_U:
            val1 = iron_stack_pop(stack);
            if (val1.type == IRON_VAL_F64) {
                stack_push_integer_bits(stack, (iron_u64)val1.value.f64, (iron_u32)(sizeof(void *) * CHAR_BIT));
            } else if (val1.type == IRON_VAL_F32) {
                stack_push_integer_bits(stack, (iron_u64)(iron_u32)(iron_i32)val1.value.f32, (iron_u32)(sizeof(void *) * CHAR_BIT));
            } else {
                stack_push_integer_bits(stack, stack_value_to_u64(&val1), (iron_u32)(sizeof(void *) * CHAR_BIT));
            }
            break;
            
        /* Array creation */
        case IRON_CEE_NEWARR:
            {
                iron_u32 type_token = iron_read_u32_le(code + ip);
                iron_stack_value_t len_val;
                iron_i32 length;
                iron_runtime_type_t *element_type = NULL;
                void *array_obj = NULL;
                
                ip += 4;
                
                /* Pop array length from stack */
                len_val = iron_stack_pop(stack);
                length = len_val.value.i32;
                IRON_TRACE_EXEC("newarr: type_token=0x%08X length=%d (val_type=%d)", 
                               type_token, length, len_val.type);
                
                if (length < 0) {
                    iron_throw_overflow(thread);
                    frame->ip = ip;
                    return IRON_INTERP_EXCEPTION;
                }
                
                element_type = resolve_runtime_type_token(thread, type_token);
                
                if (!element_type) {
                    IRON_ERROR_EXEC("Cannot resolve array element type token 0x%08X", type_token);
                    frame->ip = ip;
                    return IRON_INTERP_ERROR;
                }
                
                /* Allocate array using exec context's GC alloc */
                array_obj = iron_gc_alloc_array(thread->exec_ctx, element_type, (iron_u32)length);
                
                if (!array_obj) {
                    fprintf(stderr, "[ERROR] Failed to allocate array of length %d\n", length);
                    return IRON_INTERP_ERROR;
                }
                
                iron_stack_push_obj(stack, array_obj);
            }
            break;
            
        /* Array element access */
        case IRON_CEE_LDELEM_I1:
        case IRON_CEE_LDELEM_U1:
        case IRON_CEE_LDELEM_I2:
        case IRON_CEE_LDELEM_U2:
        case IRON_CEE_LDELEM_I4:
        case IRON_CEE_LDELEM_U4:
        case IRON_CEE_LDELEM_I8:
        case IRON_CEE_LDELEM_I:
        case IRON_CEE_LDELEM_R4:
        case IRON_CEE_LDELEM_R8:
            {
                iron_i32 index;
                void *arr;
                iron_u8 *element;

                index = iron_stack_pop_i32(stack);
                arr = iron_stack_pop_obj(stack);
                if (!arr) {
                    iron_throw_null_reference(thread);
                    frame->ip = ip;
                    return IRON_INTERP_EXCEPTION;
                }

                if (index < 0 || (iron_u32)index >= iron_array_get_length(arr)) {
                    iron_throw_index_out_of_range(thread);
                    frame->ip = ip;
                    return IRON_INTERP_EXCEPTION;
                }

                element = (iron_u8 *)iron_array_get_data(arr) + (iron_size)index * iron_array_get_element_size(arr);
                switch (opcode) {
                    case IRON_CEE_LDELEM_I1:
                        iron_stack_push_i32(stack, (iron_i32)*(iron_i8 *)element);
                        break;
                    case IRON_CEE_LDELEM_U1:
                        iron_stack_push_i32(stack, (iron_i32)*(iron_u8 *)element);
                        break;
                    case IRON_CEE_LDELEM_I2:
                        {
                            iron_i16 value;
                            memcpy(&value, element, sizeof(value));
                            iron_stack_push_i32(stack, (iron_i32)value);
                        }
                        break;
                    case IRON_CEE_LDELEM_U2:
                        {
                            iron_u16 value;
                            memcpy(&value, element, sizeof(value));
                            iron_stack_push_i32(stack, (iron_i32)value);
                        }
                        break;
                    case IRON_CEE_LDELEM_I4:
                    case IRON_CEE_LDELEM_U4:
                        {
                            iron_i32 value;
                            memcpy(&value, element, sizeof(value));
                            iron_stack_push_i32(stack, value);
                        }
                        break;
                    case IRON_CEE_LDELEM_I8:
                        {
                            iron_i64 value;
                            memcpy(&value, element, sizeof(value));
                            iron_stack_push_i64(stack, value);
                        }
                        break;
                    case IRON_CEE_LDELEM_I:
                        if (sizeof(void *) == 8) {
                            iron_i64 value;
                            memcpy(&value, element, sizeof(value));
                            iron_stack_push_i64(stack, value);
                        } else {
                            iron_i32 value;
                            memcpy(&value, element, sizeof(value));
                            iron_stack_push_i32(stack, value);
                        }
                        break;
                    case IRON_CEE_LDELEM_R4:
                        {
                            iron_f32 value;
                            memcpy(&value, element, sizeof(value));
                            iron_stack_push_f32(stack, value);
                        }
                        break;
                    case IRON_CEE_LDELEM_R8:
                        {
                            iron_f64 value;
                            memcpy(&value, element, sizeof(value));
                            iron_stack_push_f64(stack, value);
                        }
                        break;
                    default:
                        break;
                }
            }
            break;

        case IRON_CEE_STELEM_I:
        case IRON_CEE_STELEM_I1:
        case IRON_CEE_STELEM_I2:
        case IRON_CEE_STELEM_I4:
        case IRON_CEE_STELEM_I8:
        case IRON_CEE_STELEM_R4:
        case IRON_CEE_STELEM_R8:
            {
                iron_stack_value_t value;
                iron_i32 index;
                void *arr;
                iron_u8 *element;

                value = iron_stack_pop(stack);
                index = iron_stack_pop_i32(stack);
                arr = iron_stack_pop_obj(stack);
                if (!arr) {
                    iron_throw_null_reference(thread);
                    frame->ip = ip;
                    return IRON_INTERP_EXCEPTION;
                }

                if (index < 0 || (iron_u32)index >= iron_array_get_length(arr)) {
                    iron_throw_index_out_of_range(thread);
                    frame->ip = ip;
                    return IRON_INTERP_EXCEPTION;
                }

                element = (iron_u8 *)iron_array_get_data(arr) + (iron_size)index * iron_array_get_element_size(arr);
                switch (opcode) {
                    case IRON_CEE_STELEM_I1:
                        *element = (iron_u8)value.value.i32;
                        break;
                    case IRON_CEE_STELEM_I2:
                        {
                            iron_u16 converted;
                            converted = (iron_u16)value.value.i32;
                            memcpy(element, &converted, sizeof(converted));
                        }
                        break;
                    case IRON_CEE_STELEM_I4:
                        memcpy(element, &value.value.i32, sizeof(value.value.i32));
                        break;
                    case IRON_CEE_STELEM_I8:
                        memcpy(element, &value.value.i64, sizeof(value.value.i64));
                        break;
                    case IRON_CEE_STELEM_I:
                        if (sizeof(void *) == 8) {
                            iron_i64 converted;
                            converted = stack_value_to_i64(&value);
                            memcpy(element, &converted, sizeof(converted));
                        } else {
                            iron_i32 converted;
                            converted = (iron_i32)stack_value_to_i64(&value);
                            memcpy(element, &converted, sizeof(converted));
                        }
                        break;
                    case IRON_CEE_STELEM_R4:
                        {
                            iron_f32 converted;
                            converted = value.type == IRON_VAL_F32 ? value.value.f32 : (iron_f32)value.value.f64;
                            memcpy(element, &converted, sizeof(converted));
                        }
                        break;
                    case IRON_CEE_STELEM_R8:
                        {
                            iron_f64 converted;
                            converted = value.type == IRON_VAL_F32 ? (iron_f64)value.value.f32 : value.value.f64;
                            memcpy(element, &converted, sizeof(converted));
                        }
                        break;
                    default:
                        break;
                }
            }
            break;

        case IRON_CEE_LDELEM_REF:
            {
                iron_i32 index = iron_stack_pop_i32(stack);
                void *arr = iron_stack_pop_obj(stack);
                iron_u32 arr_len;
                iron_runtime_type_t *element_type;
                void **elements;
                
                if (!arr) {
                    iron_throw_null_reference(thread);
                    frame->ip = ip;
                    return IRON_INTERP_EXCEPTION;
                }
                
                arr_len = iron_array_get_length(arr);
                if (index < 0 || (iron_u32)index >= arr_len) {
                    iron_throw_index_out_of_range(thread);
                    frame->ip = ip;
                    return IRON_INTERP_EXCEPTION;
                }
                
                element_type = iron_array_get_element_type(arr);
                if (!element_type || !iron_type_is_managed_reference(element_type)) {
                    IRON_ERROR_EXEC("ldelem.ref used with a non-reference array");
                    frame->ip = ip;
                    return IRON_INTERP_ERROR;
                }
                elements = (void **)iron_array_get_data(arr);
                iron_stack_push_obj(stack, elements[index]);
            }
            break;
            
        case IRON_CEE_STELEM_REF:
            {
                void *value = iron_stack_pop_obj(stack);
                iron_i32 index = iron_stack_pop_i32(stack);
                void *arr = iron_stack_pop_obj(stack);
                iron_u32 arr_len;
                iron_runtime_type_t *element_type;
                void *element_storage;
                iron_stack_value_t value_storage;
                
                if (!arr) {
                    iron_throw_null_reference(thread);
                    frame->ip = ip;
                    return IRON_INTERP_EXCEPTION;
                }
                
                arr_len = iron_array_get_length(arr);
                if (index < 0 || (iron_u32)index >= arr_len) {
                    iron_throw_index_out_of_range(thread);
                    frame->ip = ip;
                    return IRON_INTERP_EXCEPTION;
                }
                
                element_type = iron_array_get_element_type(arr);
                element_storage = (iron_u8 *)iron_array_get_data(arr) + (iron_size)index * iron_array_get_element_size(arr);
                memset(&value_storage, 0, sizeof(value_storage));
                value_storage.type = IRON_VAL_OBJ;
                value_storage.value.obj = value;
                if (!element_type || !iron_type_is_managed_reference(element_type) || !store_storage_value(arr, element_storage, element_type, &value_storage)) {
                    IRON_ERROR_EXEC("stelem.ref value is incompatible with the array element type");
                    frame->ip = ip;
                    return IRON_INTERP_ERROR;
                }
            }
            break;
            
        case IRON_CEE_STELEM:
            {
                iron_u32 type_token = iron_read_u32_le(code + ip);
                iron_stack_value_t value_val = iron_stack_pop(stack);
                iron_stack_value_t index_val = iron_stack_pop(stack);
                iron_stack_value_t arr_val = iron_stack_pop(stack);
                iron_i32 index = index_val.value.i32;
                void *arr = (arr_val.type == IRON_VAL_OBJ) ? arr_val.value.obj : arr_val.value.ptr;
                iron_u32 arr_len;
                iron_runtime_type_t *element_type;
                void *element_storage;

                ip += 4;
                
                IRON_TRACE_EXEC("stelem: arr=%p index=%d value_type=%d", arr, index, value_val.type);
                
                if (!arr) {
                    iron_throw_null_reference(thread);
                    frame->ip = ip;
                    return IRON_INTERP_EXCEPTION;
                }
                
                arr_len = iron_array_get_length(arr);
                IRON_TRACE_EXEC("stelem: arr_len=%u", arr_len);
                if (index < 0 || (iron_u32)index >= arr_len) {
                    iron_throw_index_out_of_range(thread);
                    frame->ip = ip;
                    return IRON_INTERP_EXCEPTION;
                }
                
                element_type = iron_array_get_element_type(arr);
                element_storage = (iron_u8 *)iron_array_get_data(arr) + (iron_size)index * iron_array_get_element_size(arr);
                if (!element_type || !store_storage_value(arr, element_storage, element_type, &value_val)) {
                    IRON_ERROR_EXEC("Cannot store array element for type token 0x%08X", type_token);
                    frame->ip = ip;
                    return IRON_INTERP_ERROR;
                }
            }
            break;
            
        case IRON_CEE_LDELEMA:
            /* Load element address - type token follows */
            {
                iron_u32 type_token = iron_read_u32_le(code + ip);
                iron_stack_value_t index_val = iron_stack_pop(stack);
                iron_stack_value_t arr_val = iron_stack_pop(stack);
                iron_i32 index = index_val.value.i32;
                void *arr = (arr_val.type == IRON_VAL_OBJ) ? arr_val.value.obj : arr_val.value.ptr;
                iron_u32 arr_len;
                
                (void)type_token;
                ip += 4;
                
                if (!arr) {
                    iron_throw_null_reference(thread);
                    frame->ip = ip;
                    return IRON_INTERP_EXCEPTION;
                }
                
                arr_len = iron_array_get_length(arr);
                if (index < 0 || (iron_u32)index >= arr_len) {
                    iron_throw_index_out_of_range(thread);
                    frame->ip = ip;
                    return IRON_INTERP_EXCEPTION;
                }
                
                /* Push address of element (pointer-sized elements) */
                {
                    iron_u8 *elements = (iron_u8 *)iron_array_get_data(arr);
                    iron_stack_value_t addr_val;
                    memset(&addr_val, 0, sizeof(addr_val));
                    addr_val.type = IRON_VAL_PTR;
                    addr_val.value.ptr = elements + (iron_size)index * iron_array_get_element_size(arr);
                    iron_stack_push(stack, addr_val);
                }
            }
            break;
            
        case IRON_CEE_LDELEM:
            {
                iron_u32 type_token = iron_read_u32_le(code + ip);
                iron_stack_value_t index_val = iron_stack_pop(stack);
                iron_stack_value_t arr_val = iron_stack_pop(stack);
                iron_i32 index = index_val.value.i32;
                void *arr = (arr_val.type == IRON_VAL_OBJ) ? arr_val.value.obj : arr_val.value.ptr;
                iron_u32 arr_len;
                iron_runtime_type_t *element_type;
                void *element_storage;
                
                ip += 4;
                
                if (!arr) {
                    iron_throw_null_reference(thread);
                    frame->ip = ip;
                    return IRON_INTERP_EXCEPTION;
                }
                
                arr_len = iron_array_get_length(arr);
                if (index < 0 || (iron_u32)index >= arr_len) {
                    iron_throw_index_out_of_range(thread);
                    frame->ip = ip;
                    return IRON_INTERP_EXCEPTION;
                }
                
                element_type = iron_array_get_element_type(arr);
                element_storage = (iron_u8 *)iron_array_get_data(arr) + (iron_size)index * iron_array_get_element_size(arr);
                if (!element_type || !push_storage_value(thread->exec_ctx, stack, element_storage, element_type)) {
                    IRON_ERROR_EXEC("Cannot load array element for type token 0x%08X", type_token);
                    frame->ip = ip;
                    return IRON_INTERP_ERROR;
                }
            }
            break;
            
        case IRON_CEE_LDLEN:
            {
                void *arr = iron_stack_pop_obj(stack);
                
                if (!arr) {
                    iron_throw_null_reference(thread);
                    frame->ip = ip;
                    return IRON_INTERP_EXCEPTION;
                }
                
                iron_stack_push_i32(stack, (iron_i32)iron_array_get_length(arr));
            }
            break;
        
        /* Field access */
        case IRON_CEE_LDSFLD:
        case IRON_CEE_LDSFLDA:
            {
                iron_u32 field_token;
                iron_assembly_t *assembly;
                iron_runtime_field_t *field;
                iron_result_t init_result;
                void *field_address;

                field_token = iron_read_u32_le(code + ip);
                ip += 4;
                assembly = get_frame_assembly(frame);
                field = resolve_runtime_field(thread, assembly, field_token);
                if (!field || !field->declaring_type || (field->attrs & 0x0010) == 0) {
                    IRON_ERROR_EXEC("Cannot resolve static field token 0x%08X", field_token);
                    frame->ip = ip;
                    return IRON_INTERP_ERROR;
                }

                init_result = iron_type_init_static(field->declaring_type, thread->exec_ctx);
                if (!IRON_RESULT_OK(init_result)) {
                    frame->ip = ip;
                    return IRON_INTERP_EXCEPTION;
                }

                field_address = iron_field_get_address(field, NULL);
                if (!field_address) {
                    IRON_ERROR_EXEC("Static field token 0x%08X has no storage", field_token);
                    frame->ip = ip;
                    return IRON_INTERP_ERROR;
                }

                if (opcode == IRON_CEE_LDSFLDA) {
                    iron_stack_push_ptr(stack, field_address);
                } else {
                    if (!field->field_type || !push_storage_value(thread->exec_ctx, stack, field_address, field->field_type)) {
                        IRON_ERROR_EXEC("Failed to load static field token 0x%08X", field_token);
                        frame->ip = ip;
                        return IRON_INTERP_ERROR;
                    }
                }
            }
            break;

        case IRON_CEE_STSFLD:
            {
                iron_u32 field_token;
                iron_assembly_t *assembly;
                iron_runtime_field_t *field;
                iron_result_t init_result;
                iron_stack_value_t value;
                void *field_address;

                field_token = iron_read_u32_le(code + ip);
                ip += 4;
                value = iron_stack_pop(stack);
                assembly = get_frame_assembly(frame);
                field = resolve_runtime_field(thread, assembly, field_token);
                if (!field || !field->declaring_type || (field->attrs & 0x0010) == 0) {
                    IRON_ERROR_EXEC("Cannot resolve static field token 0x%08X", field_token);
                    frame->ip = ip;
                    return IRON_INTERP_ERROR;
                }

                init_result = iron_type_init_static(field->declaring_type, thread->exec_ctx);
                if (!IRON_RESULT_OK(init_result)) {
                    frame->ip = ip;
                    return IRON_INTERP_EXCEPTION;
                }

                field_address = iron_field_get_address(field, NULL);
                if (!field_address) {
                    IRON_ERROR_EXEC("Static field token 0x%08X has no storage", field_token);
                    frame->ip = ip;
                    return IRON_INTERP_ERROR;
                }

                if ((iron_size)field->offset > field->declaring_type->static_data_size || iron_type_storage_size(field->field_type) > field->declaring_type->static_data_size - field->offset) {
                    IRON_ERROR_EXEC("Static field storage is too small: %s::%s offset=%u field_size=%zu storage_size=%u",
                                    field->declaring_type->full_name ? field->declaring_type->full_name : "<unknown type>",
                                    field->name ? field->name : "<unknown field>",
                                    field->offset,
                                    iron_type_storage_size(field->field_type),
                                    field->declaring_type->static_data_size);
                    frame->ip = ip;
                    return IRON_INTERP_ERROR;
                }

                if (!field->field_type || !store_storage_value(NULL, field_address, field->field_type, &value)) {
                    IRON_ERROR_EXEC("Failed to store static field token 0x%08X", field_token);
                    frame->ip = ip;
                    return IRON_INTERP_ERROR;
                }
            }
            break;

        case IRON_CEE_LDFLD:
            {
                iron_u32 field_token = iron_read_u32_le(code + ip);
                iron_stack_value_t obj_val;
                void *obj;
                iron_assembly_t *assembly;
                iron_runtime_field_t *field;
                void *field_address;
                
                ip += 4;
                
                obj_val = iron_stack_pop(stack);
                obj = (obj_val.type == IRON_VAL_PTR) ? obj_val.value.ptr : obj_val.value.obj;
                
                if (!obj) {
                    iron_throw_null_reference(thread);
                    frame->ip = ip;
                    return IRON_INTERP_EXCEPTION;
                }

                assembly = get_frame_assembly(frame);
                field = resolve_runtime_field(thread, assembly, field_token);
                if (!field) {
                    IRON_ERROR_EXEC("Cannot resolve field token 0x%08X", field_token);
                    frame->ip = ip;
                    return IRON_INTERP_ERROR;
                }
                field = bind_runtime_field(obj, obj_val.type, field);

                if (!instance_field_fits_object(obj, obj_val.type, field)) {
                    frame->ip = ip;
                    return IRON_INTERP_ERROR;
                }

                field_address = (iron_u8 *)obj + field->offset;
                IRON_TRACE_EXEC("ldfld: token=0x%08X field_type=0x%02X offset=%u obj=%p", field_token, field->element_type, field->offset, obj);
                if (!field->field_type || !push_storage_value(thread->exec_ctx, stack, field_address, field->field_type)) {
                    IRON_ERROR_EXEC("Failed to load field token 0x%08X", field_token);
                    frame->ip = ip;
                    return IRON_INTERP_ERROR;
                }
            }
            break;
            
        case IRON_CEE_LDFLDA:
            /* Load field address */
            {
                iron_u32 field_token = iron_read_u32_le(code + ip);
                iron_stack_value_t obj_val;
                void *obj;
                iron_assembly_t *assembly;
                iron_runtime_field_t *field;
                
                ip += 4;
                
                obj_val = iron_stack_pop(stack);
                obj = (obj_val.type == IRON_VAL_PTR) ? obj_val.value.ptr : obj_val.value.obj;
                
                if (!obj) {
                    iron_throw_null_reference(thread);
                    frame->ip = ip;
                    return IRON_INTERP_EXCEPTION;
                }
                
                assembly = get_frame_assembly(frame);
                field = resolve_runtime_field(thread, assembly, field_token);
                if (!field) {
                    IRON_ERROR_EXEC("Cannot resolve field token 0x%08X", field_token);
                    frame->ip = ip;
                    return IRON_INTERP_ERROR;
                }
                field = bind_runtime_field(obj, obj_val.type, field);

                if (!instance_field_fits_object(obj, obj_val.type, field)) {
                    frame->ip = ip;
                    return IRON_INTERP_ERROR;
                }
                
                {
                    iron_stack_value_t addr_val;
                    memset(&addr_val, 0, sizeof(addr_val));
                    addr_val.type = IRON_VAL_PTR;
                    addr_val.value.ptr = (iron_u8 *)obj + field->offset;
                    iron_stack_push(stack, addr_val);
                }
            }
            break;
            
        case IRON_CEE_STFLD:
            {
                iron_u32 field_token = iron_read_u32_le(code + ip);
                iron_stack_value_t value_val;
                iron_stack_value_t obj_val;
                void *obj;
                iron_assembly_t *assembly;
                iron_runtime_field_t *field;
                void *field_address;
                
                ip += 4;
                
                /* Pop value first, then object */
                value_val = iron_stack_pop(stack);
                obj_val = iron_stack_pop(stack);
                obj = (obj_val.type == IRON_VAL_PTR) ? obj_val.value.ptr : obj_val.value.obj;
                
                if (!obj) {
                    iron_throw_null_reference(thread);
                    frame->ip = ip;
                    return IRON_INTERP_EXCEPTION;
                }
                
                assembly = get_frame_assembly(frame);
                field = resolve_runtime_field(thread, assembly, field_token);
                if (!field) {
                    IRON_ERROR_EXEC("Cannot resolve field token 0x%08X", field_token);
                    frame->ip = ip;
                    return IRON_INTERP_ERROR;
                }
                field = bind_runtime_field(obj, obj_val.type, field);

                if (!instance_field_fits_object(obj, obj_val.type, field)) {
                    frame->ip = ip;
                    return IRON_INTERP_ERROR;
                }

                field_address = (iron_u8 *)obj + field->offset;
                IRON_TRACE_EXEC("stfld: token=0x%08X field_type=0x%02X offset=%u value=%d obj=%p", field_token, field->element_type, field->offset, value_val.value.i32, obj);
                if (!field->field_type || !store_storage_value(obj_val.type == IRON_VAL_OBJ ? obj : NULL, field_address, field->field_type, &value_val)) {
                    iron_runtime_type_t *value_type;

                    value_type = (value_val.type == IRON_VAL_OBJ || value_val.type == IRON_VAL_VALUETYPE) && value_val.value.obj
                                     ? iron_managed_reference_get_type(thread->exec_ctx->domain, value_val.value.obj)
                                     : NULL;
                    IRON_ERROR_EXEC("Failed to store field token 0x%08X (%s::%s, field_type=%s, value_kind=%d, value_type=%s)",
                                    field_token,
                                    field->declaring_type && field->declaring_type->full_name ? field->declaring_type->full_name : "<unknown type>",
                                    field->name ? field->name : "<unknown field>",
                                    field->field_type && field->field_type->full_name ? field->field_type->full_name : "<unknown type>",
                                    value_val.type,
                                    value_type && value_type->full_name ? value_type->full_name : "<not an object>");
                    frame->ip = ip;
                    return IRON_INTERP_ERROR;
                }
            }
            break;
            
        /* ============================================================
         * Exception Handling Opcodes
         * ============================================================ */
        case IRON_CEE_THROW:
        {
            iron_stack_value_t ex_val;
            iron_exception_t *ex_obj;

            ex_val = iron_stack_pop(stack);
            ex_obj = (iron_exception_t *)ex_val.value.obj;

            if (!ex_obj) {
                iron_throw_null_reference(thread);
            } else {
                iron_throw(thread, ex_obj);
            }

            frame->ip = ip;
            return IRON_INTERP_EXCEPTION;
        }

        case IRON_CEE_RETHROW:
        {
            /* Re-throw the current exception */
            if (thread->exception_state.current_exception) {
                thread->exception_state.is_rethrow = IRON_TRUE;
            }
            frame->ip = ip;
            return IRON_INTERP_EXCEPTION;
        }

        case IRON_CEE_LEAVE:
        {
            iron_i32 offset;
            iron_u32 target;

            offset = (iron_i32)((iron_i16)(code[ip] | (code[ip + 1] << 8) |
                     (code[ip + 2] << 16) | (code[ip + 3] << 24)));
            ip += 4;
            target = (iron_u32)((iron_i32)ip + offset);

            /* Clear the evaluation stack back to frame's stack base */
            stack->size = frame->stack_base;

            /* Clear any pending exception (leave exits protected regions) */
            thread->exception_state.current_exception = NULL;

            /* Execute any finally blocks that cover the current IP but not the target */
            if (frame->method && frame->method->body) {
                iron_method_body_t *body = frame->method->body;
                iron_u32 leave_ip = ip - 4 - 1; /* IP of the leave instruction */
                iron_u32 finally_index;

                frame->leave_target = target;
                frame->leave_search_offset = leave_ip;
                frame->leave_finally_try_length = 0;
                if (find_next_leave_finally(body, leave_ip, target, 0, &finally_index)) {
                    iron_exception_clause_t *clause;

                    clause = &body->exceptions[finally_index];
                    frame->exception_handler_index = finally_index;
                    frame->leave_finally_try_length = clause->try_length;
                    frame->flags |= IRON_FRAME_FINALLY;
                    frame->ip = clause->handler_offset;
                    return IRON_INTERP_OK;
                }
            }

            /* No finally blocks to execute - jump directly to target */
            ip = target;
            break;
        }

        case IRON_CEE_LEAVE_S:
        {
            iron_i8 offset;
            iron_u32 target;

            offset = (iron_i8)code[ip];
            ip += 1;
            target = (iron_u32)((iron_i32)ip + (iron_i32)offset);

            /* Clear the evaluation stack back to frame's stack base */
            stack->size = frame->stack_base;

            /* Clear any pending exception */
            thread->exception_state.current_exception = NULL;

            /* Execute any finally blocks */
            if (frame->method && frame->method->body) {
                iron_method_body_t *body = frame->method->body;
                iron_u32 leave_ip = ip - 1 - 1; /* IP of the leave.s instruction */
                iron_u32 finally_index;

                frame->leave_target = target;
                frame->leave_search_offset = leave_ip;
                frame->leave_finally_try_length = 0;
                if (find_next_leave_finally(body, leave_ip, target, 0, &finally_index)) {
                    iron_exception_clause_t *clause;

                    clause = &body->exceptions[finally_index];
                    frame->exception_handler_index = finally_index;
                    frame->leave_finally_try_length = clause->try_length;
                    frame->flags |= IRON_FRAME_FINALLY;
                    frame->ip = clause->handler_offset;
                    return IRON_INTERP_OK;
                }
            }

            ip = target;
            break;
        }

        case IRON_CEE_ENDFINALLY:
        {
            frame->flags &= ~IRON_FRAME_FINALLY;

            /* Check if there's a pending exception to continue unwinding */
            if (thread->exception_state.current_exception) {
                /* Continue exception unwinding */
                frame->ip = ip;
                return IRON_INTERP_EXCEPTION;
            }

            if (frame->method && frame->method->body) {
                iron_u32 finally_index;

                if (find_next_leave_finally(frame->method->body,
                                            frame->leave_search_offset,
                                            frame->leave_target,
                                            frame->leave_finally_try_length,
                                            &finally_index)) {
                    iron_exception_clause_t *clause;

                    clause = &frame->method->body->exceptions[finally_index];
                    frame->exception_handler_index = finally_index;
                    frame->leave_finally_try_length = clause->try_length;
                    frame->flags |= IRON_FRAME_FINALLY;
                    ip = clause->handler_offset;
                    break;
                }
            }

            ip = frame->leave_target;
            break;
        }

        case IRON_CEE_ENDFILTER:
        {
            iron_exception_clause_t *clause;
            iron_stack_value_t exception_value;
            iron_stack_value_t filter_result;

            if ((frame->flags & IRON_FRAME_FILTER) == 0 || !frame->method || !frame->method->body ||
                frame->exception_handler_index >= frame->method->body->exception_count) {
                IRON_ERROR_EXEC("endfilter executed without an active exception filter");
                frame->ip = ip;
                return IRON_INTERP_ERROR;
            }
            if (stack->size <= frame->stack_base) {
                IRON_ERROR_EXEC("Exception filter completed without a result value");
                frame->ip = ip;
                return IRON_INTERP_ERROR;
            }

            filter_result = iron_stack_pop(stack);
            if (filter_result.type != IRON_VAL_I32) {
                IRON_ERROR_EXEC("Exception filter result must be an Int32 value");
                frame->ip = ip;
                return IRON_INTERP_ERROR;
            }

            clause = &frame->method->body->exceptions[frame->exception_handler_index];
            thread->eval_stack.size = frame->stack_base;
            if (frame->filter_exception) {
                thread->exception_state.current_exception = frame->filter_exception;
                frame->filter_exception = NULL;
            }
            frame->flags &= ~IRON_FRAME_FILTER;
            if (filter_result.value.i32 != 0) {
                memset(&exception_value, 0, sizeof(exception_value));
                exception_value.type = IRON_VAL_OBJ;
                exception_value.value.obj = thread->exception_state.current_exception;
                iron_stack_push(stack, exception_value);
                frame->flags |= IRON_FRAME_EXCEPTION;
                ip = clause->handler_offset;
                break;
            }

            frame->flags |= IRON_FRAME_FILTER_REJECTED;
            frame->ip = ip;
            return IRON_INTERP_EXCEPTION;
        }

        default:
            /* Unimplemented opcode */
            IRON_ERROR_EXEC("Unimplemented opcode: 0x%04X (%s) at IP=%u",
                           opcode,
                           opcode_info ? opcode_info->name : "unknown",
                           frame->ip);
            frame->ip = ip;
            return IRON_INTERP_ERROR;
    }
    
    if (!iron_opcode_is_prefix(opcode)) {
        frame->unaligned_prefix = 0;
        frame->volatile_prefix = IRON_FALSE;
        frame->tail_prefix = IRON_FALSE;
        frame->readonly_prefix = IRON_FALSE;
        frame->flags &= ~IRON_FRAME_TAIL_CALL;
        frame->flags &= ~IRON_FRAME_CONSTRAINED;
        frame->constrained_type = NULL;
    }

    frame->ip = ip;
    return IRON_INTERP_OK;
}

/* ============================================================================
 * Exception Throwing
 * ============================================================================ */

void iron_throw(iron_thread_context_t *thread, iron_exception_t *ex)
{
    if (!thread) return;
    thread->exception_state.current_exception = ex;
    thread->exception_state.throw_frame = thread->current_frame;
    thread->exception_state.throw_ip = thread->current_frame ? 
                                        thread->current_frame->ip : 0;
    thread->exception_state.is_rethrow = IRON_FALSE;
}

static iron_exception_t *create_managed_exception(iron_exec_context_t *ctx, const char *type_name, const char *message)
{
    iron_runtime_type_t *type;
    iron_exception_t *exception;
    iron_gc_handle_t *root;
    iron_size payload_size;

    if (!ctx || !ctx->domain || !type_name) {
        return NULL;
    }

    type = iron_domain_find_type(ctx->domain, type_name);
    if (!type) {
        type = iron_domain_find_type(ctx->domain, "System.Exception");
    }
    if (!type) {
        return NULL;
    }

    payload_size = type->instance_size;
    if (payload_size < sizeof(iron_exception_t)) {
        payload_size = sizeof(iron_exception_t);
    }
    exception = (iron_exception_t *)iron_gc_alloc_object(&ctx->gc, type, payload_size);
    if (!exception) {
        return NULL;
    }

    root = iron_gc_handle_alloc(&ctx->gc, exception, IRON_GC_HANDLE_NORMAL);
    if (message && root) {
        void *managed_message;

        managed_message = iron_gc_alloc_string_utf8(ctx, message);
        if (managed_message) {
            iron_gc_write_barrier(exception, &exception->message, managed_message);
        }
    }
    if (root) {
        iron_gc_handle_free(&ctx->gc, root);
    }
    return exception;
}

void iron_throw_null_reference(iron_thread_context_t *thread)
{
    if (!thread || !thread->exec_ctx) {
        iron_throw(thread, NULL);
        return;
    }

    iron_throw(thread, create_managed_exception(thread->exec_ctx, "System.NullReferenceException", "Object reference not set to an instance of an object."));
}

void iron_throw_index_out_of_range(iron_thread_context_t *thread)
{
    if (!thread || !thread->exec_ctx) {
        iron_throw(thread, NULL);
        return;
    }

    iron_throw(thread, create_managed_exception(thread->exec_ctx, "System.IndexOutOfRangeException", "Index was outside the bounds of the array."));
}

void iron_throw_invalid_cast(iron_thread_context_t *thread)
{
    if (!thread || !thread->exec_ctx) {
        iron_throw(thread, NULL);
        return;
    }

    iron_throw(thread, create_managed_exception(thread->exec_ctx, "System.InvalidCastException", "Specified cast is not valid."));
}

void iron_throw_overflow(iron_thread_context_t *thread)
{
    if (!thread || !thread->exec_ctx) {
        iron_throw(thread, NULL);
        return;
    }

    iron_throw(thread, create_managed_exception(thread->exec_ctx, "System.OverflowException", "Arithmetic operation resulted in an overflow."));
}

void iron_throw_arithmetic(iron_thread_context_t *thread)
{
    if (!thread || !thread->exec_ctx) {
        iron_throw(thread, NULL);
        return;
    }

    iron_throw(thread, create_managed_exception(thread->exec_ctx, "System.ArithmeticException", "Floating-point value is not finite."));
}

void iron_throw_divide_by_zero(iron_thread_context_t *thread)
{
    if (!thread || !thread->exec_ctx) {
        iron_throw(thread, NULL);
        return;
    }

    iron_throw(thread, create_managed_exception(thread->exec_ctx, "System.DivideByZeroException", "Attempted to divide by zero."));
}

static iron_result_t convert_managed_call_error(iron_thread_context_t *thread, iron_result_t result)
{
    const char *exception_type;
    iron_exception_t *exception;

    if (IRON_RESULT_OK(result) || result.error == IRON_ERR_EXCEPTION) {
        return result;
    }

    exception_type = NULL;
    switch (result.error) {
        case IRON_ERR_NULL_REFERENCE:
            exception_type = "System.NullReferenceException";
            break;
        case IRON_ERR_INDEX_OUT_OF_RANGE:
            exception_type = "System.IndexOutOfRangeException";
            break;
        case IRON_ERR_INVALID_CAST:
            exception_type = "System.InvalidCastException";
            break;
        case IRON_ERR_DIVIDE_BY_ZERO:
            exception_type = "System.DivideByZeroException";
            break;
        case IRON_ERR_ARITHMETIC_OVERFLOW:
        case IRON_ERR_BUFFER_OVERFLOW:
            exception_type = "System.OverflowException";
            break;
        case IRON_ERR_OUT_OF_MEMORY:
            exception_type = "System.OutOfMemoryException";
            break;
        case IRON_ERR_INVALID_ARGUMENT:
        case IRON_ERR_INVALID_GENERIC_ARGS:
        case IRON_ERR_CONSTRAINT_VIOLATION:
            exception_type = "System.ArgumentException";
            break;
        case IRON_ERR_ARGUMENT_NULL:
            exception_type = "System.ArgumentNullException";
            break;
        case IRON_ERR_ARGUMENT_OUT_OF_RANGE:
            exception_type = "System.ArgumentOutOfRangeException";
            break;
        case IRON_ERR_INVALID_STATE:
            exception_type = "System.InvalidOperationException";
            break;
        case IRON_ERR_NOT_IMPLEMENTED:
            exception_type = "System.NotImplementedException";
            break;
        default:
            return result;
    }

    if (!thread || !thread->exec_ctx) {
        return result;
    }

    exception = create_managed_exception(thread->exec_ctx, exception_type, result.message ? result.message : iron_error_str(result.error));
    if (!exception) {
        return result;
    }

    iron_throw(thread, exception);
    return IRON_ERROR(IRON_ERR_EXCEPTION, "Managed call threw an exception");
}

/* ============================================================================
 * Exception Handler Lookup
 * ============================================================================ */

static iron_bool exception_matches_catch(iron_stack_frame_t *frame, iron_exception_t *exception, iron_token_t catch_type_token)
{
    iron_runtime_type_t *catch_type;
    iron_runtime_type_t *exception_type;

    if (!frame || !frame->method || !frame->method->declaring_type || !frame->method->declaring_type->module || !exception) {
        return IRON_FALSE;
    }

    catch_type = iron_type_resolve_token(frame->method->declaring_type->module, catch_type_token);
    if (!catch_type) {
        IRON_ERROR_EXEC("Cannot resolve exception handler type token 0x%08X", catch_type_token);
        return IRON_FALSE;
    }

    exception_type = IRON_GC_HEADER(exception)->type;
    return exception_type && iron_type_is_assignable_to(exception_type, catch_type);
}

iron_bool iron_find_exception_handler(iron_thread_context_t *thread,
                                      iron_exception_t *ex,
                                      iron_stack_frame_t **out_frame,
                                      iron_u32 *out_handler_index)
{
    iron_stack_frame_t *frame;

    if (!thread || !out_frame || !out_handler_index) {
        return IRON_FALSE;
    }

    /* Walk up the call stack looking for a handler */
    for (frame = thread->current_frame; frame != NULL; frame = frame->prev) {
        iron_method_body_t *body;
        iron_u32 best_handler_index;
        iron_u32 best_try_length;
        iron_u32 throw_ip;
        iron_u32 i;
        iron_bool resume_after_filter;

        if (!frame->method || !frame->method->body) {
            continue;
        }

        body = frame->method->body;
        throw_ip = frame->ip;
        best_handler_index = UINT32_MAX;
        best_try_length = UINT32_MAX;
        resume_after_filter = (frame->flags & IRON_FRAME_FILTER_REJECTED) != 0;

        /* Select the innermost applicable protected region. Metadata keeps
         * sibling catches in source order, so equal-sized regions retain the
         * first matching clause. */
        for (i = 0; i < body->exception_count; i++) {
            iron_exception_clause_t *clause = &body->exceptions[i];
            iron_u32 clause_kind;
            iron_u32 try_end;
            iron_bool handles_exception;

            if (clause->try_length > UINT32_MAX - clause->try_offset) {
                continue;
            }
            try_end = clause->try_offset + clause->try_length;

            /* Check if the throw IP is within this try block */
            if (throw_ip < clause->try_offset || throw_ip >= try_end) {
                continue;
            }
            if (resume_after_filter &&
                (clause->try_length < frame->exception_filter_try_length ||
                 (clause->try_length == frame->exception_filter_try_length && i <= frame->exception_handler_index))) {
                continue;
            }

            clause_kind = clause->flags & (IRON_EX_CLAUSE_FILTER | IRON_EX_CLAUSE_FINALLY | IRON_EX_CLAUSE_FAULT);
            handles_exception = IRON_FALSE;
            if (clause_kind == IRON_EX_CLAUSE_EXCEPTION) {
                handles_exception = exception_matches_catch(frame, ex, clause->u.class_token);
            } else if (clause_kind == IRON_EX_CLAUSE_FILTER || clause_kind == IRON_EX_CLAUSE_FINALLY || clause_kind == IRON_EX_CLAUSE_FAULT) {
                handles_exception = IRON_TRUE;
            }

            if (handles_exception &&
                (clause->try_length < best_try_length || (clause->try_length == best_try_length && i < best_handler_index))) {
                best_handler_index = i;
                best_try_length = clause->try_length;
            }
        }

        if (best_handler_index != UINT32_MAX) {
            *out_frame = frame;
            *out_handler_index = best_handler_index;
            return IRON_TRUE;
        }
    }

    return IRON_FALSE;
}

/* ============================================================================
 * Entry Point Execution
 * ============================================================================ */

iron_result_t iron_exec_entry_point(iron_exec_context_t *ctx,
                                    iron_runtime_method_t *entry_point,
                                    const char **args,
                                    int argc,
                                    int *exit_code)
{
    iron_stack_value_t method_args[2];
    iron_stack_value_t ret_val;
    iron_result_t result;
    
    if (!ctx || !entry_point) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Invalid context or entry point");
    }

    ctx->entry_assembly = entry_point->declaring_type && entry_point->declaring_type->module ? entry_point->declaring_type->module->assembly : NULL;
    
    if (exit_code) {
        *exit_code = 0;
    }
    
    memset(&method_args[0], 0, sizeof(method_args));
    memset(&ret_val, 0, sizeof(ret_val));
    
    /* Check if Main takes string[] args */
    if (entry_point->param_count > 0) {
        /* Create string[] array from args */
        void *string_array = NULL;
        iron_runtime_type_t *string_type;

        string_type = iron_domain_find_type(ctx->domain, "System.String");
        if (!string_type) {
            return IRON_ERROR(IRON_ERR_TYPE_NOT_FOUND, "System.String is not loaded");
        }
        
        if (args && argc > 0) {
            string_array = iron_gc_alloc_array(ctx, string_type, (iron_u32)argc);
            
            if (string_array) {
                int i;
                void **str_ptrs;
                
                str_ptrs = (void **)iron_array_get_data(string_array);
                
                /* Create string objects for each argument */
                for (i = 0; i < argc; i++) {
                    if (args[i]) {
                        void *str_obj;

                        str_obj = iron_gc_alloc_string_utf8(ctx, args[i]);

                        if (!str_obj) {
                            return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate entry-point argument");
                        }

                        str_ptrs[i] = str_obj;
                        iron_gc_write_barrier(string_array, &str_ptrs[i], str_obj);
                    } else {
                        str_ptrs[i] = NULL;
                    }
                }
            }
        } else {
            /* Create empty string array */
            string_array = iron_gc_alloc_array(ctx, string_type, 0);
        }

        if (!string_array) {
            return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate entry-point argument array");
        }

        method_args[0].type = IRON_VAL_OBJ;
        method_args[0].value.obj = string_array;

        result = iron_exec_method(ctx, entry_point, method_args, 1, &ret_val);
    } else {
        /* Main() with no args */
        result = iron_exec_method(ctx, entry_point, NULL, 0, &ret_val);
    }

    if (IRON_RESULT_OK(result)) {
        /* Check if Main returns int */
        if (ret_val.type == IRON_VAL_I32 && exit_code) {
            *exit_code = ret_val.value.i32;
        }
    }

    return result;
}

/* ============================================================================
 * Method Resolution and Calling Helpers
 * ============================================================================ */

/* Get parameter count from method signature blob */
static iron_u32 get_method_param_count_from_sig(const iron_u8 *sig_data, iron_u32 sig_size)
{
    iron_sig_reader_t reader;
    iron_call_conv_t calling_convention;
    iron_u32 generic_param_count;
    iron_u32 param_count;

    if (!sig_data || sig_size == 0) {
        return 0;
    }

    iron_sig_init(&reader, sig_data, sig_size);
    if (!IRON_RESULT_OK(iron_sig_read_method_header(&reader, &calling_convention, &generic_param_count, &param_count))) {
        return 0;
    }

    (void)calling_convention;
    (void)generic_param_count;
    return param_count;
}

static iron_bool signature_append_text(char *output, iron_u32 output_size, iron_u32 *written, const char *text)
{
    if (!output || !written || !text || output_size == 0) {
        return IRON_FALSE;
    }

    while (*text) {
        if (*written >= output_size - 1) {
            output[output_size - 1] = '\0';
            return IRON_FALSE;
        }
        output[(*written)++] = *text++;
    }
    output[*written] = '\0';
    return IRON_TRUE;
}

static iron_bool signature_append_type(const iron_metadata_t *metadata,
                                       iron_sig_reader_t *reader,
                                       char *output,
                                       iron_u32 output_size,
                                       iron_u32 *written,
                                       iron_u32 depth)
{
    iron_element_type_t element_type;
    const char *managed_name;
    iron_result_t parse_result;

    if (depth >= 64 || !reader) {
        return IRON_FALSE;
    }

    parse_result = iron_sig_read_element_type(reader, &element_type);
    if (!IRON_RESULT_OK(parse_result)) {
        return IRON_FALSE;
    }

    if (element_type == IRON_TYPE_CMOD_OPT || element_type == IRON_TYPE_CMOD_REQD) {
        iron_token_t modifier_token;

        if (!IRON_RESULT_OK(iron_sig_read_type_def_or_ref(reader, &modifier_token))) {
            return IRON_FALSE;
        }
        return signature_append_type(metadata, reader, output, output_size, written, depth + 1);
    }

    if (element_type == IRON_TYPE_PINNED || element_type == IRON_TYPE_SENTINEL) {
        return signature_append_type(metadata, reader, output, output_size, written, depth + 1);
    }

    managed_name = iron_element_type_managed_name(element_type);
    if (managed_name) {
        return signature_append_text(output, output_size, written, managed_name);
    }

    switch (element_type) {
        case IRON_TYPE_CLASS:
        case IRON_TYPE_VALUETYPE: {
            iron_token_t token;
            const char *namespace_name;
            const char *type_name;

            if (!IRON_RESULT_OK(iron_sig_read_type_def_or_ref(reader, &token)) ||
                !IRON_RESULT_OK(iron_metadata_get_type_name(metadata, token, &namespace_name, &type_name))) {
                return IRON_FALSE;
            }
            if (namespace_name && namespace_name[0] &&
                (!signature_append_text(output, output_size, written, namespace_name) || !signature_append_text(output, output_size, written, "."))) {
                return IRON_FALSE;
            }
            return signature_append_text(output, output_size, written, type_name);
        }
        case IRON_TYPE_PTR:
            return signature_append_type(metadata, reader, output, output_size, written, depth + 1) &&
                   signature_append_text(output, output_size, written, "*");
        case IRON_TYPE_BYREF:
            return signature_append_type(metadata, reader, output, output_size, written, depth + 1) &&
                   signature_append_text(output, output_size, written, "&");
        case IRON_TYPE_SZARRAY:
            return signature_append_type(metadata, reader, output, output_size, written, depth + 1) &&
                   signature_append_text(output, output_size, written, "[]");
        case IRON_TYPE_VAR:
        case IRON_TYPE_MVAR: {
            iron_u32 generic_index;
            char generic_name[32];

            if (!IRON_RESULT_OK(iron_sig_read_compressed_u32(reader, &generic_index))) {
                return IRON_FALSE;
            }
            snprintf(generic_name, sizeof(generic_name), element_type == IRON_TYPE_VAR ? "!%u" : "!!%u", generic_index);
            return signature_append_text(output, output_size, written, generic_name);
        }
        case IRON_TYPE_GENERICINST: {
            iron_element_type_t definition_kind;
            iron_token_t definition_token;
            const char *namespace_name;
            const char *type_name;
            iron_u32 argument_count;
            iron_u32 argument_index;

            if (!IRON_RESULT_OK(iron_sig_read_element_type(reader, &definition_kind)) ||
                (definition_kind != IRON_TYPE_CLASS && definition_kind != IRON_TYPE_VALUETYPE) ||
                !IRON_RESULT_OK(iron_sig_read_type_def_or_ref(reader, &definition_token)) ||
                !IRON_RESULT_OK(iron_metadata_get_type_name(metadata, definition_token, &namespace_name, &type_name)) ||
                !IRON_RESULT_OK(iron_sig_read_compressed_u32(reader, &argument_count))) {
                return IRON_FALSE;
            }
            if (namespace_name && namespace_name[0] &&
                (!signature_append_text(output, output_size, written, namespace_name) || !signature_append_text(output, output_size, written, "."))) {
                return IRON_FALSE;
            }
            if (!signature_append_text(output, output_size, written, type_name) || !signature_append_text(output, output_size, written, "<")) {
                return IRON_FALSE;
            }
            for (argument_index = 0; argument_index < argument_count; argument_index++) {
                if ((argument_index > 0 && !signature_append_text(output, output_size, written, ",")) ||
                    !signature_append_type(metadata, reader, output, output_size, written, depth + 1)) {
                    return IRON_FALSE;
                }
            }
            return signature_append_text(output, output_size, written, ">");
        }
        case IRON_TYPE_ARRAY: {
            iron_u32 rank;
            iron_u32 size_count;
            iron_u32 bound_count;
            iron_u32 index;

            if (!signature_append_type(metadata, reader, output, output_size, written, depth + 1) ||
                !IRON_RESULT_OK(iron_sig_read_compressed_u32(reader, &rank)) ||
                !IRON_RESULT_OK(iron_sig_read_compressed_u32(reader, &size_count))) {
                return IRON_FALSE;
            }
            for (index = 0; index < size_count; index++) {
                iron_u32 size;

                if (!IRON_RESULT_OK(iron_sig_read_compressed_u32(reader, &size))) {
                    return IRON_FALSE;
                }
            }
            if (!IRON_RESULT_OK(iron_sig_read_compressed_u32(reader, &bound_count))) {
                return IRON_FALSE;
            }
            for (index = 0; index < bound_count; index++) {
                iron_i32 bound;

                if (!IRON_RESULT_OK(iron_sig_read_compressed_i32(reader, &bound))) {
                    return IRON_FALSE;
                }
            }
            if (!signature_append_text(output, output_size, written, "[")) {
                return IRON_FALSE;
            }
            for (index = 1; index < rank; index++) {
                if (!signature_append_text(output, output_size, written, ",")) {
                    return IRON_FALSE;
                }
            }
            return signature_append_text(output, output_size, written, "]");
        }
        case IRON_TYPE_FNPTR: {
            iron_call_conv_t calling_convention;
            iron_u32 generic_param_count;
            iron_u32 parameter_count;
            iron_u32 parameter_index;

            if (!IRON_RESULT_OK(iron_sig_read_method_header(reader, &calling_convention, &generic_param_count, &parameter_count)) ||
                !signature_append_text(output, output_size, written, "method(") ||
                !signature_append_type(metadata, reader, output, output_size, written, depth + 1)) {
                return IRON_FALSE;
            }
            (void)calling_convention;
            (void)generic_param_count;
            for (parameter_index = 0; parameter_index < parameter_count; parameter_index++) {
                if (!signature_append_text(output, output_size, written, ",") ||
                    !signature_append_type(metadata, reader, output, output_size, written, depth + 1)) {
                    return IRON_FALSE;
                }
            }
            return signature_append_text(output, output_size, written, ")");
        }
        default:
            return IRON_FALSE;
    }
}

/* Build a deterministic internal-call key from an ECMA-335 method signature. */
static void build_signature_string_with_metadata(const iron_metadata_t *metadata,
                                                 const iron_u8 *signature_data,
                                                 iron_u32 signature_size,
                                                 char *output,
                                                 iron_u32 output_size)
{
    iron_sig_reader_t reader;
    iron_call_conv_t calling_convention;
    iron_u32 generic_param_count;
    iron_u32 parameter_count;
    iron_u32 parameter_index;
    iron_u32 written;
    char return_type[384];
    iron_u32 return_written;

    if (!output || output_size == 0) {
        return;
    }
    output[0] = '\0';
    if (!metadata || !signature_data || signature_size == 0) {
        return;
    }

    iron_sig_init(&reader, signature_data, signature_size);
    if (!IRON_RESULT_OK(iron_sig_read_method_header(&reader, &calling_convention, &generic_param_count, &parameter_count))) {
        return;
    }
    (void)calling_convention;
    (void)generic_param_count;

    return_written = 0;
    return_type[0] = '\0';
    if (!signature_append_type(metadata, &reader, return_type, sizeof(return_type), &return_written, 0)) {
        return;
    }

    written = 0;
    for (parameter_index = 0; parameter_index < parameter_count; parameter_index++) {
        if ((parameter_index > 0 && !signature_append_text(output, output_size, &written, ",")) ||
            !signature_append_type(metadata, &reader, output, output_size, &written, 0)) {
            output[0] = '\0';
            return;
        }
    }
}

/* Resolve a method token (MethodDef or MemberRef) to a runtime method */
static iron_result_t resolve_method_token(iron_thread_context_t *thread,
                                          iron_u32 token,
                                          iron_runtime_method_t **out_method,
                                          iron_assembly_t **out_assembly)
{
    iron_exec_context_t *ctx;
    iron_assembly_t *assembly = NULL;
    iron_u32 table_id;
    iron_u32 row_index;
    
    if (!thread || !out_method) {
        return IRON_ERROR(IRON_ERR_NULL_POINTER, "Invalid parameters");
    }
    
    ctx = thread->exec_ctx;
    *out_method = NULL;
    if (out_assembly) *out_assembly = NULL;
    
    table_id = (token >> 24) & 0xFF;
    row_index = token & 0x00FFFFFF;
    
    if (row_index == 0) {
        return IRON_ERROR(IRON_ERR_INVALID_TOKEN, "Invalid method token");
    }
    
    /* Find the assembly containing this method */
    if (thread->current_frame && thread->current_frame->method &&
        thread->current_frame->method->declaring_type &&
        thread->current_frame->method->declaring_type->module) {
        assembly = thread->current_frame->method->declaring_type->module->assembly;
    }
    
    if (!assembly && ctx->domain && ctx->domain->assembly_count > 0) {
        /* Use the last loaded assembly (usually the main exe) */
        assembly = ctx->domain->assemblies[ctx->domain->assembly_count - 1];
    }
    
    if (!assembly) {
        return IRON_ERROR(IRON_ERR_NOT_FOUND, "No assembly context");
    }
    
    if (out_assembly) *out_assembly = assembly;
    
    if (table_id == IRON_TABLE_METHOD_DEF) {
        /* Direct method reference in same assembly */
        *out_method = iron_resolve_method_token(assembly, token);
        if (*out_method) {
            return IRON_SUCCESS;
        }
        return IRON_ERROR(IRON_ERR_NOT_FOUND, "Method not found");
    }
    else if (table_id == IRON_TABLE_METHOD_SPEC) {
        iron_method_spec_row_t method_spec;
        iron_result_t res;
        iron_u32 base_method_token;
        iron_u32 base_table;
        iron_runtime_method_t *base_method = NULL;
        const iron_u8 *inst_data;
        iron_u32 inst_size;
        iron_sig_reader_t reader;
        iron_u32 generic_arg_count;
        iron_runtime_type_t **generic_args;
        iron_u32 generic_index;

        if (row_index > iron_metadata_table_rows(&assembly->metadata, IRON_TABLE_METHOD_SPEC)) {
            return IRON_ERROR(IRON_ERR_INVALID_TOKEN, "MethodSpec token is outside the metadata table");
        }

        res = iron_metadata_read_row(&assembly->metadata, token, &method_spec);
        if (!IRON_RESULT_OK(res)) {
            return res;
        }
        
        /* Decode the base method (MethodDefOrRef coded index) */
        base_method_token = iron_metadata_decode_coded(&assembly->metadata,
                                                        IRON_CODED_METHOD_DEF_OR_REF,
                                                        method_spec.method);
        base_table = (base_method_token >> 24) & 0xFF;

        if (base_table == IRON_TABLE_METHOD_DEF) {
            base_method = iron_resolve_method_token(assembly, base_method_token);
        } else if (base_table == IRON_TABLE_MEMBER_REF) {
            res = resolve_method_token(thread, base_method_token, &base_method, out_assembly);
            if (!IRON_RESULT_OK(res)) {
                return res;
            }
        }

        if (!base_method) {
            return IRON_ERROR(IRON_ERR_NOT_FOUND, "MethodSpec base method not found");
        }

        res = iron_metadata_get_blob(&assembly->metadata, method_spec.instantiation, &inst_data, &inst_size);
        if (!IRON_RESULT_OK(res) || inst_size < 2 || inst_data[0] != IRON_CALL_GENERIC_INST) {
            return IRON_ERROR(IRON_ERR_INVALID_SIGNATURE, "Invalid MethodSpec instantiation signature");
        }

        iron_sig_init(&reader, inst_data, inst_size);
        reader.pos = 1;
        res = iron_sig_read_compressed_u32(&reader, &generic_arg_count);
        if (!IRON_RESULT_OK(res) || generic_arg_count == 0) {
            return IRON_ERROR(IRON_ERR_INVALID_SIGNATURE, "MethodSpec has no generic arguments");
        }

        generic_args = (iron_runtime_type_t **)iron_alloc(ctx->allocator, generic_arg_count * sizeof(iron_runtime_type_t *));
        if (!generic_args) {
            return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate MethodSpec arguments");
        }

        for (generic_index = 0; generic_index < generic_arg_count; generic_index++) {
            generic_args[generic_index] = resolve_type_signature(thread, assembly, &reader);
            if (!generic_args[generic_index]) {
                iron_free(ctx->allocator, generic_args, generic_arg_count * sizeof(iron_runtime_type_t *));
                return IRON_ERROR(IRON_ERR_INVALID_SIGNATURE, "Cannot resolve a MethodSpec generic argument");
            }

        }

        IRON_DEBUG_EXEC("MethodSpec: token=0x%08X base=0x%08X gen_args=%u", token, base_method_token, generic_arg_count);
        *out_method = iron_method_make_generic(assembly->domain, base_method, generic_args, generic_arg_count);
        iron_free(ctx->allocator, generic_args, generic_arg_count * sizeof(iron_runtime_type_t *));
        if (!*out_method) {
            return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to instantiate generic method");
        }

        return IRON_SUCCESS;
    }
    else if (table_id == IRON_TABLE_MEMBER_REF) {
        /* Cross-assembly method reference */
        iron_member_ref_row_t member_ref;
        iron_runtime_method_t *resolved_definition;
        iron_result_t res;
        const char *method_name;
        const char *type_name = NULL;
        const char *type_namespace = NULL;
        iron_u32 class_token;
        iron_u32 class_table;
        
        res = iron_metadata_read_row(&assembly->metadata, token, &member_ref);
        if (!IRON_RESULT_OK(res)) {
            return res;
        }
        
        method_name = iron_metadata_get_string(&assembly->metadata, member_ref.name);

        /* Decode the class (MemberRefParent coded index) */
        /* MemberRefParent: TypeDef(0), TypeRef(1), ModuleRef(2), MethodDef(3), TypeSpec(4) */
        class_token = iron_metadata_decode_coded(&assembly->metadata, 
                                                  IRON_CODED_MEMBER_REF_PARENT,
                                                  member_ref.class_);
        class_table = (class_token >> 24) & 0xFF;
        
        if (class_table == IRON_TABLE_TYPE_REF) {
            /* Reference to type in another assembly (usually corlib) */
            iron_type_ref_row_t type_ref;
            res = iron_metadata_read_row(&assembly->metadata, class_token, &type_ref);
            if (IRON_RESULT_OK(res)) {
                type_name = iron_metadata_get_string(&assembly->metadata, type_ref.name);
                type_namespace = iron_metadata_get_string(&assembly->metadata, type_ref.namespace_);
            }
        }
        else if (class_table == IRON_TABLE_TYPE_DEF) {
            /* Reference to type in same assembly */
            iron_type_def_row_t type_def;
            res = iron_metadata_read_row(&assembly->metadata, class_token, &type_def);
            if (IRON_RESULT_OK(res)) {
                type_name = iron_metadata_get_string(&assembly->metadata, type_def.name);
                type_namespace = iron_metadata_get_string(&assembly->metadata, type_def.namespace_);
            }
        }
        else if (class_table == IRON_TABLE_TYPE_SPEC) {
            iron_runtime_method_t *definition_method;
            iron_runtime_type_t *constructed_type;

            IRON_DEBUG_EXEC("TypeSpec: class_token=0x%08X method=%s", class_token, method_name);
            constructed_type = resolve_type_spec(thread, assembly, class_token);
            if (constructed_type && constructed_type->kind == IRON_KIND_ARRAY && method_name) {
                const iron_u8 *signature_data;
                iron_u32 signature_size;
                iron_u32 parameter_count;

                signature_data = NULL;
                signature_size = 0;
                if (!IRON_RESULT_OK(iron_metadata_get_blob(&assembly->metadata, member_ref.signature, &signature_data, &signature_size))) {
                    return IRON_ERROR(IRON_ERR_INVALID_SIGNATURE, "Cannot read an array method signature");
                }

                parameter_count = get_method_param_count_from_sig(signature_data, signature_size);
                *out_method = cache_array_method(ctx, assembly, token, method_name, constructed_type, parameter_count);
                if (*out_method) {
                    return IRON_SUCCESS;
                }
                return IRON_ERROR(IRON_ERR_INVALID_SIGNATURE, "Unsupported array method signature");
            }

            definition_method = iron_resolve_method_token(assembly, token);
            *out_method = find_bound_method(constructed_type, definition_method);
            if (!*out_method) {
                *out_method = definition_method;
            }
            if (*out_method) {
                IRON_DEBUG_EXEC("TypeSpec resolved: method=%s", (*out_method)->name);
                update_method_assembly(*out_method, out_assembly);
                return IRON_SUCCESS;
            }
            IRON_DEBUG_EXEC("TypeSpec: iron_resolve_method_token returned NULL");
        }

        resolved_definition = iron_resolve_method_token(assembly, token);
        *out_method = resolved_definition;
        if (*out_method && !(*out_method)->is_internal_call) {
            update_method_assembly(*out_method, out_assembly);
            return IRON_SUCCESS;
        }
        *out_method = NULL;
        
        /* Build full type name for internal call lookup */
        if (type_name && method_name) {
            char full_type_name[256];
            iron_internal_call_fn icall = NULL;
            const char *key_ptr;
            char key[600];
            iron_u32 param_count = 0;
            const iron_u8 *sig_data = NULL;
            iron_u32 sig_size = 0;
            
            if (type_namespace && type_namespace[0]) {
                snprintf(full_type_name, sizeof(full_type_name), "%s.%s", type_namespace, type_name);
            } else {
                snprintf(full_type_name, sizeof(full_type_name), "%s", type_name);
            }
            
            /* Get signature from blob and build signature string using metadata */
            if (member_ref.signature > 0) {
                res = iron_metadata_get_blob(&assembly->metadata, member_ref.signature,
                                              &sig_data, &sig_size);
                if (IRON_RESULT_OK(res)) {
                    char sig_str[512];
                    param_count = get_method_param_count_from_sig(sig_data, sig_size);
                    build_signature_string_with_metadata(&assembly->metadata, sig_data, sig_size, sig_str, sizeof(sig_str));
                    
                    /* Try with parsed signature first */
                    snprintf(key, sizeof(key), "%s::%s(%s)", full_type_name, method_name, sig_str);
                    IRON_DEBUG_EXEC("Looking up internal call: %s (sig_size=%u, bytes: %02X %02X %02X %02X)", 
                                   key, sig_size, 
                                   sig_size > 0 ? sig_data[0] : 0,
                                   sig_size > 1 ? sig_data[1] : 0,
                                   sig_size > 2 ? sig_data[2] : 0,
                                   sig_size > 3 ? sig_data[3] : 0);
                    key_ptr = iron_intern_cstr(&ctx->domain->interner, key);
                    if (!iron_hashmap_get(&ctx->internal_calls, &key_ptr, &icall)) {
                        /* Fallback: try with empty signature */
                        snprintf(key, sizeof(key), "%s::%s()", full_type_name, method_name);
                        IRON_DEBUG_EXEC("Fallback lookup: %s", key);
                        key_ptr = iron_intern_cstr(&ctx->domain->interner, key);
                        iron_hashmap_get(&ctx->internal_calls, &key_ptr, &icall);
                    }
                }
            } else {
                /* No signature - try empty */
                snprintf(key, sizeof(key), "%s::%s()", full_type_name, method_name);
                key_ptr = iron_intern_cstr(&ctx->domain->interner, key);
                iron_hashmap_get(&ctx->internal_calls, &key_ptr, &icall);
            }
            
            if (icall) {
                iron_bool is_static;

                if (resolved_definition) {
                    resolved_definition->internal_call = (iron_method_invoke_fn)(void *)icall;
                    *out_method = resolved_definition;
                    update_method_assembly(*out_method, out_assembly);
                    return IRON_SUCCESS;
                }

                is_static = sig_data && sig_size > 0 && (sig_data[0] & 0x20) == 0;
                *out_method = cache_internal_method(ctx, assembly, token, method_name, icall, param_count, is_static, NULL);
                if (!*out_method) {
                    return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to cache internal method");
                }

                return IRON_SUCCESS;
            } else {
                /* Log unresolved method with full details */
                IRON_WARN_EXEC("Unresolved method: %s::%s (token 0x%08X, params=%u)",
                              full_type_name, method_name, token, param_count);
            }
        }
        
        return IRON_ERROR(IRON_ERR_NOT_FOUND, "MemberRef method not resolved");
    }
    
    return IRON_ERROR(IRON_ERR_INVALID_TOKEN, "Unsupported method token type");
}

static iron_result_t allocate_array_for_constructor(iron_exec_context_t *ctx,
                                                    iron_eval_stack_t *stack,
                                                    iron_runtime_method_t *method,
                                                    void **array)
{
    iron_u32 lengths[32];
    iron_i32 lower_bounds[32];
    iron_u32 first_argument;
    iron_u32 rank;
    iron_u32 dimension;
    iron_bool has_explicit_lower_bounds;

    if (!ctx || !stack || !method || !array || !method->declaring_type || method->declaring_type->kind != IRON_KIND_ARRAY) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Invalid multidimensional array constructor state");
    }

    *array = NULL;

    rank = method->declaring_type->array_rank;
    has_explicit_lower_bounds = method->param_count == rank * 2U;
    if (rank == 0 || rank > 32 || (!has_explicit_lower_bounds && method->param_count != rank) || stack->size < method->param_count) {
        return IRON_ERROR(IRON_ERR_INVALID_PROGRAM, "Multidimensional array constructor signature does not match its rank");
    }

    first_argument = stack->size - method->param_count;
    for (dimension = 0; dimension < rank; dimension++) {
        iron_stack_value_t lower_bound_value;
        iron_stack_value_t length_value;

        memset(&lower_bound_value, 0, sizeof(lower_bound_value));
        lower_bound_value.type = IRON_VAL_I32;
        if (has_explicit_lower_bounds) {
            lower_bound_value = stack->data[first_argument + dimension * 2U];
            length_value = stack->data[first_argument + dimension * 2U + 1U];
        } else {
            length_value = stack->data[first_argument + dimension];
        }

        if (lower_bound_value.type != IRON_VAL_I32 || length_value.type != IRON_VAL_I32) {
            return IRON_ERROR(IRON_ERR_INVALID_PROGRAM, "Multidimensional array bounds must be Int32 values");
        }
        if (length_value.value.i32 < 0) {
            return IRON_ERROR(IRON_ERR_ARITHMETIC_OVERFLOW, "Array dimensions must be non-negative");
        }

        lower_bounds[dimension] = lower_bound_value.value.i32;
        lengths[dimension] = (iron_u32)length_value.value.i32;
        if ((lengths[dimension] == 0 && lower_bounds[dimension] == INT_MIN) ||
            (lengths[dimension] > 0 && (iron_i64)lower_bounds[dimension] + (iron_i64)lengths[dimension] - 1 > INT_MAX)) {
            return IRON_ERROR(IRON_ERR_ARITHMETIC_OVERFLOW, "Array bounds exceed the Int32 index range");
        }
    }

    *array = iron_gc_alloc_mdarray(ctx, method->declaring_type, lengths, lower_bounds, rank);
    if (!*array) {
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate a multidimensional array");
    }

    return IRON_SUCCESS;
}

typedef struct iron_call_argument_roots {
    iron_gc_handle_t **handles;
    iron_u32 count;
} iron_call_argument_roots_t;

static void *get_call_argument_managed_owner(iron_exec_context_t *ctx, const iron_stack_value_t *argument)
{
    const void *address;

    if (!ctx || !argument) {
        return NULL;
    }

    address = NULL;
    switch (argument->type) {
        case IRON_VAL_OBJ:
        case IRON_VAL_VALUETYPE:
            address = argument->value.obj;
            break;
        case IRON_VAL_PTR:
            address = argument->value.ptr;
            break;
        case IRON_VAL_BYREF:
            address = argument->value.byref.ptr;
            break;
        case IRON_VAL_TYPEDREF:
            address = argument->value.typedref.ptr;
            break;
        default:
            break;
    }

    return iron_gc_find_containing_object(&ctx->gc, address);
}

static void release_call_argument_roots(iron_exec_context_t *ctx, iron_call_argument_roots_t *roots)
{
    iron_u32 index;

    if (!ctx || !roots) {
        return;
    }

    for (index = 0; index < roots->count; index++) {
        if (roots->handles[index]) {
            iron_gc_handle_free(&ctx->gc, roots->handles[index]);
        }
    }

    if (roots->handles) {
        iron_free(ctx->allocator, roots->handles, (iron_size)roots->count * sizeof(*roots->handles));
    }
    memset(roots, 0, sizeof(*roots));
}

static iron_result_t root_call_arguments(iron_exec_context_t *ctx,
                                         const iron_stack_value_t *arguments,
                                         iron_u32 argument_count,
                                         iron_call_argument_roots_t *roots)
{
    iron_u32 index;

    if (!ctx || !roots || (argument_count != 0 && !arguments)) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Invalid call argument root state");
    }

    memset(roots, 0, sizeof(*roots));
    if (argument_count == 0) {
        return IRON_SUCCESS;
    }
    if ((iron_size)argument_count > ((iron_size)-1) / sizeof(*roots->handles)) {
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Call argument root table is too large");
    }

    roots->handles = (iron_gc_handle_t **)iron_alloc(ctx->allocator, (iron_size)argument_count * sizeof(*roots->handles));
    if (!roots->handles) {
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate call argument roots");
    }
    roots->count = argument_count;
    memset(roots->handles, 0, (iron_size)argument_count * sizeof(*roots->handles));

    for (index = 0; index < argument_count; index++) {
        void *object;

        object = get_call_argument_managed_owner(ctx, &arguments[index]);
        if (!object) {
            continue;
        }

        roots->handles[index] = iron_gc_handle_alloc(&ctx->gc, object, IRON_GC_HANDLE_NORMAL);
        if (!roots->handles[index]) {
            release_call_argument_roots(ctx, roots);
            return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to root a managed call argument");
        }
    }

    return IRON_SUCCESS;
}

static iron_result_t update_call_argument_root(iron_exec_context_t *ctx,
                                               iron_call_argument_roots_t *roots,
                                               iron_u32 argument_index,
                                               const iron_stack_value_t *argument)
{
    void *object;

    if (!ctx || !roots || !argument || argument_index >= roots->count) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Invalid call argument root update");
    }

    object = get_call_argument_managed_owner(ctx, argument);
    if (roots->handles[argument_index]) {
        iron_gc_handle_set_target(roots->handles[argument_index], object);
        return IRON_SUCCESS;
    }
    if (!object) {
        return IRON_SUCCESS;
    }

    roots->handles[argument_index] = iron_gc_handle_alloc(&ctx->gc, object, IRON_GC_HANDLE_NORMAL);
    if (!roots->handles[argument_index]) {
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to update a managed call argument root");
    }

    return IRON_SUCCESS;
}

/* Call a method - handles internal calls and IL methods */
static iron_result_t call_method(iron_thread_context_t *thread,
                                 iron_runtime_method_t *method,
                                 iron_assembly_t *assembly,
                                 iron_bool is_newobj)
{
    iron_exec_context_t *ctx;
    iron_eval_stack_t *stack;
    iron_u32 param_count;
    iron_u32 allocated_arg_count;
    iron_stack_value_t *args = NULL;
    iron_stack_value_t result;
    iron_result_t res;
    void *new_obj = NULL;
    iron_bool string_constructor;
    iron_u32 required_stack_values;
    iron_call_argument_roots_t argument_roots;
    
    if (!thread || !method) {
        return IRON_ERROR(IRON_ERR_NULL_POINTER, "Invalid parameters");
    }
    
    ctx = thread->exec_ctx;
    stack = &thread->eval_stack;
    param_count = method->param_count;
    allocated_arg_count = 0;
    memset(&argument_roots, 0, sizeof(argument_roots));
    string_constructor = is_newobj && method->declaring_type && method->declaring_type->full_name && strcmp(method->declaring_type->full_name, "System.String") == 0;
    required_stack_values = method->param_count;
    if (!is_newobj && (method->attrs & IRON_METHOD_STATIC) == 0) {
        if (required_stack_values == UINT32_MAX) {
            return IRON_ERROR(IRON_ERR_INVALID_PROGRAM, "Method argument count exceeds the evaluation-stack limit");
        }
        required_stack_values++;
    }
    if (stack->size < required_stack_values) {
        return IRON_ERROR(IRON_ERR_INVALID_PROGRAM, "Evaluation stack does not contain all method arguments");
    }
    
    (void)assembly; /* May be used later for method body loading */
    
    memset(&result, 0, sizeof(result));
    result.type = IRON_VAL_VOID;
    
    /* For newobj, we need to allocate the object first */
    if (is_newobj && method->declaring_type && method->declaring_type->kind == IRON_KIND_ARRAY) {
        res = allocate_array_for_constructor(ctx, stack, method, &new_obj);
        if (!IRON_RESULT_OK(res)) {
            return convert_managed_call_error(thread, res);
        }
    } else if (is_newobj && method->declaring_type && !string_constructor) {
        iron_gc_t *gc = (iron_gc_t *)&ctx->gc;
        iron_u32 alloc_size = method->declaring_type->instance_size;

        if (alloc_size == 0) {
            return IRON_ERROR(IRON_ERR_INVALID_TYPE, "Cannot allocate a type without a computed instance layout");
        }
        
        new_obj = iron_gc_alloc_object(gc, method->declaring_type, alloc_size);
        if (!new_obj) {
            return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate object");
        }
        /* Zero-initialize the object */
        memset(new_obj, 0, alloc_size);
    }
    
    /* For instance methods (not static, not newobj), we need to include 'this' */
    /* The 'this' pointer is on the stack before the arguments */
    {
        iron_bool is_instance = !is_newobj && !(method->attrs & 0x10); /* 0x10 = static */
        iron_u32 total_args = param_count + (is_instance ? 1 : 0);
        
        /* Copy the arguments before consuming them so a root-allocation failure leaves the managed references visible to the GC. */
        if (total_args > 0) {
            if ((iron_size)total_args > ((iron_size)-1) / sizeof(iron_stack_value_t)) {
                return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Method argument table is too large");
            }

            args = (iron_stack_value_t *)iron_alloc(ctx->allocator, (iron_size)total_args * sizeof(iron_stack_value_t));
            if (!args) {
                return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate args");
            }
            allocated_arg_count = total_args;

            memcpy(args, &stack->data[stack->size - total_args], (iron_size)total_args * sizeof(iron_stack_value_t));
            param_count = total_args;
        }
    }
    
    /* For newobj, 'this' is the newly allocated object */
    if (is_newobj) {
        /* Insert 'this' as first argument */
        iron_stack_value_t *new_args;

        if (param_count == UINT32_MAX || (iron_size)(param_count + 1U) > ((iron_size)-1) / sizeof(iron_stack_value_t)) {
            if (args) {
                iron_free(ctx->allocator, args, (iron_size)allocated_arg_count * sizeof(iron_stack_value_t));
            }
            return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Constructor argument table is too large");
        }

        new_args = (iron_stack_value_t *)iron_alloc(ctx->allocator, (iron_size)(param_count + 1U) * sizeof(iron_stack_value_t));
        if (!new_args) {
            if (args) {
                iron_free(ctx->allocator, args, (iron_size)allocated_arg_count * sizeof(iron_stack_value_t));
            }
            return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate constructor arguments");
        }

        memset(&new_args[0], 0, sizeof(new_args[0]));
        new_args[0].type = IRON_VAL_OBJ;
        new_args[0].value.obj = new_obj;
        if (args && param_count > 0) {
            memcpy(&new_args[1], args, (iron_size)param_count * sizeof(iron_stack_value_t));
            iron_free(ctx->allocator, args, (iron_size)allocated_arg_count * sizeof(iron_stack_value_t));
        }
        args = new_args;
        param_count++;
        allocated_arg_count = param_count;
    }

    res = root_call_arguments(ctx, args, param_count, &argument_roots);
    if (!IRON_RESULT_OK(res)) {
        if (args) {
            iron_free(ctx->allocator, args, (iron_size)allocated_arg_count * sizeof(iron_stack_value_t));
        }
        return convert_managed_call_error(thread, res);
    }

    stack->size -= required_stack_values;

    if (!is_newobj && method->is_internal_call && param_count != 0 && args[0].type == IRON_VAL_BYREF && method->declaring_type &&
        (method->declaring_type->kind == IRON_KIND_VALUETYPE || method->declaring_type->kind == IRON_KIND_ENUM)) {
        if (!push_storage_value(ctx, stack, stack_value_address(&args[0]), method->declaring_type)) {
            res = convert_managed_call_error(thread, IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to materialize a value-type receiver"));
            release_call_argument_roots(ctx, &argument_roots);
            iron_free(ctx->allocator, args, (iron_size)allocated_arg_count * sizeof(iron_stack_value_t));
            return res;
        }
        args[0] = iron_stack_pop(stack);
        res = update_call_argument_root(ctx, &argument_roots, 0, &args[0]);
        if (!IRON_RESULT_OK(res)) {
            res = convert_managed_call_error(thread, res);
            release_call_argument_roots(ctx, &argument_roots);
            iron_free(ctx->allocator, args, (iron_size)allocated_arg_count * sizeof(iron_stack_value_t));
            return res;
        }
    }
    
    /* Check if this is an internal call */
    if (type_is_delegate(method->declaring_type) && method->name && strcmp(method->name, ".ctor") == 0) {
        res = initialize_delegate(method->declaring_type, args, param_count);
    } else if (type_is_delegate(method->declaring_type) && method->name && strcmp(method->name, "Invoke") == 0) {
        res = invoke_delegate(ctx, method->declaring_type, args, param_count, &result);
    } else if (method->is_internal_call && method->internal_call) {
        iron_internal_call_fn icall = (iron_internal_call_fn)(void*)method->internal_call;
        res = icall(ctx, args, param_count, &result);
    } else if (method->is_internal_call) {
        /* Try to find internal call by name */
        iron_internal_call_fn icall = iron_lookup_internal_call(ctx, method);
        if (icall) {
            res = icall(ctx, args, param_count, &result);
        } else {
            res = IRON_ERROR(IRON_ERR_NOT_FOUND, "Internal call not registered");
        }
    } else {
        /* Execute IL method */
        res = iron_exec_method(ctx, method, args, param_count, &result);
    }
    
    if (!IRON_RESULT_OK(res)) {
        IRON_DEBUG_EXEC("Call %s::%s failed with %s: %s",
                        method->declaring_type && method->declaring_type->full_name ? method->declaring_type->full_name : "<unknown type>",
                        method->name ? method->name : "<unknown method>",
                        iron_error_str(res.error),
                        res.message ? res.message : "no diagnostic message");
        res = convert_managed_call_error(thread, res);
    }

    release_call_argument_roots(ctx, &argument_roots);

    /* Clean up args */
    if (args) {
        iron_free(ctx->allocator, args, (iron_size)allocated_arg_count * sizeof(iron_stack_value_t));
    }
    
    if (!IRON_RESULT_OK(res)) {
        return res;
    }
    
    /* Push result onto stack */
    if (is_newobj) {
        void *constructed_value;

        constructed_value = string_constructor && result.type == IRON_VAL_OBJ ? result.value.obj : new_obj;
        if (method->declaring_type &&
            (method->declaring_type->kind == IRON_KIND_VALUETYPE || method->declaring_type->kind == IRON_KIND_ENUM)) {
            iron_stack_value_t value;

            memset(&value, 0, sizeof(value));
            value.type = IRON_VAL_VALUETYPE;
            value.value.obj = constructed_value;
            iron_stack_push(stack, value);
        } else {
            iron_stack_push_obj(stack, constructed_value);
        }
    } else {
        if (result.type != IRON_VAL_VOID) {
            IRON_TRACE_EXEC("call_method: pushing result type=%d value=%d", result.type, result.value.i32);
            iron_stack_push(stack, result);
        }
    }
    
    return IRON_SUCCESS;
}

/* ============================================================================
 * GC Allocation Wrappers (exec.h API)
 * ============================================================================ */

void *iron_gc_alloc(iron_exec_context_t *ctx, iron_runtime_type_t *type)
{
    if (!ctx || !type) {
        return NULL;
    }

    return iron_gc_alloc_object(&ctx->gc, type, type->instance_size);
}

void *iron_gc_alloc_array(iron_exec_context_t *ctx,
                          iron_runtime_type_t *element_type,
                          iron_u32 length)
{
    iron_size element_size;
    
    if (!ctx) {
        return NULL;
    }
    
    element_size = iron_type_storage_size(element_type);
    if (element_size == 0) {
        return NULL;
    }
    
    return iron_gc_alloc_array_raw(&ctx->gc, element_type, element_size, length);
}

void *iron_gc_alloc_mdarray(iron_exec_context_t *ctx,
                            iron_runtime_type_t *array_type,
                            const iron_u32 *lengths,
                            const iron_i32 *lower_bounds,
                            iron_u32 rank)
{
    iron_size element_size;

    if (!ctx || !array_type || !array_type->element) {
        return NULL;
    }

    element_size = iron_type_storage_size(array_type->element);
    if (element_size == 0) {
        return NULL;
    }

    return iron_gc_alloc_mdarray_raw(&ctx->gc, array_type, array_type->element, element_size, lengths, lower_bounds, rank);
}

void *iron_gc_alloc_string(iron_exec_context_t *ctx,
                           const iron_u16 *chars,
                           iron_u32 length)
{
    iron_runtime_type_t *string_type;
    void *str;
    iron_u32 *len_ptr;
    iron_u16 *data_ptr;
    iron_size size;
    
    if (!ctx) return NULL;

    if ((iron_size)length > (((iron_size)-1) - sizeof(iron_u32)) / sizeof(iron_u16)) {
        return NULL;
    }

    /* String layout: 4-byte length + UTF-16 chars */
    size = sizeof(iron_u32) + (length * sizeof(iron_u16));
    string_type = ctx->domain ? iron_domain_find_type(ctx->domain, "System.String") : NULL;
    if (!string_type) {
        return NULL;
    }

    str = iron_gc_alloc_object(&ctx->gc, string_type, size);
    
    if (str) {
        len_ptr = (iron_u32 *)str;
        *len_ptr = length;
        data_ptr = (iron_u16 *)((iron_u8 *)str + sizeof(iron_u32));
        if (chars) {
            memcpy(data_ptr, chars, length * sizeof(iron_u16));
        }
    }
    
    return str;
}

void *iron_gc_box(iron_exec_context_t *ctx,
                  iron_runtime_type_t *type,
                  const void *value)
{
    void *boxed;
    
    if (!ctx || !type || !value) return NULL;
    
    boxed = iron_gc_alloc_object(&ctx->gc, type, type->instance_size);
    if (boxed) {
        memcpy(boxed, value, type->instance_size);
    }
    
    return boxed;
}

void *iron_gc_unbox(void *obj, iron_runtime_type_t *type)
{
    (void)type;
    /* For boxed value types, the object data IS the value */
    return obj;
}
