/*
 * IronNet CLR Interpreter
 * exec.h - Execution engine and IL interpreter
 * 
 * Defines:
 * - Execution context and thread context
 * - Stack frames and evaluation stack
 * - IL interpreter loop
 * - Exception handling
 * 
 * Pure C89 compatible
 */

#ifndef IRON_EXEC_H
#define IRON_EXEC_H

#include "platform.h"
#include "types.h"
#include "forward.h"
#include "memory.h"
#include "vtable.h"
#include "runtime.h"
#include "opcodes.h"
#include "gc.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Evaluation Stack
 * ============================================================================ */

#define IRON_DEFAULT_EVAL_STACK_SIZE 1024

/* Evaluation stack */
typedef struct iron_eval_stack {
    iron_stack_value_t *data;
    iron_u32 size;
    iron_u32 capacity;
} iron_eval_stack_t;

/* Stack operations */
IRON_API void iron_stack_init(iron_eval_stack_t *stack, iron_allocator_t *alloc,
                               iron_u32 capacity);
IRON_API void iron_stack_destroy(iron_eval_stack_t *stack, iron_allocator_t *alloc);
IRON_API void iron_stack_push(iron_eval_stack_t *stack, iron_stack_value_t value);
IRON_API iron_stack_value_t iron_stack_pop(iron_eval_stack_t *stack);
IRON_API iron_stack_value_t *iron_stack_peek(iron_eval_stack_t *stack, iron_u32 depth);
IRON_API void iron_stack_dup(iron_eval_stack_t *stack);
IRON_API void iron_stack_clear(iron_eval_stack_t *stack);

/* Push helpers */
IRON_API void iron_stack_push_i32(iron_eval_stack_t *stack, iron_i32 value);
IRON_API void iron_stack_push_i64(iron_eval_stack_t *stack, iron_i64 value);
IRON_API void iron_stack_push_f32(iron_eval_stack_t *stack, iron_f32 value);
IRON_API void iron_stack_push_f64(iron_eval_stack_t *stack, iron_f64 value);
IRON_API void iron_stack_push_ptr(iron_eval_stack_t *stack, void *value);
IRON_API void iron_stack_push_obj(iron_eval_stack_t *stack, void *obj);
IRON_API void iron_stack_push_null(iron_eval_stack_t *stack);

/* Pop helpers */
IRON_API iron_i32 iron_stack_pop_i32(iron_eval_stack_t *stack);
IRON_API iron_i64 iron_stack_pop_i64(iron_eval_stack_t *stack);
IRON_API iron_f32 iron_stack_pop_f32(iron_eval_stack_t *stack);
IRON_API iron_f64 iron_stack_pop_f64(iron_eval_stack_t *stack);
IRON_API void *iron_stack_pop_ptr(iron_eval_stack_t *stack);
IRON_API void *iron_stack_pop_obj(iron_eval_stack_t *stack);

/* ============================================================================
 * Stack Frame
 * ============================================================================ */

/* Frame flags */
typedef enum iron_frame_flags {
    IRON_FRAME_NONE          = 0x00,
    IRON_FRAME_EXCEPTION     = 0x01,  /* Exception handler active */
    IRON_FRAME_FINALLY       = 0x02,  /* Finally block active */
    IRON_FRAME_FILTER        = 0x04,  /* Filter block active */
    IRON_FRAME_TAIL_CALL     = 0x08,  /* Tail call optimization */
    IRON_FRAME_CONSTRAINED   = 0x10   /* Constrained call prefix */
} iron_frame_flags_t;

/* Stack frame */
struct iron_stack_frame {
    /* Method being executed */
    iron_runtime_method_t *method;
    
    /* Code pointer */
    const iron_u8 *code;
    iron_u32 code_size;
    iron_u32 ip;  /* Instruction pointer (offset) */
    
    /* Arguments and locals */
    iron_stack_value_t *args;
    iron_u32 arg_count;
    iron_stack_value_t *locals;
    iron_u32 local_count;
    
    /* Evaluation stack base for this frame */
    iron_u32 stack_base;
    
    /* Exception handling */
    iron_u32 exception_handler_index;
    
    /* Flags and prefixes */
    iron_u32 flags;
    iron_runtime_type_t *constrained_type;  /* For constrained. prefix */
    iron_u8 unaligned_prefix;               /* For unaligned. prefix */
    iron_bool volatile_prefix;              /* For volatile. prefix */
    iron_bool tail_prefix;                  /* For tail. prefix */
    iron_bool readonly_prefix;              /* For readonly. prefix */
    
    /* Previous frame (call stack) */
    iron_stack_frame_t *prev;
    
    /* Return value storage */
    iron_stack_value_t *return_value;
};

/* ============================================================================
 * Exception Handling
 * ============================================================================ */

/* Exception object (managed) */
struct iron_exception {
    iron_gc_header_t gc;
    iron_runtime_type_t *type;
    void *message;           /* System.String */
    void *inner_exception;   /* System.Exception */
    void *stack_trace;       /* System.String */
    iron_i32 hresult;
};

/* Exception state */
typedef struct iron_exception_state {
    iron_exception_t *current_exception;
    iron_stack_frame_t *throw_frame;
    iron_u32 throw_ip;
    iron_bool is_rethrow;
} iron_exception_state_t;

/* ============================================================================
 * Thread Context
 * ============================================================================ */

/* Thread state */
typedef enum iron_thread_state {
    IRON_THREAD_CREATED,
    IRON_THREAD_RUNNING,
    IRON_THREAD_SUSPENDED,
    IRON_THREAD_WAITING,
    IRON_THREAD_STOPPED
} iron_thread_state_t;

/* Thread context */
struct iron_thread_context {
    iron_object_t base;
    iron_exec_context_t *exec_ctx;
    iron_allocator_t *allocator;
    
    /* Thread identity */
    iron_u32 thread_id;
    const char *name;
    iron_thread_state_t state;
    
    /* Call stack */
    iron_stack_frame_t *current_frame;
    iron_u32 frame_count;
    iron_u32 max_stack_depth;
    
    /* Evaluation stack */
    iron_eval_stack_t eval_stack;
    
    /* Exception state */
    iron_exception_state_t exception_state;
    
    /* Thread-local storage */
    iron_hashmap_t tls;
    
    /* Synchronization */
    void *native_thread;     /* Platform-specific thread handle */
    void *wait_handle;       /* What we're waiting on */
    
    /* Managed thread object */
    void *managed_thread;    /* System.Threading.Thread */
    
    /* Arena for frame allocations */
    iron_arena_t frame_arena;
};

/* Thread API */
IRON_API iron_result_t iron_thread_create(iron_thread_context_t **out_thread,
                                           iron_exec_context_t *ctx,
                                           iron_runtime_method_t *start_method,
                                           void *parameter);
IRON_API void iron_thread_destroy(iron_thread_context_t *thread);
IRON_API iron_result_t iron_thread_ctx_start(iron_thread_context_t *thread);
IRON_API iron_result_t iron_thread_ctx_join(iron_thread_context_t *thread,
                                             iron_u32 timeout_ms);
IRON_API void iron_thread_ctx_suspend(iron_thread_context_t *thread);
IRON_API void iron_thread_ctx_resume(iron_thread_context_t *thread);
IRON_API void iron_thread_ctx_abort(iron_thread_context_t *thread);

/* ============================================================================
 * Execution Context
 * ============================================================================ */

/* Execution context - global interpreter state */
struct iron_exec_context {
    iron_object_t base;
    iron_allocator_t *allocator;
    
    /* Domain */
    iron_domain_t *domain;
    
    /* Threads */
    iron_thread_context_t *main_thread;
    iron_thread_context_t **threads;
    iron_u32 thread_count;
    iron_u32 thread_capacity;
    iron_u32 next_thread_id;
    
    /* Current thread (use iron_exec_get_current_thread) */
    iron_thread_context_t *current_thread;
    
    /* Internal call table */
    iron_hashmap_t internal_calls;
    
    /* GC - uses iron_gc_t from gc.h */
    iron_gc_t gc;
    
    /* Synchronization primitives pool */
    void *sync_pool;
    
    /* Debug/profiling hooks */
    void (*on_method_enter)(iron_exec_context_t *ctx, iron_runtime_method_t *method);
    void (*on_method_exit)(iron_exec_context_t *ctx, iron_runtime_method_t *method);
    void (*on_exception)(iron_exec_context_t *ctx, iron_exception_t *ex);
    void (*on_breakpoint)(iron_exec_context_t *ctx, iron_u32 ip);
    void *debug_user_data;
};

/* Execution context API */
IRON_API iron_result_t iron_exec_create(iron_exec_context_t **out_ctx,
                                         iron_domain_t *domain);
IRON_API void iron_exec_destroy(iron_exec_context_t *ctx);
IRON_API iron_thread_context_t *iron_exec_get_current_thread(iron_exec_context_t *ctx);

/* ============================================================================
 * IL Interpreter
 * ============================================================================ */

/* Interpreter result */
typedef enum iron_interp_result {
    IRON_INTERP_OK,
    IRON_INTERP_RETURN,
    IRON_INTERP_EXCEPTION,
    IRON_INTERP_CALL,
    IRON_INTERP_BRANCH,
    IRON_INTERP_BREAK,
    IRON_INTERP_ERROR
} iron_interp_result_t;

/* Execute a method */
IRON_API iron_result_t iron_exec_method(iron_exec_context_t *ctx,
                                         iron_runtime_method_t *method,
                                         iron_stack_value_t *args,
                                         iron_u32 arg_count,
                                         iron_stack_value_t *result);

/* Execute a single instruction */
IRON_API iron_interp_result_t iron_exec_instruction(iron_thread_context_t *thread);

/* Execute until return or exception */
IRON_API iron_result_t iron_exec_run(iron_thread_context_t *thread);

/* Execute assembly entry point (Main method) */
IRON_API iron_result_t iron_exec_entry_point(iron_exec_context_t *ctx,
                                              iron_runtime_method_t *entry_point,
                                              const char **args,
                                              int argc,
                                              int *exit_code);

/* Call method (creates new frame) */
IRON_API iron_result_t iron_exec_call(iron_thread_context_t *thread,
                                       iron_runtime_method_t *method,
                                       iron_u32 arg_count);

/* Virtual call dispatch */
IRON_API iron_runtime_method_t *iron_exec_resolve_virtual(iron_runtime_type_t *obj_type,
                                                           iron_runtime_method_t *method);

/* Interface call dispatch */
IRON_API iron_runtime_method_t *iron_exec_resolve_interface(iron_runtime_type_t *obj_type,
                                                             iron_runtime_type_t *interface_type,
                                                             iron_runtime_method_t *method);

/* ============================================================================
 * Object Allocation
 * ============================================================================ */

/* Allocate new object */
IRON_API void *iron_gc_alloc(iron_exec_context_t *ctx, 
                              iron_runtime_type_t *type);

/* Allocate array */
IRON_API void *iron_gc_alloc_array(iron_exec_context_t *ctx,
                                    iron_runtime_type_t *element_type,
                                    iron_u32 length);

/* Allocate string */
IRON_API void *iron_gc_alloc_string(iron_exec_context_t *ctx,
                                     const iron_u16 *chars,
                                     iron_u32 length);

/* Box value type */
IRON_API void *iron_gc_box(iron_exec_context_t *ctx,
                            iron_runtime_type_t *type,
                            const void *value);

/* Unbox value type */
IRON_API void *iron_gc_unbox(void *obj, iron_runtime_type_t *type);

/* ============================================================================
 * Exception Handling
 * ============================================================================ */

/* Throw exception */
IRON_API void iron_throw(iron_thread_context_t *thread, iron_exception_t *ex);

/* Throw new exception of type */
IRON_API void iron_throw_new(iron_thread_context_t *thread,
                              iron_runtime_type_t *type,
                              const char *message);

/* Throw common exceptions */
IRON_API void iron_throw_null_reference(iron_thread_context_t *thread);
IRON_API void iron_throw_index_out_of_range(iron_thread_context_t *thread);
IRON_API void iron_throw_invalid_cast(iron_thread_context_t *thread);
IRON_API void iron_throw_overflow(iron_thread_context_t *thread);
IRON_API void iron_throw_divide_by_zero(iron_thread_context_t *thread);

/* Find exception handler */
IRON_API iron_bool iron_find_exception_handler(iron_thread_context_t *thread,
                                                iron_exception_t *ex,
                                                iron_stack_frame_t **out_frame,
                                                iron_u32 *out_handler_index);

/* ============================================================================
 * Internal Call Registration
 * ============================================================================ */

/* Internal call signature */
typedef iron_result_t (*iron_internal_call_fn)(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result
);

/* Internal call entry */
typedef struct iron_internal_call {
    const char *type_name;
    const char *method_name;
    const char *signature;
    iron_internal_call_fn fn;
} iron_internal_call_t;

/* Register internal call */
IRON_API iron_result_t iron_register_internal_call(iron_exec_context_t *ctx,
                                                    const char *type_name,
                                                    const char *method_name,
                                                    const char *signature,
                                                    iron_internal_call_fn fn);

/* Register multiple internal calls */
IRON_API iron_result_t iron_register_internal_calls(iron_exec_context_t *ctx,
                                                     const iron_internal_call_t *calls,
                                                     iron_u32 count);

/* Lookup internal call */
IRON_API iron_internal_call_fn iron_lookup_internal_call(iron_exec_context_t *ctx,
                                                          iron_runtime_method_t *method);

/* ============================================================================
 * Synchronization Primitives
 * ============================================================================ */

/* Monitor (object lock) */
IRON_API iron_result_t iron_monitor_enter(iron_exec_context_t *ctx, void *obj);
IRON_API iron_result_t iron_monitor_exit(iron_exec_context_t *ctx, void *obj);
IRON_API iron_result_t iron_monitor_try_enter(iron_exec_context_t *ctx, void *obj,
                                               iron_u32 timeout_ms, iron_bool *acquired);
IRON_API iron_result_t iron_monitor_wait(iron_exec_context_t *ctx, void *obj,
                                          iron_u32 timeout_ms);
IRON_API iron_result_t iron_monitor_pulse(iron_exec_context_t *ctx, void *obj);
IRON_API iron_result_t iron_monitor_pulse_all(iron_exec_context_t *ctx, void *obj);

#ifdef __cplusplus
}
#endif

#endif /* IRON_EXEC_H */
