/*
 * IronNet CLR Interpreter
 * corlib/object.c - System.Object internal calls
 */

#include "iron/corlib.h"
#include "iron/gc.h"
#include "iron/vtable.h"
#include <stdio.h>
#include <string.h>

/* ============================================================================
 * System.Object Internal Calls
 * ============================================================================ */

iron_result_t icall_Object_ctor(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    /* Object constructor does nothing - just returns */
    (void)ctx;
    (void)args;
    (void)arg_count;
    (void)result;
    
    return IRON_SUCCESS;
}

iron_result_t icall_Object_GetType(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    void *obj;
    iron_runtime_type_t *type;
    
    (void)ctx;
    
    if (arg_count < 1 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "GetType requires 'this'");
    }
    
    obj = args[0].value.obj;
    if (!obj) {
        return IRON_ERROR(IRON_ERR_NULL_REFERENCE, "NullReferenceException");
    }
    
    type = iron_gc_get_type(obj);
    result->type = IRON_VAL_OBJ;
    result->value.obj = type;
    
    return IRON_SUCCESS;
}

iron_result_t icall_Object_GetHashCode(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    (void)ctx;
    
    if (arg_count < 1 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "GetHashCode requires 'this'");
    }
    
    result->type = IRON_VAL_I32;
    result->value.i32 = (iron_i32)(size_t)args[0].value.obj;
    
    return IRON_SUCCESS;
}

iron_result_t icall_Object_Equals(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    (void)ctx;
    
    if (arg_count < 2 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Equals requires 'this' and 'obj'");
    }
    
    result->type = IRON_VAL_I32;
    result->value.i32 = (args[0].value.obj == args[1].value.obj) ? 1 : 0;
    
    return IRON_SUCCESS;
}

iron_result_t icall_Object_ReferenceEquals(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    (void)ctx;
    
    if (arg_count < 2 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "ReferenceEquals requires two objects");
    }
    
    result->type = IRON_VAL_I32;
    result->value.i32 = (args[0].value.obj == args[1].value.obj) ? 1 : 0;
    
    return IRON_SUCCESS;
}

iron_result_t icall_Object_MemberwiseClone(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    void *obj;
    void *clone;
    iron_gc_header_t *header;
    iron_size obj_size;

    if (arg_count < 1 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "MemberwiseClone requires 'this'");
    }

    obj = args[0].value.obj;
    if (!obj) {
        return IRON_ERROR(IRON_ERR_NULL_REFERENCE, "NullReferenceException");
    }

    /* Get the GC header to find type and size */
    header = IRON_GC_HEADER(obj);
    obj_size = header->size;

    /* Allocate new object with same type and size */
    clone = iron_gc_alloc_object(&ctx->gc, header->type, obj_size);
    if (!clone) {
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate clone");
    }

    /* Shallow copy: copy all fields (including embedded value types and references) */
    memcpy(clone, obj, obj_size);

    result->type = IRON_VAL_OBJ;
    result->value.obj = clone;

    return IRON_SUCCESS;
}

iron_result_t icall_Object_ToString(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    void *str_obj;
    char buffer[128];
    
    if (arg_count < 1 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "ToString requires 'this'");
    }
    
    snprintf(buffer, sizeof(buffer), "[Object@%p]", args[0].value.obj);
    str_obj = iron_string_new_utf8(ctx, buffer);
    
    result->type = IRON_VAL_OBJ;
    result->value.obj = str_obj;
    
    return IRON_SUCCESS;
}
