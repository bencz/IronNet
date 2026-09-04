/*
 * IronNet CLR Interpreter
 * corlib/array.c - System.Array internal calls
 */

#include "iron/corlib.h"
#include <limits.h>

/* ============================================================================
 * System.Array Internal Calls
 * ============================================================================ */

static iron_result_t get_array_indices(void *array,
                                       iron_stack_value_t *args,
                                       iron_u32 first_index,
                                       iron_u32 argument_count,
                                       iron_i32 *indices,
                                       iron_u32 *index_count)
{
    iron_u32 rank;
    iron_u32 index;

    if (!array || !args || !indices || !index_count) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Invalid array index arguments");
    }

    rank = iron_array_get_rank(array);
    if (rank == 0 || rank > 32 || first_index > argument_count) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Invalid array rank");
    }

    if (argument_count - first_index == 1 && args[first_index].type == IRON_VAL_OBJ) {
        void *index_array;
        iron_runtime_type_t *index_element_type;

        index_array = args[first_index].value.obj;
        if (!index_array) {
            return IRON_ERROR(IRON_ERR_ARGUMENT_NULL, "Value cannot be null. Parameter name: indices");
        }

        index_element_type = iron_array_get_element_type(index_array);
        if (!index_element_type || index_element_type->element_type != IRON_TYPE_I4 || iron_array_get_rank(index_array) != 1 || iron_array_get_length(index_array) != rank) {
            return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "The number of indices must match the array rank");
        }

        iron_memcpy(indices, iron_array_get_const_data(index_array), (iron_size)rank * sizeof(iron_i32));
        *index_count = rank;
        return IRON_SUCCESS;
    }

    if (argument_count - first_index != rank) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "The number of indices must match the array rank");
    }

    for (index = 0; index < rank; index++) {
        if (args[first_index + index].type != IRON_VAL_I32) {
            return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Array indices must be Int32 values");
        }
        indices[index] = args[first_index + index].value.i32;
    }

    *index_count = rank;
    return IRON_SUCCESS;
}

static iron_result_t get_array_element_storage(void *array,
                                               iron_stack_value_t *args,
                                               iron_u32 first_index,
                                               iron_u32 argument_count,
                                               iron_runtime_type_t **element_type,
                                               void **storage)
{
    iron_i32 indices[32];
    iron_u32 index_count;
    iron_size offset;
    iron_result_t result;

    result = get_array_indices(array, args, first_index, argument_count, indices, &index_count);
    if (!IRON_RESULT_OK(result)) {
        return result;
    }

    if (!iron_array_get_element_offset(array, indices, index_count, &offset)) {
        return IRON_ERROR(IRON_ERR_INDEX_OUT_OF_RANGE, "IndexOutOfRangeException");
    }

    *element_type = iron_array_get_element_type(array);
    if (!*element_type) {
        return IRON_ERROR(IRON_ERR_INVALID_TYPE, "Array has no element type");
    }
    *storage = (iron_u8 *)iron_array_get_data(array) + offset;
    return IRON_SUCCESS;
}

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
    result->value.i32 = (iron_i32)iron_array_get_length(arr);
    
    return IRON_SUCCESS;
}

iron_result_t icall_Array_get_Rank(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    (void)ctx;
    if (arg_count < 1 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Array.Rank requires 'this'");
    }

    if (!args[0].value.obj) {
        return IRON_ERROR(IRON_ERR_NULL_REFERENCE, "NullReferenceException");
    }
    
    result->type = IRON_VAL_I32;
    result->value.i32 = (iron_i32)iron_array_get_rank(args[0].value.obj);
    
    return IRON_SUCCESS;
}

iron_result_t icall_Array_GetLength(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    iron_i32 dimension;
    iron_u32 length;

    (void)ctx;
    if (!args || arg_count != 2 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Array.GetLength requires this and a dimension");
    }
    if (!args[0].value.obj) {
        return IRON_ERROR(IRON_ERR_NULL_REFERENCE, "NullReferenceException");
    }

    dimension = args[1].value.i32;
    if (args[1].type != IRON_VAL_I32 || dimension < 0 || !iron_array_get_dimension_length(args[0].value.obj, (iron_u32)dimension, &length)) {
        return IRON_ERROR(IRON_ERR_INDEX_OUT_OF_RANGE, "IndexOutOfRangeException");
    }

    result->type = IRON_VAL_I32;
    result->value.i32 = (iron_i32)length;
    return IRON_SUCCESS;
}

iron_result_t icall_Array_GetLowerBound(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    iron_i32 dimension;
    iron_i32 lower_bound;

    (void)ctx;
    if (!args || arg_count != 2 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Array.GetLowerBound requires this and a dimension");
    }
    if (!args[0].value.obj) {
        return IRON_ERROR(IRON_ERR_NULL_REFERENCE, "NullReferenceException");
    }

    dimension = args[1].value.i32;
    if (args[1].type != IRON_VAL_I32 || dimension < 0 || !iron_array_get_lower_bound(args[0].value.obj, (iron_u32)dimension, &lower_bound)) {
        return IRON_ERROR(IRON_ERR_INDEX_OUT_OF_RANGE, "IndexOutOfRangeException");
    }

    result->type = IRON_VAL_I32;
    result->value.i32 = lower_bound;
    return IRON_SUCCESS;
}

iron_result_t icall_Array_GetUpperBound(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    iron_i32 dimension;
    iron_i32 lower_bound;
    iron_u32 length;

    (void)ctx;
    if (!args || arg_count != 2 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Array.GetUpperBound requires this and a dimension");
    }
    if (!args[0].value.obj) {
        return IRON_ERROR(IRON_ERR_NULL_REFERENCE, "NullReferenceException");
    }

    dimension = args[1].value.i32;
    if (args[1].type != IRON_VAL_I32 || dimension < 0 ||
        !iron_array_get_lower_bound(args[0].value.obj, (iron_u32)dimension, &lower_bound) ||
        !iron_array_get_dimension_length(args[0].value.obj, (iron_u32)dimension, &length)) {
        return IRON_ERROR(IRON_ERR_INDEX_OUT_OF_RANGE, "IndexOutOfRangeException");
    }

    result->type = IRON_VAL_I32;
    result->value.i32 = (iron_i32)((iron_i64)lower_bound + (iron_i64)length - 1);
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
    iron_i32 src_index;
    iron_i32 dst_index;
    iron_i32 length;
    iron_u32 src_len;
    iron_u32 dst_len;
    iron_runtime_type_t *src_element_type;
    iron_runtime_type_t *dst_element_type;
    iron_size src_element_size;
    iron_size dst_element_size;
    iron_size copy_size;
    iron_bool src_is_reference;
    iron_bool dst_is_reference;
    iron_u32 element_index;

    (void)result;

    if (arg_count == 3) {
        src_arr = args[0].value.obj;
        src_index = 0;
        dst_arr = args[1].value.obj;
        dst_index = 0;
        length = args[2].value.i32;
    } else if (arg_count == 5) {
        src_arr = args[0].value.obj;
        src_index = args[1].value.i32;
        dst_arr = args[2].value.obj;
        dst_index = args[3].value.i32;
        length = args[4].value.i32;
    } else {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Array.Copy requires 3 or 5 arguments");
    }

    if (!src_arr || !dst_arr) {
        return IRON_ERROR(IRON_ERR_ARGUMENT_NULL, "Source and destination arrays cannot be null");
    }

    src_len = iron_array_get_length(src_arr);
    dst_len = iron_array_get_length(dst_arr);
    if (src_index < 0 || dst_index < 0 || length < 0 || (iron_u32)src_index > src_len || (iron_u32)dst_index > dst_len ||
        (iron_u32)length > src_len - (iron_u32)src_index || (iron_u32)length > dst_len - (iron_u32)dst_index) {
        return IRON_ERROR(IRON_ERR_INDEX_OUT_OF_RANGE, "ArgumentException: length exceeds array bounds");
    }

    src_element_type = iron_array_get_element_type(src_arr);
    dst_element_type = iron_array_get_element_type(dst_arr);
    src_element_size = iron_array_get_element_size(src_arr);
    dst_element_size = iron_array_get_element_size(dst_arr);
    src_is_reference = src_element_type && iron_type_is_managed_reference(src_element_type);
    dst_is_reference = dst_element_type && iron_type_is_managed_reference(dst_element_type);
    if (src_is_reference != dst_is_reference) {
        return IRON_ERROR(IRON_ERR_INVALID_CAST, "ArrayTypeMismatchException");
    }

    if (!src_is_reference) {
        iron_u8 *destination;
        const iron_u8 *source;

        if (src_element_type != dst_element_type || src_element_size != dst_element_size) {
            return IRON_ERROR(IRON_ERR_INVALID_CAST, "ArrayTypeMismatchException");
        }

        source = (const iron_u8 *)iron_array_get_const_data(src_arr) + (iron_size)(iron_u32)src_index * src_element_size;
        destination = (iron_u8 *)iron_array_get_data(dst_arr) + (iron_size)(iron_u32)dst_index * dst_element_size;
        copy_size = src_element_size * (iron_size)(iron_u32)length;
        iron_memmove(destination, source, copy_size);
        return IRON_SUCCESS;
    }

    {
        void **source;
        void **destination;

        source = (void **)iron_array_get_data(src_arr) + src_index;
        destination = (void **)iron_array_get_data(dst_arr) + dst_index;

        for (element_index = 0; element_index < (iron_u32)length; element_index++) {
            if (source[element_index] && dst_element_type && !iron_managed_reference_is_assignable(ctx->domain, source[element_index], dst_element_type)) {
                return IRON_ERROR(IRON_ERR_INVALID_CAST, "ArrayTypeMismatchException");
            }
        }

        if (src_arr == dst_arr) {
            copy_size = sizeof(void *) * (iron_size)(iron_u32)length;
            iron_memmove(destination, source, copy_size);
            for (element_index = 0; element_index < (iron_u32)length; element_index++) {
                iron_gc_write_barrier(dst_arr, &destination[element_index], destination[element_index]);
            }
        } else {
            for (element_index = 0; element_index < (iron_u32)length; element_index++) {
                destination[element_index] = source[element_index];
                iron_gc_write_barrier(dst_arr, &destination[element_index], destination[element_index]);
            }
        }
    }

    return IRON_SUCCESS;
}

iron_result_t icall_Array_Clear(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    void *array;
    iron_i32 index;
    iron_i32 length;
    iron_u32 array_length;
    iron_size element_size;
    iron_u8 *destination;

    (void)ctx;
    (void)result;

    if (arg_count < 3) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Array.Clear requires array, index, and length");
    }

    array = args[0].value.obj;
    index = args[1].value.i32;
    length = args[2].value.i32;
    if (!array) {
        return IRON_ERROR(IRON_ERR_ARGUMENT_NULL, "Value cannot be null. Parameter name: array");
    }

    array_length = iron_array_get_length(array);
    if (index < 0 || length < 0 || (iron_u32)index > array_length || (iron_u32)length > array_length - (iron_u32)index) {
        return IRON_ERROR(IRON_ERR_INDEX_OUT_OF_RANGE, "ArgumentException: range exceeds array bounds");
    }

    element_size = iron_array_get_element_size(array);
    destination = (iron_u8 *)iron_array_get_data(array) + (iron_size)(iron_u32)index * element_size;
    iron_memset(destination, 0, (iron_size)(iron_u32)length * element_size);
    return IRON_SUCCESS;
}

iron_result_t icall_Array_GetValue(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    void *arr;
    iron_runtime_type_t *element_type;
    void *element_storage;
    iron_result_t storage_result;

    if (arg_count < 2 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Array.GetValue requires this and indices");
    }

    arr = args[0].value.obj;
    if (!arr) {
        return IRON_ERROR(IRON_ERR_NULL_REFERENCE, "NullReferenceException");
    }

    storage_result = get_array_element_storage(arr, args, 1, arg_count, &element_type, &element_storage);
    if (!IRON_RESULT_OK(storage_result)) {
        return storage_result;
    }

    result->type = IRON_VAL_OBJ;
    if (!iron_type_is_managed_reference(element_type)) {
        result->value.obj = iron_gc_box(ctx, element_type, element_storage);
        if (!result->value.obj) {
            return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to box array element");
        }
    } else {
        iron_memcpy(&result->value.obj, element_storage, sizeof(result->value.obj));
    }

    return IRON_SUCCESS;
}

iron_result_t icall_Array_SetValue(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    void *arr;
    void *value;
    iron_runtime_type_t *element_type;
    void *element_storage;
    iron_result_t storage_result;

    if (arg_count < 3) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Array.SetValue requires this, a value, and indices");
    }

    arr = args[0].value.obj;
    value = args[1].value.obj;
    if (!arr) {
        return IRON_ERROR(IRON_ERR_NULL_REFERENCE, "NullReferenceException");
    }

    storage_result = get_array_element_storage(arr, args, 2, arg_count, &element_type, &element_storage);
    if (!IRON_RESULT_OK(storage_result)) {
        return storage_result;
    }

    if (!iron_type_is_managed_reference(element_type)) {
        void *unboxed;

        if (!value || iron_managed_reference_get_type(ctx->domain, value) != element_type) {
            return IRON_ERROR(IRON_ERR_INVALID_CAST, "ArrayTypeMismatchException");
        }

        unboxed = iron_gc_unbox(value, element_type);
        if (!unboxed) {
            return IRON_ERROR(IRON_ERR_INVALID_CAST, "ArrayTypeMismatchException");
        }
        iron_memcpy(element_storage, unboxed, iron_array_get_element_size(arr));
    } else {
        if (value && !iron_managed_reference_is_assignable(ctx->domain, value, element_type)) {
            return IRON_ERROR(IRON_ERR_INVALID_CAST, "ArrayTypeMismatchException");
        }
        iron_gc_write_barrier(arr, (void **)element_storage, value);
    }

    if (result) {
        result->type = IRON_VAL_VOID;
    }

    return IRON_SUCCESS;
}

iron_result_t icall_Array_CreateInstance(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    iron_runtime_type_t *element_type;
    iron_runtime_type_t *array_type;
    void *length_array;
    void *lower_bound_array;
    const iron_i32 *managed_lengths;
    const iron_i32 *managed_lower_bounds;
    iron_u32 lengths[32];
    iron_i32 lower_bounds[32];
    iron_u32 rank;
    iron_u32 dimension;
    void *array;

    if (!ctx || !args || !result || (arg_count != 2 && arg_count != 3)) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Array.CreateInstance requires an element type and array bounds");
    }

    element_type = (iron_runtime_type_t *)args[0].value.obj;
    length_array = args[1].value.obj;
    lower_bound_array = arg_count == 3 ? args[2].value.obj : NULL;
    if (!element_type || !length_array || (arg_count == 3 && !lower_bound_array)) {
        return IRON_ERROR(IRON_ERR_ARGUMENT_NULL, "Array creation arguments cannot be null");
    }
    if (!iron_domain_is_type_descriptor(ctx->domain, element_type) || element_type->element_type == IRON_TYPE_VOID ||
        element_type->kind == IRON_KIND_BYREF || element_type->kind == IRON_KIND_POINTER || iron_type_contains_generic_parameters(element_type)) {
        return IRON_ERROR(IRON_ERR_INVALID_TYPE, "The element type cannot be used to create an array");
    }

    if (iron_array_get_rank(length_array) != 1 || !iron_array_get_element_type(length_array) || iron_array_get_element_type(length_array)->element_type != IRON_TYPE_I4) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Array lengths must be a one-dimensional Int32 array");
    }

    rank = iron_array_get_length(length_array);
    if (rank == 0 || rank > 32) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Array rank must be between one and thirty-two");
    }
    if (lower_bound_array && (iron_array_get_rank(lower_bound_array) != 1 || iron_array_get_length(lower_bound_array) != rank ||
        !iron_array_get_element_type(lower_bound_array) || iron_array_get_element_type(lower_bound_array)->element_type != IRON_TYPE_I4)) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Array lower bounds must match the lengths array");
    }

    managed_lengths = (const iron_i32 *)iron_array_get_const_data(length_array);
    managed_lower_bounds = lower_bound_array ? (const iron_i32 *)iron_array_get_const_data(lower_bound_array) : NULL;
    for (dimension = 0; dimension < rank; dimension++) {
        if (managed_lengths[dimension] < 0) {
            return IRON_ERROR(IRON_ERR_ARGUMENT_OUT_OF_RANGE, "Array lengths must be non-negative");
        }

        lengths[dimension] = (iron_u32)managed_lengths[dimension];
        lower_bounds[dimension] = managed_lower_bounds ? managed_lower_bounds[dimension] : 0;
        if ((lengths[dimension] == 0 && lower_bounds[dimension] == INT_MIN) ||
            (lengths[dimension] > 0 && (iron_i64)lower_bounds[dimension] + (iron_i64)lengths[dimension] - 1 > INT_MAX)) {
            return IRON_ERROR(IRON_ERR_ARITHMETIC_OVERFLOW, "Array bounds exceed the Int32 index range");
        }
    }

    array_type = rank == 1 && !lower_bound_array ? iron_type_make_array(ctx->domain, element_type, 1) : iron_type_make_mdarray(ctx->domain, element_type, rank);
    if (!array_type) {
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to create the array runtime type");
    }

    array = iron_gc_alloc_mdarray(ctx, array_type, lengths, lower_bounds, rank);
    if (!array) {
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate the array");
    }

    result->type = IRON_VAL_OBJ;
    result->value.obj = array;
    return IRON_SUCCESS;
}
