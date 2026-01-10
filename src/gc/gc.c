/*
 * IronNet CLR Interpreter
 * gc.c - Simple mark-and-sweep garbage collector
 */

#include "iron/gc.h"
#include "iron/exec.h"
#include "iron/runtime.h"
#include "iron/types.h"
#include "iron/debug.h"
#include <string.h>

/* ============================================================================
 * GC Initialization
 * ============================================================================ */

iron_result_t iron_gc_init(iron_gc_t *gc, iron_exec_context_t *ctx,
                           const iron_gc_config_t *config)
{
    if (!gc || !ctx) {
        return IRON_ERROR(IRON_ERR_NULL_POINTER, "Invalid parameters");
    }
    
    memset(gc, 0, sizeof(iron_gc_t));
    gc->allocator = ctx->allocator;
    gc->exec_ctx = ctx;
    
    if (config) {
        gc->config = *config;
    } else {
        iron_gc_config_t default_config = IRON_GC_DEFAULT_CONFIG;
        gc->config = default_config;
        /* Lower threshold for testing - trigger GC more frequently */
        gc->config.threshold = 8 * 1024; /* 8KB threshold for testing */
    }
    
    IRON_DEBUG_GC("GC initialized: threshold=%lu initial=%lu max=%lu",
                 (unsigned long)gc->config.threshold,
                 (unsigned long)gc->config.initial_heap_size,
                 (unsigned long)gc->config.max_heap_size);
    
    gc->all_objects = NULL;
    gc->finalize_queue = NULL;
    gc->weak_refs = NULL;
    gc->handles = NULL;
    gc->collection_in_progress = IRON_FALSE;
    gc->collection_generation = 0;
    
    return (iron_result_t)IRON_SUCCESS;
}

void iron_gc_shutdown(iron_gc_t *gc)
{
    iron_gc_header_t *obj, *next;
    iron_gc_handle_t *handle, *next_handle;
    
    if (!gc) return;
    
    /* Free all objects */
    for (obj = gc->all_objects; obj; obj = next) {
        next = obj->next;
        iron_free(gc->allocator, obj, obj->size + sizeof(iron_gc_header_t));
    }
    
    /* Free all handles */
    for (handle = gc->handles; handle; handle = next_handle) {
        next_handle = handle->next;
        iron_free(gc->allocator, handle, sizeof(iron_gc_handle_t));
    }
    
    memset(gc, 0, sizeof(iron_gc_t));
}

/* ============================================================================
 * Object Allocation
 * ============================================================================ */

void *iron_gc_alloc_object(iron_gc_t *gc, iron_runtime_type_t *type, iron_size size)
{
    iron_gc_header_t *header;
    iron_size total_size;
    void *obj;
    
    if (!gc || !gc->allocator) return NULL;
    
    /* Check if we need to collect */
    iron_gc_collect_if_needed(gc);
    
    total_size = sizeof(iron_gc_header_t) + size;
    header = (iron_gc_header_t *)iron_alloc(gc->allocator, total_size);
    if (!header) {
        IRON_ERROR_GC("Failed to allocate %lu bytes", (unsigned long)total_size);
        return NULL;
    }
    
    memset(header, 0, total_size);
    header->type = type;
    header->size = size;
    header->flags = 0;
    header->mark = 0;
    
    /* Add to object list */
    header->next = gc->all_objects;
    gc->all_objects = header;
    
    /* Update stats */
    gc->stats.total_allocated += total_size;
    gc->stats.current_heap_size += total_size;
    gc->stats.object_count++;
    
    if (gc->stats.current_heap_size > gc->stats.peak_heap_size) {
        gc->stats.peak_heap_size = gc->stats.current_heap_size;
    }
    
    obj = (void *)(header + 1);
    
    IRON_TRACE_GC("ALLOC %s size=%lu ptr=%p heap=%lu", 
                  type ? type->name : "<raw>",
                  (unsigned long)size, obj,
                  (unsigned long)gc->stats.current_heap_size);
    
    return obj;
}

void *iron_gc_alloc_array_raw(iron_gc_t *gc, iron_runtime_type_t *type,
                              iron_size element_size, iron_u32 length)
{
    iron_size array_size;
    void *obj;
    iron_u32 *length_ptr;
    
    /* Array layout: [length (4 bytes)] [elements...] */
    array_size = sizeof(iron_u32) + element_size * length;
    
    IRON_TRACE_GC("ALLOC_ARRAY type=%s elem_size=%lu length=%u total=%lu",
                  type ? type->name : "<raw>",
                  (unsigned long)element_size, length,
                  (unsigned long)array_size);
    
    obj = iron_gc_alloc_object(gc, type, array_size);
    if (!obj) return NULL;
    
    /* Store length at start of array */
    length_ptr = (iron_u32 *)obj;
    *length_ptr = length;
    
    return obj;
}

/* ============================================================================
 * Mark Phase
 * ============================================================================ */

void iron_gc_mark(iron_gc_t *gc, void *obj)
{
    iron_gc_header_t *header;
    iron_runtime_type_t *type;
    iron_u32 i;
    
    if (!obj) return;
    
    header = ((iron_gc_header_t *)obj) - 1;
    
    /* Already marked? */
    if (header->mark == gc->collection_generation) return;
    
    /* Mark this object */
    header->mark = gc->collection_generation;
    IRON_TRACE_GC("MARK obj=%p type=%s gen=%u", obj, 
                  header->type ? header->type->name : "<raw>",
                  gc->collection_generation);
    
    /* Scan reference fields */
    type = header->type;
    if (!type) return;
    
    /* Scan instance fields for references (only if fields array is loaded) */
    if (!type->fields || type->field_count == 0) return;
    
    for (i = 0; i < type->field_count; i++) {
        iron_runtime_field_t *field = type->fields[i];
        if (!field) continue;
        
        /* Skip static fields */
        if (field->attrs & 0x0010) continue; /* FieldAttributes.Static */
        
        /* Check if field is a reference type */
        if (field->field_type && 
            iron_element_type_is_reference(field->field_type->element_type)) {
            void **field_ptr = (void **)((char *)obj + field->offset);
            iron_gc_mark(gc, *field_ptr);
        }
    }
    
    /* If this is an array of references, scan elements */
    if (type->kind == IRON_KIND_ARRAY && type->element &&
        iron_element_type_is_reference(type->element->element_type)) {
        iron_u32 *length_ptr = (iron_u32 *)obj;
        iron_u32 length = *length_ptr;
        void **elements = (void **)(length_ptr + 1);
        
        for (i = 0; i < length; i++) {
            iron_gc_mark(gc, elements[i]);
        }
    }
}

void iron_gc_mark_range(iron_gc_t *gc, void **start, void **end)
{
    void **ptr;
    
    for (ptr = start; ptr < end; ptr++) {
        /* Conservative marking - check if this looks like a valid object pointer */
        void *potential_obj = *ptr;
        iron_gc_header_t *obj;
        
        if (!potential_obj) continue;
        
        /* Walk object list to verify this is a valid object */
        for (obj = gc->all_objects; obj; obj = obj->next) {
            if ((void *)(obj + 1) == potential_obj) {
                iron_gc_mark(gc, potential_obj);
                break;
            }
        }
    }
}

/* ============================================================================
 * Sweep Phase
 * ============================================================================ */

static void gc_sweep(iron_gc_t *gc)
{
    iron_gc_header_t **prev = &gc->all_objects;
    iron_gc_header_t *obj = gc->all_objects;
    iron_size freed_size;
    
    while (obj) {
        if (obj->mark != gc->collection_generation) {
            /* Object is not marked - free it */
            iron_gc_header_t *dead = obj;
            void *dead_obj = (void *)(dead + 1);
            *prev = obj->next;
            obj = obj->next;
            
            freed_size = dead->size + sizeof(iron_gc_header_t);
            IRON_TRACE_GC("SWEEP FREE obj=%p type=%s size=%lu", dead_obj,
                         dead->type ? dead->type->name : "<raw>",
                         (unsigned long)freed_size);
            gc->stats.total_freed += freed_size;
            gc->stats.current_heap_size -= freed_size;
            gc->stats.object_count--;
            
            iron_free(gc->allocator, dead, freed_size);
        } else {
            /* Object is alive - keep it */
            prev = &obj->next;
            obj = obj->next;
        }
    }
}

/* ============================================================================
 * Collection
 * ============================================================================ */

static void gc_mark_roots(iron_gc_t *gc)
{
    iron_exec_context_t *ctx = gc->exec_ctx;
    iron_u32 i;
    
    IRON_TRACE_GC("MARK_ROOTS: thread_count=%u main_thread=%p", 
                 ctx->thread_count, (void*)ctx->main_thread);
    
    /* Mark from main thread first (may not be in threads array) */
    if (ctx->main_thread) {
        iron_thread_context_t *thread = ctx->main_thread;
        iron_stack_frame_t *frame;
        iron_u32 j;
        
        IRON_TRACE_GC("MARK_ROOTS: main_thread eval_stack.size=%u current_frame=%p",
                     thread->eval_stack.size, (void*)thread->current_frame);
        
        /* Mark evaluation stack */
        for (j = 0; j < thread->eval_stack.size; j++) {
            iron_stack_value_t *val = &thread->eval_stack.data[j];
            if (val->type == IRON_VAL_OBJ && val->value.obj) {
                iron_gc_mark(gc, val->value.obj);
            }
        }
        
        /* Mark local variables and arguments in each frame */
        for (frame = thread->current_frame; frame; frame = frame->prev) {
            IRON_TRACE_GC("MARK_ROOTS: frame method=%s args=%u locals=%u",
                         frame->method ? frame->method->name : "?",
                         frame->arg_count, frame->local_count);
            for (j = 0; j < frame->arg_count; j++) {
                if (frame->args && frame->args[j].type == IRON_VAL_OBJ && frame->args[j].value.obj) {
                    iron_gc_mark(gc, frame->args[j].value.obj);
                }
            }
            for (j = 0; j < frame->local_count; j++) {
                if (frame->locals && frame->locals[j].type == IRON_VAL_OBJ && frame->locals[j].value.obj) {
                    iron_gc_mark(gc, frame->locals[j].value.obj);
                }
            }
        }
    }
    
    /* Mark from other threads */
    for (i = 0; i < ctx->thread_count; i++) {
        iron_thread_context_t *thread = ctx->threads[i];
        iron_stack_frame_t *frame;
        iron_u32 j;
        
        if (!thread || thread == ctx->main_thread) continue;
        
        /* Mark evaluation stack */
        for (j = 0; j < thread->eval_stack.size; j++) {
            iron_stack_value_t *val = &thread->eval_stack.data[j];
            if (val->type == IRON_VAL_OBJ && val->value.obj) {
                iron_gc_mark(gc, val->value.obj);
            }
        }
        
        /* Mark local variables and arguments in each frame */
        for (frame = thread->current_frame; frame; frame = frame->prev) {
            for (j = 0; j < frame->arg_count; j++) {
                if (frame->args && frame->args[j].type == IRON_VAL_OBJ && frame->args[j].value.obj) {
                    iron_gc_mark(gc, frame->args[j].value.obj);
                }
            }
            for (j = 0; j < frame->local_count; j++) {
                if (frame->locals && frame->locals[j].type == IRON_VAL_OBJ && frame->locals[j].value.obj) {
                    iron_gc_mark(gc, frame->locals[j].value.obj);
                }
            }
        }
    }
    
    /* Mark from GC handles */
    {
        iron_gc_handle_t *handle;
        for (handle = gc->handles; handle; handle = handle->next) {
            if (handle->type != IRON_GC_HANDLE_WEAK &&
                handle->type != IRON_GC_HANDLE_WEAK_TRACK) {
                iron_gc_mark(gc, handle->target);
            }
        }
    }
    
    /* Mark static fields */
    if (ctx->domain) {
        for (i = 0; i < ctx->domain->assembly_count; i++) {
            iron_assembly_t *assembly = ctx->domain->assemblies[i];
            iron_u32 t;
            
            if (!assembly || !assembly->module) continue;
            
            for (t = 0; t < assembly->module->type_count; t++) {
                iron_runtime_type_t *type = assembly->module->types[t];
                iron_u32 f;
                
                if (!type || !type->static_data) continue;
                
                for (f = 0; f < type->field_count; f++) {
                    iron_runtime_field_t *field = type->fields[f];
                    
                    if (!field) continue;
                    if (!(field->attrs & 0x0010)) continue; /* Not static */
                    
                    if (field->field_type &&
                        iron_element_type_is_reference(field->field_type->element_type)) {
                        void **field_ptr = (void **)((char *)type->static_data + field->offset);
                        iron_gc_mark(gc, *field_ptr);
                    }
                }
            }
        }
    }
    
    /* Call custom root scanner if set */
    if (gc->scan_roots) {
        gc->scan_roots(gc, gc->scan_roots_data);
    }
}

static void gc_update_weak_refs(iron_gc_t *gc)
{
    iron_gc_handle_t *handle;
    
    for (handle = gc->handles; handle; handle = handle->next) {
        if (handle->type == IRON_GC_HANDLE_WEAK ||
            handle->type == IRON_GC_HANDLE_WEAK_TRACK) {
            if (handle->target) {
                iron_gc_header_t *header = ((iron_gc_header_t *)handle->target) - 1;
                if (header->mark != gc->collection_generation) {
                    handle->target = NULL;
                }
            }
        }
    }
}

void iron_gc_collect(iron_gc_t *gc)
{
    iron_size heap_before;
    iron_u32 objects_before;
    
    if (!gc || gc->collection_in_progress) return;
    
    heap_before = gc->stats.current_heap_size;
    objects_before = gc->stats.object_count;
    
    IRON_DEBUG_GC("=== GC COLLECTION #%u START ===", gc->stats.collection_count + 1);
    IRON_DEBUG_GC("Heap: %lu bytes, Objects: %u", 
                  (unsigned long)heap_before, objects_before);
    
    gc->collection_in_progress = IRON_TRUE;
    gc->collection_generation++;
    
    /* Mark phase */
    IRON_TRACE_GC("Mark phase...");
    gc_mark_roots(gc);
    
    /* Update weak references */
    IRON_TRACE_GC("Updating weak refs...");
    gc_update_weak_refs(gc);
    
    /* Sweep phase */
    IRON_TRACE_GC("Sweep phase...");
    gc_sweep(gc);
    
    gc->stats.collection_count++;
    gc->collection_in_progress = IRON_FALSE;
    
    IRON_DEBUG_GC("=== GC COLLECTION #%u END ===", gc->stats.collection_count);
    IRON_DEBUG_GC("Freed: %lu bytes, %u objects", 
                  (unsigned long)(heap_before - gc->stats.current_heap_size),
                  objects_before - gc->stats.object_count);
    IRON_DEBUG_GC("Heap now: %lu bytes, Objects: %u",
                  (unsigned long)gc->stats.current_heap_size, 
                  gc->stats.object_count);
}

void iron_gc_collect_if_needed(iron_gc_t *gc)
{
    if (!gc) return;
    
    if (gc->stats.current_heap_size >= gc->config.threshold) {
        IRON_TRACE_GC("COLLECT_NEEDED heap=%lu threshold=%lu",
                     (unsigned long)gc->stats.current_heap_size,
                     (unsigned long)gc->config.threshold);
        iron_gc_collect(gc);
    }
}

void iron_gc_get_stats(iron_gc_t *gc, iron_gc_stats_t *stats)
{
    if (!gc || !stats) return;
    *stats = gc->stats;
}

/* ============================================================================
 * GC Handles
 * ============================================================================ */

iron_gc_handle_t *iron_gc_handle_alloc(iron_gc_t *gc, void *target,
                                       iron_gc_handle_type_t type)
{
    iron_gc_handle_t *handle;
    
    if (!gc) return NULL;
    
    handle = (iron_gc_handle_t *)iron_alloc(gc->allocator, sizeof(iron_gc_handle_t));
    if (!handle) return NULL;
    
    handle->target = target;
    handle->type = type;
    handle->next = gc->handles;
    gc->handles = handle;
    
    return handle;
}

void iron_gc_handle_free(iron_gc_t *gc, iron_gc_handle_t *handle)
{
    iron_gc_handle_t **prev;
    
    if (!gc || !handle) return;
    
    prev = &gc->handles;
    while (*prev) {
        if (*prev == handle) {
            *prev = handle->next;
            iron_free(gc->allocator, handle, sizeof(iron_gc_handle_t));
            return;
        }
        prev = &(*prev)->next;
    }
}

void *iron_gc_handle_get_target(iron_gc_handle_t *handle)
{
    return handle ? handle->target : NULL;
}

void iron_gc_handle_set_target(iron_gc_handle_t *handle, void *target)
{
    if (handle) handle->target = target;
}

/* ============================================================================
 * Object Operations
 * ============================================================================ */

iron_runtime_type_t *iron_gc_get_type(void *obj)
{
    iron_gc_header_t *header;
    if (!obj) return NULL;
    header = ((iron_gc_header_t *)obj) - 1;
    return header->type;
}

iron_size iron_gc_get_size(void *obj)
{
    iron_gc_header_t *header;
    if (!obj) return 0;
    header = ((iron_gc_header_t *)obj) - 1;
    return header->size;
}

void iron_gc_pin(void *obj)
{
    iron_gc_header_t *header;
    if (!obj) return;
    header = ((iron_gc_header_t *)obj) - 1;
    header->flags |= IRON_GC_PINNED;
}

void iron_gc_unpin(void *obj)
{
    iron_gc_header_t *header;
    if (!obj) return;
    header = ((iron_gc_header_t *)obj) - 1;
    header->flags &= ~IRON_GC_PINNED;
}

iron_bool iron_gc_is_pinned(void *obj)
{
    iron_gc_header_t *header;
    if (!obj) return IRON_FALSE;
    header = ((iron_gc_header_t *)obj) - 1;
    return (header->flags & IRON_GC_PINNED) != 0;
}

void iron_gc_suppress_finalize(void *obj)
{
    iron_gc_header_t *header;
    if (!obj) return;
    header = ((iron_gc_header_t *)obj) - 1;
    header->flags |= IRON_GC_NO_FINALIZE;
}

void iron_gc_reregister_finalize(void *obj)
{
    iron_gc_header_t *header;
    if (!obj) return;
    header = ((iron_gc_header_t *)obj) - 1;
    header->flags &= ~IRON_GC_NO_FINALIZE;
}

/* ============================================================================
 * Write Barriers
 * ============================================================================ */

void iron_gc_write_barrier(void *obj, void **field, void *value)
{
    /* Simple write barrier - just write the value */
    /* In a generational GC, this would record the write */
    (void)obj;
    *field = value;
}

void iron_gc_write_barrier_value(void *obj, void *field_addr,
                                 iron_runtime_type_t *field_type,
                                 const void *value)
{
    (void)obj;
    if (field_type) {
        memcpy(field_addr, value, field_type->instance_size);
    }
}

void iron_gc_set_root_scanner(iron_gc_t *gc,
                              void (*scanner)(iron_gc_t *gc, void *data),
                              void *user_data)
{
    if (!gc) return;
    gc->scan_roots = scanner;
    gc->scan_roots_data = user_data;
}
