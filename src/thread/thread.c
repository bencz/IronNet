/*
 * IronNet CLR Interpreter
 * thread.c - Platform-independent synchronization algorithms
 */

#include "iron/thread.h"
#include <limits.h>
#include <string.h>

static iron_u32 timeout_remaining(iron_u64 deadline)
{
    iron_u64 now;
    iron_u64 remaining;

    now = iron_platform_monotonic_milliseconds();
    if (now >= deadline) {
        return 0;
    }

    remaining = deadline - now;
    if (remaining > UINT32_MAX) {
        return UINT32_MAX;
    }

    return (iron_u32)remaining;
}

iron_result_t iron_thread_create_simple(iron_thread_t *thread, iron_thread_fn fn, void *arg)
{
    return iron_thread_create_ex(thread, NULL, fn, arg);
}

iron_result_t iron_rwlock_init(iron_rwlock_t *rwlock)
{
    iron_result_t result;

    if (!rwlock) {
        return IRON_ERROR(IRON_ERR_NULL_POINTER, "rwlock is NULL");
    }

    memset(rwlock, 0, sizeof(*rwlock));
    result = iron_mutex_init(&rwlock->mutex);
    if (!IRON_RESULT_OK(result)) {
        return result;
    }

    result = iron_cond_init(&rwlock->readers_changed);
    if (!IRON_RESULT_OK(result)) {
        iron_mutex_destroy(&rwlock->mutex);
        return result;
    }

    result = iron_cond_init(&rwlock->writers_changed);
    if (!IRON_RESULT_OK(result)) {
        iron_cond_destroy(&rwlock->readers_changed);
        iron_mutex_destroy(&rwlock->mutex);
        return result;
    }

    rwlock->initialized = IRON_TRUE;
    return IRON_SUCCESS;
}

void iron_rwlock_destroy(iron_rwlock_t *rwlock)
{
    if (!rwlock || !rwlock->initialized) {
        return;
    }

    iron_cond_destroy(&rwlock->writers_changed);
    iron_cond_destroy(&rwlock->readers_changed);
    iron_mutex_destroy(&rwlock->mutex);
    rwlock->initialized = IRON_FALSE;
}

void iron_rwlock_rdlock(iron_rwlock_t *rwlock)
{
    iron_mutex_lock(&rwlock->mutex);
    while (rwlock->writer_active || rwlock->waiting_writers != 0) {
        iron_cond_wait(&rwlock->readers_changed, &rwlock->mutex);
    }
    rwlock->active_readers++;
    iron_mutex_unlock(&rwlock->mutex);
}

void iron_rwlock_wrlock(iron_rwlock_t *rwlock)
{
    iron_mutex_lock(&rwlock->mutex);
    rwlock->waiting_writers++;
    while (rwlock->writer_active || rwlock->active_readers != 0) {
        iron_cond_wait(&rwlock->writers_changed, &rwlock->mutex);
    }
    rwlock->waiting_writers--;
    rwlock->writer_active = IRON_TRUE;
    rwlock->writer_owner = iron_thread_current_id();
    iron_mutex_unlock(&rwlock->mutex);
}

iron_bool iron_rwlock_tryrdlock(iron_rwlock_t *rwlock)
{
    iron_bool acquired;

    iron_mutex_lock(&rwlock->mutex);
    acquired = !rwlock->writer_active && rwlock->waiting_writers == 0;
    if (acquired) {
        rwlock->active_readers++;
    }
    iron_mutex_unlock(&rwlock->mutex);
    return acquired;
}

iron_bool iron_rwlock_trywrlock(iron_rwlock_t *rwlock)
{
    iron_bool acquired;

    iron_mutex_lock(&rwlock->mutex);
    acquired = !rwlock->writer_active && rwlock->active_readers == 0;
    if (acquired) {
        rwlock->writer_active = IRON_TRUE;
        rwlock->writer_owner = iron_thread_current_id();
    }
    iron_mutex_unlock(&rwlock->mutex);
    return acquired;
}

void iron_rwlock_unlock(iron_rwlock_t *rwlock)
{
    iron_mutex_lock(&rwlock->mutex);
    if (rwlock->writer_active && rwlock->writer_owner == iron_thread_current_id()) {
        rwlock->writer_active = IRON_FALSE;
        rwlock->writer_owner = 0;
    } else if (rwlock->active_readers != 0) {
        rwlock->active_readers--;
    }

    if (rwlock->waiting_writers != 0 && !rwlock->writer_active && rwlock->active_readers == 0) {
        iron_cond_signal(&rwlock->writers_changed);
    } else if (rwlock->waiting_writers == 0 && !rwlock->writer_active) {
        iron_cond_broadcast(&rwlock->readers_changed);
    }
    iron_mutex_unlock(&rwlock->mutex);
}

iron_result_t iron_semaphore_init(iron_semaphore_t *sem, iron_u32 initial)
{
    iron_result_t result;

    if (!sem) {
        return IRON_ERROR(IRON_ERR_NULL_POINTER, "semaphore is NULL");
    }

    memset(sem, 0, sizeof(*sem));
    result = iron_mutex_init(&sem->mutex);
    if (!IRON_RESULT_OK(result)) {
        return result;
    }

    result = iron_cond_init(&sem->cond);
    if (!IRON_RESULT_OK(result)) {
        iron_mutex_destroy(&sem->mutex);
        return result;
    }

    sem->count = initial;
    sem->initialized = IRON_TRUE;
    return IRON_SUCCESS;
}

void iron_semaphore_destroy(iron_semaphore_t *sem)
{
    if (!sem || !sem->initialized) {
        return;
    }

    iron_cond_destroy(&sem->cond);
    iron_mutex_destroy(&sem->mutex);
    sem->initialized = IRON_FALSE;
}

void iron_semaphore_wait(iron_semaphore_t *sem)
{
    iron_mutex_lock(&sem->mutex);
    while (sem->count == 0) {
        iron_cond_wait(&sem->cond, &sem->mutex);
    }
    sem->count--;
    iron_mutex_unlock(&sem->mutex);
}

iron_bool iron_semaphore_trywait(iron_semaphore_t *sem)
{
    iron_bool acquired;

    iron_mutex_lock(&sem->mutex);
    acquired = sem->count != 0;
    if (acquired) {
        sem->count--;
    }
    iron_mutex_unlock(&sem->mutex);
    return acquired;
}

iron_bool iron_semaphore_timedwait(iron_semaphore_t *sem, iron_u32 timeout_ms)
{
    iron_u64 deadline;
    iron_bool acquired;

    deadline = iron_platform_monotonic_milliseconds() + timeout_ms;
    iron_mutex_lock(&sem->mutex);
    while (sem->count == 0) {
        iron_u32 remaining;

        remaining = timeout_remaining(deadline);
        if (remaining == 0 || !iron_cond_timedwait(&sem->cond, &sem->mutex, remaining)) {
            break;
        }
    }

    acquired = sem->count != 0;
    if (acquired) {
        sem->count--;
    }
    iron_mutex_unlock(&sem->mutex);
    return acquired;
}

void iron_semaphore_post(iron_semaphore_t *sem)
{
    iron_mutex_lock(&sem->mutex);
    if (sem->count != UINT32_MAX) {
        sem->count++;
        iron_cond_signal(&sem->cond);
    }
    iron_mutex_unlock(&sem->mutex);
}

iron_result_t iron_event_init(iron_event_t *event, iron_bool manual_reset, iron_bool initial_state)
{
    iron_result_t result;

    if (!event) {
        return IRON_ERROR(IRON_ERR_NULL_POINTER, "event is NULL");
    }

    memset(event, 0, sizeof(*event));
    result = iron_mutex_init(&event->mutex);
    if (!IRON_RESULT_OK(result)) {
        return result;
    }

    result = iron_cond_init(&event->cond);
    if (!IRON_RESULT_OK(result)) {
        iron_mutex_destroy(&event->mutex);
        return result;
    }

    event->manual_reset = manual_reset;
    event->signaled = initial_state;
    event->initialized = IRON_TRUE;
    return IRON_SUCCESS;
}

void iron_event_destroy(iron_event_t *event)
{
    if (!event || !event->initialized) {
        return;
    }

    iron_cond_destroy(&event->cond);
    iron_mutex_destroy(&event->mutex);
    event->initialized = IRON_FALSE;
}

void iron_event_set(iron_event_t *event)
{
    iron_mutex_lock(&event->mutex);
    event->signaled = IRON_TRUE;
    if (event->manual_reset) {
        iron_cond_broadcast(&event->cond);
    } else {
        iron_cond_signal(&event->cond);
    }
    iron_mutex_unlock(&event->mutex);
}

void iron_event_reset(iron_event_t *event)
{
    iron_mutex_lock(&event->mutex);
    event->signaled = IRON_FALSE;
    iron_mutex_unlock(&event->mutex);
}

void iron_event_wait(iron_event_t *event)
{
    iron_mutex_lock(&event->mutex);
    while (!event->signaled) {
        iron_cond_wait(&event->cond, &event->mutex);
    }
    if (!event->manual_reset) {
        event->signaled = IRON_FALSE;
    }
    iron_mutex_unlock(&event->mutex);
}

iron_bool iron_event_timedwait(iron_event_t *event, iron_u32 timeout_ms)
{
    iron_u64 deadline;
    iron_bool signaled;

    deadline = iron_platform_monotonic_milliseconds() + timeout_ms;
    iron_mutex_lock(&event->mutex);
    while (!event->signaled) {
        iron_u32 remaining;

        remaining = timeout_remaining(deadline);
        if (remaining == 0 || !iron_cond_timedwait(&event->cond, &event->mutex, remaining)) {
            break;
        }
    }

    signaled = event->signaled;
    if (signaled && !event->manual_reset) {
        event->signaled = IRON_FALSE;
    }
    iron_mutex_unlock(&event->mutex);
    return signaled;
}

void iron_once(iron_once_t *once, void (*init_fn)(void))
{
    if (!once || !init_fn) {
        return;
    }

    if (iron_atomic_cas_i32(&once->state, 0, 1)) {
        init_fn();
        iron_atomic_store_i32(&once->state, 2);
        return;
    }

    while (iron_atomic_load_i32(&once->state) != 2) {
        iron_thread_yield();
    }
}

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
