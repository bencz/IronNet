/*
 * IronNet CLR Interpreter
 * thread_posix.c - POSIX threading and atomic backend
 */

#if defined(__linux__) && !defined(_GNU_SOURCE)
    #define _GNU_SOURCE
#endif
#if defined(_AIX) && !defined(_ALL_SOURCE)
    #define _ALL_SOURCE
#endif
#ifndef _POSIX_C_SOURCE
    #define _POSIX_C_SOURCE 200809L
#endif
#ifndef _XOPEN_SOURCE
    #define _XOPEN_SOURCE 700
#endif

#include "iron/thread.h"
#include <errno.h>
#include <sched.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

typedef struct thread_start_info {
    iron_thread_fn function;
    void *argument;
} thread_start_info_t;

static void *thread_proc(void *parameter)
{
    thread_start_info_t *info;
    iron_thread_fn function;
    void *argument;

    info = (thread_start_info_t *)parameter;
    function = info->function;
    argument = info->argument;
    free(info);
    return function(argument);
}

static iron_result_t configure_thread_attributes(pthread_attr_t *attributes, const iron_thread_attr_t *requested)
{
    int error;

    if (!requested) {
        return IRON_SUCCESS;
    }

    if (requested->stack_size != 0) {
        error = pthread_attr_setstacksize(attributes, requested->stack_size);
        if (error != 0) {
            return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Invalid POSIX thread stack size");
        }
    }

    if (requested->detached) {
        error = pthread_attr_setdetachstate(attributes, PTHREAD_CREATE_DETACHED);
        if (error != 0) {
            return IRON_ERROR(IRON_ERR_THREAD_CREATE, "Failed to configure detached POSIX thread");
        }
    }

    if (requested->priority != 0) {
        struct sched_param scheduling;
        int maximum_priority;
        int minimum_priority;

        minimum_priority = sched_get_priority_min(SCHED_RR);
        maximum_priority = sched_get_priority_max(SCHED_RR);
        if (minimum_priority == -1 || maximum_priority == -1 || requested->priority < minimum_priority || requested->priority > maximum_priority) {
            return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "POSIX thread priority is outside the SCHED_RR range");
        }

        memset(&scheduling, 0, sizeof(scheduling));
        scheduling.sched_priority = requested->priority;
        if (pthread_attr_setschedpolicy(attributes, SCHED_RR) != 0 ||
            pthread_attr_setschedparam(attributes, &scheduling) != 0 ||
            pthread_attr_setinheritsched(attributes, PTHREAD_EXPLICIT_SCHED) != 0) {
            return IRON_ERROR(IRON_ERR_THREAD_CREATE, "Failed to configure POSIX thread priority");
        }
    }

    return IRON_SUCCESS;
}

iron_result_t iron_thread_create_ex(iron_thread_t *thread, const iron_thread_attr_t *attr, iron_thread_fn fn, void *arg)
{
    thread_start_info_t *info;
    pthread_attr_t attributes;
    iron_result_t result;
    int error;

    if (!thread || !fn) {
        return IRON_ERROR(IRON_ERR_NULL_POINTER, "Invalid thread or entry point");
    }

    memset(thread, 0, sizeof(*thread));
    error = pthread_attr_init(&attributes);
    if (error != 0) {
        return IRON_ERROR(IRON_ERR_THREAD_CREATE, "pthread_attr_init failed");
    }

    result = configure_thread_attributes(&attributes, attr);
    if (!IRON_RESULT_OK(result)) {
        pthread_attr_destroy(&attributes);
        return result;
    }

    info = (thread_start_info_t *)malloc(sizeof(*info));
    if (!info) {
        pthread_attr_destroy(&attributes);
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate thread start data");
    }

    info->function = fn;
    info->argument = arg;
    error = pthread_create(&thread->handle, &attributes, thread_proc, info);
    pthread_attr_destroy(&attributes);
    if (error != 0) {
        free(info);
        return IRON_ERROR(IRON_ERR_THREAD_CREATE, "pthread_create failed");
    }

    thread->joinable = !attr || !attr->detached;
    return IRON_SUCCESS;
}

iron_result_t iron_thread_join(iron_thread_t *thread, void **result)
{
    void *thread_result;
    int error;

    if (!thread || !thread->joinable) {
        return IRON_ERROR(IRON_ERR_THREAD_JOIN, "Thread is not joinable");
    }

    error = pthread_join(thread->handle, &thread_result);
    if (error != 0) {
        return IRON_ERROR(IRON_ERR_THREAD_JOIN, "pthread_join failed");
    }

    thread->joinable = IRON_FALSE;
    if (result) {
        *result = thread_result;
    }
    return IRON_SUCCESS;
}

iron_result_t iron_thread_detach(iron_thread_t *thread)
{
    if (!thread || !thread->joinable) {
        return IRON_ERROR(IRON_ERR_THREAD_JOIN, "Thread is not joinable");
    }

    if (pthread_detach(thread->handle) != 0) {
        return IRON_ERROR(IRON_ERR_THREAD_JOIN, "pthread_detach failed");
    }

    thread->joinable = IRON_FALSE;
    return IRON_SUCCESS;
}

iron_u32 iron_thread_current_id(void)
{
    pthread_t self;
    const iron_u8 *bytes;
    iron_u32 hash;
    iron_size index;

    self = pthread_self();
    bytes = (const iron_u8 *)&self;
    hash = 2166136261u;
    for (index = 0; index < sizeof(self); index++) {
        hash = (hash ^ bytes[index]) * 16777619u;
    }

    return hash ? hash : 1;
}

void iron_thread_yield(void)
{
    sched_yield();
}

void iron_thread_sleep(iron_u32 ms)
{
    struct timespec requested;
    struct timespec remaining;

    requested.tv_sec = (time_t)(ms / 1000);
    requested.tv_nsec = (long)(ms % 1000) * 1000000L;
    while (nanosleep(&requested, &remaining) != 0 && errno == EINTR) {
        requested = remaining;
    }
}

void iron_thread_set_name(const char *name)
{
    if (!name) {
        return;
    }

#if defined(IRON_OS_LINUX)
    {
        char truncated[16];

        strncpy(truncated, name, sizeof(truncated) - 1);
        truncated[sizeof(truncated) - 1] = '\0';
        pthread_setname_np(pthread_self(), truncated);
    }
#elif defined(IRON_OS_MACOS)
    pthread_setname_np(name);
#elif defined(IRON_OS_AIX)
    pthread_setname_np(pthread_self(), name);
#else
    (void)name;
#endif
}

iron_result_t iron_mutex_init(iron_mutex_t *mutex)
{
    if (!mutex) {
        return IRON_ERROR(IRON_ERR_NULL_POINTER, "mutex is NULL");
    }

    memset(mutex, 0, sizeof(*mutex));
    if (pthread_mutex_init(&mutex->handle, NULL) != 0) {
        return IRON_ERROR(IRON_ERR_MUTEX_ERROR, "pthread_mutex_init failed");
    }
    mutex->initialized = IRON_TRUE;
    return IRON_SUCCESS;
}

void iron_mutex_destroy(iron_mutex_t *mutex)
{
    if (mutex && mutex->initialized) {
        pthread_mutex_destroy(&mutex->handle);
        mutex->initialized = IRON_FALSE;
    }
}

void iron_mutex_lock(iron_mutex_t *mutex) { pthread_mutex_lock(&mutex->handle); }
iron_bool iron_mutex_trylock(iron_mutex_t *mutex) { return pthread_mutex_trylock(&mutex->handle) == 0; }
void iron_mutex_unlock(iron_mutex_t *mutex) { pthread_mutex_unlock(&mutex->handle); }

iron_result_t iron_rmutex_init(iron_rmutex_t *mutex)
{
    pthread_mutexattr_t attributes;
    int error;

    if (!mutex) {
        return IRON_ERROR(IRON_ERR_NULL_POINTER, "recursive mutex is NULL");
    }

    memset(mutex, 0, sizeof(*mutex));
    error = pthread_mutexattr_init(&attributes);
    if (error != 0) {
        return IRON_ERROR(IRON_ERR_MUTEX_ERROR, "pthread_mutexattr_init failed");
    }

    error = pthread_mutexattr_settype(&attributes, PTHREAD_MUTEX_RECURSIVE);
    if (error == 0) {
        error = pthread_mutex_init(&mutex->handle, &attributes);
    }
    pthread_mutexattr_destroy(&attributes);
    if (error != 0) {
        return IRON_ERROR(IRON_ERR_MUTEX_ERROR, "Failed to initialize recursive POSIX mutex");
    }

    mutex->initialized = IRON_TRUE;
    return IRON_SUCCESS;
}

void iron_rmutex_destroy(iron_rmutex_t *mutex)
{
    if (mutex && mutex->initialized) {
        pthread_mutex_destroy(&mutex->handle);
        mutex->initialized = IRON_FALSE;
    }
}

void iron_rmutex_lock(iron_rmutex_t *mutex) { pthread_mutex_lock(&mutex->handle); }
iron_bool iron_rmutex_trylock(iron_rmutex_t *mutex) { return pthread_mutex_trylock(&mutex->handle) == 0; }
void iron_rmutex_unlock(iron_rmutex_t *mutex) { pthread_mutex_unlock(&mutex->handle); }

iron_result_t iron_cond_init(iron_cond_t *cond)
{
    if (!cond) {
        return IRON_ERROR(IRON_ERR_NULL_POINTER, "condition variable is NULL");
    }

    memset(cond, 0, sizeof(*cond));
    if (pthread_cond_init(&cond->handle, NULL) != 0) {
        return IRON_ERROR(IRON_ERR_MUTEX_ERROR, "pthread_cond_init failed");
    }
    cond->initialized = IRON_TRUE;
    return IRON_SUCCESS;
}

void iron_cond_destroy(iron_cond_t *cond)
{
    if (cond && cond->initialized) {
        pthread_cond_destroy(&cond->handle);
        cond->initialized = IRON_FALSE;
    }
}

void iron_cond_wait(iron_cond_t *cond, iron_mutex_t *mutex) { pthread_cond_wait(&cond->handle, &mutex->handle); }

iron_bool iron_cond_timedwait(iron_cond_t *cond, iron_mutex_t *mutex, iron_u32 timeout_ms)
{
    struct timespec deadline;

    if (clock_gettime(CLOCK_REALTIME, &deadline) != 0) {
        return IRON_FALSE;
    }

    deadline.tv_sec += (time_t)(timeout_ms / 1000);
    deadline.tv_nsec += (long)(timeout_ms % 1000) * 1000000L;
    if (deadline.tv_nsec >= 1000000000L) {
        deadline.tv_sec++;
        deadline.tv_nsec -= 1000000000L;
    }

    return pthread_cond_timedwait(&cond->handle, &mutex->handle, &deadline) == 0;
}

void iron_cond_signal(iron_cond_t *cond) { pthread_cond_signal(&cond->handle); }
void iron_cond_broadcast(iron_cond_t *cond) { pthread_cond_broadcast(&cond->handle); }

iron_result_t iron_tls_create(iron_tls_key_t *key)
{
    return iron_tls_create_dtor(key, NULL);
}

iron_result_t iron_tls_create_dtor(iron_tls_key_t *key, void (*destructor)(void *))
{
    if (!key) {
        return IRON_ERROR(IRON_ERR_NULL_POINTER, "TLS key is NULL");
    }

    if (pthread_key_create(&key->key, destructor) != 0) {
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "pthread_key_create failed");
    }
    key->initialized = IRON_TRUE;
    return IRON_SUCCESS;
}

void iron_tls_destroy(iron_tls_key_t *key)
{
    if (key && key->initialized) {
        pthread_key_delete(key->key);
        key->initialized = IRON_FALSE;
    }
}

void *iron_tls_get(iron_tls_key_t *key)
{
    return key && key->initialized ? pthread_getspecific(key->key) : NULL;
}

void iron_tls_set(iron_tls_key_t *key, void *value)
{
    if (key && key->initialized) {
        pthread_setspecific(key->key, value);
    }
}

#if !defined(__GNUC__) && !defined(__clang__)
    #error "The POSIX backend requires compiler atomic builtins"
#endif

void iron_atomic_fence_acquire(void) { __atomic_thread_fence(__ATOMIC_ACQUIRE); }
void iron_atomic_fence_release(void) { __atomic_thread_fence(__ATOMIC_RELEASE); }
void iron_atomic_fence_seq_cst(void) { __atomic_thread_fence(__ATOMIC_SEQ_CST); }
iron_i32 iron_atomic_load_i32(const volatile iron_i32 *ptr) { return __atomic_load_n(ptr, __ATOMIC_SEQ_CST); }
void iron_atomic_store_i32(volatile iron_i32 *ptr, iron_i32 value) { __atomic_store_n(ptr, value, __ATOMIC_SEQ_CST); }
iron_i32 iron_atomic_exchange_i32(volatile iron_i32 *ptr, iron_i32 value) { return __atomic_exchange_n(ptr, value, __ATOMIC_SEQ_CST); }

iron_bool iron_atomic_cas_i32(volatile iron_i32 *ptr, iron_i32 expected, iron_i32 desired)
{
    return __atomic_compare_exchange_n(ptr, &expected, desired, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

iron_i32 iron_atomic_add_i32(volatile iron_i32 *ptr, iron_i32 value) { return __atomic_fetch_add(ptr, value, __ATOMIC_SEQ_CST); }
iron_i32 iron_atomic_sub_i32(volatile iron_i32 *ptr, iron_i32 value) { return __atomic_fetch_sub(ptr, value, __ATOMIC_SEQ_CST); }
iron_i32 iron_atomic_inc_i32(volatile iron_i32 *ptr) { return __atomic_fetch_add(ptr, 1, __ATOMIC_SEQ_CST); }
iron_i32 iron_atomic_dec_i32(volatile iron_i32 *ptr) { return __atomic_fetch_sub(ptr, 1, __ATOMIC_SEQ_CST); }
iron_i32 iron_atomic_and_i32(volatile iron_i32 *ptr, iron_i32 value) { return __atomic_fetch_and(ptr, value, __ATOMIC_SEQ_CST); }
iron_i32 iron_atomic_or_i32(volatile iron_i32 *ptr, iron_i32 value) { return __atomic_fetch_or(ptr, value, __ATOMIC_SEQ_CST); }
iron_i32 iron_atomic_xor_i32(volatile iron_i32 *ptr, iron_i32 value) { return __atomic_fetch_xor(ptr, value, __ATOMIC_SEQ_CST); }
void *iron_atomic_load_ptr(void *const volatile *ptr) { return __atomic_load_n(ptr, __ATOMIC_SEQ_CST); }
void iron_atomic_store_ptr(void *volatile *ptr, void *value) { __atomic_store_n(ptr, value, __ATOMIC_SEQ_CST); }
void *iron_atomic_exchange_ptr(void *volatile *ptr, void *value) { return __atomic_exchange_n(ptr, value, __ATOMIC_SEQ_CST); }

iron_bool iron_atomic_cas_ptr(void *volatile *ptr, void *expected, void *desired)
{
    return __atomic_compare_exchange_n(ptr, &expected, desired, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

#ifndef IRON_NO_NATIVE_64BIT
iron_i64 iron_atomic_load_i64(const volatile iron_i64 *ptr) { return __atomic_load_n(ptr, __ATOMIC_SEQ_CST); }
void iron_atomic_store_i64(volatile iron_i64 *ptr, iron_i64 value) { __atomic_store_n(ptr, value, __ATOMIC_SEQ_CST); }
iron_i64 iron_atomic_exchange_i64(volatile iron_i64 *ptr, iron_i64 value) { return __atomic_exchange_n(ptr, value, __ATOMIC_SEQ_CST); }

iron_bool iron_atomic_cas_i64(volatile iron_i64 *ptr, iron_i64 expected, iron_i64 desired)
{
    return __atomic_compare_exchange_n(ptr, &expected, desired, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

iron_i64 iron_atomic_add_i64(volatile iron_i64 *ptr, iron_i64 value) { return __atomic_fetch_add(ptr, value, __ATOMIC_SEQ_CST); }
#endif
