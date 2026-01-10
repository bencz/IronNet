/*
 * IronNet CLR Interpreter
 * thread.c - Platform-independent threading implementation
 */

/* Enable POSIX features */
#if !defined(_WIN32) && !defined(_WIN64)
    #define _POSIX_C_SOURCE 200809L
    #define _XOPEN_SOURCE 700
#endif

#include "iron/thread.h"
#include <string.h>
#include <stdlib.h>

#if defined(IRON_OS_WINDOWS)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
#elif defined(IRON_OS_POSIX)
    #include <pthread.h>
    #include <unistd.h>
    #include <time.h>
    #include <errno.h>
    #if !defined(IRON_OS_MACOS)
        #include <semaphore.h>
    #endif
#endif

/* ============================================================================
 * Thread Implementation
 * ============================================================================ */

#if defined(IRON_OS_WINDOWS)

typedef struct {
    iron_thread_fn fn;
    void *arg;
} thread_start_info_t;

static DWORD WINAPI thread_proc(LPVOID param)
{
    thread_start_info_t *info = (thread_start_info_t *)param;
    iron_thread_fn fn = info->fn;
    void *arg = info->arg;
    free(info);
    fn(arg);
    return 0;
}

iron_result_t iron_thread_create_ex(iron_thread_t *thread,
                                    const iron_thread_attr_t *attr,
                                    iron_thread_fn fn,
                                    void *arg)
{
    thread_start_info_t *info;
    DWORD flags = 0;
    
    info = (thread_start_info_t *)malloc(sizeof(thread_start_info_t));
    if (!info) {
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate thread info");
    }
    
    info->fn = fn;
    info->arg = arg;
    
    thread->handle = CreateThread(NULL, 
                                  attr ? attr->stack_size : 0,
                                  thread_proc, 
                                  info, 
                                  flags, 
                                  &thread->id);
    
    if (!thread->handle) {
        free(info);
        return IRON_ERROR(IRON_ERR_THREAD_CREATE, "CreateThread failed");
    }
    
    thread->joinable = attr ? !attr->detached : IRON_TRUE;
    return (iron_result_t)IRON_SUCCESS;
}

iron_result_t iron_thread_join(iron_thread_t *thread, void **result)
{
    DWORD wait_result;
    
    if (!thread->joinable) {
        return IRON_ERROR(IRON_ERR_THREAD_JOIN, "Thread not joinable");
    }
    
    wait_result = WaitForSingleObject(thread->handle, INFINITE);
    if (wait_result != WAIT_OBJECT_0) {
        return IRON_ERROR(IRON_ERR_THREAD_JOIN, "WaitForSingleObject failed");
    }
    
    CloseHandle(thread->handle);
    thread->handle = NULL;
    thread->joinable = IRON_FALSE;
    
    if (result) *result = NULL;
    return (iron_result_t)IRON_SUCCESS;
}

iron_result_t iron_thread_detach(iron_thread_t *thread)
{
    if (!thread->joinable) {
        return IRON_ERROR(IRON_ERR_THREAD_JOIN, "Thread already detached");
    }
    
    CloseHandle(thread->handle);
    thread->handle = NULL;
    thread->joinable = IRON_FALSE;
    return (iron_result_t)IRON_SUCCESS;
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
    /* Windows 10+ supports SetThreadDescription */
    (void)name;
}

#elif defined(IRON_OS_POSIX)

typedef struct {
    iron_thread_fn fn;
    void *arg;
} thread_start_info_t;

static void *thread_proc(void *param)
{
    thread_start_info_t *info = (thread_start_info_t *)param;
    iron_thread_fn fn = info->fn;
    void *arg = info->arg;
    free(info);
    return fn(arg);
}

iron_result_t iron_thread_create_ex(iron_thread_t *thread,
                                    const iron_thread_attr_t *attr,
                                    iron_thread_fn fn,
                                    void *arg)
{
    thread_start_info_t *info;
    pthread_attr_t pattr;
    int err;
    
    info = (thread_start_info_t *)malloc(sizeof(thread_start_info_t));
    if (!info) {
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate thread info");
    }
    
    info->fn = fn;
    info->arg = arg;
    
    pthread_attr_init(&pattr);
    
    if (attr) {
        if (attr->stack_size > 0) {
            pthread_attr_setstacksize(&pattr, attr->stack_size);
        }
        if (attr->detached) {
            pthread_attr_setdetachstate(&pattr, PTHREAD_CREATE_DETACHED);
        }
    }
    
    err = pthread_create(&thread->handle, &pattr, thread_proc, info);
    pthread_attr_destroy(&pattr);
    
    if (err != 0) {
        free(info);
        return IRON_ERROR(IRON_ERR_THREAD_CREATE, "pthread_create failed");
    }
    
    thread->joinable = attr ? !attr->detached : IRON_TRUE;
    return (iron_result_t)IRON_SUCCESS;
}

iron_result_t iron_thread_join(iron_thread_t *thread, void **result)
{
    int err;
    
    if (!thread->joinable) {
        return IRON_ERROR(IRON_ERR_THREAD_JOIN, "Thread not joinable");
    }
    
    err = pthread_join(thread->handle, result);
    if (err != 0) {
        return IRON_ERROR(IRON_ERR_THREAD_JOIN, "pthread_join failed");
    }
    
    thread->joinable = IRON_FALSE;
    return (iron_result_t)IRON_SUCCESS;
}

iron_result_t iron_thread_detach(iron_thread_t *thread)
{
    int err;
    
    if (!thread->joinable) {
        return IRON_ERROR(IRON_ERR_THREAD_JOIN, "Thread already detached");
    }
    
    err = pthread_detach(thread->handle);
    if (err != 0) {
        return IRON_ERROR(IRON_ERR_THREAD_JOIN, "pthread_detach failed");
    }
    
    thread->joinable = IRON_FALSE;
    return (iron_result_t)IRON_SUCCESS;
}

iron_u32 iron_thread_current_id(void)
{
    return (iron_u32)(iron_size)pthread_self();
}

void iron_thread_yield(void)
{
    sched_yield();
}

void iron_thread_sleep(iron_u32 ms)
{
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

void iron_thread_set_name(const char *name)
{
#if defined(IRON_OS_LINUX) && defined(_GNU_SOURCE)
    pthread_setname_np(pthread_self(), name);
#elif defined(IRON_OS_MACOS)
    /* pthread_setname_np on macOS requires _DARWIN_C_SOURCE */
    (void)name;
#else
    (void)name;
#endif
}

#else

/* Stub implementation for unsupported platforms */
iron_result_t iron_thread_create_ex(iron_thread_t *thread,
                                    const iron_thread_attr_t *attr,
                                    iron_thread_fn fn,
                                    void *arg)
{
    (void)thread; (void)attr; (void)fn; (void)arg;
    return IRON_ERROR(IRON_ERR_NOT_IMPLEMENTED, "Threading not supported");
}

iron_result_t iron_thread_join(iron_thread_t *thread, void **result)
{
    (void)thread; (void)result;
    return IRON_ERROR(IRON_ERR_NOT_IMPLEMENTED, "Threading not supported");
}

iron_result_t iron_thread_detach(iron_thread_t *thread)
{
    (void)thread;
    return IRON_ERROR(IRON_ERR_NOT_IMPLEMENTED, "Threading not supported");
}

iron_u32 iron_thread_current_id(void) { return 0; }
void iron_thread_yield(void) {}
void iron_thread_sleep(iron_u32 ms) { (void)ms; }
void iron_thread_set_name(const char *name) { (void)name; }

#endif

iron_result_t iron_thread_create_simple(iron_thread_t *thread,
                                        iron_thread_fn fn,
                                        void *arg)
{
    return iron_thread_create_ex(thread, NULL, fn, arg);
}

/* ============================================================================
 * Mutex Implementation
 * ============================================================================ */

#if defined(IRON_OS_WINDOWS)

iron_result_t iron_mutex_init(iron_mutex_t *mutex)
{
    mutex->handle = malloc(sizeof(CRITICAL_SECTION));
    if (!mutex->handle) {
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate mutex");
    }
    InitializeCriticalSection((CRITICAL_SECTION *)mutex->handle);
    mutex->initialized = IRON_TRUE;
    return (iron_result_t)IRON_SUCCESS;
}

void iron_mutex_destroy(iron_mutex_t *mutex)
{
    if (mutex->initialized && mutex->handle) {
        DeleteCriticalSection((CRITICAL_SECTION *)mutex->handle);
        free(mutex->handle);
        mutex->handle = NULL;
        mutex->initialized = IRON_FALSE;
    }
}

void iron_mutex_lock(iron_mutex_t *mutex)
{
    EnterCriticalSection((CRITICAL_SECTION *)mutex->handle);
}

iron_bool iron_mutex_trylock(iron_mutex_t *mutex)
{
    return TryEnterCriticalSection((CRITICAL_SECTION *)mutex->handle) != 0;
}

void iron_mutex_unlock(iron_mutex_t *mutex)
{
    LeaveCriticalSection((CRITICAL_SECTION *)mutex->handle);
}

#elif defined(IRON_OS_POSIX)

iron_result_t iron_mutex_init(iron_mutex_t *mutex)
{
    int err = pthread_mutex_init(&mutex->handle, NULL);
    if (err != 0) {
        return IRON_ERROR(IRON_ERR_MUTEX_ERROR, "pthread_mutex_init failed");
    }
    mutex->initialized = IRON_TRUE;
    return (iron_result_t)IRON_SUCCESS;
}

void iron_mutex_destroy(iron_mutex_t *mutex)
{
    if (mutex->initialized) {
        pthread_mutex_destroy(&mutex->handle);
        mutex->initialized = IRON_FALSE;
    }
}

void iron_mutex_lock(iron_mutex_t *mutex)
{
    pthread_mutex_lock(&mutex->handle);
}

iron_bool iron_mutex_trylock(iron_mutex_t *mutex)
{
    return pthread_mutex_trylock(&mutex->handle) == 0;
}

void iron_mutex_unlock(iron_mutex_t *mutex)
{
    pthread_mutex_unlock(&mutex->handle);
}

#else

iron_result_t iron_mutex_init(iron_mutex_t *mutex)
{
    mutex->lock = 0;
    mutex->initialized = IRON_TRUE;
    return (iron_result_t)IRON_SUCCESS;
}

void iron_mutex_destroy(iron_mutex_t *mutex)
{
    mutex->initialized = IRON_FALSE;
}

void iron_mutex_lock(iron_mutex_t *mutex)
{
    while (mutex->lock) {}
    mutex->lock = 1;
}

iron_bool iron_mutex_trylock(iron_mutex_t *mutex)
{
    if (mutex->lock) return IRON_FALSE;
    mutex->lock = 1;
    return IRON_TRUE;
}

void iron_mutex_unlock(iron_mutex_t *mutex)
{
    mutex->lock = 0;
}

#endif

/* ============================================================================
 * Condition Variable Implementation
 * ============================================================================ */

#if defined(IRON_OS_WINDOWS)

iron_result_t iron_cond_init(iron_cond_t *cond)
{
    cond->handle = malloc(sizeof(CONDITION_VARIABLE));
    if (!cond->handle) {
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate cond var");
    }
    InitializeConditionVariable((CONDITION_VARIABLE *)cond->handle);
    cond->initialized = IRON_TRUE;
    return (iron_result_t)IRON_SUCCESS;
}

void iron_cond_destroy(iron_cond_t *cond)
{
    if (cond->initialized && cond->handle) {
        free(cond->handle);
        cond->handle = NULL;
        cond->initialized = IRON_FALSE;
    }
}

void iron_cond_wait(iron_cond_t *cond, iron_mutex_t *mutex)
{
    SleepConditionVariableCS((CONDITION_VARIABLE *)cond->handle,
                             (CRITICAL_SECTION *)mutex->handle,
                             INFINITE);
}

iron_bool iron_cond_timedwait(iron_cond_t *cond, iron_mutex_t *mutex,
                              iron_u32 timeout_ms)
{
    return SleepConditionVariableCS((CONDITION_VARIABLE *)cond->handle,
                                    (CRITICAL_SECTION *)mutex->handle,
                                    timeout_ms) != 0;
}

void iron_cond_signal(iron_cond_t *cond)
{
    WakeConditionVariable((CONDITION_VARIABLE *)cond->handle);
}

void iron_cond_broadcast(iron_cond_t *cond)
{
    WakeAllConditionVariable((CONDITION_VARIABLE *)cond->handle);
}

#elif defined(IRON_OS_POSIX)

iron_result_t iron_cond_init(iron_cond_t *cond)
{
    int err = pthread_cond_init(&cond->handle, NULL);
    if (err != 0) {
        return IRON_ERROR(IRON_ERR_MUTEX_ERROR, "pthread_cond_init failed");
    }
    cond->initialized = IRON_TRUE;
    return (iron_result_t)IRON_SUCCESS;
}

void iron_cond_destroy(iron_cond_t *cond)
{
    if (cond->initialized) {
        pthread_cond_destroy(&cond->handle);
        cond->initialized = IRON_FALSE;
    }
}

void iron_cond_wait(iron_cond_t *cond, iron_mutex_t *mutex)
{
    pthread_cond_wait(&cond->handle, &mutex->handle);
}

iron_bool iron_cond_timedwait(iron_cond_t *cond, iron_mutex_t *mutex,
                              iron_u32 timeout_ms)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += timeout_ms / 1000;
    ts.tv_nsec += (timeout_ms % 1000) * 1000000;
    if (ts.tv_nsec >= 1000000000) {
        ts.tv_sec++;
        ts.tv_nsec -= 1000000000;
    }
    return pthread_cond_timedwait(&cond->handle, &mutex->handle, &ts) == 0;
}

void iron_cond_signal(iron_cond_t *cond)
{
    pthread_cond_signal(&cond->handle);
}

void iron_cond_broadcast(iron_cond_t *cond)
{
    pthread_cond_broadcast(&cond->handle);
}

#else

iron_result_t iron_cond_init(iron_cond_t *cond)
{
    cond->initialized = IRON_TRUE;
    return (iron_result_t)IRON_SUCCESS;
}

void iron_cond_destroy(iron_cond_t *cond)
{
    cond->initialized = IRON_FALSE;
}

void iron_cond_wait(iron_cond_t *cond, iron_mutex_t *mutex)
{
    (void)cond; (void)mutex;
}

iron_bool iron_cond_timedwait(iron_cond_t *cond, iron_mutex_t *mutex,
                              iron_u32 timeout_ms)
{
    (void)cond; (void)mutex; (void)timeout_ms;
    return IRON_FALSE;
}

void iron_cond_signal(iron_cond_t *cond) { (void)cond; }
void iron_cond_broadcast(iron_cond_t *cond) { (void)cond; }

#endif

/* ============================================================================
 * Spinlock Implementation
 * ============================================================================ */

void iron_spinlock_init(iron_spinlock_t *lock)
{
    lock->lock = 0;
}

void iron_spinlock_lock(iron_spinlock_t *lock)
{
    while (!iron_atomic_cas_i32(&lock->lock, 0, 1)) {
        iron_thread_yield();
    }
}

iron_bool iron_spinlock_trylock(iron_spinlock_t *lock)
{
    return iron_atomic_cas_i32(&lock->lock, 0, 1);
}

void iron_spinlock_unlock(iron_spinlock_t *lock)
{
    iron_atomic_store_i32(&lock->lock, 0);
}

/* ============================================================================
 * Atomic Operations
 * ============================================================================ */

#if defined(IRON_OS_WINDOWS)

void iron_atomic_fence_acquire(void) { MemoryBarrier(); }
void iron_atomic_fence_release(void) { MemoryBarrier(); }
void iron_atomic_fence_seq_cst(void) { MemoryBarrier(); }

iron_i32 iron_atomic_load_i32(const volatile iron_i32 *ptr)
{
    return InterlockedCompareExchange((volatile LONG *)ptr, 0, 0);
}

void iron_atomic_store_i32(volatile iron_i32 *ptr, iron_i32 value)
{
    InterlockedExchange((volatile LONG *)ptr, value);
}

iron_i32 iron_atomic_exchange_i32(volatile iron_i32 *ptr, iron_i32 value)
{
    return InterlockedExchange((volatile LONG *)ptr, value);
}

iron_bool iron_atomic_cas_i32(volatile iron_i32 *ptr, iron_i32 expected, iron_i32 desired)
{
    return InterlockedCompareExchange((volatile LONG *)ptr, desired, expected) == expected;
}

iron_i32 iron_atomic_add_i32(volatile iron_i32 *ptr, iron_i32 value)
{
    return InterlockedExchangeAdd((volatile LONG *)ptr, value);
}

iron_i32 iron_atomic_sub_i32(volatile iron_i32 *ptr, iron_i32 value)
{
    return InterlockedExchangeAdd((volatile LONG *)ptr, -value);
}

iron_i32 iron_atomic_inc_i32(volatile iron_i32 *ptr)
{
    return InterlockedIncrement((volatile LONG *)ptr) - 1;
}

iron_i32 iron_atomic_dec_i32(volatile iron_i32 *ptr)
{
    return InterlockedDecrement((volatile LONG *)ptr) + 1;
}

iron_i32 iron_atomic_and_i32(volatile iron_i32 *ptr, iron_i32 value)
{
    return InterlockedAnd((volatile LONG *)ptr, value);
}

iron_i32 iron_atomic_or_i32(volatile iron_i32 *ptr, iron_i32 value)
{
    return InterlockedOr((volatile LONG *)ptr, value);
}

iron_i32 iron_atomic_xor_i32(volatile iron_i32 *ptr, iron_i32 value)
{
    return InterlockedXor((volatile LONG *)ptr, value);
}

void *iron_atomic_load_ptr(void *const volatile *ptr)
{
    return InterlockedCompareExchangePointer((void *volatile *)ptr, NULL, NULL);
}

void iron_atomic_store_ptr(void *volatile *ptr, void *value)
{
    InterlockedExchangePointer(ptr, value);
}

void *iron_atomic_exchange_ptr(void *volatile *ptr, void *value)
{
    return InterlockedExchangePointer(ptr, value);
}

iron_bool iron_atomic_cas_ptr(void *volatile *ptr, void *expected, void *desired)
{
    return InterlockedCompareExchangePointer(ptr, desired, expected) == expected;
}

#elif defined(__GNUC__) || defined(__clang__)

void iron_atomic_fence_acquire(void) { __atomic_thread_fence(__ATOMIC_ACQUIRE); }
void iron_atomic_fence_release(void) { __atomic_thread_fence(__ATOMIC_RELEASE); }
void iron_atomic_fence_seq_cst(void) { __atomic_thread_fence(__ATOMIC_SEQ_CST); }

iron_i32 iron_atomic_load_i32(const volatile iron_i32 *ptr)
{
    return __atomic_load_n(ptr, __ATOMIC_SEQ_CST);
}

void iron_atomic_store_i32(volatile iron_i32 *ptr, iron_i32 value)
{
    __atomic_store_n(ptr, value, __ATOMIC_SEQ_CST);
}

iron_i32 iron_atomic_exchange_i32(volatile iron_i32 *ptr, iron_i32 value)
{
    return __atomic_exchange_n(ptr, value, __ATOMIC_SEQ_CST);
}

iron_bool iron_atomic_cas_i32(volatile iron_i32 *ptr, iron_i32 expected, iron_i32 desired)
{
    return __atomic_compare_exchange_n(ptr, &expected, desired, 0,
                                       __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

iron_i32 iron_atomic_add_i32(volatile iron_i32 *ptr, iron_i32 value)
{
    return __atomic_fetch_add(ptr, value, __ATOMIC_SEQ_CST);
}

iron_i32 iron_atomic_sub_i32(volatile iron_i32 *ptr, iron_i32 value)
{
    return __atomic_fetch_sub(ptr, value, __ATOMIC_SEQ_CST);
}

iron_i32 iron_atomic_inc_i32(volatile iron_i32 *ptr)
{
    return __atomic_fetch_add(ptr, 1, __ATOMIC_SEQ_CST);
}

iron_i32 iron_atomic_dec_i32(volatile iron_i32 *ptr)
{
    return __atomic_fetch_sub(ptr, 1, __ATOMIC_SEQ_CST);
}

iron_i32 iron_atomic_and_i32(volatile iron_i32 *ptr, iron_i32 value)
{
    return __atomic_fetch_and(ptr, value, __ATOMIC_SEQ_CST);
}

iron_i32 iron_atomic_or_i32(volatile iron_i32 *ptr, iron_i32 value)
{
    return __atomic_fetch_or(ptr, value, __ATOMIC_SEQ_CST);
}

iron_i32 iron_atomic_xor_i32(volatile iron_i32 *ptr, iron_i32 value)
{
    return __atomic_fetch_xor(ptr, value, __ATOMIC_SEQ_CST);
}

void *iron_atomic_load_ptr(void *const volatile *ptr)
{
    return __atomic_load_n(ptr, __ATOMIC_SEQ_CST);
}

void iron_atomic_store_ptr(void *volatile *ptr, void *value)
{
    __atomic_store_n(ptr, value, __ATOMIC_SEQ_CST);
}

void *iron_atomic_exchange_ptr(void *volatile *ptr, void *value)
{
    return __atomic_exchange_n(ptr, value, __ATOMIC_SEQ_CST);
}

iron_bool iron_atomic_cas_ptr(void *volatile *ptr, void *expected, void *desired)
{
    return __atomic_compare_exchange_n(ptr, &expected, desired, 0,
                                       __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

#ifndef IRON_NO_NATIVE_64BIT
iron_i64 iron_atomic_load_i64(const volatile iron_i64 *ptr)
{
    return __atomic_load_n(ptr, __ATOMIC_SEQ_CST);
}

void iron_atomic_store_i64(volatile iron_i64 *ptr, iron_i64 value)
{
    __atomic_store_n(ptr, value, __ATOMIC_SEQ_CST);
}

iron_i64 iron_atomic_exchange_i64(volatile iron_i64 *ptr, iron_i64 value)
{
    return __atomic_exchange_n(ptr, value, __ATOMIC_SEQ_CST);
}

iron_bool iron_atomic_cas_i64(volatile iron_i64 *ptr, iron_i64 expected, iron_i64 desired)
{
    return __atomic_compare_exchange_n(ptr, &expected, desired, 0,
                                       __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

iron_i64 iron_atomic_add_i64(volatile iron_i64 *ptr, iron_i64 value)
{
    return __atomic_fetch_add(ptr, value, __ATOMIC_SEQ_CST);
}
#endif

#else

/* Fallback non-atomic implementation */
void iron_atomic_fence_acquire(void) {}
void iron_atomic_fence_release(void) {}
void iron_atomic_fence_seq_cst(void) {}

iron_i32 iron_atomic_load_i32(const volatile iron_i32 *ptr) { return *ptr; }
void iron_atomic_store_i32(volatile iron_i32 *ptr, iron_i32 value) { *ptr = value; }
iron_i32 iron_atomic_exchange_i32(volatile iron_i32 *ptr, iron_i32 value)
{
    iron_i32 old = *ptr;
    *ptr = value;
    return old;
}
iron_bool iron_atomic_cas_i32(volatile iron_i32 *ptr, iron_i32 expected, iron_i32 desired)
{
    if (*ptr == expected) { *ptr = desired; return IRON_TRUE; }
    return IRON_FALSE;
}
iron_i32 iron_atomic_add_i32(volatile iron_i32 *ptr, iron_i32 value)
{
    iron_i32 old = *ptr;
    *ptr += value;
    return old;
}
iron_i32 iron_atomic_sub_i32(volatile iron_i32 *ptr, iron_i32 value)
{
    iron_i32 old = *ptr;
    *ptr -= value;
    return old;
}
iron_i32 iron_atomic_inc_i32(volatile iron_i32 *ptr) { return (*ptr)++; }
iron_i32 iron_atomic_dec_i32(volatile iron_i32 *ptr) { return (*ptr)--; }
iron_i32 iron_atomic_and_i32(volatile iron_i32 *ptr, iron_i32 value)
{
    iron_i32 old = *ptr;
    *ptr &= value;
    return old;
}
iron_i32 iron_atomic_or_i32(volatile iron_i32 *ptr, iron_i32 value)
{
    iron_i32 old = *ptr;
    *ptr |= value;
    return old;
}
iron_i32 iron_atomic_xor_i32(volatile iron_i32 *ptr, iron_i32 value)
{
    iron_i32 old = *ptr;
    *ptr ^= value;
    return old;
}
void *iron_atomic_load_ptr(void *const volatile *ptr) { return (void *)*ptr; }
void iron_atomic_store_ptr(void *volatile *ptr, void *value) { *ptr = value; }
void *iron_atomic_exchange_ptr(void *volatile *ptr, void *value)
{
    void *old = *ptr;
    *ptr = value;
    return old;
}
iron_bool iron_atomic_cas_ptr(void *volatile *ptr, void *expected, void *desired)
{
    if (*ptr == expected) { *ptr = desired; return IRON_TRUE; }
    return IRON_FALSE;
}

#endif
