/*
 * IronNet CLR Interpreter
 * forward.h - Consolidated forward declarations
 *
 * All forward typedefs live here to avoid C99 redefinition errors.
 * Include this header instead of writing forward declarations inline.
 */

#ifndef IRON_FORWARD_H
#define IRON_FORWARD_H

/* Core infrastructure */
typedef struct iron_allocator iron_allocator_t;
typedef struct iron_string_builder iron_string_builder_t;

/* Runtime type system */
typedef struct iron_domain iron_domain_t;
typedef struct iron_assembly iron_assembly_t;
typedef struct iron_module iron_module_t;
typedef struct iron_runtime_type iron_runtime_type_t;
typedef struct iron_runtime_method iron_runtime_method_t;
typedef struct iron_runtime_field iron_runtime_field_t;
typedef struct iron_runtime_property iron_runtime_property_t;
typedef struct iron_runtime_event iron_runtime_event_t;
typedef struct iron_runtime_param iron_runtime_param_t;
typedef struct iron_generic_inst iron_generic_inst_t;

/* Execution engine */
typedef struct iron_exec_context iron_exec_context_t;
typedef struct iron_thread_context iron_thread_context_t;
typedef struct iron_stack_frame iron_stack_frame_t;
typedef struct iron_exception iron_exception_t;

#endif /* IRON_FORWARD_H */
