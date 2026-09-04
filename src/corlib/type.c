/*
 * IronNet CLR Interpreter
 * corlib/type.c - System.Type internal calls
 */

#include "iron/corlib.h"
#include "iron/runtime.h"
#include <stdio.h>

static iron_runtime_type_t *get_runtime_type_argument(iron_stack_value_t *args, iron_u32 arg_count)
{
    if (!args || arg_count == 0) {
        return NULL;
    }

    return (iron_runtime_type_t *)args[0].value.obj;
}

static iron_type_kind_t get_effective_type_kind(const iron_runtime_type_t *type)
{
    if (type && type->kind == IRON_KIND_GENERIC_INST && type->generic_definition) {
        return type->generic_definition->kind;
    }

    return type ? type->kind : IRON_KIND_CLASS;
}

static iron_result_t return_type_boolean(iron_stack_value_t *result, iron_bool value)
{
    if (!result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Type property requires a result slot");
    }

    result->type = IRON_VAL_I32;
    result->value.i32 = value ? 1 : 0;
    return IRON_SUCCESS;
}

/* ============================================================================
 * System.Type Internal Calls
 * ============================================================================ */

iron_result_t icall_Type_GetTypeFromHandle(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    void *handle;

    (void)ctx;

    if (arg_count < 1 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "GetTypeFromHandle requires handle");
    }

    /*
     * RuntimeTypeHandle is a value type wrapping a pointer to the runtime type.
     * The handle is passed as a pointer/obj on the stack.
     */
    handle = args[0].value.obj;
    if (!handle) {
        handle = args[0].value.ptr;
    }

    result->type = IRON_VAL_OBJ;
    result->value.obj = handle;

    return IRON_SUCCESS;
}

iron_result_t icall_Type_get_Name(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    iron_runtime_type_t *type;
    void *str_obj;

    if (arg_count < 1 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Type.Name requires 'this'");
    }

    type = get_runtime_type_argument(args, arg_count);
    if (!type) {
        return IRON_ERROR(IRON_ERR_NULL_REFERENCE, "NullReferenceException");
    }

    str_obj = iron_string_new_utf8(ctx, type->name ? type->name : "");

    result->type = IRON_VAL_OBJ;
    result->value.obj = str_obj;

    return IRON_SUCCESS;
}

iron_result_t icall_Type_get_FullName(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    iron_runtime_type_t *type;
    void *str_obj;

    if (arg_count < 1 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Type.FullName requires 'this'");
    }

    type = get_runtime_type_argument(args, arg_count);
    if (!type) {
        return IRON_ERROR(IRON_ERR_NULL_REFERENCE, "NullReferenceException");
    }

    result->type = IRON_VAL_OBJ;
    if (type->kind == IRON_KIND_GENERIC_PARAM || (!type->is_generic_definition && iron_type_contains_generic_parameters(type))) {
        result->value.obj = NULL;
        return IRON_SUCCESS;
    }

    str_obj = iron_string_new_utf8(ctx, type->full_name ? type->full_name : (type->name ? type->name : ""));
    result->value.obj = str_obj;
    return str_obj ? IRON_SUCCESS : IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate Type.FullName");
}

iron_result_t icall_Type_get_Namespace(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_type_t *type;

    if (!result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Type.Namespace requires a result slot");
    }

    type = get_runtime_type_argument(args, arg_count);
    if (!type) {
        return IRON_ERROR(IRON_ERR_NULL_REFERENCE, "Type.Namespace requires 'this'");
    }

    result->type = IRON_VAL_OBJ;
    if (type->kind == IRON_KIND_GENERIC_PARAM) {
        result->value.obj = NULL;
        return IRON_SUCCESS;
    }
    result->value.obj = iron_string_new_utf8(ctx, type->namespace_ ? type->namespace_ : "");
    return result->value.obj ? IRON_SUCCESS : IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate Type.Namespace");
}

iron_result_t icall_Type_get_BaseType(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_type_t *type;

    (void)ctx;
    if (!result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Type.BaseType requires a result slot");
    }

    type = get_runtime_type_argument(args, arg_count);
    if (!type) {
        return IRON_ERROR(IRON_ERR_NULL_REFERENCE, "Type.BaseType requires 'this'");
    }

    result->type = IRON_VAL_OBJ;
    result->value.obj = type->base_type;
    return IRON_SUCCESS;
}

iron_result_t icall_Type_get_DeclaringType(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_type_t *type;

    (void)ctx;
    if (!result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Type.DeclaringType requires a result slot");
    }

    type = get_runtime_type_argument(args, arg_count);
    if (!type) {
        return IRON_ERROR(IRON_ERR_NULL_REFERENCE, "Type.DeclaringType requires 'this'");
    }

    result->type = IRON_VAL_OBJ;
    result->value.obj = type->declaring_type;
    return IRON_SUCCESS;
}

iron_result_t icall_Type_get_Attributes(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_type_t *type;

    (void)ctx;
    type = get_runtime_type_argument(args, arg_count);
    if (!type || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Type.Attributes requires an instance and result slot");
    }

    result->type = IRON_VAL_I32;
    result->value.i32 = (iron_i32)type->attrs;
    return IRON_SUCCESS;
}

iron_result_t icall_Type_get_IsClass(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_type_t *type;
    iron_type_kind_t kind;

    (void)ctx;
    type = get_runtime_type_argument(args, arg_count);
    if (!type) {
        return IRON_ERROR(IRON_ERR_NULL_REFERENCE, "Type.IsClass requires 'this'");
    }

    kind = get_effective_type_kind(type);
    return return_type_boolean(result, kind == IRON_KIND_CLASS || kind == IRON_KIND_DELEGATE);
}

iron_result_t icall_Type_get_IsValueType(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_type_t *type;
    iron_type_kind_t kind;

    (void)ctx;
    type = get_runtime_type_argument(args, arg_count);
    if (!type) {
        return IRON_ERROR(IRON_ERR_NULL_REFERENCE, "Type.IsValueType requires 'this'");
    }

    kind = get_effective_type_kind(type);
    return return_type_boolean(result, kind == IRON_KIND_VALUETYPE || kind == IRON_KIND_ENUM);
}

iron_result_t icall_Type_get_IsInterface(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_type_t *type;

    (void)ctx;
    type = get_runtime_type_argument(args, arg_count);
    if (!type) {
        return IRON_ERROR(IRON_ERR_NULL_REFERENCE, "Type.IsInterface requires 'this'");
    }

    return return_type_boolean(result, get_effective_type_kind(type) == IRON_KIND_INTERFACE);
}

iron_result_t icall_Type_get_IsArray(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_type_t *type;

    (void)ctx;
    type = get_runtime_type_argument(args, arg_count);
    if (!type) {
        return IRON_ERROR(IRON_ERR_NULL_REFERENCE, "Type.IsArray requires 'this'");
    }

    return return_type_boolean(result, type->kind == IRON_KIND_ARRAY);
}

iron_result_t icall_Type_get_IsEnum(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_type_t *type;

    (void)ctx;
    type = get_runtime_type_argument(args, arg_count);
    if (!type) {
        return IRON_ERROR(IRON_ERR_NULL_REFERENCE, "Type.IsEnum requires 'this'");
    }

    return return_type_boolean(result, get_effective_type_kind(type) == IRON_KIND_ENUM);
}

iron_result_t icall_Type_get_HasElementType(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_type_t *type;

    (void)ctx;
    type = get_runtime_type_argument(args, arg_count);
    if (!type) {
        return IRON_ERROR(IRON_ERR_NULL_REFERENCE, "Type.HasElementType requires 'this'");
    }
    return return_type_boolean(result, type->element != NULL);
}

iron_result_t icall_Type_get_IsPointer(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_type_t *type;

    (void)ctx;
    type = get_runtime_type_argument(args, arg_count);
    if (!type) {
        return IRON_ERROR(IRON_ERR_NULL_REFERENCE, "Type.IsPointer requires 'this'");
    }
    return return_type_boolean(result, type->kind == IRON_KIND_POINTER);
}

iron_result_t icall_Type_get_IsByRef(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_type_t *type;

    (void)ctx;
    type = get_runtime_type_argument(args, arg_count);
    if (!type) {
        return IRON_ERROR(IRON_ERR_NULL_REFERENCE, "Type.IsByRef requires 'this'");
    }
    return return_type_boolean(result, type->kind == IRON_KIND_BYREF);
}

iron_result_t icall_Type_get_IsGenericType(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_type_t *type;

    (void)ctx;
    type = get_runtime_type_argument(args, arg_count);
    if (!type) {
        return IRON_ERROR(IRON_ERR_NULL_REFERENCE, "Type.IsGenericType requires 'this'");
    }
    return return_type_boolean(result, type->is_generic_definition || type->is_generic_instance);
}

iron_result_t icall_Type_get_IsGenericTypeDefinition(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_type_t *type;

    (void)ctx;
    type = get_runtime_type_argument(args, arg_count);
    if (!type) {
        return IRON_ERROR(IRON_ERR_NULL_REFERENCE, "Type.IsGenericTypeDefinition requires 'this'");
    }
    return return_type_boolean(result, type->is_generic_definition);
}

iron_result_t icall_Type_get_IsGenericParameter(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_type_t *type;

    (void)ctx;
    type = get_runtime_type_argument(args, arg_count);
    if (!type) {
        return IRON_ERROR(IRON_ERR_NULL_REFERENCE, "Type.IsGenericParameter requires 'this'");
    }
    return return_type_boolean(result, type->kind == IRON_KIND_GENERIC_PARAM);
}

iron_result_t icall_Type_get_ContainsGenericParameters(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_type_t *type;

    (void)ctx;
    type = get_runtime_type_argument(args, arg_count);
    if (!type) {
        return IRON_ERROR(IRON_ERR_NULL_REFERENCE, "Type.ContainsGenericParameters requires 'this'");
    }
    return return_type_boolean(result, iron_type_contains_generic_parameters(type));
}

iron_result_t icall_Type_get_GenericParameterPosition(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_type_t *type;

    (void)ctx;
    type = get_runtime_type_argument(args, arg_count);
    if (!type || type->kind != IRON_KIND_GENERIC_PARAM || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Type.GenericParameterPosition requires a generic parameter Type");
    }

    result->type = IRON_VAL_I32;
    result->value.i32 = (iron_i32)type->generic_parameter_position;
    return IRON_SUCCESS;
}

iron_result_t icall_Type_get_GenericParameterAttributes(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_type_t *type;

    (void)ctx;
    type = get_runtime_type_argument(args, arg_count);
    if (!type || type->kind != IRON_KIND_GENERIC_PARAM || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Type.GenericParameterAttributes requires a generic parameter Type");
    }

    result->type = IRON_VAL_I32;
    result->value.i32 = (iron_i32)type->attrs;
    return IRON_SUCCESS;
}

iron_result_t icall_Type_get_DeclaringMethod(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_type_t *type;

    type = get_runtime_type_argument(args, arg_count);
    if (!type || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Type.DeclaringMethod requires an instance and result slot");
    }

    result->type = IRON_VAL_OBJ;
    result->value.obj = type->declaring_method ? iron_reflection_create_method_info(ctx, type->declaring_method) : NULL;
    if (type->declaring_method && !result->value.obj) {
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate a declaring method reflection object");
    }
    return IRON_SUCCESS;
}

iron_result_t icall_Type_GetElementType(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_type_t *type;

    (void)ctx;
    type = get_runtime_type_argument(args, arg_count);
    if (!type || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Type.GetElementType requires an instance and result slot");
    }

    result->type = IRON_VAL_OBJ;
    result->value.obj = type->element;
    return IRON_SUCCESS;
}

iron_result_t icall_Type_GetArrayRank(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_type_t *type;

    (void)ctx;
    type = get_runtime_type_argument(args, arg_count);
    if (!type || !result || type->kind != IRON_KIND_ARRAY) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Type.GetArrayRank requires an array Type");
    }

    result->type = IRON_VAL_I32;
    result->value.i32 = (iron_i32)type->array_rank;
    return IRON_SUCCESS;
}

iron_result_t iron_corlib_return_type_array(iron_exec_context_t *ctx,
                                            iron_runtime_type_t **types,
                                            iron_u32 type_count,
                                            iron_stack_value_t *result)
{
    void *array;
    void **elements;
    iron_u32 type_index;

    if (!ctx || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Type array creation requires an execution context and result slot");
    }

    array = iron_gc_alloc_array(ctx, ctx->domain->type_type, type_count);
    if (!array) {
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate a reflected Type array");
    }

    elements = (void **)iron_array_get_data(array);
    for (type_index = 0; type_index < type_count; type_index++) {
        iron_gc_write_barrier(array, &elements[type_index], types[type_index]);
    }

    result->type = IRON_VAL_OBJ;
    result->value.obj = array;
    return IRON_SUCCESS;
}

iron_result_t icall_Type_GetGenericParameterConstraints(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_type_t *type;
    iron_runtime_type_t **constraints;
    iron_u32 constraint_count;
    iron_result_t operation_result;

    type = get_runtime_type_argument(args, arg_count);
    if (!type || type->kind != IRON_KIND_GENERIC_PARAM || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Type.GetGenericParameterConstraints requires a generic parameter Type");
    }

    constraints = NULL;
    constraint_count = 0;
    operation_result = iron_generic_parameter_get_constraints(type, ctx->allocator, &constraints, &constraint_count);
    if (!IRON_RESULT_OK(operation_result)) {
        return operation_result;
    }

    operation_result = iron_corlib_return_type_array(ctx, constraints, constraint_count, result);
    if (constraints) {
        iron_free(ctx->allocator, constraints, (iron_size)constraint_count * sizeof(iron_runtime_type_t *));
    }
    return operation_result;
}

iron_result_t icall_Type_GetGenericArguments(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_type_t *type;
    iron_runtime_type_t **arguments;
    iron_u32 argument_count;

    type = get_runtime_type_argument(args, arg_count);
    if (!type) {
        return IRON_ERROR(IRON_ERR_NULL_REFERENCE, "Type.GetGenericArguments requires 'this'");
    }

    arguments = type->is_generic_instance ? type->generic_args : type->generic_params;
    argument_count = type->is_generic_instance ? type->generic_arg_count : type->generic_param_count;
    return iron_corlib_return_type_array(ctx, arguments, argument_count, result);
}

iron_result_t icall_Type_GetGenericTypeDefinitionCore(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_type_t *type;

    (void)ctx;
    type = get_runtime_type_argument(args, arg_count);
    if (!type || !result || (!type->is_generic_definition && !type->is_generic_instance)) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Type.GetGenericTypeDefinition requires a generic type");
    }

    result->type = IRON_VAL_OBJ;
    result->value.obj = type->is_generic_instance ? type->generic_definition : type;
    return IRON_SUCCESS;
}

static iron_bool append_interface(iron_allocator_t *allocator,
                                  iron_runtime_type_t ***interfaces,
                                  iron_u32 *interface_count,
                                  iron_u32 *interface_capacity,
                                  iron_runtime_type_t *interface_type,
                                  iron_bool *added)
{
    iron_runtime_type_t **resized;
    iron_u32 interface_index;
    iron_u32 new_capacity;

    for (interface_index = 0; interface_index < *interface_count; interface_index++) {
        if ((*interfaces)[interface_index] == interface_type) {
            *added = IRON_FALSE;
            return IRON_TRUE;
        }
    }

    if (*interface_count == *interface_capacity) {
        new_capacity = *interface_capacity == 0 ? 8 : *interface_capacity * 2;
        if (new_capacity < *interface_capacity || (iron_size)new_capacity > (iron_size)-1 / sizeof(iron_runtime_type_t *)) {
            return IRON_FALSE;
        }
        resized = (iron_runtime_type_t **)iron_realloc(allocator,
                                                       *interfaces,
                                                       (iron_size)*interface_capacity * sizeof(iron_runtime_type_t *),
                                                       (iron_size)new_capacity * sizeof(iron_runtime_type_t *));
        if (!resized) {
            return IRON_FALSE;
        }
        *interfaces = resized;
        *interface_capacity = new_capacity;
    }

    (*interfaces)[(*interface_count)++] = interface_type;
    *added = IRON_TRUE;
    return IRON_TRUE;
}

static iron_bool collect_interfaces(iron_allocator_t *allocator,
                                    iron_runtime_type_t *type,
                                    iron_runtime_type_t ***interfaces,
                                    iron_u32 *interface_count,
                                    iron_u32 *interface_capacity)
{
    iron_u32 interface_index;

    if (!type) {
        return IRON_TRUE;
    }

    for (interface_index = 0; interface_index < type->interface_count; interface_index++) {
        iron_runtime_type_t *interface_type;
        iron_bool added;

        interface_type = type->interfaces[interface_index];
        if (!append_interface(allocator, interfaces, interface_count, interface_capacity, interface_type, &added) ||
            (added && !collect_interfaces(allocator, interface_type, interfaces, interface_count, interface_capacity))) {
            return IRON_FALSE;
        }
    }
    return collect_interfaces(allocator, type->base_type, interfaces, interface_count, interface_capacity);
}

iron_result_t icall_Type_GetInterfaces(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_type_t *type;
    iron_runtime_type_t **interfaces;
    iron_u32 interface_count;
    iron_u32 interface_capacity;
    iron_result_t operation_result;

    type = get_runtime_type_argument(args, arg_count);
    if (!type) {
        return IRON_ERROR(IRON_ERR_NULL_REFERENCE, "Type.GetInterfaces requires 'this'");
    }

    interfaces = NULL;
    interface_count = 0;
    interface_capacity = 0;
    if (!collect_interfaces(ctx->allocator, type, &interfaces, &interface_count, &interface_capacity)) {
        if (interfaces) {
            iron_free(ctx->allocator, interfaces, (iron_size)interface_capacity * sizeof(iron_runtime_type_t *));
        }
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to collect implemented interfaces");
    }

    operation_result = iron_corlib_return_type_array(ctx, interfaces, interface_count, result);
    if (interfaces) {
        iron_free(ctx->allocator, interfaces, (iron_size)interface_capacity * sizeof(iron_runtime_type_t *));
    }
    return operation_result;
}

iron_result_t icall_Type_MakeGenericType(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_type_t *definition;
    void *argument_array;
    void **argument_objects;
    iron_runtime_type_t **type_arguments;
    iron_runtime_type_t *constructed_type;
    iron_u32 argument_count;
    iron_u32 argument_index;

    if (!args || arg_count < 2 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Type.MakeGenericType requires type arguments and a result slot");
    }
    definition = get_runtime_type_argument(args, arg_count);
    argument_array = iron_corlib_object_argument(&args[1]);
    if (!definition || !definition->is_generic_definition || !argument_array) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Type.MakeGenericType requires a generic type definition and a non-null argument array");
    }

    argument_count = iron_array_get_length(argument_array);
    if (argument_count != definition->generic_param_count) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Generic type argument count does not match the type definition");
    }
    argument_objects = (void **)iron_array_get_data(argument_array);
    type_arguments = (iron_runtime_type_t **)iron_alloc(ctx->allocator, (iron_size)argument_count * sizeof(iron_runtime_type_t *));
    if (!type_arguments) {
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate generic type argument storage");
    }

    for (argument_index = 0; argument_index < argument_count; argument_index++) {
        if (!iron_domain_is_type_descriptor(ctx->domain, argument_objects[argument_index])) {
            iron_free(ctx->allocator, type_arguments, (iron_size)argument_count * sizeof(iron_runtime_type_t *));
            return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Generic type arguments must contain valid Type values");
        }
        type_arguments[argument_index] = (iron_runtime_type_t *)argument_objects[argument_index];
    }

    constructed_type = iron_type_make_generic(ctx->domain, definition, type_arguments, argument_count);
    iron_free(ctx->allocator, type_arguments, (iron_size)argument_count * sizeof(iron_runtime_type_t *));
    if (!constructed_type) {
        return IRON_ERROR(IRON_ERR_INVALID_TYPE, "Failed to construct the requested generic type");
    }

    result->type = IRON_VAL_OBJ;
    result->value.obj = constructed_type;
    return IRON_SUCCESS;
}

static iron_result_t return_constructed_element_type(iron_exec_context_t *ctx,
                                                      iron_stack_value_t *args,
                                                      iron_u32 arg_count,
                                                      iron_stack_value_t *result,
                                                      iron_type_kind_t kind,
                                                      iron_u32 rank)
{
    iron_runtime_type_t *element_type;
    iron_runtime_type_t *constructed_type;

    element_type = get_runtime_type_argument(args, arg_count);
    if (!element_type || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Constructed element Type requires an instance and result slot");
    }

    if (kind == IRON_KIND_ARRAY) {
        constructed_type = rank >= 1 && rank <= 32 ? iron_type_make_array(ctx->domain, element_type, rank) : NULL;
    } else if (kind == IRON_KIND_POINTER) {
        constructed_type = iron_type_make_pointer(ctx->domain, element_type);
    } else {
        constructed_type = element_type->element_type == IRON_TYPE_VOID ? NULL : iron_type_make_byref(ctx->domain, element_type);
    }
    if (!constructed_type) {
        return IRON_ERROR(IRON_ERR_INVALID_TYPE, "Failed to construct the requested element Type");
    }

    result->type = IRON_VAL_OBJ;
    result->value.obj = constructed_type;
    return IRON_SUCCESS;
}

iron_result_t icall_Type_MakeArrayType(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_u32 rank;

    rank = arg_count >= 2 ? (iron_u32)args[1].value.i32 : 1;
    return return_constructed_element_type(ctx, args, arg_count, result, IRON_KIND_ARRAY, rank);
}

iron_result_t icall_Type_MakePointerType(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    return return_constructed_element_type(ctx, args, arg_count, result, IRON_KIND_POINTER, 0);
}

iron_result_t icall_Type_MakeByRefType(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    return return_constructed_element_type(ctx, args, arg_count, result, IRON_KIND_BYREF, 0);
}
