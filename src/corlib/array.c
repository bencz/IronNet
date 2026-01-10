/*
 * IronNet CLR Interpreter
 * corlib/array.c - System.Array internal calls
 */

#include "iron/corlib.h"

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
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Array.Length requires 'this'");
    }
    
    arr = args[0].value.obj;
    if (!arr) {
        return IRON_ERROR(IRON_ERR_NULL_REFERENCE, "NullReferenceException");
    }
    
    result->type = IRON_VAL_I32;
    result->value.i32 = *((iron_i32 *)arr);
    
    return IRON_SUCCESS;
}

iron_result_t icall_Array_get_Rank(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    (void)ctx;
    (void)args;
    (void)arg_count;
    
    if (!result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Array.Rank requires result");
    }
    
    result->type = IRON_VAL_I32;
    result->value.i32 = 1;
    
    return IRON_SUCCESS;
}

iron_result_t icall_Array_Copy(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    void *src_arr;
    void *dst_arr;
    iron_i32 length;
    iron_i32 src_len;
    iron_i32 dst_len;
    void **src_elements;
    void **dst_elements;
    iron_i32 i;
    
    (void)ctx;
    (void)result;
    
    if (arg_count < 3) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Array.Copy requires 3 arguments");
    }
    
    src_arr = args[0].value.obj;
    dst_arr = args[1].value.obj;
    length = args[2].value.i32;
    
    if (!src_arr || !dst_arr) {
        return IRON_ERROR(IRON_ERR_NULL_REFERENCE, "NullReferenceException");
    }
    
    src_len = *((iron_i32 *)src_arr);
    dst_len = *((iron_i32 *)dst_arr);
    
    if (length > src_len || length > dst_len) {
        return IRON_ERROR(IRON_ERR_INDEX_OUT_OF_RANGE, "ArgumentException: length exceeds array bounds");
    }
    
    /* Copy elements (pointer-sized) */
    src_elements = (void **)((iron_u8 *)src_arr + sizeof(iron_i32));
    dst_elements = (void **)((iron_u8 *)dst_arr + sizeof(iron_i32));
    
    for (i = 0; i < length; i++) {
        dst_elements[i] = src_elements[i];
    }
    
    return IRON_SUCCESS;
}
