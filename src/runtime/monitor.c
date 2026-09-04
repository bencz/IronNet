/*
 * IronNet CLR Interpreter
 * monitor.c - Reentrant managed object monitors
 */

#include "iron/exec.h"
#include "iron/thread.h"
#include <string.h>

typedef struct iron_object_monitor {
    iron_mutex_t mutex;
    iron_cond_t ownership_changed;
    iron_cond_t pulse_received;
    iron_thread_context_t *owner;
    iron_u32 recursion;
    iron_u64 pulse_generation;
    iron_u32 waiters;
} iron_object_monitor_t;

static iron_u32 remaining_timeout(iron_u64 deadline)
{
    iron_u64 now;
    iron_u64 remaining;

    now = iron_platform_monotonic_milliseconds();
    if (now >= deadline) {
        return 0;
    }

    remaining = deadline - now;
    return remaining > UINT32_MAX ? UINT32_MAX : (iron_u32)remaining;
}

static iron_object_monitor_t *get_object_monitor(iron_exec_context_t *ctx, void *obj, iron_bool create)
{
    iron_gc_header_t *header;
    iron_object_monitor_t *monitor;
    iron_result_t result;

    if (!ctx || !obj || !ctx->thread_lock) {
        return NULL;
    }

    header = IRON_GC_HEADER(obj);
    iron_mutex_lock((iron_mutex_t *)ctx->thread_lock);
    monitor = (iron_object_monitor_t *)header->sync_block;
    if (!monitor && create) {
        monitor = (iron_object_monitor_t *)iron_alloc(ctx->allocator, sizeof(*monitor));
        if (monitor) {
            memset(monitor, 0, sizeof(*monitor));
            result = iron_mutex_init(&monitor->mutex);
            if (IRON_RESULT_OK(result)) {
                result = iron_cond_init(&monitor->ownership_changed);
                if (!IRON_RESULT_OK(result)) {
                    iron_mutex_destroy(&monitor->mutex);
                }
            }
            if (IRON_RESULT_OK(result)) {
                result = iron_cond_init(&monitor->pulse_received);
                if (!IRON_RESULT_OK(result)) {
                    iron_cond_destroy(&monitor->ownership_changed);
                    iron_mutex_destroy(&monitor->mutex);
                }
            }

            if (IRON_RESULT_OK(result)) {
                header->sync_block = monitor;
            } else {
                iron_free(ctx->allocator, monitor, sizeof(*monitor));
                monitor = NULL;
            }
        }
    }
    iron_mutex_unlock((iron_mutex_t *)ctx->thread_lock);
    return monitor;
}

static iron_gc_handle_t *protect_monitor_object(iron_exec_context_t *ctx, void *obj)
{
    return iron_gc_handle_alloc(&ctx->gc, obj, IRON_GC_HANDLE_NORMAL);
}

iron_result_t iron_monitor_enter(iron_exec_context_t *ctx, void *obj)
{
    iron_object_monitor_t *monitor;
    iron_thread_context_t *thread;
    iron_gc_handle_t *root;
    iron_bool execution_suspended;

    if (!ctx || !obj) {
        return IRON_ERROR(IRON_ERR_NULL_REFERENCE, "Monitor.Enter received a null object");
    }

    thread = iron_exec_get_current_thread(ctx);
    monitor = get_object_monitor(ctx, obj, IRON_TRUE);
    root = protect_monitor_object(ctx, obj);
    if (!thread || !monitor || !root) {
        if (root) {
            iron_gc_handle_free(&ctx->gc, root);
        }
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to initialize an object monitor");
    }

    execution_suspended = iron_exec_suspend_execution(ctx);
    iron_mutex_lock(&monitor->mutex);
    while (monitor->owner && monitor->owner != thread) {
        iron_cond_wait(&monitor->ownership_changed, &monitor->mutex);
    }
    monitor->owner = thread;
    monitor->recursion++;
    iron_mutex_unlock(&monitor->mutex);
    iron_exec_resume_execution(ctx, execution_suspended);
    iron_gc_handle_free(&ctx->gc, root);
    return IRON_SUCCESS;
}

iron_result_t iron_monitor_exit(iron_exec_context_t *ctx, void *obj)
{
    iron_object_monitor_t *monitor;
    iron_thread_context_t *thread;

    if (!ctx || !obj) {
        return IRON_ERROR(IRON_ERR_NULL_REFERENCE, "Monitor.Exit received a null object");
    }

    thread = iron_exec_get_current_thread(ctx);
    monitor = get_object_monitor(ctx, obj, IRON_FALSE);
    if (!thread || !monitor) {
        return IRON_ERROR(IRON_ERR_MUTEX_ERROR, "The current thread does not own the object monitor");
    }

    iron_mutex_lock(&monitor->mutex);
    if (monitor->owner != thread || monitor->recursion == 0) {
        iron_mutex_unlock(&monitor->mutex);
        return IRON_ERROR(IRON_ERR_MUTEX_ERROR, "The current thread does not own the object monitor");
    }

    monitor->recursion--;
    if (monitor->recursion == 0) {
        monitor->owner = NULL;
        iron_cond_signal(&monitor->ownership_changed);
    }
    iron_mutex_unlock(&monitor->mutex);
    return IRON_SUCCESS;
}

iron_result_t iron_monitor_try_enter(iron_exec_context_t *ctx, void *obj, iron_u32 timeout_ms, iron_bool *acquired)
{
    iron_object_monitor_t *monitor;
    iron_thread_context_t *thread;
    iron_gc_handle_t *root;
    iron_bool execution_suspended;
    iron_u64 deadline;

    if (!ctx || !obj || !acquired) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Monitor.TryEnter received invalid arguments");
    }

    *acquired = IRON_FALSE;
    thread = iron_exec_get_current_thread(ctx);
    monitor = get_object_monitor(ctx, obj, IRON_TRUE);
    root = protect_monitor_object(ctx, obj);
    if (!thread || !monitor || !root) {
        if (root) {
            iron_gc_handle_free(&ctx->gc, root);
        }
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to initialize an object monitor");
    }

    deadline = timeout_ms == UINT32_MAX ? UINT64_MAX : iron_platform_monotonic_milliseconds() + timeout_ms;
    execution_suspended = iron_exec_suspend_execution(ctx);
    iron_mutex_lock(&monitor->mutex);
    while (monitor->owner && monitor->owner != thread) {
        iron_u32 remaining;

        if (deadline == UINT64_MAX) {
            iron_cond_wait(&monitor->ownership_changed, &monitor->mutex);
            continue;
        }

        remaining = remaining_timeout(deadline);
        if (remaining == 0 || !iron_cond_timedwait(&monitor->ownership_changed, &monitor->mutex, remaining)) {
            break;
        }
    }

    if (!monitor->owner || monitor->owner == thread) {
        monitor->owner = thread;
        monitor->recursion++;
        *acquired = IRON_TRUE;
    }
    iron_mutex_unlock(&monitor->mutex);
    iron_exec_resume_execution(ctx, execution_suspended);
    iron_gc_handle_free(&ctx->gc, root);
    return IRON_SUCCESS;
}

iron_result_t iron_monitor_wait(iron_exec_context_t *ctx, void *obj, iron_u32 timeout_ms, iron_bool *signaled)
{
    iron_object_monitor_t *monitor;
    iron_thread_context_t *thread;
    iron_gc_handle_t *root;
    iron_bool execution_suspended;
    iron_u32 recursion;
    iron_u64 generation;
    iron_u64 deadline;

    if (!ctx || !obj || !signaled) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Monitor.Wait received invalid arguments");
    }

    thread = iron_exec_get_current_thread(ctx);
    monitor = get_object_monitor(ctx, obj, IRON_FALSE);
    root = protect_monitor_object(ctx, obj);
    if (!thread || !monitor || !root) {
        if (root) {
            iron_gc_handle_free(&ctx->gc, root);
        }
        return IRON_ERROR(IRON_ERR_MUTEX_ERROR, "The current thread does not own the object monitor");
    }

    execution_suspended = iron_exec_suspend_execution(ctx);
    iron_mutex_lock(&monitor->mutex);
    if (monitor->owner != thread || monitor->recursion == 0) {
        iron_mutex_unlock(&monitor->mutex);
        iron_exec_resume_execution(ctx, execution_suspended);
        iron_gc_handle_free(&ctx->gc, root);
        return IRON_ERROR(IRON_ERR_MUTEX_ERROR, "The current thread does not own the object monitor");
    }

    recursion = monitor->recursion;
    generation = monitor->pulse_generation;
    deadline = timeout_ms == UINT32_MAX ? UINT64_MAX : iron_platform_monotonic_milliseconds() + timeout_ms;
    monitor->owner = NULL;
    monitor->recursion = 0;
    monitor->waiters++;
    iron_cond_signal(&monitor->ownership_changed);

    while (monitor->pulse_generation == generation) {
        iron_u32 remaining;

        if (deadline == UINT64_MAX) {
            iron_cond_wait(&monitor->pulse_received, &monitor->mutex);
            continue;
        }

        remaining = remaining_timeout(deadline);
        if (remaining == 0 || !iron_cond_timedwait(&monitor->pulse_received, &monitor->mutex, remaining)) {
            break;
        }
    }

    *signaled = monitor->pulse_generation != generation;
    monitor->waiters--;
    while (monitor->owner && monitor->owner != thread) {
        iron_cond_wait(&monitor->ownership_changed, &monitor->mutex);
    }
    monitor->owner = thread;
    monitor->recursion = recursion;
    iron_mutex_unlock(&monitor->mutex);
    iron_exec_resume_execution(ctx, execution_suspended);
    iron_gc_handle_free(&ctx->gc, root);
    return IRON_SUCCESS;
}

iron_result_t iron_monitor_pulse(iron_exec_context_t *ctx, void *obj)
{
    iron_object_monitor_t *monitor;
    iron_thread_context_t *thread;

    if (!ctx || !obj) {
        return IRON_ERROR(IRON_ERR_NULL_REFERENCE, "Monitor.Pulse received a null object");
    }

    thread = iron_exec_get_current_thread(ctx);
    monitor = get_object_monitor(ctx, obj, IRON_FALSE);
    if (!thread || !monitor) {
        return IRON_ERROR(IRON_ERR_MUTEX_ERROR, "The current thread does not own the object monitor");
    }

    iron_mutex_lock(&monitor->mutex);
    if (monitor->owner != thread || monitor->recursion == 0) {
        iron_mutex_unlock(&monitor->mutex);
        return IRON_ERROR(IRON_ERR_MUTEX_ERROR, "The current thread does not own the object monitor");
    }

    if (monitor->waiters != 0) {
        monitor->pulse_generation++;
        iron_cond_signal(&monitor->pulse_received);
    }
    iron_mutex_unlock(&monitor->mutex);
    return IRON_SUCCESS;
}

iron_result_t iron_monitor_pulse_all(iron_exec_context_t *ctx, void *obj)
{
    iron_object_monitor_t *monitor;
    iron_thread_context_t *thread;

    if (!ctx || !obj) {
        return IRON_ERROR(IRON_ERR_NULL_REFERENCE, "Monitor.PulseAll received a null object");
    }

    thread = iron_exec_get_current_thread(ctx);
    monitor = get_object_monitor(ctx, obj, IRON_FALSE);
    if (!thread || !monitor) {
        return IRON_ERROR(IRON_ERR_MUTEX_ERROR, "The current thread does not own the object monitor");
    }

    iron_mutex_lock(&monitor->mutex);
    if (monitor->owner != thread || monitor->recursion == 0) {
        iron_mutex_unlock(&monitor->mutex);
        return IRON_ERROR(IRON_ERR_MUTEX_ERROR, "The current thread does not own the object monitor");
    }

    if (monitor->waiters != 0) {
        monitor->pulse_generation++;
        iron_cond_broadcast(&monitor->pulse_received);
    }
    iron_mutex_unlock(&monitor->mutex);
    return IRON_SUCCESS;
}

void iron_monitor_destroy_object(iron_exec_context_t *ctx, void *obj)
{
    iron_gc_header_t *header;
    iron_object_monitor_t *monitor;

    if (!ctx || !obj || !ctx->thread_lock) {
        return;
    }

    header = IRON_GC_HEADER(obj);
    iron_mutex_lock((iron_mutex_t *)ctx->thread_lock);
    monitor = (iron_object_monitor_t *)header->sync_block;
    header->sync_block = NULL;
    iron_mutex_unlock((iron_mutex_t *)ctx->thread_lock);
    if (!monitor) {
        return;
    }

    iron_cond_destroy(&monitor->pulse_received);
    iron_cond_destroy(&monitor->ownership_changed);
    iron_mutex_destroy(&monitor->mutex);
    iron_free(ctx->allocator, monitor, sizeof(*monitor));
}
