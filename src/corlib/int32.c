/*
 * IronNet CLR Interpreter
 * corlib/int32.c - System.Int32 internal calls
 */

#include "iron/corlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * System.Int32 Internal Calls
 * ============================================================================ */

iron_result_t icall_Int32_ToString(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    char buffer[32];
    void *str_obj;
    iron_i32 value;
    
    if (arg_count < 1 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Int32.ToString requires 'this'");
    }
    
    /* 'this' for value types is passed as a pointer (from ldloca) */
    if (args[0].type == IRON_VAL_PTR && args[0].value.ptr) {
        value = *((iron_i32 *)args[0].value.ptr);
    } else {
        value = args[0].value.i32;
    }
    
    snprintf(buffer, sizeof(buffer), "%d", value);
    
    str_obj = iron_string_new_utf8(ctx, buffer);
    if (!str_obj) {
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate string");
    }
    
    result->type = IRON_VAL_PTR;
    result->value.ptr = str_obj;
    
    return IRON_SUCCESS;
}

iron_result_t icall_Int32_Parse(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    void *str_obj;
    char *utf8;
    iron_i32 value = 0;
    
    if (arg_count < 1 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Int32.Parse requires string");
    }
    
    str_obj = (args[0].type == IRON_VAL_PTR) ? args[0].value.ptr : args[0].value.obj;
    if (!str_obj) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "ArgumentNullException");
    }
    
    utf8 = iron_string_to_utf8(ctx, str_obj);
    if (utf8) {
        value = atoi(utf8);
        iron_free(ctx->allocator, utf8, strlen(utf8) + 1);
    }
    
    result->type = IRON_VAL_I32;
    result->value.i32 = value;
    
    return IRON_SUCCESS;
}
