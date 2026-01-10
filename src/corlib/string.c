/*
 * IronNet CLR Interpreter
 * corlib/string.c - System.String internal calls and helpers
 */

#include "iron/corlib.h"
#include <string.h>

/* ============================================================================
 * String Helpers
 * ============================================================================ */

iron_u32 iron_string_get_length(void *str_obj)
{
    if (!str_obj) return 0;
    return *((iron_u32 *)str_obj);
}

iron_u16 *iron_string_get_chars(void *str_obj)
{
    if (!str_obj) return NULL;
    return (iron_u16 *)((iron_u8 *)str_obj + sizeof(iron_u32));
}

void *iron_string_new_utf8(iron_exec_context_t *ctx, const char *utf8)
{
    iron_u32 len, i;
    void *str_obj;
    iron_u16 *chars;
    
    if (!ctx || !utf8) return NULL;
    
    len = 0;
    while (utf8[len]) len++;
    
    str_obj = iron_alloc(ctx->allocator, sizeof(iron_u32) + len * sizeof(iron_u16));
    if (!str_obj) return NULL;
    
    *((iron_u32 *)str_obj) = len;
    chars = iron_string_get_chars(str_obj);
    for (i = 0; i < len; i++) {
        chars[i] = (iron_u16)(unsigned char)utf8[i];
    }
    
    return str_obj;
}

void *iron_string_new_utf16(iron_exec_context_t *ctx, 
                            const iron_u16 *src_chars, iron_u32 length)
{
    void *str_obj;
    iron_u16 *chars;
    
    if (!ctx) return NULL;
    
    str_obj = iron_alloc(ctx->allocator, sizeof(iron_u32) + length * sizeof(iron_u16));
    if (!str_obj) return NULL;
    
    *((iron_u32 *)str_obj) = length;
    
    if (src_chars && length > 0) {
        chars = iron_string_get_chars(str_obj);
        memcpy(chars, src_chars, length * sizeof(iron_u16));
    }
    
    return str_obj;
}

char *iron_string_to_utf8(iron_exec_context_t *ctx, void *str_obj)
{
    iron_u32 len, i;
    iron_u16 *chars;
    char *result;
    
    if (!ctx || !str_obj) return NULL;
    
    len = iron_string_get_length(str_obj);
    chars = iron_string_get_chars(str_obj);
    
    result = (char *)iron_alloc(ctx->allocator, len + 1);
    if (!result) return NULL;
    
    for (i = 0; i < len; i++) {
        result[i] = (chars[i] < 128) ? (char)chars[i] : '?';
    }
    result[len] = '\0';
    
    return result;
}

/* ============================================================================
 * System.String Internal Calls
 * ============================================================================ */

iron_result_t icall_String_get_Length(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    void *str_obj;
    
    (void)ctx;
    
    if (arg_count < 1 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "String.Length requires 'this'");
    }
    
    str_obj = (args[0].type == IRON_VAL_PTR) ? args[0].value.ptr : args[0].value.obj;
    
    result->type = IRON_VAL_I32;
    result->value.i32 = str_obj ? (iron_i32)iron_string_get_length(str_obj) : 0;
    
    return IRON_SUCCESS;
}

iron_result_t icall_String_get_Chars(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    void *str_obj;
    iron_i32 index;
    iron_u32 len;
    iron_u16 *chars;
    
    (void)ctx;
    
    if (arg_count < 2 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "String.Chars requires 'this' and index");
    }
    
    str_obj = (args[0].type == IRON_VAL_PTR) ? args[0].value.ptr : args[0].value.obj;
    index = args[1].value.i32;
    
    if (!str_obj) {
        return IRON_ERROR(IRON_ERR_NULL_REFERENCE, "NullReferenceException");
    }
    
    len = iron_string_get_length(str_obj);
    if (index < 0 || (iron_u32)index >= len) {
        return IRON_ERROR(IRON_ERR_INDEX_OUT_OF_RANGE, "IndexOutOfRangeException");
    }
    
    chars = iron_string_get_chars(str_obj);
    result->type = IRON_VAL_I32;
    result->value.i32 = chars[index];
    
    return IRON_SUCCESS;
}

iron_result_t icall_String_Concat(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    void *str1, *str2, *new_str;
    iron_u32 len1, len2, total_len;
    iron_u16 *chars1, *chars2, *new_chars;
    
    if (arg_count < 2 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "String.Concat requires two strings");
    }
    
    str1 = (args[0].type == IRON_VAL_PTR) ? args[0].value.ptr : args[0].value.obj;
    str2 = (args[1].type == IRON_VAL_PTR) ? args[1].value.ptr : args[1].value.obj;
    
    len1 = str1 ? iron_string_get_length(str1) : 0;
    len2 = str2 ? iron_string_get_length(str2) : 0;
    total_len = len1 + len2;
    
    if (total_len == 0) {
        result->type = IRON_VAL_PTR;
        result->value.ptr = iron_string_new_utf8(ctx, "");
        return IRON_SUCCESS;
    }
    
    new_str = iron_alloc(ctx->allocator, sizeof(iron_u32) + total_len * sizeof(iron_u16));
    if (!new_str) {
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate string");
    }
    
    *((iron_u32 *)new_str) = total_len;
    new_chars = iron_string_get_chars(new_str);
    
    if (len1 > 0) {
        chars1 = iron_string_get_chars(str1);
        memcpy(new_chars, chars1, len1 * sizeof(iron_u16));
    }
    if (len2 > 0) {
        chars2 = iron_string_get_chars(str2);
        memcpy(new_chars + len1, chars2, len2 * sizeof(iron_u16));
    }
    
    result->type = IRON_VAL_PTR;
    result->value.ptr = new_str;
    
    return IRON_SUCCESS;
}
