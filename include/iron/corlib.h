/*
 * IronNet CLR Interpreter
 * corlib.h - Base Class Library internal call definitions
 * 
 * This header defines the internal call handlers for the corlib.
 * Each handler follows the signature:
 *   iron_result_t handler(iron_exec_context_t *ctx,
 *                         iron_stack_value_t *args,
 *                         iron_u32 arg_count,
 *                         iron_stack_value_t *result);
 * 
 * Parameters:
 *   ctx       - The execution context
 *   args      - Array of arguments (args[0] is 'this' for instance methods)
 *   arg_count - Number of arguments
 *   result    - Output parameter for return value (NULL for void methods)
 * 
 * Return:
 *   IRON_SUCCESS on success, or an error result
 */

#ifndef IRON_CORLIB_H
#define IRON_CORLIB_H

#include "platform.h"
#include "types.h"
#include "exec.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Internal Call Registration
 * ============================================================================ */

/* Register all corlib internal calls with the execution context */
IRON_API iron_result_t iron_register_corlib(iron_exec_context_t *ctx);

/* Obtain the managed storage represented by an internal-call argument. */
IRON_API void *iron_corlib_object_argument(const iron_stack_value_t *argument);

/* Obtain the bytes of a direct, by-reference, or boxed value argument. */
IRON_API const void *iron_corlib_value_argument_data(const iron_stack_value_t *argument);

/* Materialize canonical runtime Type descriptors as a managed Type array. */
IRON_API iron_result_t iron_corlib_return_type_array(iron_exec_context_t *ctx,
                                                     iron_runtime_type_t **types,
                                                     iron_u32 type_count,
                                                     iron_stack_value_t *result);

/* Materialize a runtime method descriptor as the appropriate reflection object. */
IRON_API void *iron_reflection_create_method_info(iron_exec_context_t *ctx, iron_runtime_method_t *method);

IRON_API iron_result_t icall_RuntimeHelpers_InitializeArray(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result);

/* ============================================================================
 * System.Object Internal Calls
 * ============================================================================ */

IRON_API iron_result_t icall_Object_GetType(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result);

IRON_API iron_result_t icall_Object_GetHashCode(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result);

IRON_API iron_result_t icall_Object_Equals(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result);

IRON_API iron_result_t icall_Object_MemberwiseClone(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result);

/* ============================================================================
 * System.String Internal Calls
 * ============================================================================ */

IRON_API iron_result_t icall_String_ctor_CharCount(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result);

IRON_API iron_result_t icall_String_ctor_CharArray(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result);

IRON_API iron_result_t icall_String_ctor_CharArrayRange(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result);

IRON_API iron_result_t icall_String_ctor_CharPointer(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result);

IRON_API iron_result_t icall_String_ctor_CharPointerRange(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result);

IRON_API iron_result_t icall_String_get_Length(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result);

IRON_API iron_result_t icall_String_get_Item(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result);

IRON_API iron_result_t icall_String_Concat(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result);

IRON_API iron_result_t icall_String_Equals(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result);

IRON_API iron_result_t icall_String_InternalAllocateStr(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result);

/* ============================================================================
 * System.Console Internal Calls
 * ============================================================================ */

IRON_API iron_result_t icall_Console_WriteLine_String(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result);

IRON_API iron_result_t icall_Console_WriteLine_Int32(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result);

IRON_API iron_result_t icall_Console_Write_String(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result);

IRON_API iron_result_t icall_Console_ReadLine(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result);

/* ============================================================================
 * System.Environment Internal Calls
 * ============================================================================ */

IRON_API iron_result_t icall_Environment_get_TickCount(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result);

IRON_API iron_result_t icall_Environment_GetUtcNowTicks(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result);

IRON_API iron_result_t icall_Environment_GetLocalNowTicks(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result);

IRON_API iron_result_t icall_Environment_Exit(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result);

IRON_API iron_result_t icall_Environment_get_CurrentDirectory(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result);

/* ============================================================================
 * System.GC Internal Calls
 * ============================================================================ */

IRON_API iron_result_t icall_GC_Collect(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result);

IRON_API iron_result_t icall_GC_GetTotalMemory(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result);

IRON_API iron_result_t icall_GC_SuppressFinalize(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result);

IRON_API iron_result_t icall_GC_ReRegisterForFinalize(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result);

/* ============================================================================
 * System.Array Internal Calls
 * ============================================================================ */

IRON_API iron_result_t icall_Array_get_Length(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result);

IRON_API iron_result_t icall_Array_get_Rank(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result);

IRON_API iron_result_t icall_Array_GetLength(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result);

IRON_API iron_result_t icall_Array_GetLowerBound(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result);

IRON_API iron_result_t icall_Array_GetUpperBound(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result);

IRON_API iron_result_t icall_Array_GetValue(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result);

IRON_API iron_result_t icall_Array_SetValue(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result);

IRON_API iron_result_t icall_Array_Copy(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result);

IRON_API iron_result_t icall_Array_Clear(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result);

IRON_API iron_result_t icall_Array_CreateInstance(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result);

/* ============================================================================
 * System.Math Internal Calls
 * ============================================================================ */

IRON_API iron_result_t icall_Math_Sin(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result);

IRON_API iron_result_t icall_Math_Cos(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result);

IRON_API iron_result_t icall_Math_Sqrt(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result);

IRON_API iron_result_t icall_Math_Abs_Int32(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result);

IRON_API iron_result_t icall_Math_Abs_Double(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result);

/* ============================================================================
 * System.Type Internal Calls
 * ============================================================================ */

IRON_API iron_result_t icall_Type_GetTypeFromHandle(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result);

IRON_API iron_result_t icall_Type_get_Name(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result);

IRON_API iron_result_t icall_Type_get_FullName(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result);

/* ============================================================================
 * String Helpers
 * ============================================================================ */

/* 
 * String object layout in memory:
 *   [GC Header]
 *   [iron_u32 length]     - Number of characters
 *   [iron_u16 chars[]]    - UTF-16 character data
 * 
 * Use these helpers to work with managed strings:
 */

/* Get length of a managed string object */
IRON_API iron_u32 iron_string_get_length(void *str_obj);

/* Get pointer to character data */
IRON_API iron_u16 *iron_string_get_chars(void *str_obj);

/* Create a new managed string from C string (UTF-8) */
IRON_API void *iron_string_new_utf8(iron_exec_context_t *ctx, const char *utf8);

/* Create a new managed string from UTF-16 data */
IRON_API void *iron_string_new_utf16(iron_exec_context_t *ctx, 
                                      const iron_u16 *chars, iron_u32 length);

/* Convert managed string to C string (caller must free) */
IRON_API char *iron_string_to_utf8(iron_exec_context_t *ctx, void *str_obj);

#ifdef __cplusplus
}
#endif

#endif /* IRON_CORLIB_H */
