/*
 * IronNet CLR Interpreter
 * corlib/math.c - System.Math internal calls
 */

#include "iron/corlib.h"
#include <math.h>

/* ============================================================================
 * System.Math Internal Calls
 * ============================================================================ */

iron_result_t icall_Math_Abs_Int32(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    (void)ctx;
    
    if (arg_count < 1 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Math.Abs requires value");
    }
    
    result->type = IRON_VAL_I32;
    result->value.i32 = (args[0].value.i32 < 0) ? -args[0].value.i32 : args[0].value.i32;
    
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
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Math.Abs requires value");
    }
    
    result->type = IRON_VAL_F64;
    result->value.f64 = fabs(args[0].value.f64);
    
    return IRON_SUCCESS;
}

iron_result_t icall_Math_Sin(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    (void)ctx;
    
    if (arg_count < 1 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Math.Sin requires value");
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
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Math.Cos requires value");
    }
    
    result->type = IRON_VAL_F64;
    result->value.f64 = cos(args[0].value.f64);
    
    return IRON_SUCCESS;
}

iron_result_t icall_Math_Tan(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    (void)ctx;
    
    if (arg_count < 1 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Math.Tan requires value");
    }
    
    result->type = IRON_VAL_F64;
    result->value.f64 = tan(args[0].value.f64);
    
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
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Math.Sqrt requires value");
    }
    
    result->type = IRON_VAL_F64;
    result->value.f64 = sqrt(args[0].value.f64);
    
    return IRON_SUCCESS;
}

iron_result_t icall_Math_Pow(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    (void)ctx;
    
    if (arg_count < 2 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Math.Pow requires x and y");
    }
    
    result->type = IRON_VAL_F64;
    result->value.f64 = pow(args[0].value.f64, args[1].value.f64);
    
    return IRON_SUCCESS;
}

iron_result_t icall_Math_Log(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    (void)ctx;
    
    if (arg_count < 1 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Math.Log requires value");
    }
    
    result->type = IRON_VAL_F64;
    result->value.f64 = log(args[0].value.f64);
    
    return IRON_SUCCESS;
}

iron_result_t icall_Math_Log10(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    (void)ctx;
    
    if (arg_count < 1 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Math.Log10 requires value");
    }
    
    result->type = IRON_VAL_F64;
    result->value.f64 = log10(args[0].value.f64);
    
    return IRON_SUCCESS;
}

iron_result_t icall_Math_Exp(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    (void)ctx;
    
    if (arg_count < 1 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Math.Exp requires value");
    }
    
    result->type = IRON_VAL_F64;
    result->value.f64 = exp(args[0].value.f64);
    
    return IRON_SUCCESS;
}

iron_result_t icall_Math_Floor(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    (void)ctx;
    
    if (arg_count < 1 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Math.Floor requires value");
    }
    
    result->type = IRON_VAL_F64;
    result->value.f64 = floor(args[0].value.f64);
    
    return IRON_SUCCESS;
}

iron_result_t icall_Math_Ceiling(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    (void)ctx;
    
    if (arg_count < 1 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Math.Ceiling requires value");
    }
    
    result->type = IRON_VAL_F64;
    result->value.f64 = ceil(args[0].value.f64);
    
    return IRON_SUCCESS;
}

iron_result_t icall_Math_Round(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    (void)ctx;
    
    if (arg_count < 1 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Math.Round requires value");
    }
    
    result->type = IRON_VAL_F64;
    result->value.f64 = round(args[0].value.f64);
    
    return IRON_SUCCESS;
}

iron_result_t icall_Math_Min_Int32(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    (void)ctx;
    
    if (arg_count < 2 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Math.Min requires two values");
    }
    
    result->type = IRON_VAL_I32;
    result->value.i32 = (args[0].value.i32 < args[1].value.i32) ? 
                        args[0].value.i32 : args[1].value.i32;
    
    return IRON_SUCCESS;
}

iron_result_t icall_Math_Max_Int32(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    (void)ctx;
    
    if (arg_count < 2 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Math.Max requires two values");
    }
    
    result->type = IRON_VAL_I32;
    result->value.i32 = (args[0].value.i32 > args[1].value.i32) ? 
                        args[0].value.i32 : args[1].value.i32;
    
    return IRON_SUCCESS;
}
