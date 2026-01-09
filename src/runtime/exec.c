/*
 * IronNet CLR Interpreter
 * exec.c - IL Execution engine implementation
 */

#include "iron/exec.h"
#include "iron/opcodes.h"
#include "iron/metadata.h"
#include "iron/runtime.h"
#include "iron/gc.h"
#include "iron/types.h"
#include <string.h>
#include <stdio.h>

/* Forward declarations for helper functions */
static iron_result_t resolve_method_token(iron_thread_context_t *thread,
                                          iron_u32 token,
                                          iron_runtime_method_t **out_method,
                                          iron_assembly_t **out_assembly);
static iron_result_t call_method(iron_thread_context_t *thread,
                                 iron_runtime_method_t *method,
                                 iron_assembly_t *assembly,
                                 iron_bool is_newobj);
static iron_u32 get_method_param_count_from_sig(const iron_u8 *sig_data, iron_u32 sig_size);

/* ============================================================================
 * Evaluation Stack Implementation
 * ============================================================================ */

void iron_stack_init(iron_eval_stack_t *stack, iron_allocator_t *alloc,
                     iron_u32 capacity)
{
    stack->data = (iron_stack_value_t *)iron_alloc(alloc, 
        capacity * sizeof(iron_stack_value_t));
    stack->size = 0;
    stack->capacity = capacity;
}

void iron_stack_destroy(iron_eval_stack_t *stack, iron_allocator_t *alloc)
{
    if (stack->data) {
        iron_free(alloc, stack->data, stack->capacity * sizeof(iron_stack_value_t));
        stack->data = NULL;
    }
    stack->size = 0;
    stack->capacity = 0;
}

void iron_stack_push(iron_eval_stack_t *stack, iron_stack_value_t value)
{
    IRON_ASSERT(stack->size < stack->capacity);
    stack->data[stack->size++] = value;
}

iron_stack_value_t iron_stack_pop(iron_eval_stack_t *stack)
{
    IRON_ASSERT(stack->size > 0);
    return stack->data[--stack->size];
}

iron_stack_value_t *iron_stack_peek(iron_eval_stack_t *stack, iron_u32 depth)
{
    IRON_ASSERT(stack->size > depth);
    return &stack->data[stack->size - 1 - depth];
}

void iron_stack_dup(iron_eval_stack_t *stack)
{
    IRON_ASSERT(stack->size > 0 && stack->size < stack->capacity);
    stack->data[stack->size] = stack->data[stack->size - 1];
    stack->size++;
}

void iron_stack_clear(iron_eval_stack_t *stack)
{
    stack->size = 0;
}

void iron_stack_push_i32(iron_eval_stack_t *stack, iron_i32 value)
{
    iron_stack_value_t sv;
    sv.type = IRON_VAL_I32;
    sv.value.i32 = value;
    iron_stack_push(stack, sv);
}

void iron_stack_push_i64(iron_eval_stack_t *stack, iron_i64 value)
{
    iron_stack_value_t sv;
    sv.type = IRON_VAL_I64;
    sv.value.i64 = value;
    iron_stack_push(stack, sv);
}

void iron_stack_push_f32(iron_eval_stack_t *stack, iron_f32 value)
{
    iron_stack_value_t sv;
    sv.type = IRON_VAL_F32;
    sv.value.f32 = value;
    iron_stack_push(stack, sv);
}

void iron_stack_push_f64(iron_eval_stack_t *stack, iron_f64 value)
{
    iron_stack_value_t sv;
    sv.type = IRON_VAL_F64;
    sv.value.f64 = value;
    iron_stack_push(stack, sv);
}

void iron_stack_push_ptr(iron_eval_stack_t *stack, void *value)
{
    iron_stack_value_t sv;
    sv.type = IRON_VAL_PTR;
    sv.value.ptr = value;
    iron_stack_push(stack, sv);
}

void iron_stack_push_obj(iron_eval_stack_t *stack, void *obj)
{
    iron_stack_value_t sv;
    sv.type = IRON_VAL_OBJ;
    sv.value.obj = obj;
    iron_stack_push(stack, sv);
}

void iron_stack_push_null(iron_eval_stack_t *stack)
{
    iron_stack_value_t sv;
    sv.type = IRON_VAL_OBJ;
    sv.value.obj = NULL;
    iron_stack_push(stack, sv);
}

iron_i32 iron_stack_pop_i32(iron_eval_stack_t *stack)
{
    iron_stack_value_t sv = iron_stack_pop(stack);
    switch (sv.type) {
        case IRON_VAL_I32: return sv.value.i32;
        case IRON_VAL_I64: return (iron_i32)sv.value.i64;
        case IRON_VAL_PTR: return (iron_i32)(iron_size)sv.value.ptr;
        default: return 0;
    }
}

iron_i64 iron_stack_pop_i64(iron_eval_stack_t *stack)
{
    iron_stack_value_t sv = iron_stack_pop(stack);
    switch (sv.type) {
        case IRON_VAL_I32: return (iron_i64)sv.value.i32;
        case IRON_VAL_I64: return sv.value.i64;
        case IRON_VAL_PTR: return (iron_i64)(iron_size)sv.value.ptr;
        default: return 0;
    }
}

iron_f32 iron_stack_pop_f32(iron_eval_stack_t *stack)
{
    iron_stack_value_t sv = iron_stack_pop(stack);
    return (sv.type == IRON_VAL_F32) ? sv.value.f32 : (iron_f32)sv.value.f64;
}

iron_f64 iron_stack_pop_f64(iron_eval_stack_t *stack)
{
    iron_stack_value_t sv = iron_stack_pop(stack);
    return (sv.type == IRON_VAL_F64) ? sv.value.f64 : (iron_f64)sv.value.f32;
}

void *iron_stack_pop_ptr(iron_eval_stack_t *stack)
{
    iron_stack_value_t sv = iron_stack_pop(stack);
    return sv.value.ptr;
}

void *iron_stack_pop_obj(iron_eval_stack_t *stack)
{
    iron_stack_value_t sv = iron_stack_pop(stack);
    return sv.value.obj;
}

/* ============================================================================
 * Execution Context
 * ============================================================================ */

iron_result_t iron_exec_create(iron_exec_context_t **out_ctx, iron_domain_t *domain)
{
    iron_exec_context_t *ctx;
    iron_thread_context_t *main_thread;
    iron_allocator_t *alloc;
    
    if (!out_ctx || !domain) {
        return IRON_ERROR(IRON_ERR_NULL_POINTER, "Invalid parameters");
    }
    
    alloc = domain->allocator;
    ctx = (iron_exec_context_t *)iron_alloc(alloc, sizeof(iron_exec_context_t));
    if (!ctx) {
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate exec context");
    }
    
    memset(ctx, 0, sizeof(iron_exec_context_t));
    ctx->allocator = alloc;
    ctx->domain = domain;
    
    /* Initialize internal call table */
    iron_hashmap_init(&ctx->internal_calls, alloc, 
                      sizeof(const char*), sizeof(iron_internal_call_fn),
                      iron_hash_string, iron_string_eq);
    
    /* Create main thread context */
    main_thread = (iron_thread_context_t *)iron_alloc(alloc, sizeof(iron_thread_context_t));
    if (!main_thread) {
        iron_hashmap_destroy(&ctx->internal_calls);
        iron_free(alloc, ctx, sizeof(iron_exec_context_t));
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate main thread");
    }
    
    memset(main_thread, 0, sizeof(iron_thread_context_t));
    main_thread->exec_ctx = ctx;
    main_thread->allocator = alloc;
    main_thread->thread_id = 1;
    main_thread->name = "Main";
    main_thread->state = IRON_THREAD_RUNNING;
    main_thread->max_stack_depth = 1024;
    
    /* Initialize evaluation stack */
    iron_stack_init(&main_thread->eval_stack, alloc, 256);
    
    ctx->main_thread = main_thread;
    
    *out_ctx = ctx;
    return (iron_result_t)IRON_SUCCESS;
}

void iron_exec_destroy(iron_exec_context_t *ctx)
{
    if (!ctx) return;
    
    iron_hashmap_destroy(&ctx->internal_calls);
    iron_free(ctx->allocator, ctx, sizeof(iron_exec_context_t));
}

/* ============================================================================
 * Internal Call Registration
 * ============================================================================ */

iron_result_t iron_register_internal_call(iron_exec_context_t *ctx,
                                          const char *type_name,
                                          const char *method_name,
                                          const char *signature,
                                          iron_internal_call_fn fn)
{
    char key[512];
    const char *key_ptr;
    
    if (!ctx || !type_name || !method_name || !fn) {
        return IRON_ERROR(IRON_ERR_NULL_POINTER, "Invalid parameters");
    }
    
    /* Build key: "TypeName::MethodName(Signature)" */
    snprintf(key, sizeof(key), "%s::%s(%s)", type_name, method_name, 
             signature ? signature : "");
    
    key_ptr = iron_intern_cstr(&ctx->domain->interner, key);
    iron_hashmap_set(&ctx->internal_calls, &key_ptr, &fn);
    
    return (iron_result_t)IRON_SUCCESS;
}

iron_result_t iron_register_internal_calls(iron_exec_context_t *ctx,
                                           const iron_internal_call_t *calls,
                                           iron_u32 count)
{
    iron_u32 i;
    iron_result_t result;
    
    for (i = 0; i < count; i++) {
        result = iron_register_internal_call(ctx, calls[i].type_name,
                                             calls[i].method_name,
                                             calls[i].signature,
                                             calls[i].fn);
        if (!IRON_RESULT_OK(result)) {
            return result;
        }
    }
    
    return (iron_result_t)IRON_SUCCESS;
}

iron_internal_call_fn iron_lookup_internal_call(iron_exec_context_t *ctx,
                                                iron_runtime_method_t *method)
{
    char key[512];
    const char *key_ptr;
    iron_internal_call_fn fn = NULL;
    
    if (!ctx || !method) return NULL;
    
    /* Build key from method info */
    snprintf(key, sizeof(key), "%s::%s()", 
             method->declaring_type ? method->declaring_type->full_name : "",
             method->name ? method->name : "");
    
    key_ptr = key;
    if (iron_hashmap_get(&ctx->internal_calls, &key_ptr, &fn)) {
        return fn;
    }
    
    return NULL;
}

/* ============================================================================
 * IL Interpreter - Single Instruction Execution
 * ============================================================================ */

iron_interp_result_t iron_exec_instruction(iron_thread_context_t *thread)
{
    iron_stack_frame_t *frame;
    iron_eval_stack_t *stack;
    const iron_u8 *code;
    iron_u32 ip;
    iron_opcode_t opcode;
    iron_u32 opcode_size;
    iron_i32 i32_val;
    iron_i64 i64_val;
    iron_stack_value_t val1, val2;
    
    if (!thread || !thread->current_frame) {
        return IRON_INTERP_ERROR;
    }
    
    frame = thread->current_frame;
    stack = &thread->eval_stack;
    code = frame->code;
    ip = frame->ip;
    
    if (ip >= frame->code_size) {
        return IRON_INTERP_ERROR;
    }
    
    /* Decode opcode */
    opcode = iron_opcode_decode(code + ip, &opcode_size);
    ip += opcode_size;
    
    switch (opcode) {
        case IRON_CEE_NOP:
            break;
            
        case IRON_CEE_BREAK:
            frame->ip = ip;
            return IRON_INTERP_BREAK;
            
        /* Load argument instructions */
        case IRON_CEE_LDARG_0:
            iron_stack_push(stack, frame->args[0]);
            break;
        case IRON_CEE_LDARG_1:
            iron_stack_push(stack, frame->args[1]);
            break;
        case IRON_CEE_LDARG_2:
            iron_stack_push(stack, frame->args[2]);
            break;
        case IRON_CEE_LDARG_3:
            iron_stack_push(stack, frame->args[3]);
            break;
        case IRON_CEE_LDARG_S:
            iron_stack_push(stack, frame->args[code[ip++]]);
            break;
        case IRON_CEE_LDARG:
            iron_stack_push(stack, frame->args[iron_read_u16_le(code + ip)]);
            ip += 2;
            break;
            
        /* Load local instructions */
        case IRON_CEE_LDLOC_0:
            iron_stack_push(stack, frame->locals[0]);
            break;
        case IRON_CEE_LDLOC_1:
            iron_stack_push(stack, frame->locals[1]);
            break;
        case IRON_CEE_LDLOC_2:
            iron_stack_push(stack, frame->locals[2]);
            break;
        case IRON_CEE_LDLOC_3:
            iron_stack_push(stack, frame->locals[3]);
            break;
        case IRON_CEE_LDLOC_S:
            iron_stack_push(stack, frame->locals[code[ip++]]);
            break;
        case IRON_CEE_LDLOC:
            iron_stack_push(stack, frame->locals[iron_read_u16_le(code + ip)]);
            ip += 2;
            break;
            
        /* Store local instructions */
        case IRON_CEE_STLOC_0:
            frame->locals[0] = iron_stack_pop(stack);
            break;
        case IRON_CEE_STLOC_1:
            frame->locals[1] = iron_stack_pop(stack);
            break;
        case IRON_CEE_STLOC_2:
            frame->locals[2] = iron_stack_pop(stack);
            break;
        case IRON_CEE_STLOC_3:
            frame->locals[3] = iron_stack_pop(stack);
            break;
        case IRON_CEE_STLOC_S:
            frame->locals[code[ip++]] = iron_stack_pop(stack);
            break;
        case IRON_CEE_STLOC:
            frame->locals[iron_read_u16_le(code + ip)] = iron_stack_pop(stack);
            ip += 2;
            break;
            
        /* Load constant instructions */
        case IRON_CEE_LDNULL:
            iron_stack_push_null(stack);
            break;
        case IRON_CEE_LDC_I4_M1:
            iron_stack_push_i32(stack, -1);
            break;
        case IRON_CEE_LDC_I4_0:
            iron_stack_push_i32(stack, 0);
            break;
        case IRON_CEE_LDC_I4_1:
            iron_stack_push_i32(stack, 1);
            break;
        case IRON_CEE_LDC_I4_2:
            iron_stack_push_i32(stack, 2);
            break;
        case IRON_CEE_LDC_I4_3:
            iron_stack_push_i32(stack, 3);
            break;
        case IRON_CEE_LDC_I4_4:
            iron_stack_push_i32(stack, 4);
            break;
        case IRON_CEE_LDC_I4_5:
            iron_stack_push_i32(stack, 5);
            break;
        case IRON_CEE_LDC_I4_6:
            iron_stack_push_i32(stack, 6);
            break;
        case IRON_CEE_LDC_I4_7:
            iron_stack_push_i32(stack, 7);
            break;
        case IRON_CEE_LDC_I4_8:
            iron_stack_push_i32(stack, 8);
            break;
        case IRON_CEE_LDC_I4_S:
            iron_stack_push_i32(stack, (iron_i8)code[ip++]);
            break;
        case IRON_CEE_LDC_I4:
            iron_stack_push_i32(stack, iron_read_i32_le(code + ip));
            ip += 4;
            break;
        case IRON_CEE_LDC_I8:
            iron_stack_push_i64(stack, iron_read_i64_le(code + ip));
            ip += 8;
            break;
        case IRON_CEE_LDC_R4:
            iron_stack_push_f32(stack, iron_read_f32_le(code + ip));
            ip += 4;
            break;
        case IRON_CEE_LDC_R8:
            iron_stack_push_f64(stack, iron_read_f64_le(code + ip));
            ip += 8;
            break;
            
        /* Stack manipulation */
        case IRON_CEE_DUP:
            iron_stack_dup(stack);
            break;
        case IRON_CEE_POP:
            iron_stack_pop(stack);
            break;
            
        /* String loading */
        case IRON_CEE_LDSTR:
            {
                iron_u32 str_token = iron_read_u32_le(code + ip);
                iron_exec_context_t *exec_ctx = thread->exec_ctx;
                iron_assembly_t *asm_ = NULL;
                const iron_u16 *str_data;
                iron_u32 str_len;
                iron_u32 str_index;
                iron_result_t res;
                
                ip += 4;
                str_index = str_token & 0x00FFFFFF;
                
                /* Get assembly context */
                if (frame->method && frame->method->declaring_type && 
                    frame->method->declaring_type->module &&
                    frame->method->declaring_type->module->assembly) {
                    asm_ = frame->method->declaring_type->module->assembly;
                } else if (exec_ctx && exec_ctx->domain && exec_ctx->domain->assembly_count > 0) {
                    asm_ = exec_ctx->domain->assemblies[exec_ctx->domain->assembly_count - 1];
                }
                
                if (asm_) {
                    res = iron_metadata_get_user_string(&asm_->metadata, str_index, &str_data, &str_len);
                    if (IRON_RESULT_OK(res) && str_len > 0) {
                        /* Allocate null-terminated copy of the string */
                        iron_u16 *str_copy = (iron_u16 *)iron_alloc(exec_ctx->allocator, 
                                                                     (str_len + 1) * sizeof(iron_u16));
                        if (str_copy) {
                            iron_u32 i;
                            for (i = 0; i < str_len; i++) {
                                str_copy[i] = str_data[i];
                            }
                            str_copy[str_len] = 0;
                            iron_stack_push_ptr(stack, str_copy);
                        } else {
                            iron_stack_push_null(stack);
                        }
                    } else {
                        iron_stack_push_null(stack);
                    }
                } else {
                    iron_stack_push_null(stack);
                }
            }
            break;
            
        /* Method calls */
        case IRON_CEE_CALL:
        case IRON_CEE_CALLVIRT:
            {
                iron_u32 method_token = iron_read_u32_le(code + ip);
                iron_runtime_method_t *target_method = NULL;
                iron_assembly_t *target_assembly = NULL;
                iron_result_t call_res;
                
                ip += 4;
                frame->ip = ip; /* Save IP before call */
                
                call_res = resolve_method_token(thread, method_token, 
                                                &target_method, &target_assembly);
                if (IRON_RESULT_OK(call_res) && target_method) {
                    call_res = call_method(thread, target_method, target_assembly, IRON_FALSE);
                    if (!IRON_RESULT_OK(call_res)) {
                        fprintf(stderr, "[ERROR] Method call failed: %s\n", 
                                call_res.message ? call_res.message : "Unknown error");
                        return IRON_INTERP_ERROR;
                    }
                } else {
                    /* Method not resolved - log error but continue */
                    fprintf(stderr, "[WARN] Unresolved method token 0x%08lX\n", 
                            (unsigned long)method_token);
                }
            }
            break;
            
        /* Object creation */
        case IRON_CEE_NEWOBJ:
            {
                iron_u32 ctor_token = iron_read_u32_le(code + ip);
                iron_runtime_method_t *ctor_method = NULL;
                iron_assembly_t *ctor_assembly = NULL;
                iron_result_t newobj_res;
                
                ip += 4;
                frame->ip = ip;
                
                newobj_res = resolve_method_token(thread, ctor_token,
                                                  &ctor_method, &ctor_assembly);
                if (IRON_RESULT_OK(newobj_res) && ctor_method) {
                    newobj_res = call_method(thread, ctor_method, ctor_assembly, IRON_TRUE);
                    if (!IRON_RESULT_OK(newobj_res)) {
                        fprintf(stderr, "[ERROR] newobj failed: %s\n",
                                newobj_res.message ? newobj_res.message : "Unknown error");
                        return IRON_INTERP_ERROR;
                    }
                } else {
                    fprintf(stderr, "[ERROR] Cannot resolve constructor token 0x%08X\n", ctor_token);
                    return IRON_INTERP_ERROR;
                }
            }
            break;
            
        /* Arithmetic operations */
        case IRON_CEE_ADD:
            val2 = iron_stack_pop(stack);
            val1 = iron_stack_pop(stack);
            if (val1.type == IRON_VAL_I32 && val2.type == IRON_VAL_I32) {
                iron_stack_push_i32(stack, val1.value.i32 + val2.value.i32);
            } else if (val1.type == IRON_VAL_I64 || val2.type == IRON_VAL_I64) {
                i64_val = (val1.type == IRON_VAL_I64 ? val1.value.i64 : val1.value.i32) +
                          (val2.type == IRON_VAL_I64 ? val2.value.i64 : val2.value.i32);
                iron_stack_push_i64(stack, i64_val);
            } else if (val1.type == IRON_VAL_F64 || val2.type == IRON_VAL_F64) {
                iron_stack_push_f64(stack, 
                    (val1.type == IRON_VAL_F64 ? val1.value.f64 : val1.value.f32) +
                    (val2.type == IRON_VAL_F64 ? val2.value.f64 : val2.value.f32));
            } else {
                iron_stack_push_f32(stack, val1.value.f32 + val2.value.f32);
            }
            break;
            
        case IRON_CEE_SUB:
            val2 = iron_stack_pop(stack);
            val1 = iron_stack_pop(stack);
            if (val1.type == IRON_VAL_I32 && val2.type == IRON_VAL_I32) {
                iron_stack_push_i32(stack, val1.value.i32 - val2.value.i32);
            } else if (val1.type == IRON_VAL_I64 || val2.type == IRON_VAL_I64) {
                i64_val = (val1.type == IRON_VAL_I64 ? val1.value.i64 : val1.value.i32) -
                          (val2.type == IRON_VAL_I64 ? val2.value.i64 : val2.value.i32);
                iron_stack_push_i64(stack, i64_val);
            } else {
                iron_stack_push_f64(stack, val1.value.f64 - val2.value.f64);
            }
            break;
            
        case IRON_CEE_MUL:
            val2 = iron_stack_pop(stack);
            val1 = iron_stack_pop(stack);
            if (val1.type == IRON_VAL_I32 && val2.type == IRON_VAL_I32) {
                iron_stack_push_i32(stack, val1.value.i32 * val2.value.i32);
            } else if (val1.type == IRON_VAL_I64 || val2.type == IRON_VAL_I64) {
                i64_val = (val1.type == IRON_VAL_I64 ? val1.value.i64 : val1.value.i32) *
                          (val2.type == IRON_VAL_I64 ? val2.value.i64 : val2.value.i32);
                iron_stack_push_i64(stack, i64_val);
            } else {
                iron_stack_push_f64(stack, val1.value.f64 * val2.value.f64);
            }
            break;
            
        case IRON_CEE_DIV:
            val2 = iron_stack_pop(stack);
            val1 = iron_stack_pop(stack);
            if (val1.type == IRON_VAL_I32 && val2.type == IRON_VAL_I32) {
                if (val2.value.i32 == 0) {
                    iron_throw_divide_by_zero(thread);
                    frame->ip = ip;
                    return IRON_INTERP_EXCEPTION;
                }
                iron_stack_push_i32(stack, val1.value.i32 / val2.value.i32);
            } else {
                iron_stack_push_f64(stack, val1.value.f64 / val2.value.f64);
            }
            break;
            
        case IRON_CEE_REM:
            val2 = iron_stack_pop(stack);
            val1 = iron_stack_pop(stack);
            if (val1.type == IRON_VAL_I32 && val2.type == IRON_VAL_I32) {
                if (val2.value.i32 == 0) {
                    iron_throw_divide_by_zero(thread);
                    frame->ip = ip;
                    return IRON_INTERP_EXCEPTION;
                }
                iron_stack_push_i32(stack, val1.value.i32 % val2.value.i32);
            }
            break;
            
        /* Bitwise operations */
        case IRON_CEE_AND:
            val2 = iron_stack_pop(stack);
            val1 = iron_stack_pop(stack);
            iron_stack_push_i32(stack, val1.value.i32 & val2.value.i32);
            break;
            
        case IRON_CEE_OR:
            val2 = iron_stack_pop(stack);
            val1 = iron_stack_pop(stack);
            iron_stack_push_i32(stack, val1.value.i32 | val2.value.i32);
            break;
            
        case IRON_CEE_XOR:
            val2 = iron_stack_pop(stack);
            val1 = iron_stack_pop(stack);
            iron_stack_push_i32(stack, val1.value.i32 ^ val2.value.i32);
            break;
            
        case IRON_CEE_SHL:
            val2 = iron_stack_pop(stack);
            val1 = iron_stack_pop(stack);
            iron_stack_push_i32(stack, val1.value.i32 << (val2.value.i32 & 0x1F));
            break;
            
        case IRON_CEE_SHR:
            val2 = iron_stack_pop(stack);
            val1 = iron_stack_pop(stack);
            iron_stack_push_i32(stack, val1.value.i32 >> (val2.value.i32 & 0x1F));
            break;
            
        case IRON_CEE_NEG:
            val1 = iron_stack_pop(stack);
            if (val1.type == IRON_VAL_I32) {
                iron_stack_push_i32(stack, -val1.value.i32);
            } else if (val1.type == IRON_VAL_I64) {
                iron_stack_push_i64(stack, -val1.value.i64);
            } else {
                iron_stack_push_f64(stack, -val1.value.f64);
            }
            break;
            
        case IRON_CEE_NOT:
            val1 = iron_stack_pop(stack);
            iron_stack_push_i32(stack, ~val1.value.i32);
            break;
            
        /* Comparison operations */
        case IRON_CEE_CEQ:
            val2 = iron_stack_pop(stack);
            val1 = iron_stack_pop(stack);
            iron_stack_push_i32(stack, (val1.value.i64 == val2.value.i64) ? 1 : 0);
            break;
            
        case IRON_CEE_CGT:
            val2 = iron_stack_pop(stack);
            val1 = iron_stack_pop(stack);
            if (val1.type == IRON_VAL_I32) {
                iron_stack_push_i32(stack, (val1.value.i32 > val2.value.i32) ? 1 : 0);
            } else {
                iron_stack_push_i32(stack, (val1.value.i64 > val2.value.i64) ? 1 : 0);
            }
            break;
            
        case IRON_CEE_CLT:
            val2 = iron_stack_pop(stack);
            val1 = iron_stack_pop(stack);
            if (val1.type == IRON_VAL_I32) {
                iron_stack_push_i32(stack, (val1.value.i32 < val2.value.i32) ? 1 : 0);
            } else {
                iron_stack_push_i32(stack, (val1.value.i64 < val2.value.i64) ? 1 : 0);
            }
            break;
            
        /* Branch instructions */
        case IRON_CEE_BR:
            i32_val = iron_read_i32_le(code + ip);
            ip += 4 + i32_val;
            break;
            
        case IRON_CEE_BR_S:
            i32_val = (iron_i8)code[ip++];
            ip += i32_val;
            break;
            
        case IRON_CEE_BRFALSE:
        case IRON_CEE_BRTRUE:
            i32_val = iron_read_i32_le(code + ip);
            ip += 4;
            val1 = iron_stack_pop(stack);
            if ((opcode == IRON_CEE_BRFALSE && val1.value.i64 == 0) ||
                (opcode == IRON_CEE_BRTRUE && val1.value.i64 != 0)) {
                ip += i32_val;
            }
            break;
            
        case IRON_CEE_BRFALSE_S:
        case IRON_CEE_BRTRUE_S:
            i32_val = (iron_i8)code[ip++];
            val1 = iron_stack_pop(stack);
            if ((opcode == IRON_CEE_BRFALSE_S && val1.value.i64 == 0) ||
                (opcode == IRON_CEE_BRTRUE_S && val1.value.i64 != 0)) {
                ip += i32_val;
            }
            break;
            
        case IRON_CEE_BEQ:
        case IRON_CEE_BEQ_S:
            if (opcode == IRON_CEE_BEQ) {
                i32_val = iron_read_i32_le(code + ip);
                ip += 4;
            } else {
                i32_val = (iron_i8)code[ip++];
            }
            val2 = iron_stack_pop(stack);
            val1 = iron_stack_pop(stack);
            if (val1.value.i64 == val2.value.i64) {
                ip += i32_val;
            }
            break;
            
        /* Return */
        case IRON_CEE_RET:
            frame->ip = ip;
            return IRON_INTERP_RETURN;
            
        /* Conversion instructions */
        case IRON_CEE_CONV_I1:
            val1 = iron_stack_pop(stack);
            iron_stack_push_i32(stack, (iron_i8)val1.value.i32);
            break;
        case IRON_CEE_CONV_I2:
            val1 = iron_stack_pop(stack);
            iron_stack_push_i32(stack, (iron_i16)val1.value.i32);
            break;
        case IRON_CEE_CONV_I4:
            val1 = iron_stack_pop(stack);
            iron_stack_push_i32(stack, (iron_i32)val1.value.i64);
            break;
        case IRON_CEE_CONV_I8:
            val1 = iron_stack_pop(stack);
            iron_stack_push_i64(stack, (iron_i64)val1.value.i32);
            break;
        case IRON_CEE_CONV_U1:
            val1 = iron_stack_pop(stack);
            iron_stack_push_i32(stack, (iron_u8)val1.value.i32);
            break;
        case IRON_CEE_CONV_U2:
            val1 = iron_stack_pop(stack);
            iron_stack_push_i32(stack, (iron_u16)val1.value.i32);
            break;
            
        default:
            /* Unimplemented opcode */
            fprintf(stderr, "[DEBUG] Unimplemented opcode: 0x%04X at IP=%u\n", opcode, frame->ip);
            frame->ip = ip;
            return IRON_INTERP_ERROR;
    }
    
    frame->ip = ip;
    return IRON_INTERP_OK;
}

/* ============================================================================
 * Exception Throwing
 * ============================================================================ */

void iron_throw(iron_thread_context_t *thread, iron_exception_t *ex)
{
    if (!thread) return;
    thread->exception_state.current_exception = ex;
    thread->exception_state.throw_frame = thread->current_frame;
    thread->exception_state.throw_ip = thread->current_frame ? 
                                        thread->current_frame->ip : 0;
    thread->exception_state.is_rethrow = IRON_FALSE;
}

void iron_throw_null_reference(iron_thread_context_t *thread)
{
    /* TODO: Create proper NullReferenceException object */
    iron_throw(thread, NULL);
}

void iron_throw_index_out_of_range(iron_thread_context_t *thread)
{
    iron_throw(thread, NULL);
}

void iron_throw_invalid_cast(iron_thread_context_t *thread)
{
    iron_throw(thread, NULL);
}

void iron_throw_overflow(iron_thread_context_t *thread)
{
    iron_throw(thread, NULL);
}

void iron_throw_divide_by_zero(iron_thread_context_t *thread)
{
    iron_throw(thread, NULL);
}

/* ============================================================================
 * Entry Point Execution
 * ============================================================================ */

iron_result_t iron_exec_entry_point(iron_exec_context_t *ctx,
                                    iron_runtime_method_t *entry_point,
                                    const char **args,
                                    int argc,
                                    int *exit_code)
{
    iron_stack_value_t method_args[2];
    iron_stack_value_t ret_val;
    iron_result_t result;
    
    if (!ctx || !entry_point) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Invalid context or entry point");
    }
    
    if (exit_code) {
        *exit_code = 0;
    }
    
    memset(&method_args[0], 0, sizeof(method_args));
    memset(&ret_val, 0, sizeof(ret_val));
    
    /* Check if Main takes string[] args */
    if (entry_point->param_count > 0) {
        /* TODO: Create string[] array from args */
        /* For now, pass null */
        method_args[0].type = IRON_VAL_OBJ;
        method_args[0].value.obj = NULL;
        (void)args;
        (void)argc;
        
        result = iron_exec_method(ctx, entry_point, method_args, 1, &ret_val);
    } else {
        /* Main() with no args */
        result = iron_exec_method(ctx, entry_point, NULL, 0, &ret_val);
    }
    
    if (IRON_RESULT_OK(result)) {
        /* Check if Main returns int */
        if (ret_val.type == IRON_VAL_I32 && exit_code) {
            *exit_code = ret_val.value.i32;
        }
    }
    
    return result;
}

/* ============================================================================
 * Method Resolution and Calling Helpers
 * ============================================================================ */

/* Get parameter count from method signature blob */
static iron_u32 get_method_param_count_from_sig(const iron_u8 *sig_data, iron_u32 sig_size)
{
    iron_u32 param_count = 0;
    
    if (!sig_data || sig_size < 2) return 0;
    
    /* Skip calling convention byte */
    /* Read param count (compressed integer) */
    if (sig_data[1] < 0x80) {
        param_count = sig_data[1];
    } else if (sig_data[1] < 0xC0) {
        if (sig_size >= 3) {
            param_count = ((sig_data[1] & 0x3F) << 8) | sig_data[2];
        }
    }
    
    return param_count;
}

/* Resolve a method token (MethodDef or MemberRef) to a runtime method */
static iron_result_t resolve_method_token(iron_thread_context_t *thread,
                                          iron_u32 token,
                                          iron_runtime_method_t **out_method,
                                          iron_assembly_t **out_assembly)
{
    iron_exec_context_t *ctx;
    iron_assembly_t *assembly = NULL;
    iron_u32 table_id;
    iron_u32 row_index;
    
    if (!thread || !out_method) {
        return IRON_ERROR(IRON_ERR_NULL_POINTER, "Invalid parameters");
    }
    
    ctx = thread->exec_ctx;
    *out_method = NULL;
    if (out_assembly) *out_assembly = NULL;
    
    table_id = (token >> 24) & 0xFF;
    row_index = token & 0x00FFFFFF;
    
    if (row_index == 0) {
        return IRON_ERROR(IRON_ERR_INVALID_TOKEN, "Invalid method token");
    }
    
    /* Find the assembly containing this method */
    if (thread->current_frame && thread->current_frame->method &&
        thread->current_frame->method->declaring_type &&
        thread->current_frame->method->declaring_type->module) {
        assembly = thread->current_frame->method->declaring_type->module->assembly;
    }
    
    if (!assembly && ctx->domain && ctx->domain->assembly_count > 0) {
        /* Use the last loaded assembly (usually the main exe) */
        assembly = ctx->domain->assemblies[ctx->domain->assembly_count - 1];
    }
    
    if (!assembly) {
        return IRON_ERROR(IRON_ERR_NOT_FOUND, "No assembly context");
    }
    
    if (out_assembly) *out_assembly = assembly;
    
    if (table_id == IRON_TABLE_METHOD_DEF) {
        /* Direct method reference in same assembly */
        *out_method = iron_resolve_method_token(assembly, token);
        if (*out_method) {
            return (iron_result_t)IRON_SUCCESS;
        }
        return IRON_ERROR(IRON_ERR_NOT_FOUND, "Method not found");
    }
    else if (table_id == IRON_TABLE_MEMBER_REF) {
        /* Cross-assembly method reference */
        iron_member_ref_row_t member_ref;
        iron_result_t res;
        const char *method_name;
        const char *type_name = NULL;
        const char *type_namespace = NULL;
        iron_u32 class_token;
        iron_u32 class_table;
        
        res = iron_metadata_read_row(&assembly->metadata, token, &member_ref);
        if (!IRON_RESULT_OK(res)) {
            return res;
        }
        
        method_name = iron_metadata_get_string(&assembly->metadata, member_ref.name);
        
        /* Decode the class (MemberRefParent coded index) */
        /* MemberRefParent: TypeDef(0), TypeRef(1), ModuleRef(2), MethodDef(3), TypeSpec(4) */
        class_token = iron_metadata_decode_coded(&assembly->metadata, 
                                                  IRON_CODED_MEMBER_REF_PARENT,
                                                  member_ref.class_);
        class_table = (class_token >> 24) & 0xFF;
        
        if (class_table == IRON_TABLE_TYPE_REF) {
            /* Reference to type in another assembly (usually corlib) */
            iron_type_ref_row_t type_ref;
            res = iron_metadata_read_row(&assembly->metadata, class_token, &type_ref);
            if (IRON_RESULT_OK(res)) {
                type_name = iron_metadata_get_string(&assembly->metadata, type_ref.name);
                type_namespace = iron_metadata_get_string(&assembly->metadata, type_ref.namespace_);
            }
        }
        else if (class_table == IRON_TABLE_TYPE_DEF) {
            /* Reference to type in same assembly */
            iron_type_def_row_t type_def;
            res = iron_metadata_read_row(&assembly->metadata, class_token, &type_def);
            if (IRON_RESULT_OK(res)) {
                type_name = iron_metadata_get_string(&assembly->metadata, type_def.name);
                type_namespace = iron_metadata_get_string(&assembly->metadata, type_def.namespace_);
            }
        }
        
        /* Build full type name for internal call lookup */
        if (type_name && method_name) {
            char full_type_name[512];
            iron_internal_call_fn icall = NULL;
            const char *key_ptr;
            char key[512];
            iron_u32 param_count = 0;
            const iron_u8 *sig_data = NULL;
            iron_u32 sig_size = 0;
            
            if (type_namespace && type_namespace[0]) {
                snprintf(full_type_name, sizeof(full_type_name), "%s.%s", type_namespace, type_name);
            } else {
                snprintf(full_type_name, sizeof(full_type_name), "%s", type_name);
            }
            
            /* Get param count from signature blob */
            if (member_ref.signature > 0) {
                res = iron_metadata_get_blob(&assembly->metadata, member_ref.signature,
                                              &sig_data, &sig_size);
                if (IRON_RESULT_OK(res)) {
                    param_count = get_method_param_count_from_sig(sig_data, sig_size);
                }
            }
            
            /* Try to find internal call - first try with empty signature (most common) */
            snprintf(key, sizeof(key), "%s::%s()", full_type_name, method_name);
            key_ptr = iron_intern_cstr(&ctx->domain->interner, key);
            if (!iron_hashmap_get(&ctx->internal_calls, &key_ptr, &icall)) {
                /* Try with System.String for string methods */
                snprintf(key, sizeof(key), "%s::%s(System.String)", full_type_name, method_name);
                key_ptr = iron_intern_cstr(&ctx->domain->interner, key);
                if (!iron_hashmap_get(&ctx->internal_calls, &key_ptr, &icall)) {
                    /* Try with System.Int32 */
                    snprintf(key, sizeof(key), "%s::%s(System.Int32)", full_type_name, method_name);
                    key_ptr = iron_intern_cstr(&ctx->domain->interner, key);
                    iron_hashmap_get(&ctx->internal_calls, &key_ptr, &icall);
                }
            }
            
            if (icall) {
                /* Create a temporary method structure for internal call */
                static iron_runtime_method_t temp_method;
                memset(&temp_method, 0, sizeof(temp_method));
                temp_method.name = method_name;
                temp_method.token = token;
                temp_method.is_internal_call = IRON_TRUE;
                temp_method.internal_call = (iron_method_invoke_fn)icall;
                temp_method.param_count = param_count;
                
                *out_method = &temp_method;
                return (iron_result_t)IRON_SUCCESS;
            }
        }
        
        return IRON_ERROR(IRON_ERR_NOT_FOUND, "MemberRef method not resolved");
    }
    
    return IRON_ERROR(IRON_ERR_INVALID_TOKEN, "Unsupported method token type");
}

/* Call a method - handles internal calls and IL methods */
static iron_result_t call_method(iron_thread_context_t *thread,
                                 iron_runtime_method_t *method,
                                 iron_assembly_t *assembly,
                                 iron_bool is_newobj)
{
    iron_exec_context_t *ctx;
    iron_eval_stack_t *stack;
    iron_u32 param_count;
    iron_u32 i;
    iron_stack_value_t *args = NULL;
    iron_stack_value_t result;
    iron_result_t res;
    void *new_obj = NULL;
    
    if (!thread || !method) {
        return IRON_ERROR(IRON_ERR_NULL_POINTER, "Invalid parameters");
    }
    
    ctx = thread->exec_ctx;
    stack = &thread->eval_stack;
    param_count = method->param_count;
    
    (void)assembly; /* May be used later for method body loading */
    
    memset(&result, 0, sizeof(result));
    
    /* For newobj, we need to allocate the object first */
    if (is_newobj && method->declaring_type) {
        iron_gc_t *gc = (iron_gc_t *)&ctx->gc;
        new_obj = iron_gc_alloc_object(gc, method->declaring_type, 
                                        method->declaring_type->instance_size);
        if (!new_obj) {
            return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate object");
        }
    }
    
    /* Pop arguments from stack (in reverse order) */
    if (param_count > 0) {
        args = (iron_stack_value_t *)iron_alloc(ctx->allocator, 
                                                 param_count * sizeof(iron_stack_value_t));
        if (!args) {
            return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate args");
        }
        
        /* Pop args in reverse order */
        for (i = param_count; i > 0; i--) {
            if (stack->size > 0) {
                args[i - 1] = iron_stack_pop(stack);
            } else {
                memset(&args[i - 1], 0, sizeof(iron_stack_value_t));
            }
        }
    }
    
    /* For instance methods (not static), pop 'this' pointer */
    /* For newobj, 'this' is the newly allocated object */
    if (is_newobj && new_obj) {
        /* Insert 'this' as first argument */
        iron_stack_value_t *new_args = (iron_stack_value_t *)iron_alloc(
            ctx->allocator, (param_count + 1) * sizeof(iron_stack_value_t));
        if (new_args) {
            new_args[0].type = IRON_VAL_OBJ;
            new_args[0].value.obj = new_obj;
            if (args && param_count > 0) {
                memcpy(&new_args[1], args, param_count * sizeof(iron_stack_value_t));
                iron_free(ctx->allocator, args, param_count * sizeof(iron_stack_value_t));
            }
            args = new_args;
            param_count++;
        }
    }
    
    /* Check if this is an internal call */
    if (method->is_internal_call && method->internal_call) {
        iron_internal_call_fn icall = (iron_internal_call_fn)method->internal_call;
        res = icall(ctx, args, param_count, &result);
    } else if (method->is_internal_call) {
        /* Try to find internal call by name */
        iron_internal_call_fn icall = iron_lookup_internal_call(ctx, method);
        if (icall) {
            res = icall(ctx, args, param_count, &result);
        } else {
            res = IRON_ERROR(IRON_ERR_NOT_FOUND, "Internal call not registered");
        }
    } else {
        /* Execute IL method */
        res = iron_exec_method(ctx, method, args, param_count, &result);
    }
    
    /* Clean up args */
    if (args) {
        iron_free(ctx->allocator, args, 
                  (is_newobj ? param_count : method->param_count) * sizeof(iron_stack_value_t));
    }
    
    if (!IRON_RESULT_OK(res)) {
        return res;
    }
    
    /* Push result onto stack */
    if (is_newobj) {
        /* For newobj, push the new object reference */
        iron_stack_push_obj(stack, new_obj);
    } else if (result.type != 0) {
        /* Push return value if method returns something (type 0 means void/no return) */
        iron_stack_push(stack, result);
    }
    
    return (iron_result_t)IRON_SUCCESS;
}
