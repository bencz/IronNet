/*
 * IronNet CLR Interpreter
 * corlib/environment.c - System.Environment internal calls
 */

#include "iron/corlib.h"
#include <stdlib.h>

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
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "TickCount requires result");
    }
    
    result->value.i32 = (iron_i32)iron_platform_monotonic_milliseconds();
    result->type = IRON_VAL_I32;
    
    return IRON_SUCCESS;
}

iron_result_t icall_Environment_GetUtcNowTicks(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    iron_i64 ticks;

    (void)ctx;
    (void)args;
    (void)arg_count;
    if (!result || !iron_platform_get_utc_ticks(&ticks)) {
        return IRON_ERROR(IRON_ERR_INVALID_STATE, "Cannot read the UTC system clock");
    }

    result->value.i64 = ticks;
    result->type = IRON_VAL_I64;
    return IRON_SUCCESS;
}

iron_result_t icall_Environment_GetLocalNowTicks(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    iron_i64 ticks;

    (void)ctx;
    (void)args;
    (void)arg_count;
    if (!result || !iron_platform_get_local_ticks(&ticks)) {
        return IRON_ERROR(IRON_ERR_INVALID_STATE, "Cannot read the local system clock");
    }

    result->value.i64 = ticks;
    result->type = IRON_VAL_I64;
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
        exit_code = args[0].value.i32;
    }
    
    exit(exit_code);
}
