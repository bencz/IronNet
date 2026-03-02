/*
 * IronNet CLR Interpreter
 * gc.h - Garbage Collector interface
 * 
 * Simple mark-and-sweep GC with:
 * - Object pinning
 * - Weak references
 * - Finalization support
 * 
 * Pure C89 compatible
 */

#ifndef IRON_GC_H
#define IRON_GC_H

#include "platform.h"
#include "types.h"
#include "forward.h"
#include "memory.h"
#include "vtable.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * GC Configuration
 * ============================================================================ */

typedef struct iron_gc_config {
    iron_size initial_heap_size;
    iron_size max_heap_size;
    iron_size threshold;           /* Bytes allocated before collection */
    iron_bool enable_finalization;
    iron_bool enable_compaction;   /* Not implemented yet */
} iron_gc_config_t;

#define IRON_GC_DEFAULT_CONFIG { \
    1024 * 1024,      /* 1MB initial */ \
    256 * 1024 * 1024, /* 256MB max */ \
    512 * 1024,       /* 512KB threshold */ \
    IRON_TRUE,        /* Enable finalization */ \
    IRON_FALSE        /* No compaction */ \
}

/* ============================================================================
 * GC Statistics
 * ============================================================================ */

typedef struct iron_gc_stats {
    iron_size total_allocated;
    iron_size total_freed;
    iron_size current_heap_size;
    iron_size peak_heap_size;
    iron_u32 collection_count;
    iron_u32 object_count;
    iron_u32 finalized_count;
} iron_gc_stats_t;

/* ============================================================================
 * GC Handle Types
 * ============================================================================ */

typedef enum iron_gc_handle_type {
    IRON_GC_HANDLE_NORMAL,     /* Normal strong reference */
    IRON_GC_HANDLE_PINNED,     /* Pinned (address won't change) */
    IRON_GC_HANDLE_WEAK,       /* Weak reference (doesn't prevent collection) */
    IRON_GC_HANDLE_WEAK_TRACK  /* Weak with resurrection tracking */
} iron_gc_handle_type_t;

/* GC handle (opaque) */
typedef struct iron_gc_handle {
    void *target;
    iron_gc_handle_type_t type;
    struct iron_gc_handle *next;
} iron_gc_handle_t;

/* ============================================================================
 * GC State
 * ============================================================================ */

typedef struct iron_gc {
    iron_allocator_t *allocator;
    iron_exec_context_t *exec_ctx;
    
    /* Configuration */
    iron_gc_config_t config;
    
    /* Object lists */
    iron_gc_header_t *all_objects;
    iron_gc_header_t *finalize_queue;
    iron_gc_header_t *weak_refs;
    
    /* Handles */
    iron_gc_handle_t *handles;
    
    /* Statistics */
    iron_gc_stats_t stats;
    
    /* State */
    iron_bool collection_in_progress;
    iron_u32 collection_generation;
    
    /* Root scanning callback */
    void (*scan_roots)(struct iron_gc *gc, void *user_data);
    void *scan_roots_data;
} iron_gc_t;

/* ============================================================================
 * GC API
 * ============================================================================ */

/* Initialize GC */
IRON_API iron_result_t iron_gc_init(iron_gc_t *gc, 
                                     iron_exec_context_t *ctx,
                                     const iron_gc_config_t *config);

/* Shutdown GC */
IRON_API void iron_gc_shutdown(iron_gc_t *gc);

/* Allocate object */
IRON_API void *iron_gc_alloc_object(iron_gc_t *gc, 
                                     iron_runtime_type_t *type,
                                     iron_size size);

/* Allocate array */
IRON_API void *iron_gc_alloc_array_raw(iron_gc_t *gc,
                                        iron_runtime_type_t *type,
                                        iron_size element_size,
                                        iron_u32 length);

/* Force collection */
IRON_API void iron_gc_collect(iron_gc_t *gc);

/* Collect if threshold reached */
IRON_API void iron_gc_collect_if_needed(iron_gc_t *gc);

/* Get statistics */
IRON_API void iron_gc_get_stats(iron_gc_t *gc, iron_gc_stats_t *stats);

/* ============================================================================
 * GC Handles
 * ============================================================================ */

/* Allocate handle */
IRON_API iron_gc_handle_t *iron_gc_handle_alloc(iron_gc_t *gc, 
                                                 void *target,
                                                 iron_gc_handle_type_t type);

/* Free handle */
IRON_API void iron_gc_handle_free(iron_gc_t *gc, iron_gc_handle_t *handle);

/* Get target from handle */
IRON_API void *iron_gc_handle_get_target(iron_gc_handle_t *handle);

/* Set target in handle */
IRON_API void iron_gc_handle_set_target(iron_gc_handle_t *handle, void *target);

/* ============================================================================
 * Object Operations
 * ============================================================================ */

/* Get object type */
IRON_API iron_runtime_type_t *iron_gc_get_type(void *obj);

/* Get object size */
IRON_API iron_size iron_gc_get_size(void *obj);

/* Pin object */
IRON_API void iron_gc_pin(void *obj);

/* Unpin object */
IRON_API void iron_gc_unpin(void *obj);

/* Check if object is pinned */
IRON_API iron_bool iron_gc_is_pinned(void *obj);

/* Suppress finalization */
IRON_API void iron_gc_suppress_finalize(void *obj);

/* Re-register for finalization */
IRON_API void iron_gc_reregister_finalize(void *obj);

/* ============================================================================
 * Write Barriers (for future generational GC)
 * ============================================================================ */

/* Write barrier for reference field */
IRON_API void iron_gc_write_barrier(void *obj, void **field, void *value);

/* Write barrier for value type containing references */
IRON_API void iron_gc_write_barrier_value(void *obj, void *field_addr, 
                                           iron_runtime_type_t *field_type,
                                           const void *value);

/* ============================================================================
 * Root Scanning
 * ============================================================================ */

/* Mark object as reachable (called during collection) */
IRON_API void iron_gc_mark(iron_gc_t *gc, void *obj);

/* Mark range of potential references */
IRON_API void iron_gc_mark_range(iron_gc_t *gc, void **start, void **end);

/* Set root scanner callback */
IRON_API void iron_gc_set_root_scanner(iron_gc_t *gc,
                                        void (*scanner)(iron_gc_t *gc, void *data),
                                        void *user_data);

#ifdef __cplusplus
}
#endif

#endif /* IRON_GC_H */
