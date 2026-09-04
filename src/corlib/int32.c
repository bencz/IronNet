/*
 * IronNet CLR Interpreter
 * corlib/int32.c - System.Int32 internal calls
 */

#include "iron/corlib.h"
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
    char buffer[12];
    char *cursor;
    const void *value_data;
    void *str_obj;
    iron_i32 value;
    iron_u32 magnitude;
    
    if (arg_count < 1 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Int32.ToString requires 'this'");
    }
    
    value_data = iron_corlib_value_argument_data(&args[0]);
    if (!value_data) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Int32.ToString received an invalid 'this' value");
    }

    memcpy(&value, value_data, sizeof(value));
    
    cursor = buffer + sizeof(buffer);
    *--cursor = '\0';
    magnitude = value < 0 ? (iron_u32)0 - (iron_u32)value : (iron_u32)value;

    do {
        *--cursor = (char)('0' + magnitude % 10);
        magnitude /= 10;
    } while (magnitude != 0);

    if (value < 0) {
        *--cursor = '-';
    }
    
    str_obj = iron_string_new_utf8(ctx, cursor);
    if (!str_obj) {
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate string");
    }
    
    result->type = IRON_VAL_OBJ;
    result->value.obj = str_obj;
    
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
