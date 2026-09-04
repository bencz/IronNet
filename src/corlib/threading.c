/*
 * IronNet CLR Interpreter
 * corlib/threading.c - Managed threading primitive internal calls
 */

#include "iron/corlib.h"
#include "iron/runtime.h"
#include "iron/thread.h"
#include <string.h>

static void *byref_address(const iron_stack_value_t *argument)
{
    if (!argument) {
        return NULL;
    }
    if (argument->type == IRON_VAL_BYREF) {
        return argument->value.byref.ptr;
    }
    if (argument->type == IRON_VAL_PTR) {
        return argument->value.ptr;
    }
    return NULL;
}

static iron_result_t require_arguments(iron_stack_value_t *args, iron_u32 arg_count, iron_u32 required, iron_stack_value_t *result)
{
    if (!args || arg_count < required || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Invalid Interlocked arguments");
    }
    if (!byref_address(&args[0])) {
        return IRON_ERROR(IRON_ERR_NULL_REFERENCE, "Interlocked received a null managed reference");
    }
    return IRON_SUCCESS;
}

iron_result_t icall_Interlocked_Increment_Int32(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_result_t validation;

    (void)ctx;
    validation = require_arguments(args, arg_count, 1, result);
    if (!IRON_RESULT_OK(validation)) {
        return validation;
    }
    result->type = IRON_VAL_I32;
    result->value.i32 = iron_atomic_inc_i32((volatile iron_i32 *)byref_address(&args[0])) + 1;
    return IRON_SUCCESS;
}

iron_result_t icall_Interlocked_Increment_Int64(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_result_t validation;

    (void)ctx;
    validation = require_arguments(args, arg_count, 1, result);
    if (!IRON_RESULT_OK(validation)) {
        return validation;
    }
    result->type = IRON_VAL_I64;
    result->value.i64 = iron_atomic_add_i64((volatile iron_i64 *)byref_address(&args[0]), 1) + 1;
    return IRON_SUCCESS;
}

iron_result_t icall_Interlocked_Decrement_Int32(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_result_t validation;

    (void)ctx;
    validation = require_arguments(args, arg_count, 1, result);
    if (!IRON_RESULT_OK(validation)) {
        return validation;
    }
    result->type = IRON_VAL_I32;
    result->value.i32 = iron_atomic_dec_i32((volatile iron_i32 *)byref_address(&args[0])) - 1;
    return IRON_SUCCESS;
}

iron_result_t icall_Interlocked_Decrement_Int64(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_result_t validation;

    (void)ctx;
    validation = require_arguments(args, arg_count, 1, result);
    if (!IRON_RESULT_OK(validation)) {
        return validation;
    }
    result->type = IRON_VAL_I64;
    result->value.i64 = iron_atomic_add_i64((volatile iron_i64 *)byref_address(&args[0]), -1) - 1;
    return IRON_SUCCESS;
}

iron_result_t icall_Interlocked_Exchange_Int32(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_result_t validation;

    (void)ctx;
    validation = require_arguments(args, arg_count, 2, result);
    if (!IRON_RESULT_OK(validation)) {
        return validation;
    }
    result->type = IRON_VAL_I32;
    result->value.i32 = iron_atomic_exchange_i32((volatile iron_i32 *)byref_address(&args[0]), args[1].value.i32);
    return IRON_SUCCESS;
}

iron_result_t icall_Interlocked_Exchange_Int64(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_result_t validation;

    (void)ctx;
    validation = require_arguments(args, arg_count, 2, result);
    if (!IRON_RESULT_OK(validation)) {
        return validation;
    }
    result->type = IRON_VAL_I64;
    result->value.i64 = iron_atomic_exchange_i64((volatile iron_i64 *)byref_address(&args[0]), args[1].value.i64);
    return IRON_SUCCESS;
}

iron_result_t icall_Interlocked_Exchange_Object(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_result_t validation;

    (void)ctx;
    validation = require_arguments(args, arg_count, 2, result);
    if (!IRON_RESULT_OK(validation)) {
        return validation;
    }
    result->type = IRON_VAL_OBJ;
    result->value.obj = iron_atomic_exchange_ptr((void *volatile *)byref_address(&args[0]), args[1].value.obj);
    return IRON_SUCCESS;
}

iron_result_t icall_Interlocked_CompareExchange_Int32(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    volatile iron_i32 *address;
    iron_i32 original;
    iron_result_t validation;

    (void)ctx;
    validation = require_arguments(args, arg_count, 3, result);
    if (!IRON_RESULT_OK(validation)) {
        return validation;
    }
    address = (volatile iron_i32 *)byref_address(&args[0]);
    for (;;) {
        original = iron_atomic_load_i32(address);
        if (original != args[2].value.i32 || iron_atomic_cas_i32(address, original, args[1].value.i32)) {
            break;
        }
    }
    result->type = IRON_VAL_I32;
    result->value.i32 = original;
    return IRON_SUCCESS;
}

iron_result_t icall_Interlocked_CompareExchange_Int64(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    volatile iron_i64 *address;
    iron_i64 original;
    iron_result_t validation;

    (void)ctx;
    validation = require_arguments(args, arg_count, 3, result);
    if (!IRON_RESULT_OK(validation)) {
        return validation;
    }
    address = (volatile iron_i64 *)byref_address(&args[0]);
    for (;;) {
        original = iron_atomic_load_i64(address);
        if (original != args[2].value.i64 || iron_atomic_cas_i64(address, original, args[1].value.i64)) {
            break;
        }
    }
    result->type = IRON_VAL_I64;
    result->value.i64 = original;
    return IRON_SUCCESS;
}

iron_result_t icall_Interlocked_CompareExchange_Object(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    void *volatile *address;
    void *original;
    iron_result_t validation;

    (void)ctx;
    validation = require_arguments(args, arg_count, 3, result);
    if (!IRON_RESULT_OK(validation)) {
        return validation;
    }
    address = (void *volatile *)byref_address(&args[0]);
    for (;;) {
        original = iron_atomic_load_ptr(address);
        if (original != args[2].value.obj || iron_atomic_cas_ptr(address, original, args[1].value.obj)) {
            break;
        }
    }
    result->type = IRON_VAL_OBJ;
    result->value.obj = original;
    return IRON_SUCCESS;
}

iron_result_t icall_Interlocked_Add_Int32(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_result_t validation;

    (void)ctx;
    validation = require_arguments(args, arg_count, 2, result);
    if (!IRON_RESULT_OK(validation)) {
        return validation;
    }
    result->type = IRON_VAL_I32;
    result->value.i32 = iron_atomic_add_i32((volatile iron_i32 *)byref_address(&args[0]), args[1].value.i32) + args[1].value.i32;
    return IRON_SUCCESS;
}

iron_result_t icall_Interlocked_Add_Int64(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_result_t validation;

    (void)ctx;
    validation = require_arguments(args, arg_count, 2, result);
    if (!IRON_RESULT_OK(validation)) {
        return validation;
    }
    result->type = IRON_VAL_I64;
    result->value.i64 = iron_atomic_add_i64((volatile iron_i64 *)byref_address(&args[0]), args[1].value.i64) + args[1].value.i64;
    return IRON_SUCCESS;
}

iron_result_t icall_Interlocked_MemoryBarrier(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    (void)ctx;
    (void)args;
    (void)arg_count;
    iron_atomic_fence_seq_cst();
    if (result) {
        result->type = IRON_VAL_VOID;
    }
    return IRON_SUCCESS;
}

iron_result_t icall_Thread_Sleep(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_bool execution_suspended;
    iron_i32 milliseconds;

    if (!args || arg_count < 1) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Thread.Sleep requires a timeout");
    }
    milliseconds = args[0].value.i32;
    if (milliseconds < -1) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Thread.Sleep timeout must be -1 or non-negative");
    }

    execution_suspended = iron_exec_suspend_execution(ctx);
    if (milliseconds == -1) {
        for (;;) {
            iron_thread_sleep(0xFFFFFFFFU);
        }
    }
    iron_thread_sleep((iron_u32)milliseconds);
    iron_exec_resume_execution(ctx, execution_suspended);
    if (result) {
        result->type = IRON_VAL_VOID;
    }
    return IRON_SUCCESS;
}

iron_result_t icall_Thread_StartInternal(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_field_t *managed_id_field;
    iron_thread_context_t *thread;
    void *managed_thread;
    void *start_delegate;
    void *parameter;
    iron_result_t create_result;
    iron_result_t start_result;

    if (!ctx || !args || arg_count < 4) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Thread.StartInternal requires a thread, delegate, parameter, and parameter flag");
    }

    managed_thread = iron_corlib_object_argument(&args[0]);
    start_delegate = iron_corlib_object_argument(&args[1]);
    parameter = iron_corlib_object_argument(&args[2]);
    if (!managed_thread || !start_delegate) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Thread.StartInternal received an invalid managed thread or delegate");
    }
    if (iron_exec_find_managed_thread(ctx, managed_thread)) {
        return IRON_ERROR(IRON_ERR_INVALID_STATE, "The managed thread has already been started");
    }

    create_result = iron_thread_create_delegate(&thread, ctx, managed_thread, start_delegate, parameter, args[3].value.i32 != 0);
    if (!IRON_RESULT_OK(create_result)) {
        return create_result;
    }

    managed_id_field = iron_type_find_instance_field(iron_gc_get_type(managed_thread), "_managedThreadId");
    if (managed_id_field) {
        memcpy(&thread->thread_id, (const iron_u8 *)managed_thread + managed_id_field->offset, sizeof(thread->thread_id));
    }

    start_result = iron_thread_ctx_start(thread);
    if (!IRON_RESULT_OK(start_result)) {
        iron_thread_destroy(thread);
        return start_result;
    }

    if (result) {
        result->type = IRON_VAL_VOID;
    }
    return IRON_SUCCESS;
}

iron_result_t icall_Thread_GetIsAliveInternal(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_thread_context_t *thread;
    void *managed_thread;
    iron_i32 state;

    if (!ctx || !args || arg_count < 1 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Thread.GetIsAliveInternal requires a thread and result slot");
    }

    managed_thread = iron_corlib_object_argument(&args[0]);
    thread = iron_exec_find_managed_thread(ctx, managed_thread);
    state = thread ? iron_atomic_load_i32((const volatile iron_i32 *)&thread->state) : IRON_THREAD_CREATED;
    result->type = IRON_VAL_I32;
    result->value.i32 = state == IRON_THREAD_RUNNING || state == IRON_THREAD_WAITING || state == IRON_THREAD_SUSPENDED;
    return IRON_SUCCESS;
}

iron_result_t icall_Thread_JoinInternal(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_thread_context_t *thread;
    void *managed_thread;
    iron_i32 milliseconds;
    iron_result_t join_result;

    if (!ctx || !args || arg_count < 2 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Thread.JoinInternal requires a thread, timeout, and result slot");
    }

    managed_thread = iron_corlib_object_argument(&args[0]);
    thread = iron_exec_find_managed_thread(ctx, managed_thread);
    if (!thread) {
        return IRON_ERROR(IRON_ERR_INVALID_STATE, "The managed thread has not been started");
    }

    milliseconds = args[1].value.i32;
    join_result = iron_thread_ctx_join(thread, milliseconds < 0 ? UINT32_MAX : (iron_u32)milliseconds);
    result->type = IRON_VAL_I32;
    result->value.i32 = IRON_RESULT_OK(join_result) ? 1 : 0;
    if (join_result.error == IRON_ERR_TIMEOUT) {
        return IRON_SUCCESS;
    }

    return join_result;
}

iron_result_t icall_Thread_GetCurrentThreadInternal(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_field_t *managed_id_field;
    iron_runtime_type_t *thread_type;
    iron_thread_context_t *thread;
    void *managed_thread;

    (void)args;
    (void)arg_count;

    if (!ctx || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Thread.GetCurrentThreadInternal requires a context and result slot");
    }

    thread = iron_exec_get_current_thread(ctx);
    if (!thread) {
        return IRON_ERROR(IRON_ERR_INVALID_STATE, "No managed thread is associated with the current native thread");
    }

    managed_thread = thread->managed_thread;
    if (!managed_thread) {
        thread_type = iron_domain_find_type(ctx->domain, "System.Threading.Thread");
        if (!thread_type || !IRON_RESULT_OK(iron_type_compute_layout(thread_type))) {
            return IRON_ERROR(IRON_ERR_TYPE_NOT_FOUND, "System.Threading.Thread is not available");
        }

        managed_thread = iron_gc_alloc_object(&ctx->gc, thread_type, thread_type->instance_size);
        if (!managed_thread) {
            return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate the current managed Thread object");
        }

        managed_id_field = iron_type_find_instance_field(thread_type, "_managedThreadId");
        if (!managed_id_field) {
            return IRON_ERROR(IRON_ERR_FIELD_NOT_FOUND, "System.Threading.Thread._managedThreadId is unavailable");
        }

        memcpy((iron_u8 *)managed_thread + managed_id_field->offset, &thread->thread_id, sizeof(thread->thread_id));
        thread->managed_thread = managed_thread;
    }

    result->type = IRON_VAL_OBJ;
    result->value.obj = managed_thread;
    return IRON_SUCCESS;
}

iron_result_t icall_Monitor_Enter(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    void *object;

    if (!args || arg_count < 1) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Monitor.Enter requires an object");
    }

    object = iron_corlib_object_argument(&args[0]);
    if (result) {
        result->type = IRON_VAL_VOID;
    }
    return iron_monitor_enter(ctx, object);
}

iron_result_t icall_Monitor_Exit(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    void *object;

    if (!args || arg_count < 1) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Monitor.Exit requires an object");
    }

    object = iron_corlib_object_argument(&args[0]);
    if (result) {
        result->type = IRON_VAL_VOID;
    }
    return iron_monitor_exit(ctx, object);
}

iron_result_t icall_Monitor_TryEnter(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_bool acquired;
    iron_i32 milliseconds;
    iron_result_t enter_result;
    void *object;

    if (!args || arg_count < 2 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Monitor.TryEnter requires an object, timeout, and result slot");
    }

    milliseconds = args[1].value.i32;
    if (milliseconds < -1) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Monitor.TryEnter timeout must be -1 or non-negative");
    }

    object = iron_corlib_object_argument(&args[0]);
    enter_result = iron_monitor_try_enter(ctx, object, milliseconds < 0 ? UINT32_MAX : (iron_u32)milliseconds, &acquired);
    if (!IRON_RESULT_OK(enter_result)) {
        return enter_result;
    }

    result->type = IRON_VAL_I32;
    result->value.i32 = acquired ? 1 : 0;
    return IRON_SUCCESS;
}

iron_result_t icall_Monitor_Wait(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_bool signaled;
    iron_i32 milliseconds;
    iron_result_t wait_result;
    void *object;

    if (!args || arg_count < 2 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Monitor.Wait requires an object, timeout, and result slot");
    }

    milliseconds = args[1].value.i32;
    if (milliseconds < -1) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Monitor.Wait timeout must be -1 or non-negative");
    }

    object = iron_corlib_object_argument(&args[0]);
    wait_result = iron_monitor_wait(ctx, object, milliseconds < 0 ? UINT32_MAX : (iron_u32)milliseconds, &signaled);
    if (!IRON_RESULT_OK(wait_result)) {
        return wait_result;
    }

    result->type = IRON_VAL_I32;
    result->value.i32 = signaled ? 1 : 0;
    return IRON_SUCCESS;
}

iron_result_t icall_Monitor_Pulse(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    void *object;

    if (!args || arg_count < 1) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Monitor.Pulse requires an object");
    }

    object = iron_corlib_object_argument(&args[0]);
    if (result) {
        result->type = IRON_VAL_VOID;
    }
    return iron_monitor_pulse(ctx, object);
}

iron_result_t icall_Monitor_PulseAll(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    void *object;

    if (!args || arg_count < 1) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Monitor.PulseAll requires an object");
    }

    object = iron_corlib_object_argument(&args[0]);
    if (result) {
        result->type = IRON_VAL_VOID;
    }
    return iron_monitor_pulse_all(ctx, object);
}
