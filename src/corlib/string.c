/*
 * IronNet CLR Interpreter
 * corlib/string.c - System.String internal calls and helpers
 */

#include "iron/corlib.h"
#include "iron/exec.h"
#include "iron/gc.h"
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

void *iron_gc_alloc_string_utf8(iron_exec_context_t *ctx, const char *utf8)
{
    const iron_u8 *input;
    iron_size byte_length;
    iron_size input_index;
    iron_u32 utf16_length;
    void *str_obj;
    iron_u16 *chars;
    
    if (!ctx || !utf8) return NULL;

    byte_length = strlen(utf8);
    if (byte_length > UINT32_MAX) {
        return NULL;
    }

    chars = (iron_u16 *)iron_alloc(ctx->allocator, byte_length * sizeof(iron_u16));
    if (!chars && byte_length != 0) return NULL;

    input = (const iron_u8 *)utf8;
    input_index = 0;
    utf16_length = 0;
    while (input_index < byte_length) {
        iron_u32 code_point;
        iron_size sequence_length;
        iron_u8 first;

        first = input[input_index];
        code_point = 0xFFFD;
        sequence_length = 1;
        if (first < 0x80) {
            code_point = first;
        } else if (first >= 0xC2 && first <= 0xDF && input_index + 1 < byte_length && (input[input_index + 1] & 0xC0) == 0x80) {
            code_point = ((iron_u32)(first & 0x1F) << 6) | (iron_u32)(input[input_index + 1] & 0x3F);
            sequence_length = 2;
        } else if (first >= 0xE0 && first <= 0xEF && input_index + 2 < byte_length && (input[input_index + 1] & 0xC0) == 0x80 &&
                   (input[input_index + 2] & 0xC0) == 0x80 && !(first == 0xE0 && input[input_index + 1] < 0xA0) &&
                   !(first == 0xED && input[input_index + 1] >= 0xA0)) {
            code_point = ((iron_u32)(first & 0x0F) << 12) | ((iron_u32)(input[input_index + 1] & 0x3F) << 6) | (iron_u32)(input[input_index + 2] & 0x3F);
            sequence_length = 3;
        } else if (first >= 0xF0 && first <= 0xF4 && input_index + 3 < byte_length && (input[input_index + 1] & 0xC0) == 0x80 &&
                   (input[input_index + 2] & 0xC0) == 0x80 && (input[input_index + 3] & 0xC0) == 0x80 &&
                   !(first == 0xF0 && input[input_index + 1] < 0x90) && !(first == 0xF4 && input[input_index + 1] >= 0x90)) {
            code_point = ((iron_u32)(first & 0x07) << 18) | ((iron_u32)(input[input_index + 1] & 0x3F) << 12) |
                         ((iron_u32)(input[input_index + 2] & 0x3F) << 6) | (iron_u32)(input[input_index + 3] & 0x3F);
            sequence_length = 4;
        }

        input_index += sequence_length;
        if (code_point <= 0xFFFF) {
            chars[utf16_length++] = (iron_u16)code_point;
        } else {
            code_point -= 0x10000;
            chars[utf16_length++] = (iron_u16)(0xD800 + (code_point >> 10));
            chars[utf16_length++] = (iron_u16)(0xDC00 + (code_point & 0x3FF));
        }
    }

    str_obj = iron_gc_alloc_string(ctx, chars, utf16_length);
    if (chars) {
        iron_free(ctx->allocator, chars, byte_length * sizeof(iron_u16));
    }

    return str_obj;
}

void *iron_string_new_utf8(iron_exec_context_t *ctx, const char *utf8)
{
    return iron_gc_alloc_string_utf8(ctx, utf8);
}

void *iron_string_new_utf16(iron_exec_context_t *ctx, 
                            const iron_u16 *src_chars, iron_u32 length)
{
    if (!ctx) return NULL;

    return iron_gc_alloc_string(ctx, src_chars, length);
}

static iron_u32 utf16_next_code_point(const iron_u16 *characters, iron_u32 length, iron_u32 *index)
{
    iron_u32 code_point;

    code_point = characters[*index];
    if (code_point >= 0xD800 && code_point <= 0xDBFF && *index + 1 < length && characters[*index + 1] >= 0xDC00 && characters[*index + 1] <= 0xDFFF) {
        code_point = 0x10000 + ((code_point - 0xD800) << 10) + ((iron_u32)characters[*index + 1] - 0xDC00);
        (*index)++;
    } else if (code_point >= 0xD800 && code_point <= 0xDFFF) {
        code_point = 0xFFFD;
    }

    return code_point;
}

static iron_size utf8_code_point_size(iron_u32 code_point)
{
    if (code_point < 0x80) {
        return 1;
    }
    if (code_point < 0x800) {
        return 2;
    }
    if (code_point < 0x10000) {
        return 3;
    }

    return 4;
}

char *iron_string_to_utf8(iron_exec_context_t *ctx, void *str_obj)
{
    iron_u32 len, i;
    iron_u16 *chars;
    char *result;
    iron_size byte_length;
    iron_size output_index;
    
    if (!ctx || !str_obj) return NULL;
    
    len = iron_string_get_length(str_obj);
    chars = iron_string_get_chars(str_obj);
    
    byte_length = 0;
    for (i = 0; i < len; i++) {
        iron_u32 code_point;
        iron_size encoded_size;

        code_point = utf16_next_code_point(chars, len, &i);
        encoded_size = utf8_code_point_size(code_point);
        if (byte_length > (iron_size)-1 - encoded_size) {
            return NULL;
        }
        byte_length += encoded_size;
    }

    if (byte_length == (iron_size)-1) {
        return NULL;
    }

    result = (char *)iron_alloc(ctx->allocator, byte_length + 1);
    if (!result) return NULL;

    output_index = 0;
    for (i = 0; i < len; i++) {
        iron_u32 code_point;

        code_point = utf16_next_code_point(chars, len, &i);

        if (code_point < 0x80) {
            result[output_index++] = (char)code_point;
        } else if (code_point < 0x800) {
            result[output_index++] = (char)(0xC0 | (code_point >> 6));
            result[output_index++] = (char)(0x80 | (code_point & 0x3F));
        } else if (code_point < 0x10000) {
            result[output_index++] = (char)(0xE0 | (code_point >> 12));
            result[output_index++] = (char)(0x80 | ((code_point >> 6) & 0x3F));
            result[output_index++] = (char)(0x80 | (code_point & 0x3F));
        } else {
            result[output_index++] = (char)(0xF0 | (code_point >> 18));
            result[output_index++] = (char)(0x80 | ((code_point >> 12) & 0x3F));
            result[output_index++] = (char)(0x80 | ((code_point >> 6) & 0x3F));
            result[output_index++] = (char)(0x80 | (code_point & 0x3F));
        }
    }
    result[output_index] = '\0';
    
    return result;
}

/* ============================================================================
 * System.String Internal Calls
 * ============================================================================ */

static iron_result_t string_constructor_result(void *string_object, iron_stack_value_t *result)
{
    if (!string_object) {
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate string");
    }
    if (!result) {
        return IRON_ERROR(IRON_ERR_NULL_POINTER, "String constructor requires a result slot");
    }

    result->type = IRON_VAL_OBJ;
    result->value.obj = string_object;
    return IRON_SUCCESS;
}

iron_result_t icall_String_ctor_CharCount(iron_exec_context_t *ctx,
                                          iron_stack_value_t *args,
                                          iron_u32 arg_count,
                                          iron_stack_value_t *result)
{
    void *string_object;
    iron_u16 *characters;
    iron_i32 count;
    iron_u32 index;

    if (!ctx || !args || arg_count < 3) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "String(char, int) requires a character and count");
    }

    count = args[2].value.i32;
    if (count < 0) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "String character count cannot be negative");
    }

    string_object = iron_gc_alloc_string(ctx, NULL, (iron_u32)count);
    if (!string_object) {
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate string");
    }

    characters = iron_string_get_chars(string_object);
    for (index = 0; index < (iron_u32)count; index++) {
        characters[index] = (iron_u16)args[1].value.i32;
    }

    return string_constructor_result(string_object, result);
}

iron_result_t icall_String_ctor_CharArray(iron_exec_context_t *ctx,
                                          iron_stack_value_t *args,
                                          iron_u32 arg_count,
                                          iron_stack_value_t *result)
{
    void *array;
    iron_u32 length;

    if (!ctx || !args || arg_count < 2) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "String(char[]) requires a character array");
    }

    array = iron_corlib_object_argument(&args[1]);
    if (!array) {
        return IRON_ERROR(IRON_ERR_NULL_REFERENCE, "String character array cannot be null");
    }
    if (iron_array_get_element_size(array) != sizeof(iron_u16)) {
        return IRON_ERROR(IRON_ERR_INVALID_TYPE, "String constructor requires a character array");
    }

    length = iron_array_get_length(array);
    return string_constructor_result(iron_gc_alloc_string(ctx, (const iron_u16 *)iron_array_get_const_data(array), length), result);
}

iron_result_t icall_String_ctor_CharArrayRange(iron_exec_context_t *ctx,
                                               iron_stack_value_t *args,
                                               iron_u32 arg_count,
                                               iron_stack_value_t *result)
{
    void *array;
    const iron_u16 *characters;
    iron_u32 array_length;
    iron_i32 start_index;
    iron_i32 length;

    if (!ctx || !args || arg_count < 4) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "String(char[], int, int) requires an array, start index, and length");
    }

    array = iron_corlib_object_argument(&args[1]);
    if (!array) {
        return IRON_ERROR(IRON_ERR_NULL_REFERENCE, "String character array cannot be null");
    }
    if (iron_array_get_element_size(array) != sizeof(iron_u16)) {
        return IRON_ERROR(IRON_ERR_INVALID_TYPE, "String constructor requires a character array");
    }

    array_length = iron_array_get_length(array);
    start_index = args[2].value.i32;
    length = args[3].value.i32;
    if (start_index < 0 || (iron_u32)start_index > array_length) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "String start index is outside the character array");
    }
    if (length < 0 || (iron_u32)length > array_length - (iron_u32)start_index) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "String length is outside the character array");
    }

    characters = (const iron_u16 *)iron_array_get_const_data(array);
    return string_constructor_result(iron_gc_alloc_string(ctx, characters + start_index, (iron_u32)length), result);
}

iron_result_t icall_String_ctor_CharPointer(iron_exec_context_t *ctx,
                                            iron_stack_value_t *args,
                                            iron_u32 arg_count,
                                            iron_stack_value_t *result)
{
    const iron_u16 *characters;
    iron_size length;

    if (!ctx || !args || arg_count < 2) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "String(char*) requires a character pointer");
    }

    characters = (const iron_u16 *)iron_corlib_object_argument(&args[1]);
    if (!characters) {
        return IRON_ERROR(IRON_ERR_NULL_REFERENCE, "String character pointer cannot be null");
    }

    length = 0;
    while (characters[length] != 0) {
        if (length == UINT32_MAX) {
            return IRON_ERROR(IRON_ERR_ARITHMETIC_OVERFLOW, "String character pointer is too long");
        }
        length++;
    }

    return string_constructor_result(iron_gc_alloc_string(ctx, characters, (iron_u32)length), result);
}

iron_result_t icall_String_ctor_CharPointerRange(iron_exec_context_t *ctx,
                                                 iron_stack_value_t *args,
                                                 iron_u32 arg_count,
                                                 iron_stack_value_t *result)
{
    const iron_u16 *characters;
    iron_i32 start_index;
    iron_i32 length;

    if (!ctx || !args || arg_count < 4) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "String(char*, int, int) requires a pointer, start index, and length");
    }

    characters = (const iron_u16 *)iron_corlib_object_argument(&args[1]);
    start_index = args[2].value.i32;
    length = args[3].value.i32;
    if (!characters) {
        return IRON_ERROR(IRON_ERR_NULL_REFERENCE, "String character pointer cannot be null");
    }
    if (start_index < 0 || length < 0 || (iron_u32)start_index > UINT32_MAX - (iron_u32)length) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "String character pointer range is invalid");
    }

    return string_constructor_result(iron_gc_alloc_string(ctx, characters + start_index, (iron_u32)length), result);
}

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

iron_result_t icall_String_get_Item(
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

iron_result_t icall_String_Equals(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    void *str1, *str2;
    iron_u32 len1, len2, i;
    iron_u16 *chars1, *chars2;

    (void)ctx;

    if (arg_count < 2 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "String.Equals requires two strings");
    }

    str1 = (args[0].type == IRON_VAL_PTR) ? args[0].value.ptr : args[0].value.obj;
    str2 = (args[1].type == IRON_VAL_PTR) ? args[1].value.ptr : args[1].value.obj;

    if (str1 == str2) {
        result->type = IRON_VAL_I32;
        result->value.i32 = 1;
        return IRON_SUCCESS;
    }

    if (!str1 || !str2) {
        result->type = IRON_VAL_I32;
        result->value.i32 = 0;
        return IRON_SUCCESS;
    }

    len1 = iron_string_get_length(str1);
    len2 = iron_string_get_length(str2);

    if (len1 != len2) {
        result->type = IRON_VAL_I32;
        result->value.i32 = 0;
        return IRON_SUCCESS;
    }

    chars1 = iron_string_get_chars(str1);
    chars2 = iron_string_get_chars(str2);

    for (i = 0; i < len1; i++) {
        if (chars1[i] != chars2[i]) {
            result->type = IRON_VAL_I32;
            result->value.i32 = 0;
            return IRON_SUCCESS;
        }
    }

    result->type = IRON_VAL_I32;
    result->value.i32 = 1;
    return IRON_SUCCESS;
}

iron_result_t icall_String_InternalAllocateStr(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    iron_i32 length;
    void *str_obj;

    if (arg_count < 1 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "InternalAllocateStr requires length");
    }

    length = args[0].value.i32;
    if (length < 0) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "String length cannot be negative");
    }

    /* Allocate a string with the given length, chars zeroed */
    str_obj = iron_gc_alloc_string(ctx, NULL, (iron_u32)length);
    if (!str_obj) {
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate string");
    }

    result->type = IRON_VAL_OBJ;
    result->value.obj = str_obj;

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
        result->type = IRON_VAL_OBJ;
        result->value.obj = iron_string_new_utf8(ctx, "");
        return IRON_SUCCESS;
    }

    if (total_len < len1) {
        return IRON_ERROR(IRON_ERR_ARITHMETIC_OVERFLOW, "Combined string length overflow");
    }

    new_str = iron_gc_alloc_string(ctx, NULL, total_len);
    if (!new_str) {
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate string");
    }

    new_chars = iron_string_get_chars(new_str);
    
    if (len1 > 0) {
        chars1 = iron_string_get_chars(str1);
        memcpy(new_chars, chars1, len1 * sizeof(iron_u16));
    }
    if (len2 > 0) {
        chars2 = iron_string_get_chars(str2);
        memcpy(new_chars + len1, chars2, len2 * sizeof(iron_u16));
    }
    
    result->type = IRON_VAL_OBJ;
    result->value.obj = new_str;
    
    return IRON_SUCCESS;
}
