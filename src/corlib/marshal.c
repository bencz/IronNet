/*
 * IronNet CLR Interpreter
 * marshal.c - System.Runtime.InteropServices.Marshal internal calls
 */

#include "iron/corlib.h"
#include "iron/runtime.h"
#include <limits.h>
#include <stdint.h>
#include <string.h>

static void *native_pointer_argument(const iron_stack_value_t *argument)
{
    void *pointer;

    if (!argument) {
        return NULL;
    }

    switch (argument->type) {
        case IRON_VAL_PTR:
        case IRON_VAL_METHOD_PTR:
            return argument->value.ptr;

        case IRON_VAL_BYREF:
            return argument->value.byref.ptr;

        case IRON_VAL_I32:
            return (void *)(uintptr_t)(iron_u32)argument->value.i32;

        case IRON_VAL_I64:
            return (void *)(uintptr_t)(iron_u64)argument->value.i64;

        case IRON_VAL_OBJ:
        case IRON_VAL_VALUETYPE:
            if (!argument->value.obj) {
                return NULL;
            }

            pointer = NULL;
            memcpy(&pointer, argument->value.obj, sizeof(pointer));
            return pointer;

        default:
            return NULL;
    }
}

static iron_result_t validate_byte_array_range(void *array, iron_i32 start_index, iron_i32 length, const char *parameter_name)
{
    iron_u32 array_length;

    if (!array) {
        return IRON_ERROR(IRON_ERR_NULL_REFERENCE, parameter_name);
    }
    if (start_index < 0 || length < 0) {
        return IRON_ERROR(IRON_ERR_INDEX_OUT_OF_RANGE, "Marshal.Copy received a negative array range");
    }
    if (iron_array_get_element_size(array) != sizeof(iron_u8)) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Marshal.Copy requires a byte array");
    }

    array_length = iron_array_get_length(array);
    if ((iron_u32)start_index > array_length || (iron_u32)length > array_length - (iron_u32)start_index) {
        return IRON_ERROR(IRON_ERR_INDEX_OUT_OF_RANGE, "Marshal.Copy range exceeds the byte array bounds");
    }

    return IRON_SUCCESS;
}

iron_result_t icall_Marshal_SizeOfCore(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_type_t *type;
    iron_result_t layout_result;

    (void)ctx;

    if (!args || arg_count < 1 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Marshal.SizeOf requires a Type argument and a result slot");
    }

    type = (iron_runtime_type_t *)iron_corlib_object_argument(&args[0]);
    if (!type) {
        return IRON_ERROR(IRON_ERR_NULL_REFERENCE, "Marshal.SizeOf received a null Type");
    }

    layout_result = iron_type_compute_layout(type);
    if (!IRON_RESULT_OK(layout_result)) {
        return layout_result;
    }
    if (type->instance_size > INT32_MAX) {
        return IRON_ERROR(IRON_ERR_BUFFER_OVERFLOW, "The unmanaged type size exceeds System.Int32.MaxValue");
    }

    result->type = IRON_VAL_I32;
    result->value.i32 = (iron_i32)type->instance_size;
    return IRON_SUCCESS;
}

iron_result_t icall_Marshal_AllocHGlobalCore(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    void *allocation;

    (void)ctx;

    if (!args || arg_count < 1 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Marshal.AllocHGlobal requires a byte count and a result slot");
    }

    allocation = NULL;
    if (args[0].value.i32 >= 0) {
        allocation = iron_platform_alloc_hglobal((iron_size)args[0].value.i32);
    }

    result->type = IRON_VAL_PTR;
    result->value.ptr = allocation;
    return IRON_SUCCESS;
}

iron_result_t icall_Marshal_FreeHGlobal(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    (void)ctx;
    (void)result;

    if (!args || arg_count < 1) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Marshal.FreeHGlobal requires a native pointer");
    }

    iron_platform_free_hglobal(native_pointer_argument(&args[0]));
    return IRON_SUCCESS;
}

iron_result_t icall_Marshal_CopyToNative(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    void *array;
    void *destination;
    iron_i32 start_index;
    iron_i32 length;
    iron_result_t validation;

    (void)ctx;
    (void)result;

    if (!args || arg_count < 4) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Marshal.CopyToNative requires four arguments");
    }

    array = iron_corlib_object_argument(&args[0]);
    start_index = args[1].value.i32;
    destination = native_pointer_argument(&args[2]);
    length = args[3].value.i32;
    validation = validate_byte_array_range(array, start_index, length, "Marshal.Copy received a null source array");
    if (!IRON_RESULT_OK(validation)) {
        return validation;
    }
    if (length == 0) {
        return IRON_SUCCESS;
    }
    if (!destination) {
        return IRON_ERROR(IRON_ERR_NULL_POINTER, "Marshal.Copy received a null native destination");
    }

    iron_memmove(destination, (const iron_u8 *)iron_array_get_const_data(array) + (iron_size)start_index, (iron_size)length);
    return IRON_SUCCESS;
}

iron_result_t icall_Marshal_CopyFromNative(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    const void *source;
    void *array;
    iron_i32 start_index;
    iron_i32 length;
    iron_result_t validation;

    (void)ctx;
    (void)result;

    if (!args || arg_count < 4) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Marshal.CopyFromNative requires four arguments");
    }

    source = native_pointer_argument(&args[0]);
    array = iron_corlib_object_argument(&args[1]);
    start_index = args[2].value.i32;
    length = args[3].value.i32;
    validation = validate_byte_array_range(array, start_index, length, "Marshal.Copy received a null destination array");
    if (!IRON_RESULT_OK(validation)) {
        return validation;
    }
    if (length == 0) {
        return IRON_SUCCESS;
    }
    if (!source) {
        return IRON_ERROR(IRON_ERR_NULL_POINTER, "Marshal.Copy received a null native source");
    }

    iron_memmove((iron_u8 *)iron_array_get_data(array) + (iron_size)start_index, source, (iron_size)length);
    return IRON_SUCCESS;
}

iron_result_t icall_Marshal_PtrToStringAnsiCore(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    const char *source;

    if (!args || arg_count < 1 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Marshal.PtrToStringAnsi requires a native pointer and a result slot");
    }

    source = (const char *)native_pointer_argument(&args[0]);
    if (!source) {
        return IRON_ERROR(IRON_ERR_NULL_POINTER, "Marshal.PtrToStringAnsi received a null pointer");
    }

    result->type = IRON_VAL_OBJ;
    result->value.obj = iron_string_new_utf8(ctx, source);
    if (!result->value.obj) {
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate the managed ANSI string");
    }

    return IRON_SUCCESS;
}

iron_result_t icall_Marshal_PtrToStringUniCore(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    const iron_u16 *source;
    iron_size length;

    if (!args || arg_count < 1 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Marshal.PtrToStringUni requires a native pointer and a result slot");
    }

    source = (const iron_u16 *)native_pointer_argument(&args[0]);
    if (!source) {
        return IRON_ERROR(IRON_ERR_NULL_POINTER, "Marshal.PtrToStringUni received a null pointer");
    }

    length = 0;
    while (source[length] != 0) {
        if (length == UINT32_MAX) {
            return IRON_ERROR(IRON_ERR_BUFFER_OVERFLOW, "The native UTF-16 string is too long");
        }
        length++;
    }

    result->type = IRON_VAL_OBJ;
    result->value.obj = iron_string_new_utf16(ctx, source, (iron_u32)length);
    if (!result->value.obj) {
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate the managed UTF-16 string");
    }

    return IRON_SUCCESS;
}

iron_result_t icall_Marshal_StringToHGlobalAnsiCore(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    void *string_object;
    char *utf8;
    iron_size byte_count;
    void *allocation;

    if (!args || arg_count < 1 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Marshal.StringToHGlobalAnsi requires a string and a result slot");
    }

    string_object = iron_corlib_object_argument(&args[0]);
    utf8 = iron_string_to_utf8(ctx, string_object);
    allocation = NULL;
    if (utf8) {
        byte_count = strlen(utf8) + 1;
        allocation = iron_platform_alloc_hglobal(byte_count);
        if (allocation) {
            iron_memcpy(allocation, utf8, byte_count);
        }
        iron_free(ctx->allocator, utf8, byte_count);
    }

    result->type = IRON_VAL_PTR;
    result->value.ptr = allocation;
    return IRON_SUCCESS;
}

iron_result_t icall_Marshal_StringToHGlobalUniCore(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    void *string_object;
    iron_u32 length;
    iron_size byte_count;
    iron_u16 *allocation;

    (void)ctx;

    if (!args || arg_count < 1 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Marshal.StringToHGlobalUni requires a string and a result slot");
    }

    string_object = iron_corlib_object_argument(&args[0]);
    length = iron_string_get_length(string_object);
    if ((iron_size)length > ((iron_size)-1 / sizeof(iron_u16)) - 1) {
        return IRON_ERROR(IRON_ERR_BUFFER_OVERFLOW, "The managed string is too large for native UTF-16 storage");
    }

    byte_count = ((iron_size)length + 1) * sizeof(iron_u16);
    allocation = (iron_u16 *)iron_platform_alloc_hglobal(byte_count);
    if (allocation) {
        iron_memcpy(allocation, iron_string_get_chars(string_object), (iron_size)length * sizeof(iron_u16));
        allocation[length] = 0;
    }

    result->type = IRON_VAL_PTR;
    result->value.ptr = allocation;
    return IRON_SUCCESS;
}

iron_result_t icall_Marshal_GetLastWin32Error(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    (void)ctx;
    (void)args;
    (void)arg_count;

    if (!result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Marshal.GetLastWin32Error requires a result slot");
    }

    result->type = IRON_VAL_I32;
    result->value.i32 = iron_platform_get_last_error();
    return IRON_SUCCESS;
}
