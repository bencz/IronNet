/*
 * IronNet CLR Interpreter
 * thread.h - Platform-independent threading primitives
 * 
 * Provides:
 * - Thread creation and management
 * - Mutexes and condition variables
 * - Thread-local storage
 * - Atomic operations
 * 
 * Strict C99 compatible
 */

#ifndef IRON_THREAD_H
#define IRON_THREAD_H

/* Enable POSIX features before any includes */
#if !defined(_WIN32) && !defined(_WIN64)
    #ifndef _POSIX_C_SOURCE
        #define _POSIX_C_SOURCE 200809L
    #endif
    #ifndef _XOPEN_SOURCE
        #define _XOPEN_SOURCE 700
    #endif
#endif

#include "platform.h"
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Platform-specific includes
 * ============================================================================ */

#if defined(IRON_OS_WINDOWS)
    /* Windows threading */
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    /* Forward declare - actual include in .c file */
#elif defined(IRON_OS_POSIX)
    /* POSIX threading */
    #include <pthread.h>
#endif

/* ============================================================================
 * Thread Types
 * ============================================================================ */

/* Thread handle */
typedef struct iron_thread {
#if defined(IRON_OS_WINDOWS)
    void *handle;
    unsigned long id;
#elif defined(IRON_OS_POSIX)
    pthread_t handle;
#else
    #error "No IronNet thread backend is available for this platform"
#endif
    iron_bool joinable;
    void *platform_data;
} iron_thread_t;

/* Thread function type */
typedef void *(*iron_thread_fn)(void *arg);

/* Thread attributes */
typedef struct iron_thread_attr {
    iron_size stack_size;
    iron_bool detached;
    int priority;
} iron_thread_attr_t;

#define IRON_THREAD_ATTR_DEFAULT { 0, IRON_FALSE, 0 }

/* ============================================================================
 * Mutex Types
 * ============================================================================ */

typedef struct iron_mutex {
#if defined(IRON_OS_WINDOWS)
    void *handle;  /* CRITICAL_SECTION or SRWLOCK */
#elif defined(IRON_OS_POSIX)
    pthread_mutex_t handle;
#else
    #error "No IronNet mutex backend is available for this platform"
#endif
    iron_bool initialized;
} iron_mutex_t;

/* Recursive mutex */
typedef struct iron_rmutex {
#if defined(IRON_OS_WINDOWS)
    void *handle;
#elif defined(IRON_OS_POSIX)
    pthread_mutex_t handle;
#else
    #error "No IronNet recursive mutex backend is available for this platform"
#endif
    iron_bool initialized;
} iron_rmutex_t;

/* ============================================================================
 * Condition Variable
 * ============================================================================ */

typedef struct iron_cond {
#if defined(IRON_OS_WINDOWS)
    void *handle;
#elif defined(IRON_OS_POSIX)
    pthread_cond_t handle;
#else
    #error "No IronNet condition-variable backend is available for this platform"
#endif
    iron_bool initialized;
} iron_cond_t;

/* ============================================================================
 * Read-Write Lock
 * ============================================================================ */

typedef struct iron_rwlock {
    iron_mutex_t mutex;
    iron_cond_t readers_changed;
    iron_cond_t writers_changed;
    iron_u32 active_readers;
    iron_u32 waiting_writers;
    iron_u32 writer_owner;
    iron_bool writer_active;
    iron_bool initialized;
} iron_rwlock_t;

/* ============================================================================
 * Semaphore
 * ============================================================================ */

typedef struct iron_semaphore {
    iron_mutex_t mutex;
    iron_cond_t cond;
    iron_u32 count;
    iron_bool initialized;
} iron_semaphore_t;

/* ============================================================================
 * Event (Manual/Auto Reset)
 * ============================================================================ */

typedef struct iron_event {
    iron_mutex_t mutex;
    iron_cond_t cond;
    iron_bool signaled;
    iron_bool manual_reset;
    iron_bool initialized;
} iron_event_t;

/* ============================================================================
 * Thread-Local Storage
 * ============================================================================ */

typedef struct iron_tls_key {
#if defined(IRON_OS_WINDOWS)
    unsigned long key;
#elif defined(IRON_OS_POSIX)
    pthread_key_t key;
#else
    #error "No IronNet TLS backend is available for this platform"
#endif
    iron_bool initialized;
} iron_tls_key_t;

/* ============================================================================
 * Once Control (for one-time initialization)
 * ============================================================================ */

typedef struct iron_once {
    volatile iron_i32 state;
} iron_once_t;

#define IRON_ONCE_INIT { 0 }

/* ============================================================================
 * Thread API
 * ============================================================================ */

/* Create and start thread */
IRON_API iron_result_t iron_thread_create_ex(iron_thread_t *thread,
                                              const iron_thread_attr_t *attr,
                                              iron_thread_fn fn,
                                              void *arg);

/* Simple thread creation */
IRON_API iron_result_t iron_thread_create_simple(iron_thread_t *thread,
                                                  iron_thread_fn fn,
                                                  void *arg);

/* Wait for thread to finish */
IRON_API iron_result_t iron_thread_join(iron_thread_t *thread, void **result);

/* Detach thread */
IRON_API iron_result_t iron_thread_detach(iron_thread_t *thread);

/* Get current thread ID */
IRON_API iron_u32 iron_thread_current_id(void);

/* Yield execution */
IRON_API void iron_thread_yield(void);

/* Sleep for milliseconds */
IRON_API void iron_thread_sleep(iron_u32 ms);

/* Set thread name (for debugging) */
IRON_API void iron_thread_set_name(const char *name);

/* ============================================================================
 * Mutex API
 * ============================================================================ */

/* Initialize mutex */
IRON_API iron_result_t iron_mutex_init(iron_mutex_t *mutex);

/* Destroy mutex */
IRON_API void iron_mutex_destroy(iron_mutex_t *mutex);

/* Lock mutex */
IRON_API void iron_mutex_lock(iron_mutex_t *mutex);

/* Try to lock mutex (non-blocking) */
IRON_API iron_bool iron_mutex_trylock(iron_mutex_t *mutex);

/* Unlock mutex */
IRON_API void iron_mutex_unlock(iron_mutex_t *mutex);

/* ============================================================================
 * Recursive Mutex API
 * ============================================================================ */

IRON_API iron_result_t iron_rmutex_init(iron_rmutex_t *mutex);
IRON_API void iron_rmutex_destroy(iron_rmutex_t *mutex);
IRON_API void iron_rmutex_lock(iron_rmutex_t *mutex);
IRON_API iron_bool iron_rmutex_trylock(iron_rmutex_t *mutex);
IRON_API void iron_rmutex_unlock(iron_rmutex_t *mutex);

/* ============================================================================
 * Condition Variable API
 * ============================================================================ */

IRON_API iron_result_t iron_cond_init(iron_cond_t *cond);
IRON_API void iron_cond_destroy(iron_cond_t *cond);
IRON_API void iron_cond_wait(iron_cond_t *cond, iron_mutex_t *mutex);
IRON_API iron_bool iron_cond_timedwait(iron_cond_t *cond, iron_mutex_t *mutex,
                                        iron_u32 timeout_ms);
IRON_API void iron_cond_signal(iron_cond_t *cond);
IRON_API void iron_cond_broadcast(iron_cond_t *cond);

/* ============================================================================
 * Read-Write Lock API
 * ============================================================================ */

IRON_API iron_result_t iron_rwlock_init(iron_rwlock_t *rwlock);
IRON_API void iron_rwlock_destroy(iron_rwlock_t *rwlock);
IRON_API void iron_rwlock_rdlock(iron_rwlock_t *rwlock);
IRON_API void iron_rwlock_wrlock(iron_rwlock_t *rwlock);
IRON_API iron_bool iron_rwlock_tryrdlock(iron_rwlock_t *rwlock);
IRON_API iron_bool iron_rwlock_trywrlock(iron_rwlock_t *rwlock);
IRON_API void iron_rwlock_unlock(iron_rwlock_t *rwlock);

/* ============================================================================
 * Semaphore API
 * ============================================================================ */

IRON_API iron_result_t iron_semaphore_init(iron_semaphore_t *sem, iron_u32 initial);
IRON_API void iron_semaphore_destroy(iron_semaphore_t *sem);
IRON_API void iron_semaphore_wait(iron_semaphore_t *sem);
IRON_API iron_bool iron_semaphore_trywait(iron_semaphore_t *sem);
IRON_API iron_bool iron_semaphore_timedwait(iron_semaphore_t *sem, iron_u32 timeout_ms);
IRON_API void iron_semaphore_post(iron_semaphore_t *sem);

/* ============================================================================
 * Event API
 * ============================================================================ */

IRON_API iron_result_t iron_event_init(iron_event_t *event, iron_bool manual_reset,
                                        iron_bool initial_state);
IRON_API void iron_event_destroy(iron_event_t *event);
IRON_API void iron_event_set(iron_event_t *event);
IRON_API void iron_event_reset(iron_event_t *event);
IRON_API void iron_event_wait(iron_event_t *event);
IRON_API iron_bool iron_event_timedwait(iron_event_t *event, iron_u32 timeout_ms);

/* ============================================================================
 * Thread-Local Storage API
 * ============================================================================ */

IRON_API iron_result_t iron_tls_create(iron_tls_key_t *key);
IRON_API iron_result_t iron_tls_create_dtor(iron_tls_key_t *key, 
                                             void (*destructor)(void*));
IRON_API void iron_tls_destroy(iron_tls_key_t *key);
IRON_API void *iron_tls_get(iron_tls_key_t *key);
IRON_API void iron_tls_set(iron_tls_key_t *key, void *value);

/* ============================================================================
 * Once API
 * ============================================================================ */

IRON_API void iron_once(iron_once_t *once, void (*init_fn)(void));

/* ============================================================================
 * Atomic Operations
 * ============================================================================ */

/* Memory barriers */
IRON_API void iron_atomic_fence_acquire(void);
IRON_API void iron_atomic_fence_release(void);
IRON_API void iron_atomic_fence_seq_cst(void);

/* Atomic load/store */
IRON_API iron_i32 iron_atomic_load_i32(const volatile iron_i32 *ptr);
IRON_API void iron_atomic_store_i32(volatile iron_i32 *ptr, iron_i32 value);
IRON_API void *iron_atomic_load_ptr(void *const volatile *ptr);
IRON_API void iron_atomic_store_ptr(void *volatile *ptr, void *value);

/* Atomic exchange */
IRON_API iron_i32 iron_atomic_exchange_i32(volatile iron_i32 *ptr, iron_i32 value);
IRON_API void *iron_atomic_exchange_ptr(void *volatile *ptr, void *value);

/* Atomic compare-and-swap */
IRON_API iron_bool iron_atomic_cas_i32(volatile iron_i32 *ptr, 
                                        iron_i32 expected, iron_i32 desired);
IRON_API iron_bool iron_atomic_cas_ptr(void *volatile *ptr,
                                        void *expected, void *desired);

/* Atomic arithmetic */
IRON_API iron_i32 iron_atomic_add_i32(volatile iron_i32 *ptr, iron_i32 value);
IRON_API iron_i32 iron_atomic_sub_i32(volatile iron_i32 *ptr, iron_i32 value);
IRON_API iron_i32 iron_atomic_inc_i32(volatile iron_i32 *ptr);
IRON_API iron_i32 iron_atomic_dec_i32(volatile iron_i32 *ptr);

/* Atomic bitwise */
IRON_API iron_i32 iron_atomic_and_i32(volatile iron_i32 *ptr, iron_i32 value);
IRON_API iron_i32 iron_atomic_or_i32(volatile iron_i32 *ptr, iron_i32 value);
IRON_API iron_i32 iron_atomic_xor_i32(volatile iron_i32 *ptr, iron_i32 value);

/* 64-bit atomics (if available) */
#ifndef IRON_NO_NATIVE_64BIT
IRON_API iron_i64 iron_atomic_load_i64(const volatile iron_i64 *ptr);
IRON_API void iron_atomic_store_i64(volatile iron_i64 *ptr, iron_i64 value);
IRON_API iron_i64 iron_atomic_exchange_i64(volatile iron_i64 *ptr, iron_i64 value);
IRON_API iron_bool iron_atomic_cas_i64(volatile iron_i64 *ptr,
                                        iron_i64 expected, iron_i64 desired);
IRON_API iron_i64 iron_atomic_add_i64(volatile iron_i64 *ptr, iron_i64 value);
#endif

/* ============================================================================
 * Spin Lock (for very short critical sections)
 * ============================================================================ */

typedef struct iron_spinlock {
    volatile iron_i32 lock;
} iron_spinlock_t;

#define IRON_SPINLOCK_INIT { 0 }

IRON_API void iron_spinlock_init(iron_spinlock_t *lock);
IRON_API void iron_spinlock_lock(iron_spinlock_t *lock);
IRON_API iron_bool iron_spinlock_trylock(iron_spinlock_t *lock);
IRON_API void iron_spinlock_unlock(iron_spinlock_t *lock);

#ifdef __cplusplus
}
#endif

#endif /* IRON_THREAD_H */
