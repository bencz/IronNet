/*
 * IronNet CLR Interpreter
 * corlib.c - Base Class Library internal call implementations
 */

#include "iron/corlib.h"
#include "iron/gc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#if defined(_WIN32)
    #include <windows.h>
#else
    #include <unistd.h>
#endif

/* ============================================================================
 * String Helpers Implementation
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
 * System.Object Internal Calls
 * ============================================================================ */

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
    void *obj;
    
    (void)ctx;
    
    if (arg_count < 1 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "GetHashCode requires 'this'");
    }
    
    obj = args[0].value.obj;
    if (!obj) {
        return IRON_ERROR(IRON_ERR_NULL_REFERENCE, "NullReferenceException");
    }
    
    result->type = IRON_VAL_I32;
    result->value.i32 = (iron_i32)(iron_size)obj;
    
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
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Equals requires 2 arguments");
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
    void *obj, *clone;
    iron_size size;
    
    if (arg_count < 1 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "MemberwiseClone requires 'this'");
    }
    
    obj = args[0].value.obj;
    if (!obj) {
        return IRON_ERROR(IRON_ERR_NULL_REFERENCE, "NullReferenceException");
    }
    
    size = iron_gc_get_size(obj);
    clone = iron_alloc(ctx->allocator, size);
    if (!clone) {
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate clone");
    }
    
    memcpy(clone, obj, size);
    result->type = IRON_VAL_OBJ;
    result->value.obj = clone;
    
    return IRON_SUCCESS;
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
    (void)ctx;
    
    if (arg_count < 1 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "get_Length requires 'this'");
    }
    
    if (!args[0].value.obj) {
        return IRON_ERROR(IRON_ERR_NULL_REFERENCE, "NullReferenceException");
    }
    
    result->type = IRON_VAL_I32;
    result->value.i32 = (iron_i32)iron_string_get_length(args[0].value.obj);
    
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
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "get_Chars requires 2 arguments");
    }
    
    str_obj = args[0].value.obj;
    if (!str_obj) {
        return IRON_ERROR(IRON_ERR_NULL_REFERENCE, "NullReferenceException");
    }
    
    index = args[1].value.i32;
    len = iron_string_get_length(str_obj);
    
    if (index < 0 || (iron_u32)index >= len) {
        return IRON_ERROR(IRON_ERR_INDEX_OUT_OF_RANGE, "IndexOutOfRangeException");
    }
    
    chars = iron_string_get_chars(str_obj);
    result->type = IRON_VAL_I32;
    result->value.i32 = (iron_i32)chars[index];
    
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
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Concat requires 2 arguments");
    }
    
    str1 = args[0].value.obj;
    str2 = args[1].value.obj;
    
    len1 = str1 ? iron_string_get_length(str1) : 0;
    len2 = str2 ? iron_string_get_length(str2) : 0;
    total_len = len1 + len2;
    
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
    
    result->type = IRON_VAL_OBJ;
    result->value.obj = new_str;
    
    return IRON_SUCCESS;
}

iron_result_t icall_String_Equals(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    void *str1, *str2;
    iron_u32 len1, len2;
    iron_u16 *chars1, *chars2;
    
    (void)ctx;
    
    if (arg_count < 2 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Equals requires 2 arguments");
    }
    
    str1 = args[0].value.obj;
    str2 = args[1].value.obj;
    
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
    
    result->type = IRON_VAL_I32;
    result->value.i32 = (memcmp(chars1, chars2, len1 * sizeof(iron_u16)) == 0) ? 1 : 0;
    
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
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Length cannot be negative");
    }
    
    str_obj = iron_alloc(ctx->allocator, sizeof(iron_u32) + length * sizeof(iron_u16));
    if (!str_obj) {
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate string");
    }
    
    *((iron_u32 *)str_obj) = (iron_u32)length;
    memset(iron_string_get_chars(str_obj), 0, length * sizeof(iron_u16));
    
    result->type = IRON_VAL_OBJ;
    result->value.obj = str_obj;
    
    return IRON_SUCCESS;
}

/* ============================================================================
 * System.Console Internal Calls
 * ============================================================================ */

iron_result_t icall_Console_WriteLine_String(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    void *str_ptr;
    const iron_u16 *utf16;
    iron_u32 i;
    iron_u32 max_chars = 4096;
    
    (void)ctx;
    (void)result;
    
    if (arg_count < 1) {
        printf("\n");
        return IRON_SUCCESS;
    }
    
    /* Handle both PTR (from ldstr) and OBJ types */
    str_ptr = (args[0].type == IRON_VAL_PTR) ? args[0].value.ptr : args[0].value.obj;
    
    if (!str_ptr) {
        printf("\n");
        return IRON_SUCCESS;
    }
    
    /* Print UTF-16 string as ASCII - stop at null or max chars */
    utf16 = (const iron_u16 *)str_ptr;
    for (i = 0; i < max_chars && utf16[i] != 0; i++) {
        putchar((char)utf16[i]);
    }
    putchar('\n');
    fflush(stdout);
    
    return IRON_SUCCESS;
}

iron_result_t icall_Console_WriteLine_Int32(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    (void)ctx;
    (void)result;
    
    if (arg_count < 1) {
        printf("\n");
        return IRON_SUCCESS;
    }
    
    printf("%d\n", (int)args[0].value.i32);
    
    return IRON_SUCCESS;
}

iron_result_t icall_Console_WriteLine_Object(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    (void)result;
    
    if (arg_count < 1 || !args[0].value.obj) {
        printf("\n");
        return IRON_SUCCESS;
    }
    
    /* TODO: Call ToString() on the object */
    printf("[Object@%p]\n", args[0].value.obj);
    
    (void)ctx;
    return IRON_SUCCESS;
}

iron_result_t icall_Console_Write_String(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    char *utf8;
    
    (void)result;
    
    if (arg_count < 1 || !args[0].value.obj) {
        return IRON_SUCCESS;
    }
    
    utf8 = iron_string_to_utf8(ctx, args[0].value.obj);
    if (utf8) {
        printf("%s", utf8);
        fflush(stdout);
        iron_free(ctx->allocator, utf8, strlen(utf8) + 1);
    }
    
    return IRON_SUCCESS;
}

iron_result_t icall_Console_ReadLine(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    char buffer[4096];
    void *str_obj;
    
    (void)args;
    (void)arg_count;
    
    if (!result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "ReadLine requires result");
    }
    
    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        result->type = IRON_VAL_OBJ;
        result->value.obj = NULL;
        return IRON_SUCCESS;
    }
    
    /* Remove trailing newline */
    {
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
        }
    }
    
    str_obj = iron_string_new_utf8(ctx, buffer);
    result->type = IRON_VAL_OBJ;
    result->value.obj = str_obj;
    
    return IRON_SUCCESS;
}

/* ============================================================================
 * System.Environment Internal Calls
 * ============================================================================ */

iron_result_t icall_Environment_get_TickCount(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    (void)ctx;
    (void)args;
    (void)arg_count;
    
    if (!result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "get_TickCount requires result");
    }
    
    result->type = IRON_VAL_I32;
    result->value.i32 = (iron_i32)(clock() * 1000 / CLOCKS_PER_SEC);
    
    return IRON_SUCCESS;
}

iron_result_t icall_Environment_Exit(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    int exit_code = 0;
    
    (void)ctx;
    (void)result;
    
    if (arg_count >= 1) {
        exit_code = (int)args[0].value.i32;
    }
    
    exit(exit_code);
    
    return IRON_SUCCESS;
}

iron_result_t icall_Environment_get_CurrentDirectory(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    char buffer[4096];
    void *str_obj;
    
    (void)args;
    (void)arg_count;
    
    if (!result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "get_CurrentDirectory requires result");
    }
    
#if defined(_WIN32)
    if (GetCurrentDirectoryA(sizeof(buffer), buffer) == 0) {
        buffer[0] = '.';
        buffer[1] = '\0';
    }
#else
    if (getcwd(buffer, sizeof(buffer)) == NULL) {
        buffer[0] = '.';
        buffer[1] = '\0';
    }
#endif
    
    str_obj = iron_string_new_utf8(ctx, buffer);
    result->type = IRON_VAL_OBJ;
    result->value.obj = str_obj;
    
    return IRON_SUCCESS;
}

/* ============================================================================
 * System.GC Internal Calls
 * ============================================================================ */

iron_result_t icall_GC_Collect(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    (void)ctx;
    (void)args;
    (void)arg_count;
    (void)result;
    
    /* TODO: Trigger GC collection */
    
    return IRON_SUCCESS;
}

iron_result_t icall_GC_GetTotalMemory(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    (void)args;
    (void)arg_count;
    
    if (!result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "GetTotalMemory requires result");
    }
    
    result->type = IRON_VAL_I64;
    result->value.i64 = (iron_i64)ctx->gc.total_allocated;
    
    return IRON_SUCCESS;
}

iron_result_t icall_GC_SuppressFinalize(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    (void)ctx;
    (void)result;
    
    if (arg_count < 1) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "SuppressFinalize requires object");
    }
    
    if (args[0].value.obj) {
        iron_gc_suppress_finalize(args[0].value.obj);
    }
    
    return IRON_SUCCESS;
}

/* ============================================================================
 * System.Array Internal Calls
 * ============================================================================ */

iron_result_t icall_Array_get_Length(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    void *arr;
    
    (void)ctx;
    
    if (arg_count < 1 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "get_Length requires 'this'");
    }
    
    arr = args[0].value.obj;
    if (!arr) {
        return IRON_ERROR(IRON_ERR_NULL_REFERENCE, "NullReferenceException");
    }
    
    /* Array layout: [length (4 bytes)] [elements...] */
    result->type = IRON_VAL_I32;
    result->value.i32 = (iron_i32)*((iron_u32 *)arr);
    
    return IRON_SUCCESS;
}

iron_result_t icall_Array_GetValue(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    /* TODO: Implement array element access */
    (void)ctx;
    (void)args;
    (void)arg_count;
    (void)result;
    
    return IRON_ERROR(IRON_ERR_NOT_IMPLEMENTED, "Array.GetValue not implemented");
}

iron_result_t icall_Array_SetValue(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    /* TODO: Implement array element assignment */
    (void)ctx;
    (void)args;
    (void)arg_count;
    (void)result;
    
    return IRON_ERROR(IRON_ERR_NOT_IMPLEMENTED, "Array.SetValue not implemented");
}

iron_result_t icall_Array_Copy(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    /* TODO: Implement array copy */
    (void)ctx;
    (void)args;
    (void)arg_count;
    (void)result;
    
    return IRON_ERROR(IRON_ERR_NOT_IMPLEMENTED, "Array.Copy not implemented");
}

/* ============================================================================
 * System.Math Internal Calls
 * ============================================================================ */

iron_result_t icall_Math_Sin(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    (void)ctx;
    
    if (arg_count < 1 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Sin requires 1 argument");
    }
    
    result->type = IRON_VAL_F64;
    result->value.f64 = sin(args[0].value.f64);
    
    return IRON_SUCCESS;
}

iron_result_t icall_Math_Cos(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    (void)ctx;
    
    if (arg_count < 1 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Cos requires 1 argument");
    }
    
    result->type = IRON_VAL_F64;
    result->value.f64 = cos(args[0].value.f64);
    
    return IRON_SUCCESS;
}

iron_result_t icall_Math_Sqrt(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    (void)ctx;
    
    if (arg_count < 1 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Sqrt requires 1 argument");
    }
    
    result->type = IRON_VAL_F64;
    result->value.f64 = sqrt(args[0].value.f64);
    
    return IRON_SUCCESS;
}

iron_result_t icall_Math_Abs_Int32(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    (void)ctx;
    
    if (arg_count < 1 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Abs requires 1 argument");
    }
    
    result->type = IRON_VAL_I32;
    result->value.i32 = args[0].value.i32 < 0 ? -args[0].value.i32 : args[0].value.i32;
    
    return IRON_SUCCESS;
}

iron_result_t icall_Math_Abs_Double(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    (void)ctx;
    
    if (arg_count < 1 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Abs requires 1 argument");
    }
    
    result->type = IRON_VAL_F64;
    result->value.f64 = fabs(args[0].value.f64);
    
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
    (void)ctx;
    
    if (arg_count < 1 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "GetTypeFromHandle requires handle");
    }
    
    /* RuntimeTypeHandle contains a pointer to the type */
    result->type = IRON_VAL_OBJ;
    result->value.obj = args[0].value.obj;
    
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
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "get_Name requires 'this'");
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
    char buffer[512];
    void *str_obj;
    
    if (arg_count < 1 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "get_FullName requires 'this'");
    }
    
    type = (iron_runtime_type_t *)args[0].value.obj;
    if (!type) {
        return IRON_ERROR(IRON_ERR_NULL_REFERENCE, "NullReferenceException");
    }
    
    if (type->namespace_ && type->namespace_[0]) {
        snprintf(buffer, sizeof(buffer), "%s.%s", 
                 type->namespace_, type->name ? type->name : "");
    } else {
        snprintf(buffer, sizeof(buffer), "%s", type->name ? type->name : "");
    }
    
    str_obj = iron_string_new_utf8(ctx, buffer);
    result->type = IRON_VAL_OBJ;
    result->value.obj = str_obj;
    
    return IRON_SUCCESS;
}

/* ============================================================================
 * Registration
 * ============================================================================ */

iron_result_t iron_register_corlib(iron_exec_context_t *ctx)
{
    static const iron_internal_call_t corlib_calls[] = {
        /* System.Object */
        { "System.Object", "GetType", "", icall_Object_GetType },
        { "System.Object", "GetHashCode", "", icall_Object_GetHashCode },
        { "System.Object", "Equals", "System.Object", icall_Object_Equals },
        { "System.Object", "MemberwiseClone", "", icall_Object_MemberwiseClone },
        
        /* System.String */
        { "System.String", "get_Length", "", icall_String_get_Length },
        { "System.String", "get_Chars", "System.Int32", icall_String_get_Chars },
        { "System.String", "Concat", "System.String,System.String", icall_String_Concat },
        { "System.String", "Equals", "System.String", icall_String_Equals },
        { "System.String", "InternalAllocateStr", "System.Int32", icall_String_InternalAllocateStr },
        
        /* System.Console */
        { "System.Console", "WriteLine", "System.String", icall_Console_WriteLine_String },
        { "System.Console", "WriteLine", "System.Int32", icall_Console_WriteLine_Int32 },
        { "System.Console", "WriteLine", "System.Object", icall_Console_WriteLine_Object },
        { "System.Console", "WriteLine", "", icall_Console_WriteLine_String },
        { "System.Console", "Write", "System.String", icall_Console_Write_String },
        { "System.Console", "ReadLine", "", icall_Console_ReadLine },
        
        /* System.Environment */
        { "System.Environment", "get_TickCount", "", icall_Environment_get_TickCount },
        { "System.Environment", "Exit", "System.Int32", icall_Environment_Exit },
        { "System.Environment", "get_CurrentDirectory", "", icall_Environment_get_CurrentDirectory },
        
        /* System.GC */
        { "System.GC", "Collect", "", icall_GC_Collect },
        { "System.GC", "GetTotalMemory", "System.Boolean", icall_GC_GetTotalMemory },
        { "System.GC", "SuppressFinalize", "System.Object", icall_GC_SuppressFinalize },
        
        /* System.Array */
        { "System.Array", "get_Length", "", icall_Array_get_Length },
        { "System.Array", "GetValue", "System.Int32", icall_Array_GetValue },
        { "System.Array", "SetValue", "System.Object,System.Int32", icall_Array_SetValue },
        { "System.Array", "Copy", "System.Array,System.Array,System.Int32", icall_Array_Copy },
        
        /* System.Math */
        { "System.Math", "Sin", "System.Double", icall_Math_Sin },
        { "System.Math", "Cos", "System.Double", icall_Math_Cos },
        { "System.Math", "Sqrt", "System.Double", icall_Math_Sqrt },
        { "System.Math", "Abs", "System.Int32", icall_Math_Abs_Int32 },
        { "System.Math", "Abs", "System.Double", icall_Math_Abs_Double },
        
        /* System.Type */
        { "System.Type", "GetTypeFromHandle", "System.RuntimeTypeHandle", icall_Type_GetTypeFromHandle },
        { "System.Type", "get_Name", "", icall_Type_get_Name },
        { "System.Type", "get_FullName", "", icall_Type_get_FullName }
    };
    
    return iron_register_internal_calls(ctx, corlib_calls,
                                        sizeof(corlib_calls) / sizeof(corlib_calls[0]));
}
