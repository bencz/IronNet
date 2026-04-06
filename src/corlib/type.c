/*
 * IronNet CLR Interpreter
 * corlib/type.c - System.Type internal calls
 */

#include "iron/corlib.h"
#include "iron/runtime.h"
#include <stdio.h>

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

    type = (iron_runtime_type_t *)args[0].value.obj;
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
    char fullname[512];

    if (arg_count < 1 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Type.FullName requires 'this'");
    }

    type = (iron_runtime_type_t *)args[0].value.obj;
    if (!type) {
        return IRON_ERROR(IRON_ERR_NULL_REFERENCE, "NullReferenceException");
    }

    /* Build full name: namespace.name */
    if (type->namespace_ && type->namespace_[0]) {
        snprintf(fullname, sizeof(fullname), "%s.%s",
                 type->namespace_, type->name ? type->name : "");
    } else {
        snprintf(fullname, sizeof(fullname), "%s",
                 type->name ? type->name : "");
    }

    str_obj = iron_string_new_utf8(ctx, fullname);

    result->type = IRON_VAL_OBJ;
    result->value.obj = str_obj;

    return IRON_SUCCESS;
}
