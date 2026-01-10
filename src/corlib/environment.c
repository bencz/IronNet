/*
 * IronNet CLR Interpreter
 * corlib/environment.c - System.Environment internal calls
 */

#include "iron/corlib.h"
#include <stdlib.h>
#include <time.h>

#if defined(_WIN32)
    #include <windows.h>
#else
    #include <unistd.h>
#endif

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
    
#if defined(_WIN32)
    result->value.i32 = (iron_i32)GetTickCount();
#else
    {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        result->value.i32 = (iron_i32)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
    }
#endif
    result->type = IRON_VAL_I32;
    
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
    
    return IRON_SUCCESS;
}
