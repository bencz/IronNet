/*
 * IronNet CLR Interpreter
 * gc.c - Simple mark-and-sweep garbage collector
 */

#include "iron/gc.h"
#include "iron/exec.h"
#include "iron/runtime.h"
#include "iron/types.h"
#include "iron/debug.h"
#include <limits.h>
#include <string.h>

typedef struct iron_array_payload {
    iron_u32 length;
    iron_u32 rank;
    iron_size element_size;
    iron_runtime_type_t *element_type;
    iron_size data_offset;
    iron_size alignment_padding;
} iron_array_payload_t;

static iron_u32 *array_lengths(iron_array_payload_t *array)
{
    return (iron_u32 *)(array + 1);
}

static const iron_u32 *array_const_lengths(const iron_array_payload_t *array)
{
    return (const iron_u32 *)(array + 1);
}

static iron_i32 *array_lower_bounds(iron_array_payload_t *array)
{
    return (iron_i32 *)(array_lengths(array) + array->rank);
}

static const iron_i32 *array_const_lower_bounds(const iron_array_payload_t *array)
{
    return (const iron_i32 *)(array_const_lengths(array) + array->rank);
}

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
    }
    
    IRON_DEBUG_GC("GC initialized: threshold=%lu initial=%lu max=%lu",
                 (unsigned long)gc->config.threshold,
                 (unsigned long)gc->config.initial_heap_size,
                 (unsigned long)gc->config.max_heap_size);
    
    gc->all_objects = NULL;
    gc->handles = NULL;
    gc->collection_in_progress = IRON_FALSE;
    gc->collection_generation = 0;
    
    return IRON_SUCCESS;
}

void iron_gc_shutdown(iron_gc_t *gc)
{
    iron_gc_header_t *obj, *next;
    iron_gc_handle_t *handle, *next_handle;
    iron_u32 freed_objects;
    
    if (!gc) return;
    
    freed_objects = 0;
    IRON_DEBUG_GC("GC shutdown started with %u tracked objects", gc->stats.object_count);

    /* Free all objects */
    for (obj = gc->all_objects; obj && freed_objects < gc->stats.object_count; obj = next) {
        next = obj->next;
        IRON_TRACE_GC("GC shutdown object=%p next=%p size=%lu type=%s",
                      (void *)obj,
                      (void *)next,
                      (unsigned long)obj->size,
                      obj->type && obj->type->full_name ? obj->type->full_name : "<raw>");
        iron_monitor_destroy_object(gc->exec_ctx, IRON_GC_OBJECT(obj));
        iron_free(gc->allocator, obj, obj->size + sizeof(iron_gc_header_t));
        freed_objects++;
    }
    if (obj) {
        IRON_ERROR_GC("GC allocation list is cyclic or corrupted after %u objects", freed_objects);
    }
    
    /* Free all handles */
    for (handle = gc->handles; handle; handle = next_handle) {
        next_handle = handle->next;
        iron_free(gc->allocator, handle, sizeof(iron_gc_handle_t));
    }
    
    IRON_DEBUG_GC("GC shutdown released %u objects", freed_objects);
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

    if (type && type->kind != IRON_KIND_ARRAY && type->kind != IRON_KIND_POINTER && type->kind != IRON_KIND_BYREF && type->kind != IRON_KIND_FNPTR) {
        iron_result_t layout_result;

        layout_result = iron_type_compute_layout(type);
        if (!IRON_RESULT_OK(layout_result)) {
            return NULL;
        }
        if (size < type->instance_size) {
            size = type->instance_size;
        }
    }

    if (size > (iron_size)-1 - sizeof(iron_gc_header_t)) {
        return NULL;
    }

    /* Check if we need to collect */
    iron_gc_collect_if_needed(gc);

    total_size = sizeof(iron_gc_header_t) + size;
    if (gc->config.max_heap_size != 0 &&
        (total_size > gc->config.max_heap_size || gc->stats.current_heap_size > gc->config.max_heap_size - total_size)) {
        iron_gc_collect(gc);
        if (total_size > gc->config.max_heap_size || gc->stats.current_heap_size > gc->config.max_heap_size - total_size) {
            IRON_ERROR_GC("Managed heap limit reached while allocating %lu bytes", (unsigned long)total_size);
            return NULL;
        }
    }

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

void *iron_gc_alloc_array_raw(iron_gc_t *gc, iron_runtime_type_t *element_type,
                              iron_size element_size, iron_u32 length)
{
    iron_runtime_type_t *array_type;
    iron_u32 lengths[1];

    if (!gc || !gc->exec_ctx || !gc->exec_ctx->domain || element_size == 0) {
        return NULL;
    }

    array_type = element_type ? iron_type_make_array(gc->exec_ctx->domain, element_type, 1) : NULL;
    lengths[0] = length;
    return iron_gc_alloc_mdarray_raw(gc, array_type, element_type, element_size, lengths, NULL, 1);
}

void *iron_gc_alloc_mdarray_raw(iron_gc_t *gc,
                                iron_runtime_type_t *array_type,
                                iron_runtime_type_t *element_type,
                                iron_size element_size,
                                const iron_u32 *lengths,
                                const iron_i32 *lower_bounds,
                                iron_u32 rank)
{
    iron_array_payload_t *array;
    iron_size bounds_size;
    iron_size array_size;
    iron_size element_count;
    iron_u32 dimension;

    if (!gc || !array_type || !element_type || !lengths || element_size == 0 || rank == 0 || array_type->kind != IRON_KIND_ARRAY ||
        array_type->array_rank != rank || array_type->element != element_type) {
        return NULL;
    }

    if ((iron_size)rank > (((iron_size)-1) - sizeof(iron_array_payload_t)) / (sizeof(iron_u32) + sizeof(iron_i32))) {
        return NULL;
    }

    bounds_size = (iron_size)rank * (sizeof(iron_u32) + sizeof(iron_i32));
    element_count = 1;
    for (dimension = 0; dimension < rank; dimension++) {
        iron_i32 lower_bound;

        lower_bound = lower_bounds ? lower_bounds[dimension] : 0;
        if ((lengths[dimension] == 0 && lower_bound == INT_MIN) ||
            (lengths[dimension] > 0 && (iron_i64)lower_bound + (iron_i64)lengths[dimension] - 1 > INT_MAX)) {
            return NULL;
        }
        if (lengths[dimension] != 0 && (element_count > (iron_size)0xFFFFFFFFU / lengths[dimension] || element_count > ((iron_size)-1) / lengths[dimension])) {
            return NULL;
        }
        element_count *= lengths[dimension];
    }

    if (element_count > (((iron_size)-1) - sizeof(iron_array_payload_t) - bounds_size) / element_size) {
        return NULL;
    }

    array_size = sizeof(iron_array_payload_t) + bounds_size + element_size * element_count;

    IRON_TRACE_GC("ALLOC_ARRAY type=%s elem_size=%lu length=%lu rank=%u total=%lu",
                  element_type ? element_type->name : "<raw>",
                  (unsigned long)element_size, (unsigned long)element_count, rank,
                  (unsigned long)array_size);

    array = (iron_array_payload_t *)iron_gc_alloc_object(gc, array_type, array_size);
    if (!array) {
        return NULL;
    }

    array->length = (iron_u32)element_count;
    array->rank = rank;
    array->element_size = element_size;
    array->element_type = element_type;
    array->data_offset = sizeof(iron_array_payload_t) + bounds_size;
    iron_memcpy(array_lengths(array), lengths, (iron_size)rank * sizeof(iron_u32));
    if (lower_bounds) {
        iron_memcpy(array_lower_bounds(array), lower_bounds, (iron_size)rank * sizeof(iron_i32));
    }

    return array;
}

iron_u32 iron_array_get_length(const void *array)
{
    const iron_array_payload_t *payload;

    if (!array) {
        return 0;
    }

    payload = (const iron_array_payload_t *)array;
    return payload->length;
}

iron_u32 iron_array_get_rank(const void *array)
{
    const iron_array_payload_t *payload;

    if (!array) {
        return 0;
    }

    payload = (const iron_array_payload_t *)array;
    return payload->rank;
}

iron_bool iron_array_get_dimension_length(const void *array, iron_u32 dimension, iron_u32 *length)
{
    const iron_array_payload_t *payload;

    if (!array || !length) {
        return IRON_FALSE;
    }

    payload = (const iron_array_payload_t *)array;
    if (dimension >= payload->rank) {
        return IRON_FALSE;
    }

    *length = array_const_lengths(payload)[dimension];
    return IRON_TRUE;
}

iron_bool iron_array_get_lower_bound(const void *array, iron_u32 dimension, iron_i32 *lower_bound)
{
    const iron_array_payload_t *payload;

    if (!array || !lower_bound) {
        return IRON_FALSE;
    }

    payload = (const iron_array_payload_t *)array;
    if (dimension >= payload->rank) {
        return IRON_FALSE;
    }

    *lower_bound = array_const_lower_bounds(payload)[dimension];
    return IRON_TRUE;
}

iron_bool iron_array_get_element_offset(const void *array, const iron_i32 *indices, iron_u32 rank, iron_size *offset)
{
    const iron_array_payload_t *payload;
    const iron_u32 *lengths;
    const iron_i32 *lower_bounds;
    iron_size flattened_index;
    iron_u32 dimension;

    if (!array || !indices || !offset) {
        return IRON_FALSE;
    }

    payload = (const iron_array_payload_t *)array;
    if (rank != payload->rank) {
        return IRON_FALSE;
    }

    lengths = array_const_lengths(payload);
    lower_bounds = array_const_lower_bounds(payload);
    flattened_index = 0;
    for (dimension = 0; dimension < rank; dimension++) {
        iron_i64 relative_index;

        relative_index = (iron_i64)indices[dimension] - (iron_i64)lower_bounds[dimension];
        if (relative_index < 0 || (iron_u64)relative_index >= (iron_u64)lengths[dimension]) {
            return IRON_FALSE;
        }

        flattened_index = flattened_index * lengths[dimension] + (iron_size)relative_index;
    }

    if (flattened_index > ((iron_size)-1) / payload->element_size) {
        return IRON_FALSE;
    }

    *offset = flattened_index * payload->element_size;
    return IRON_TRUE;
}

iron_size iron_array_get_element_size(const void *array)
{
    const iron_array_payload_t *payload;

    if (!array) {
        return 0;
    }

    payload = (const iron_array_payload_t *)array;
    return payload->element_size;
}

iron_runtime_type_t *iron_array_get_element_type(const void *array)
{
    const iron_array_payload_t *payload;

    if (!array) {
        return NULL;
    }

    payload = (const iron_array_payload_t *)array;
    return payload->element_type;
}

void *iron_array_get_data(void *array)
{
    iron_array_payload_t *payload;

    if (!array) {
        return NULL;
    }

    payload = (iron_array_payload_t *)array;
    return (iron_u8 *)payload + payload->data_offset;
}

const void *iron_array_get_const_data(const void *array)
{
    const iron_array_payload_t *payload;

    if (!array) {
        return NULL;
    }

    payload = (const iron_array_payload_t *)array;
    return (const iron_u8 *)payload + payload->data_offset;
}

/* ============================================================================
 * Mark Phase
 * ============================================================================ */

static iron_gc_header_t *gc_find_object_header(const iron_gc_t *gc, const void *object)
{
    iron_gc_header_t *header;

    if (!gc || !object) {
        return NULL;
    }

    for (header = gc->all_objects; header; header = header->next) {
        if (IRON_GC_OBJECT(header) == object) {
            return header;
        }
    }

    return NULL;
}

void *iron_gc_find_containing_object(const iron_gc_t *gc, const void *address)
{
    iron_gc_header_t *header;
    uintptr_t candidate;

    if (!gc || !address) {
        return NULL;
    }

    candidate = (uintptr_t)address;
    for (header = gc->all_objects; header; header = header->next) {
        uintptr_t object_start;
        uintptr_t object_end;

        object_start = (uintptr_t)IRON_GC_OBJECT(header);
        if (candidate == object_start) {
            return IRON_GC_OBJECT(header);
        }
        if (header->size > (iron_size)(UINTPTR_MAX - object_start)) {
            continue;
        }

        object_end = object_start + (uintptr_t)header->size;
        if (candidate >= object_start && candidate < object_end) {
            return IRON_GC_OBJECT(header);
        }
    }

    return NULL;
}

static void gc_mark_stack_value(iron_gc_t *gc, const iron_stack_value_t *value)
{
    void *object;

    if (!gc || !value) {
        return;
    }

    object = NULL;
    switch (value->type) {
        case IRON_VAL_OBJ:
        case IRON_VAL_VALUETYPE:
            object = value->value.obj;
            break;
        case IRON_VAL_PTR:
            object = iron_gc_find_containing_object(gc, value->value.ptr);
            break;
        case IRON_VAL_BYREF:
            object = iron_gc_find_containing_object(gc, value->value.byref.ptr);
            break;
        case IRON_VAL_TYPEDREF:
            object = iron_gc_find_containing_object(gc, value->value.typedref.ptr);
            break;
        default:
            break;
    }

    if (object) {
        iron_gc_mark(gc, object);
    }
}

static void gc_mark_value_fields(iron_gc_t *gc, void *storage, iron_runtime_type_t *type)
{
    iron_runtime_type_t *current_type;
    iron_u32 field_index;

    if (!gc || !storage || !type) {
        return;
    }

    for (current_type = type; current_type; current_type = current_type->base_type) {
        for (field_index = 0; field_index < current_type->field_count; field_index++) {
            iron_runtime_field_t *field;
            void *field_storage;

            field = current_type->fields[field_index];
            if (!field || (field->attrs & 0x0010) != 0) {
                continue;
            }

            field_storage = (iron_u8 *)storage + field->offset;
            if ((field->field_type && iron_type_is_managed_reference(field->field_type)) ||
                (!field->field_type && iron_element_type_is_reference(field->element_type))) {
                void *referenced_object;

                memcpy(&referenced_object, field_storage, sizeof(referenced_object));
                iron_gc_mark(gc, referenced_object);
            } else if (field->field_type && (field->field_type->kind == IRON_KIND_VALUETYPE || field->field_type->kind == IRON_KIND_ENUM)) {
                gc_mark_value_fields(gc, field_storage, field->field_type);
            }
        }
    }
}

void iron_gc_mark(iron_gc_t *gc, void *obj)
{
    iron_gc_header_t *header;
    iron_runtime_type_t *type;
    iron_u32 i;
    
    if (!gc || !obj) return;

    header = gc_find_object_header(gc, obj);
    if (!header) {
        IRON_WARN_GC("Ignored invalid managed reference %p", obj);
        return;
    }
    
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
    
    if (type->kind == IRON_KIND_ARRAY && type->element) {
        iron_u32 length;
        iron_u8 *elements;
        iron_size element_size;

        length = iron_array_get_length(obj);
        elements = (iron_u8 *)iron_array_get_data(obj);
        element_size = iron_array_get_element_size(obj);
        
        for (i = 0; i < length; i++) {
            void *element_storage;

            element_storage = elements + (iron_size)i * element_size;
            if (iron_type_is_managed_reference(type->element)) {
                void *referenced_object;

                memcpy(&referenced_object, element_storage, sizeof(referenced_object));
                iron_gc_mark(gc, referenced_object);
            } else if (type->element->kind == IRON_KIND_VALUETYPE || type->element->kind == IRON_KIND_ENUM) {
                gc_mark_value_fields(gc, element_storage, type->element);
            }
        }

        return;
    }

    gc_mark_value_fields(gc, obj, type);
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

            iron_monitor_destroy_object(gc->exec_ctx, dead_obj);
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

static void gc_mark_thread_roots(iron_gc_t *gc, iron_thread_context_t *thread)
{
    iron_stack_frame_t *frame;
    iron_u32 index;

    if (!gc || !thread) {
        return;
    }

    iron_gc_mark(gc, thread->managed_thread);
    iron_gc_mark(gc, thread->start_delegate);
    iron_gc_mark(gc, thread->start_parameter);
    iron_gc_mark(gc, thread->exception_state.current_exception);

    IRON_TRACE_GC("MARK_ROOTS: thread=%p eval_stack.size=%u current_frame=%p",
                  (void *)thread,
                  thread->eval_stack.size,
                  (void *)thread->current_frame);

    for (index = 0; index < thread->eval_stack.size; index++) {
        gc_mark_stack_value(gc, &thread->eval_stack.data[index]);
    }

    for (frame = thread->current_frame; frame; frame = frame->prev) {
        IRON_TRACE_GC("MARK_ROOTS: frame method=%s args=%u locals=%u",
                      frame->method ? frame->method->name : "?",
                      frame->arg_count,
                      frame->local_count);

        iron_gc_mark(gc, frame->filter_exception);

        for (index = 0; frame->args && index < frame->arg_count; index++) {
            gc_mark_stack_value(gc, &frame->args[index]);
        }
        for (index = 0; frame->locals && index < frame->local_count; index++) {
            gc_mark_stack_value(gc, &frame->locals[index]);
        }
    }
}

static void gc_mark_roots(iron_gc_t *gc)
{
    iron_exec_context_t *ctx = gc->exec_ctx;
    iron_u32 i;
    
    IRON_TRACE_GC("MARK_ROOTS: thread_count=%u main_thread=%p", 
                 ctx->thread_count, (void*)ctx->main_thread);
    
    /* Mark from main thread first because it may not be present in the worker array. */
    gc_mark_thread_roots(gc, ctx->main_thread);
    
    /* Mark from other threads */
    for (i = 0; i < ctx->thread_count; i++) {
        iron_thread_context_t *thread = ctx->threads[i];
        
        if (thread && thread != ctx->main_thread) {
            gc_mark_thread_roots(gc, thread);
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

            if (!assembly) {
                continue;
            }

            iron_gc_mark(gc, assembly->managed_object);
            if (!assembly->module) {
                continue;
            }
            
            for (t = 0; t < assembly->module->type_count; t++) {
                iron_runtime_type_t *type = assembly->module->types[t];
                iron_u32 f;
                
                if (!type || !type->static_data) continue;
                
                for (f = 0; f < type->field_count; f++) {
                    iron_runtime_field_t *field = type->fields[f];
                    
                    if (!field) continue;
                    if (!(field->attrs & 0x0010)) continue; /* Not static */
                    
                    if ((field->field_type && iron_type_is_managed_reference(field->field_type)) ||
                        (!field->field_type && iron_element_type_is_reference(field->element_type))) {
                        void **field_ptr = (void **)((char *)type->static_data + field->offset);
                        iron_gc_mark(gc, *field_ptr);
                    } else if (field->field_type && (field->field_type->kind == IRON_KIND_VALUETYPE || field->field_type->kind == IRON_KIND_ENUM)) {
                        gc_mark_value_fields(gc, (iron_u8 *)type->static_data + field->offset, field->field_type);
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

static void gc_update_weak_refs(iron_gc_t *gc, iron_gc_handle_type_t handle_type)
{
    iron_gc_handle_t *handle;
    
    for (handle = gc->handles; handle; handle = handle->next) {
        iron_gc_header_t *header;

        if (handle->type != handle_type || !handle->target) {
            continue;
        }

        header = gc_find_object_header(gc, handle->target);
        if (!header || header->mark != gc->collection_generation) {
            handle->target = NULL;
        }
    }
}

static iron_runtime_method_t *gc_find_finalizer(iron_runtime_type_t *type)
{
    iron_u32 method_index;

    if (!type || !type->methods || (type->full_name && strcmp(type->full_name, "System.Object") == 0)) {
        return NULL;
    }

    for (method_index = 0; method_index < type->method_count; method_index++) {
        iron_runtime_method_t *method;

        method = type->methods[method_index];
        if (method && method->name && strcmp(method->name, "Finalize") == 0 && (method->attrs & 0x0010) == 0 && method->param_count == 0) {
            return method;
        }
    }

    return NULL;
}

static void gc_run_finalizers(iron_gc_t *gc)
{
    iron_gc_header_t *header;

    for (header = gc->all_objects; header;) {
        iron_gc_header_t *next;
        iron_runtime_method_t *finalizer;

        next = header->next;
        if (header->mark == gc->collection_generation || (header->flags & (IRON_GC_FINALIZED | IRON_GC_NO_FINALIZE)) != 0) {
            header = next;
            continue;
        }

        finalizer = gc_find_finalizer(header->type);
        if (finalizer) {
            iron_stack_value_t argument;
            iron_stack_value_t result_value;
            iron_result_t result;
            void *object;

            object = IRON_GC_OBJECT(header);
            iron_gc_mark(gc, object);
            header->flags |= IRON_GC_FINALIZED;

            memset(&argument, 0, sizeof(argument));
            argument.type = IRON_VAL_OBJ;
            argument.value.obj = object;

            result = iron_exec_method(gc->exec_ctx, finalizer, &argument, 1, &result_value);
            if (!IRON_RESULT_OK(result)) {
                IRON_ERROR_GC("Finalizer %s.%s failed: %s",
                              header->type && header->type->full_name ? header->type->full_name : "<unknown>",
                              finalizer->name,
                              result.message ? result.message : "unknown execution error");
            }
            gc->stats.finalized_count++;
        }

        header = next;
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
    
    /* Short weak references are cleared before finalization. */
    IRON_TRACE_GC("Updating short weak references...");
    gc_update_weak_refs(gc, IRON_GC_HANDLE_WEAK);

    if (gc->config.enable_finalization) {
        IRON_TRACE_GC("Running finalizers...");
        gc_run_finalizers(gc);

        /* Finalizers can resurrect objects or publish newly allocated objects. */
        gc_mark_roots(gc);
    }

    /* Resurrection-tracking handles survive until the object is truly reclaimable. */
    IRON_TRACE_GC("Updating resurrection-tracking weak references...");
    gc_update_weak_refs(gc, IRON_GC_HANDLE_WEAK_TRACK);
    
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
    header->flags &= ~(IRON_GC_NO_FINALIZE | IRON_GC_FINALIZED);
}

/* ============================================================================
 * Write Barriers
 * ============================================================================ */

void iron_gc_write_barrier(void *obj, void **field, void *value)
{
    (void)obj;
    memcpy(field, &value, sizeof(value));
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
