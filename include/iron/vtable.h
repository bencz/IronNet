/*
 * IronNet CLR Interpreter
 * vtable.h - Virtual table infrastructure for polymorphic types
 * 
 * Provides a C99-compatible vtable system for:
 * - Runtime type information
 * - Object polymorphism
 * - Visitor pattern support
 * 
 * Strict C99 compatible
 */

#ifndef IRON_VTABLE_H
#define IRON_VTABLE_H

#include "platform.h"
#include "types.h"
#include "forward.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Type Information
 * ============================================================================ */

/* Type ID enumeration for runtime type checking */
typedef enum iron_type_id {
    IRON_TID_OBJECT = 0,
    
    /* Runtime types */
    IRON_TID_RUNTIME_TYPE,
    IRON_TID_RUNTIME_METHOD,
    IRON_TID_RUNTIME_FIELD,
    IRON_TID_RUNTIME_PROPERTY,
    IRON_TID_RUNTIME_EVENT,
    IRON_TID_RUNTIME_PARAM,
    IRON_TID_RUNTIME_GENERIC_PARAM,
    IRON_TID_RUNTIME_ASSEMBLY,
    IRON_TID_RUNTIME_MODULE,
    
    /* Type system */
    IRON_TID_TYPE_DEF,
    IRON_TID_TYPE_REF,
    IRON_TID_TYPE_SPEC,
    IRON_TID_ARRAY_TYPE,
    IRON_TID_POINTER_TYPE,
    IRON_TID_BYREF_TYPE,
    IRON_TID_GENERIC_INST,
    IRON_TID_GENERIC_PARAM,
    IRON_TID_FNPTR_TYPE,
    
    /* Execution */
    IRON_TID_EXEC_CONTEXT,
    IRON_TID_THREAD_CONTEXT,
    IRON_TID_STACK_FRAME,
    
    /* GC objects */
    IRON_TID_GC_OBJECT,
    IRON_TID_GC_STRING,
    IRON_TID_GC_ARRAY,
    IRON_TID_GC_DELEGATE,
    IRON_TID_GC_EXCEPTION,
    
    IRON_TID_COUNT
} iron_type_id_t;

/* Type information structure */
typedef struct iron_type_info {
    const char *name;
    iron_size size;
    iron_size alignment;
    const struct iron_type_info *parent;
    iron_type_id_t type_id;
} iron_type_info_t;

/* ============================================================================
 * Base Object Vtable
 * ============================================================================ */

typedef struct iron_object iron_object_t;

/* Base vtable with common operations */
typedef struct iron_object_vtable {
    const iron_type_info_t *type_info;
    void (*destroy)(iron_object_t *self, iron_allocator_t *alloc);
    iron_object_t *(*clone)(const iron_object_t *self, iron_allocator_t *alloc);
    iron_bool (*equals)(const iron_object_t *self, const iron_object_t *other);
    iron_u32 (*hash)(const iron_object_t *self);
    void (*to_string)(const iron_object_t *self, iron_string_builder_t *sb);
} iron_object_vtable_t;

/* Base object structure - all polymorphic types embed this */
struct iron_object {
    const iron_object_vtable_t *vt;
};

/* Object interface macros */
#define IRON_OBJECT(ptr)        ((iron_object_t*)(ptr))
#define IRON_VTABLE(ptr)        (IRON_OBJECT(ptr)->vt)
#define IRON_TYPE_INFO(ptr)     (IRON_VTABLE(ptr)->type_info)
#define IRON_TYPE_NAME(ptr)     (IRON_TYPE_INFO(ptr)->name)
#define IRON_TYPE_ID(ptr)       (IRON_TYPE_INFO(ptr)->type_id)

#define iron_obj_destroy(ptr, alloc) \
    (IRON_VTABLE(ptr)->destroy(IRON_OBJECT(ptr), (alloc)))

#define iron_obj_clone(ptr, alloc) \
    (IRON_VTABLE(ptr)->clone(IRON_OBJECT(ptr), (alloc)))

#define iron_obj_equals(a, b) \
    (IRON_VTABLE(a)->equals(IRON_OBJECT(a), IRON_OBJECT(b)))

#define iron_obj_hash(ptr) \
    (IRON_VTABLE(ptr)->hash(IRON_OBJECT(ptr)))

#define iron_obj_to_string(ptr, sb) \
    (IRON_VTABLE(ptr)->to_string(IRON_OBJECT(ptr), (sb)))

/* ============================================================================
 * Type Checking
 * ============================================================================ */

/* Check if object is of specific type (including inheritance) */
IRON_API iron_bool iron_is_type(const iron_object_t *obj, 
                                 const iron_type_info_t *type);

/* Check if object is exactly of specific type */
IRON_API iron_bool iron_is_exact_type(const iron_object_t *obj,
                                       const iron_type_info_t *type);

/* Safe downcast with type checking */
#define IRON_CAST(T, ptr) \
    (iron_is_type(IRON_OBJECT(ptr), &T##_type_info) ? (T*)(ptr) : NULL)

/* Unsafe cast (no checking) */
#define IRON_UNSAFE_CAST(T, ptr) ((T*)(ptr))

/* ============================================================================
 * Type Registration Macros
 * ============================================================================ */

/* Declare type info extern */
#define IRON_DECLARE_TYPE(T) \
    extern const iron_type_info_t T##_type_info; \
    extern const iron_object_vtable_t T##_vtable

/* Define type info */
#define IRON_DEFINE_TYPE(T, parent_info, tid) \
    const iron_type_info_t T##_type_info = { \
        #T, \
        sizeof(T), \
        sizeof(void*), \
        parent_info, \
        tid \
    }

/* Define vtable */
#define IRON_DEFINE_VTABLE(T, destroy_fn, clone_fn, equals_fn, hash_fn, tostr_fn) \
    const iron_object_vtable_t T##_vtable = { \
        &T##_type_info, \
        (void (*)(iron_object_t*, iron_allocator_t*))destroy_fn, \
        (iron_object_t* (*)(const iron_object_t*, iron_allocator_t*))clone_fn, \
        (iron_bool (*)(const iron_object_t*, const iron_object_t*))equals_fn, \
        (iron_u32 (*)(const iron_object_t*))hash_fn, \
        (void (*)(const iron_object_t*, iron_string_builder_t*))tostr_fn \
    }

/* Initialize object with vtable */
#define IRON_INIT_OBJECT(ptr, T) \
    (IRON_OBJECT(ptr)->vt = (const iron_object_vtable_t*)&T##_vtable)

/* ============================================================================
 * Extended Vtable for Runtime Types
 * ============================================================================ */

/* Runtime type vtable */
typedef struct iron_runtime_type_vtable {
    iron_object_vtable_t base;
    
    /* Type queries */
    iron_bool (*is_value_type)(const iron_runtime_type_t *self);
    iron_bool (*is_reference_type)(const iron_runtime_type_t *self);
    iron_bool (*is_interface)(const iron_runtime_type_t *self);
    iron_bool (*is_abstract)(const iron_runtime_type_t *self);
    iron_bool (*is_sealed)(const iron_runtime_type_t *self);
    iron_bool (*is_generic)(const iron_runtime_type_t *self);
    iron_bool (*is_generic_definition)(const iron_runtime_type_t *self);
    iron_bool (*is_array)(const iron_runtime_type_t *self);
    iron_bool (*is_pointer)(const iron_runtime_type_t *self);
    iron_bool (*is_byref)(const iron_runtime_type_t *self);
    iron_bool (*is_enum)(const iron_runtime_type_t *self);
    
    /* Type relationships */
    iron_bool (*is_assignable_from)(const iron_runtime_type_t *self,
                                     const iron_runtime_type_t *other);
    iron_bool (*is_subclass_of)(const iron_runtime_type_t *self,
                                 const iron_runtime_type_t *other);
    iron_bool (*implements_interface)(const iron_runtime_type_t *self,
                                       const iron_runtime_type_t *iface);
    
    /* Type info */
    iron_element_type_t (*get_element_type)(const iron_runtime_type_t *self);
    iron_size (*get_size)(const iron_runtime_type_t *self);
    iron_size (*get_alignment)(const iron_runtime_type_t *self);
    iron_runtime_type_t *(*get_base_type)(const iron_runtime_type_t *self);
    iron_runtime_type_t *(*get_element)(const iron_runtime_type_t *self);
    
    /* Generic support */
    iron_u32 (*get_generic_param_count)(const iron_runtime_type_t *self);
    iron_runtime_type_t *(*get_generic_param)(const iron_runtime_type_t *self, 
                                               iron_u32 index);
    iron_runtime_type_t *(*make_generic_type)(const iron_runtime_type_t *self,
                                               iron_runtime_type_t **args,
                                               iron_u32 arg_count,
                                               iron_allocator_t *alloc);
    
    /* Members */
    iron_u32 (*get_method_count)(const iron_runtime_type_t *self);
    iron_runtime_method_t *(*get_method)(const iron_runtime_type_t *self, 
                                          iron_u32 index);
    iron_runtime_method_t *(*find_method)(const iron_runtime_type_t *self,
                                           const char *name,
                                           iron_runtime_type_t **param_types,
                                           iron_u32 param_count);
    
    iron_u32 (*get_field_count)(const iron_runtime_type_t *self);
    iron_runtime_field_t *(*get_field)(const iron_runtime_type_t *self, 
                                        iron_u32 index);
    iron_runtime_field_t *(*find_field)(const iron_runtime_type_t *self,
                                         const char *name);
} iron_runtime_type_vtable_t;

/* Runtime type interface macros */
#define IRON_TYPE_VT(ptr) \
    ((const iron_runtime_type_vtable_t*)IRON_VTABLE(ptr))

#define iron_type_is_value(t) \
    (IRON_TYPE_VT(t)->is_value_type((t)))

#define iron_type_is_reference(t) \
    (IRON_TYPE_VT(t)->is_reference_type((t)))

#define iron_type_is_generic(t) \
    (IRON_TYPE_VT(t)->is_generic((t)))

#define iron_type_get_size(t) \
    (IRON_TYPE_VT(t)->get_size((t)))

#define iron_type_get_base(t) \
    (IRON_TYPE_VT(t)->get_base_type((t)))

/* ============================================================================
 * Extended Vtable for Runtime Methods
 * ============================================================================ */

/* Method invocation function pointer */
typedef iron_result_t (*iron_method_invoke_fn)(
    iron_exec_context_t *ctx,
    iron_runtime_method_t *method,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result
);

/* Runtime method vtable */
typedef struct iron_runtime_method_vtable {
    iron_object_vtable_t base;
    
    /* Method info */
    const char *(*get_name)(const iron_runtime_method_t *self);
    iron_runtime_type_t *(*get_declaring_type)(const iron_runtime_method_t *self);
    iron_runtime_type_t *(*get_return_type)(const iron_runtime_method_t *self);
    iron_u32 (*get_param_count)(const iron_runtime_method_t *self);
    iron_runtime_type_t *(*get_param_type)(const iron_runtime_method_t *self, 
                                            iron_u32 index);
    
    /* Method attributes */
    iron_bool (*is_static)(const iron_runtime_method_t *self);
    iron_bool (*is_virtual)(const iron_runtime_method_t *self);
    iron_bool (*is_abstract)(const iron_runtime_method_t *self);
    iron_bool (*is_final)(const iron_runtime_method_t *self);
    iron_bool (*is_constructor)(const iron_runtime_method_t *self);
    iron_bool (*is_internal_call)(const iron_runtime_method_t *self);
    
    /* Generic support */
    iron_bool (*is_generic)(const iron_runtime_method_t *self);
    iron_u32 (*get_generic_param_count)(const iron_runtime_method_t *self);
    iron_runtime_method_t *(*make_generic_method)(
        const iron_runtime_method_t *self,
        iron_runtime_type_t **args,
        iron_u32 arg_count,
        iron_allocator_t *alloc
    );
    
    /* Invocation */
    iron_method_invoke_fn get_invoke_fn;
} iron_runtime_method_vtable_t;

/* Method interface macros */
#define IRON_METHOD_VT(ptr) \
    ((const iron_runtime_method_vtable_t*)IRON_VTABLE(ptr))

#define iron_method_get_name(m) \
    (IRON_METHOD_VT(m)->get_name((m)))

#define iron_method_is_static(m) \
    (IRON_METHOD_VT(m)->is_static((m)))

#define iron_method_is_internal_call(m) \
    (IRON_METHOD_VT(m)->is_internal_call((m)))

/* ============================================================================
 * GC Object Header
 * ============================================================================ */

/* GC flags */
typedef enum iron_gc_flags {
    IRON_GC_MARKED       = 0x01,
    IRON_GC_PINNED       = 0x02,
    IRON_GC_FINALIZED    = 0x04,
    IRON_GC_WEAK_REF     = 0x08,
    IRON_GC_NO_FINALIZE  = 0x10
} iron_gc_flags_t;

/* GC object header - prepended to all managed objects */
typedef struct iron_gc_header {
    iron_runtime_type_t *type;      /* Runtime type */
    iron_u32 flags;                  /* GC flags */
    iron_size size;                  /* Object payload size */
    iron_u32 mark;                   /* Mark generation */
    struct iron_gc_header *next;     /* Next in allocation list */
    void *sync_block;                /* Lazily allocated object monitor */
} iron_gc_header_t;

/* Get GC header from object pointer */
#define IRON_GC_HEADER(obj) \
    (((iron_gc_header_t*)(obj)) - 1)

/* Get object pointer from GC header */
#define IRON_GC_OBJECT(header) \
    ((void*)((header) + 1))

#ifdef __cplusplus
}
#endif

#endif /* IRON_VTABLE_H */
