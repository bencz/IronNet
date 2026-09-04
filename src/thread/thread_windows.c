/*
 * IronNet CLR Interpreter
 * thread_windows.c - Windows threading and atomic backend
 */

#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif
#ifndef _WIN32_WINNT
    #define _WIN32_WINNT 0x0A00
#endif

#include "iron/thread.h"
#include <windows.h>
#include <stdlib.h>
#include <string.h>

typedef struct thread_shared_state {
    iron_thread_fn function;
    void *argument;
    void *result;
    volatile LONG references;
} thread_shared_state_t;

static void release_thread_state(thread_shared_state_t *state)
{
    if (InterlockedDecrement(&state->references) == 0) {
        free(state);
    }
}

static DWORD WINAPI thread_proc(LPVOID parameter)
{
    thread_shared_state_t *state;
    iron_thread_fn function;
    void *argument;

    state = (thread_shared_state_t *)parameter;
    function = state->function;
    argument = state->argument;

    state->result = function(argument);
    release_thread_state(state);

    return 0;
}

iron_result_t iron_thread_create_ex(iron_thread_t *thread, const iron_thread_attr_t *attr, iron_thread_fn fn, void *arg)
{
    thread_shared_state_t *state;
    DWORD creation_flags;

    if (!thread || !fn) {
        return IRON_ERROR(IRON_ERR_NULL_POINTER, "Invalid thread or entry point");
    }

    memset(thread, 0, sizeof(*thread));
    state = (thread_shared_state_t *)malloc(sizeof(*state));
    if (!state) {
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate thread start data");
    }

    thread->joinable = !attr || !attr->detached;
    state->function = fn;
    state->argument = arg;
    state->result = NULL;
    state->references = 2;
    thread->platform_data = state;
    creation_flags = attr && attr->priority != 0 ? CREATE_SUSPENDED : 0;

    thread->handle = CreateThread(NULL, attr ? attr->stack_size : 0, thread_proc, state, creation_flags, &thread->id);
    if (!thread->handle) {
        free(state);
        thread->platform_data = NULL;
        return IRON_ERROR(IRON_ERR_THREAD_CREATE, "CreateThread failed");
    }

    if (creation_flags != 0) {
        if (!SetThreadPriority((HANDLE)thread->handle, attr->priority) || ResumeThread((HANDLE)thread->handle) == (DWORD)-1) {
            TerminateThread((HANDLE)thread->handle, 1);
            WaitForSingleObject((HANDLE)thread->handle, INFINITE);
            CloseHandle((HANDLE)thread->handle);
            free(state);
            memset(thread, 0, sizeof(*thread));
            return IRON_ERROR(IRON_ERR_THREAD_CREATE, "Failed to configure Windows thread priority");
        }
    }

    if (!thread->joinable) {
        CloseHandle((HANDLE)thread->handle);
        thread->handle = NULL;
        release_thread_state(state);
        thread->platform_data = NULL;
    }

    return IRON_SUCCESS;
}

iron_result_t iron_thread_join(iron_thread_t *thread, void **result)
{
    DWORD wait_result;

    if (!thread || !thread->joinable || !thread->handle) {
        return IRON_ERROR(IRON_ERR_THREAD_JOIN, "Thread is not joinable");
    }

    wait_result = WaitForSingleObject((HANDLE)thread->handle, INFINITE);
    if (wait_result != WAIT_OBJECT_0) {
        return IRON_ERROR(IRON_ERR_THREAD_JOIN, "WaitForSingleObject failed");
    }

    if (result) {
        *result = ((thread_shared_state_t *)thread->platform_data)->result;
    }

    CloseHandle((HANDLE)thread->handle);
    release_thread_state((thread_shared_state_t *)thread->platform_data);
    thread->handle = NULL;
    thread->platform_data = NULL;
    thread->joinable = IRON_FALSE;
    return IRON_SUCCESS;
}

iron_result_t iron_thread_detach(iron_thread_t *thread)
{
    if (!thread || !thread->joinable || !thread->handle) {
        return IRON_ERROR(IRON_ERR_THREAD_JOIN, "Thread is not joinable");
    }

    CloseHandle((HANDLE)thread->handle);
    release_thread_state((thread_shared_state_t *)thread->platform_data);
    thread->handle = NULL;
    thread->platform_data = NULL;
    thread->joinable = IRON_FALSE;
    return IRON_SUCCESS;
}

iron_u32 iron_thread_current_id(void)
{
    return (iron_u32)GetCurrentThreadId();
}

void iron_thread_yield(void)
{
    SwitchToThread();
}

void iron_thread_sleep(iron_u32 ms)
{
    Sleep(ms);
}

void iron_thread_set_name(const char *name)
{
    wchar_t *wide_name;
    int wide_length;

    if (!name) {
        return;
    }

    wide_length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, name, -1, NULL, 0);
    if (wide_length == 0) {
        return;
    }

    wide_name = (wchar_t *)malloc((iron_size)wide_length * sizeof(wchar_t));
    if (!wide_name) {
        return;
    }

    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, name, -1, wide_name, wide_length) != 0) {
        SetThreadDescription(GetCurrentThread(), wide_name);
    }
    free(wide_name);
}

iron_result_t iron_mutex_init(iron_mutex_t *mutex)
{
    if (!mutex) {
        return IRON_ERROR(IRON_ERR_NULL_POINTER, "mutex is NULL");
    }

    memset(mutex, 0, sizeof(*mutex));
    mutex->handle = malloc(sizeof(SRWLOCK));
    if (!mutex->handle) {
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate mutex");
    }

    InitializeSRWLock((SRWLOCK *)mutex->handle);
    mutex->initialized = IRON_TRUE;
    return IRON_SUCCESS;
}

void iron_mutex_destroy(iron_mutex_t *mutex)
{
    if (mutex && mutex->initialized) {
        free(mutex->handle);
        mutex->handle = NULL;
        mutex->initialized = IRON_FALSE;
    }
}

void iron_mutex_lock(iron_mutex_t *mutex)
{
    AcquireSRWLockExclusive((SRWLOCK *)mutex->handle);
}

iron_bool iron_mutex_trylock(iron_mutex_t *mutex)
{
    return TryAcquireSRWLockExclusive((SRWLOCK *)mutex->handle) != 0;
}

void iron_mutex_unlock(iron_mutex_t *mutex)
{
    ReleaseSRWLockExclusive((SRWLOCK *)mutex->handle);
}

iron_result_t iron_rmutex_init(iron_rmutex_t *mutex)
{
    if (!mutex) {
        return IRON_ERROR(IRON_ERR_NULL_POINTER, "recursive mutex is NULL");
    }

    memset(mutex, 0, sizeof(*mutex));
    mutex->handle = malloc(sizeof(CRITICAL_SECTION));
    if (!mutex->handle) {
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate recursive mutex");
    }

    if (!InitializeCriticalSectionEx((CRITICAL_SECTION *)mutex->handle, 0, 0)) {
        free(mutex->handle);
        mutex->handle = NULL;
        return IRON_ERROR(IRON_ERR_MUTEX_ERROR, "InitializeCriticalSectionEx failed");
    }

    mutex->initialized = IRON_TRUE;
    return IRON_SUCCESS;
}

void iron_rmutex_destroy(iron_rmutex_t *mutex)
{
    if (mutex && mutex->initialized) {
        DeleteCriticalSection((CRITICAL_SECTION *)mutex->handle);
        free(mutex->handle);
        mutex->handle = NULL;
        mutex->initialized = IRON_FALSE;
    }
}

void iron_rmutex_lock(iron_rmutex_t *mutex)
{
    EnterCriticalSection((CRITICAL_SECTION *)mutex->handle);
}

iron_bool iron_rmutex_trylock(iron_rmutex_t *mutex)
{
    return TryEnterCriticalSection((CRITICAL_SECTION *)mutex->handle) != 0;
}

void iron_rmutex_unlock(iron_rmutex_t *mutex)
{
    LeaveCriticalSection((CRITICAL_SECTION *)mutex->handle);
}

iron_result_t iron_cond_init(iron_cond_t *cond)
{
    if (!cond) {
        return IRON_ERROR(IRON_ERR_NULL_POINTER, "condition variable is NULL");
    }

    memset(cond, 0, sizeof(*cond));
    cond->handle = malloc(sizeof(CONDITION_VARIABLE));
    if (!cond->handle) {
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate condition variable");
    }

    InitializeConditionVariable((CONDITION_VARIABLE *)cond->handle);
    cond->initialized = IRON_TRUE;
    return IRON_SUCCESS;
}

void iron_cond_destroy(iron_cond_t *cond)
{
    if (cond && cond->initialized) {
        free(cond->handle);
        cond->handle = NULL;
        cond->initialized = IRON_FALSE;
    }
}

void iron_cond_wait(iron_cond_t *cond, iron_mutex_t *mutex)
{
    SleepConditionVariableSRW((CONDITION_VARIABLE *)cond->handle, (SRWLOCK *)mutex->handle, INFINITE, 0);
}

iron_bool iron_cond_timedwait(iron_cond_t *cond, iron_mutex_t *mutex, iron_u32 timeout_ms)
{
    return SleepConditionVariableSRW((CONDITION_VARIABLE *)cond->handle, (SRWLOCK *)mutex->handle, timeout_ms, 0) != 0;
}

void iron_cond_signal(iron_cond_t *cond)
{
    WakeConditionVariable((CONDITION_VARIABLE *)cond->handle);
}

void iron_cond_broadcast(iron_cond_t *cond)
{
    WakeAllConditionVariable((CONDITION_VARIABLE *)cond->handle);
}

iron_result_t iron_tls_create(iron_tls_key_t *key)
{
    return iron_tls_create_dtor(key, NULL);
}

iron_result_t iron_tls_create_dtor(iron_tls_key_t *key, void (*destructor)(void *))
{
    DWORD index;

    if (!key) {
        return IRON_ERROR(IRON_ERR_NULL_POINTER, "TLS key is NULL");
    }

    index = FlsAlloc((PFLS_CALLBACK_FUNCTION)destructor);
    if (index == FLS_OUT_OF_INDEXES) {
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "FlsAlloc failed");
    }

    key->key = index;
    key->initialized = IRON_TRUE;
    return IRON_SUCCESS;
}

void iron_tls_destroy(iron_tls_key_t *key)
{
    if (key && key->initialized) {
        FlsFree(key->key);
        key->initialized = IRON_FALSE;
    }
}

void *iron_tls_get(iron_tls_key_t *key)
{
    if (!key || !key->initialized) {
        return NULL;
    }

    return FlsGetValue(key->key);
}

void iron_tls_set(iron_tls_key_t *key, void *value)
{
    if (key && key->initialized) {
        FlsSetValue(key->key, value);
    }
}

void iron_atomic_fence_acquire(void) { MemoryBarrier(); }
void iron_atomic_fence_release(void) { MemoryBarrier(); }
void iron_atomic_fence_seq_cst(void) { MemoryBarrier(); }

iron_i32 iron_atomic_load_i32(const volatile iron_i32 *ptr) { return InterlockedCompareExchange((volatile LONG *)ptr, 0, 0); }
void iron_atomic_store_i32(volatile iron_i32 *ptr, iron_i32 value) { InterlockedExchange((volatile LONG *)ptr, value); }
iron_i32 iron_atomic_exchange_i32(volatile iron_i32 *ptr, iron_i32 value) { return InterlockedExchange((volatile LONG *)ptr, value); }
iron_bool iron_atomic_cas_i32(volatile iron_i32 *ptr, iron_i32 expected, iron_i32 desired) { return InterlockedCompareExchange((volatile LONG *)ptr, desired, expected) == expected; }
iron_i32 iron_atomic_add_i32(volatile iron_i32 *ptr, iron_i32 value) { return InterlockedExchangeAdd((volatile LONG *)ptr, value); }
iron_i32 iron_atomic_sub_i32(volatile iron_i32 *ptr, iron_i32 value) { return InterlockedExchangeAdd((volatile LONG *)ptr, -value); }
iron_i32 iron_atomic_inc_i32(volatile iron_i32 *ptr) { return InterlockedIncrement((volatile LONG *)ptr) - 1; }
iron_i32 iron_atomic_dec_i32(volatile iron_i32 *ptr) { return InterlockedDecrement((volatile LONG *)ptr) + 1; }
iron_i32 iron_atomic_and_i32(volatile iron_i32 *ptr, iron_i32 value) { return InterlockedAnd((volatile LONG *)ptr, value); }
iron_i32 iron_atomic_or_i32(volatile iron_i32 *ptr, iron_i32 value) { return InterlockedOr((volatile LONG *)ptr, value); }
iron_i32 iron_atomic_xor_i32(volatile iron_i32 *ptr, iron_i32 value) { return InterlockedXor((volatile LONG *)ptr, value); }
void *iron_atomic_load_ptr(void *const volatile *ptr) { return InterlockedCompareExchangePointer((void *volatile *)ptr, NULL, NULL); }
void iron_atomic_store_ptr(void *volatile *ptr, void *value) { InterlockedExchangePointer(ptr, value); }
void *iron_atomic_exchange_ptr(void *volatile *ptr, void *value) { return InterlockedExchangePointer(ptr, value); }
iron_bool iron_atomic_cas_ptr(void *volatile *ptr, void *expected, void *desired) { return InterlockedCompareExchangePointer(ptr, desired, expected) == expected; }

#ifndef IRON_NO_NATIVE_64BIT
iron_i64 iron_atomic_load_i64(const volatile iron_i64 *ptr) { return InterlockedCompareExchange64((volatile LONG64 *)ptr, 0, 0); }
void iron_atomic_store_i64(volatile iron_i64 *ptr, iron_i64 value) { InterlockedExchange64((volatile LONG64 *)ptr, value); }
iron_i64 iron_atomic_exchange_i64(volatile iron_i64 *ptr, iron_i64 value) { return InterlockedExchange64((volatile LONG64 *)ptr, value); }
iron_bool iron_atomic_cas_i64(volatile iron_i64 *ptr, iron_i64 expected, iron_i64 desired) { return InterlockedCompareExchange64((volatile LONG64 *)ptr, desired, expected) == expected; }
iron_i64 iron_atomic_add_i64(volatile iron_i64 *ptr, iron_i64 value) { return InterlockedExchangeAdd64((volatile LONG64 *)ptr, value); }
#endif
