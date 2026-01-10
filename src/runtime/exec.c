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
#include "iron/debug.h"
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
static iron_u32 resolve_field_offset(iron_assembly_t *assembly, iron_u32 field_token);

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
    
    /* Initialize GC */
    {
        iron_result_t gc_res = iron_gc_init(&ctx->gc, ctx, NULL);
        if (!IRON_RESULT_OK(gc_res)) {
            iron_stack_destroy(&main_thread->eval_stack, alloc);
            iron_free(alloc, main_thread, sizeof(iron_thread_context_t));
            iron_hashmap_destroy(&ctx->internal_calls);
            iron_free(alloc, ctx, sizeof(iron_exec_context_t));
            return gc_res;
        }
    }
    
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
    const char *type_name;
    
    if (!ctx || !method) return NULL;
    
    /* Get type name - prefer full_name, fallback to namespace.name */
    type_name = "";
    if (method->declaring_type) {
        if (method->declaring_type->full_name) {
            type_name = method->declaring_type->full_name;
        } else if (method->declaring_type->namespace_ && method->declaring_type->name) {
            static char type_buf[256];
            snprintf(type_buf, sizeof(type_buf), "%s.%s", 
                     method->declaring_type->namespace_, method->declaring_type->name);
            type_name = type_buf;
        } else if (method->declaring_type->name) {
            type_name = method->declaring_type->name;
        }
    }
    
    /* Build key from method info - try with empty signature first */
    snprintf(key, sizeof(key), "%s::%s()", type_name, method->name ? method->name : "");
    
    key_ptr = key;
    if (iron_hashmap_get(&ctx->internal_calls, &key_ptr, &fn)) {
        return fn;
    }
    
    /* Try with signature if we have one */
    /* For now, just try some common patterns */
    
    return NULL;
}

/* ============================================================================
 * Field Offset Resolution
 * ============================================================================ */

/*
 * Resolve field offset from field token.
 * 
 * For Field tokens (0x04xxxxxx), we need to find which TypeDef owns this field
 * and calculate the offset based on the field's position within that type.
 * 
 * Layout: Each field is 8 bytes (64-bit aligned for simplicity).
 * The offset is calculated as: (field_index_within_type) * 8
 */
static iron_u32 resolve_field_offset(iron_assembly_t *assembly, iron_u32 field_token)
{
    iron_u32 table_id = (field_token >> 24) & 0xFF;
    iron_u32 field_row = field_token & 0x00FFFFFF;
    
    if (!assembly || field_row == 0) {
        return 0;
    }
    
    if (table_id == IRON_TABLE_MEMBER_REF) {
        /* MemberRef for field - resolve to actual field token */
        iron_member_ref_row_t member_ref;
        iron_result_t res;
        const char *field_name;
        iron_u32 class_token;
        iron_u32 class_table;
        
        res = iron_metadata_read_row(&assembly->metadata, field_token, &member_ref);
        if (!IRON_RESULT_OK(res)) {
            return 0;
        }
        
        field_name = iron_metadata_get_string(&assembly->metadata, member_ref.name);
        
        /* Decode the class (MemberRefParent coded index) */
        class_token = iron_metadata_decode_coded(&assembly->metadata, 
                                                  IRON_CODED_MEMBER_REF_PARENT,
                                                  member_ref.class_);
        class_table = (class_token >> 24) & 0xFF;
        
        if (class_table == IRON_TABLE_TYPE_DEF) {
            /* Find field by name in the type */
            iron_type_def_row_t type_def;
            res = iron_metadata_read_row(&assembly->metadata, class_token, &type_def);
            if (IRON_RESULT_OK(res)) {
                iron_u32 field_start = type_def.field_list;
                iron_u32 field_end;
                iron_u32 type_count = assembly->metadata.tables[IRON_TABLE_TYPE_DEF].row_count;
                iron_u32 type_row = (class_token & 0x00FFFFFF);
                iron_u32 f;
                
                if (type_row < type_count) {
                    iron_type_def_row_t next_type;
                    iron_u32 next_token = (IRON_TABLE_TYPE_DEF << 24) | (type_row + 1);
                    if (IRON_RESULT_OK(iron_metadata_read_row(&assembly->metadata, next_token, &next_type))) {
                        field_end = next_type.field_list;
                    } else {
                        field_end = assembly->metadata.tables[IRON_TABLE_FIELD].row_count + 1;
                    }
                } else {
                    field_end = assembly->metadata.tables[IRON_TABLE_FIELD].row_count + 1;
                }
                
                /* Find field by name and return its offset */
                for (f = field_start; f < field_end; f++) {
                    iron_u32 f_token = (IRON_TABLE_FIELD << 24) | f;
                    iron_field_row_t f_row;
                    if (IRON_RESULT_OK(iron_metadata_read_row(&assembly->metadata, f_token, &f_row))) {
                        const char *f_name = iron_metadata_get_string(&assembly->metadata, f_row.name);
                        if (f_name && field_name && strcmp(f_name, field_name) == 0) {
                            /* Found the field - return absolute offset */
                            return (f - 1) * sizeof(void*);
                        }
                    }
                }
            }
        }
        else if (class_table == IRON_TABLE_TYPE_SPEC) {
            /* Generic type - resolve through TypeSpec */
            iron_type_spec_row_t type_spec;
            res = iron_metadata_read_row(&assembly->metadata, class_token, &type_spec);
            if (IRON_RESULT_OK(res)) {
                const iron_u8 *sig_data;
                iron_u32 sig_size;
                
                if (IRON_RESULT_OK(iron_metadata_get_blob(&assembly->metadata, type_spec.signature, &sig_data, &sig_size))) {
                    if (sig_size >= 3 && sig_data[0] == 0x15) { /* GENERICINST */
                        iron_u32 type_token_coded;
                        iron_u32 idx = 2;
                        iron_u32 generic_type_token;
                        
                        type_token_coded = sig_data[idx++];
                        if (type_token_coded & 0x80) {
                            type_token_coded = ((type_token_coded & 0x3F) << 8) | sig_data[idx++];
                        }
                        
                        generic_type_token = iron_metadata_decode_coded(&assembly->metadata,
                                                                        IRON_CODED_TYPE_DEF_OR_REF,
                                                                        type_token_coded);
                        
                        if (((generic_type_token >> 24) & 0xFF) == IRON_TABLE_TYPE_DEF) {
                            /* Recursively resolve using the generic type definition */
                            iron_type_def_row_t gen_type_def;
                            if (IRON_RESULT_OK(iron_metadata_read_row(&assembly->metadata, generic_type_token, &gen_type_def))) {
                                iron_u32 field_start = gen_type_def.field_list;
                                iron_u32 field_end;
                                iron_u32 type_count = assembly->metadata.tables[IRON_TABLE_TYPE_DEF].row_count;
                                iron_u32 type_row = (generic_type_token & 0x00FFFFFF);
                                iron_u32 f;
                                
                                if (type_row < type_count) {
                                    iron_type_def_row_t next_type;
                                    iron_u32 next_token = (IRON_TABLE_TYPE_DEF << 24) | (type_row + 1);
                                    if (IRON_RESULT_OK(iron_metadata_read_row(&assembly->metadata, next_token, &next_type))) {
                                        field_end = next_type.field_list;
                                    } else {
                                        field_end = assembly->metadata.tables[IRON_TABLE_FIELD].row_count + 1;
                                    }
                                } else {
                                    field_end = assembly->metadata.tables[IRON_TABLE_FIELD].row_count + 1;
                                }
                                
                                for (f = field_start; f < field_end; f++) {
                                    iron_u32 f_token = (IRON_TABLE_FIELD << 24) | f;
                                    iron_field_row_t f_row;
                                    if (IRON_RESULT_OK(iron_metadata_read_row(&assembly->metadata, f_token, &f_row))) {
                                        const char *f_name = iron_metadata_get_string(&assembly->metadata, f_row.name);
                                        if (f_name && field_name && strcmp(f_name, field_name) == 0) {
                                            return (f - 1) * sizeof(void*);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        
        return 0;
    }
    
    if (table_id != IRON_TABLE_FIELD) {
        /* Not a field token - return 0 */
        return 0;
    }
    
    /* Use absolute field index for offset calculation.
     * This works correctly for inheritance because:
     * - Base class fields come first in the Field table
     * - Derived class fields come after
     * - When accessing a base class field from a derived object,
     *   the field token still refers to the base class field row,
     *   which has a lower index than derived class fields.
     * 
     * Layout: each slot is 8 bytes (pointer-aligned).
     * Field row is 1-based, so subtract 1 to get 0-based index.
     */
    return (field_row - 1) * sizeof(void*);
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
    const iron_opcode_info_t *opcode_info;
    
    if (!thread || !thread->current_frame) {
        IRON_ERROR_EXEC("No thread or current frame");
        return IRON_INTERP_ERROR;
    }
    
    frame = thread->current_frame;
    stack = &thread->eval_stack;
    code = frame->code;
    ip = frame->ip;
    
    if (ip >= frame->code_size) {
        IRON_ERROR_EXEC("IP %u out of bounds (code_size=%u)", ip, frame->code_size);
        return IRON_INTERP_ERROR;
    }
    
    /* Decode opcode */
    opcode = iron_opcode_decode(code + ip, &opcode_size);
    ip += opcode_size;
    
    /* Trace opcode execution */
    opcode_info = iron_opcode_info(opcode);
    IRON_TRACE_EXEC("IP=%04X opcode=0x%04X %-12s stack=%u locals=%u args=%u", 
                    frame->ip, opcode, 
                    opcode_info ? opcode_info->name : "???",
                    stack->size,
                    frame->local_count,
                    frame->arg_count);
    
    /* Debug: show stack top values */
    if (stack->size > 0) {
        IRON_TRACE_EXEC("  Stack[0]: type=%d val=0x%llX", 
                        stack->data[stack->size-1].type,
                        (unsigned long long)stack->data[stack->size-1].value.i64);
    }
    if (stack->size > 1) {
        IRON_TRACE_EXEC("  Stack[1]: type=%d val=0x%llX", 
                        stack->data[stack->size-2].type,
                        (unsigned long long)stack->data[stack->size-2].value.i64);
    }
    
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
        case IRON_CEE_LDARGA_S:
            /* Load address of argument (short form) */
            {
                iron_u8 arg_idx = code[ip++];
                iron_stack_value_t val;
                val.type = IRON_VAL_PTR;
                val.value.ptr = &frame->args[arg_idx];
                iron_stack_push(stack, val);
            }
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
        case IRON_CEE_LDLOCA_S:
            /* Load address of local variable (short form) */
            iron_stack_push_ptr(stack, &frame->locals[code[ip++]]);
            break;
        case IRON_CEE_LDLOCA:
            /* Load address of local variable */
            iron_stack_push_ptr(stack, &frame->locals[iron_read_u16_le(code + ip)]);
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
                        /* Allocate string object: length (u32) + chars (u16[]) */
                        void *str_obj = iron_alloc(exec_ctx->allocator, 
                                                   sizeof(iron_u32) + str_len * sizeof(iron_u16));
                        if (str_obj) {
                            iron_u32 i;
                            iron_u16 *chars;
                            /* Store length first */
                            *((iron_u32 *)str_obj) = str_len;
                            /* Copy characters after length */
                            chars = (iron_u16 *)((iron_u8 *)str_obj + sizeof(iron_u32));
                            for (i = 0; i < str_len; i++) {
                                chars[i] = str_data[i];
                            }
                            iron_stack_push_ptr(stack, str_obj);
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
            
        case IRON_CEE_CONSTRAINED:
            /* Prefix for callvirt on value types - skip the type token and continue */
            /* The next instruction should be callvirt */
            ip += 4; /* Skip the type token */
            break;
            
        case IRON_CEE_READONLY:
            /* Prefix for ldelema - indicates the result won't be used to modify the array */
            /* Just skip it, the next instruction will handle the actual operation */
            break;
            
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
                    /* For virtual/interface calls, resolve the actual method based on object's runtime type */
                    /* Get 'this' from stack without popping (it's under the args) */
                    iron_u32 arg_count = target_method->param_count;
                    if (stack->size > arg_count) {
                        iron_stack_value_t this_val = stack->data[stack->size - arg_count - 1];
                        void *this_obj = (this_val.type == IRON_VAL_OBJ) ? 
                                         this_val.value.obj : this_val.value.ptr;
                        if (this_obj) {
                            /* Get runtime type from GC header */
                            iron_gc_header_t *header = ((iron_gc_header_t *)this_obj) - 1;
                            iron_runtime_type_t *runtime_type = header->type;
                            
                                    /* Always try to find method in runtime type for callvirt */
                            /* This handles both virtual method override and interface implementation */
                            if (runtime_type && target_method->name) {
                                iron_runtime_method_t *impl = 
                                    iron_type_find_method(runtime_type, target_method->name);
                                if (impl && impl->body) {
                                    target_method = impl;
                                    /* Update assembly to the one containing the implementation */
                                    if (impl->declaring_type && impl->declaring_type->module &&
                                        impl->declaring_type->module->assembly) {
                                        target_assembly = impl->declaring_type->module->assembly;
                                    }
                                } else if (!impl || !impl->body) {
                                    /* Method not found in type - search in assembly by method name */
                                    /* This handles interface method calls where the implementation is in a base class */
                                    if (runtime_type->module && runtime_type->module->assembly) {
                                        iron_assembly_t *obj_asm = runtime_type->module->assembly;
                                        iron_u32 m;
                                        for (m = 1; m <= obj_asm->metadata.tables[IRON_TABLE_METHOD_DEF].row_count; m++) {
                                            iron_u32 m_token = (IRON_TABLE_METHOD_DEF << 24) | m;
                                            iron_runtime_method_t *candidate = iron_resolve_method_token(obj_asm, m_token);
                                            if (candidate && candidate->name && candidate->body &&
                                                strcmp(candidate->name, target_method->name) == 0) {
                                                /* Found a method with matching name and body */
                                                target_method = candidate;
                                                target_assembly = obj_asm;
                                                break;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    
                    /* If method still has no body, it's an interface/abstract method without implementation */
                    if (!target_method->body && !target_method->is_internal_call) {
                        fprintf(stderr, "[ERROR] Method call failed: Method has no IL body\n");
                        return IRON_INTERP_ERROR;
                    }
                    
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
                    IRON_TRACE_EXEC("newobj: method=%s param_count=%u stack_before=%u",
                                   ctor_method->name ? ctor_method->name : "?",
                                   ctor_method->param_count,
                                   stack->size);
                    newobj_res = call_method(thread, ctor_method, ctor_assembly, IRON_TRUE);
                    IRON_TRACE_EXEC("newobj: stack_after=%u", stack->size);
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
            
        case IRON_CEE_CGT_UN:
            val2 = iron_stack_pop(stack);
            val1 = iron_stack_pop(stack);
            /* Unsigned comparison - treat values as unsigned */
            if (val1.type == IRON_VAL_I32) {
                iron_stack_push_i32(stack, ((iron_u32)val1.value.i32 > (iron_u32)val2.value.i32) ? 1 : 0);
            } else {
                iron_stack_push_i32(stack, ((iron_u64)val1.value.i64 > (iron_u64)val2.value.i64) ? 1 : 0);
            }
            break;
            
        case IRON_CEE_CLT_UN:
            val2 = iron_stack_pop(stack);
            val1 = iron_stack_pop(stack);
            /* Unsigned comparison - treat values as unsigned */
            if (val1.type == IRON_VAL_I32) {
                iron_stack_push_i32(stack, ((iron_u32)val1.value.i32 < (iron_u32)val2.value.i32) ? 1 : 0);
            } else {
                iron_stack_push_i32(stack, ((iron_u64)val1.value.i64 < (iron_u64)val2.value.i64) ? 1 : 0);
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
            
        case IRON_CEE_BNE_UN:
        case IRON_CEE_BNE_UN_S:
            if (opcode == IRON_CEE_BNE_UN) {
                i32_val = iron_read_i32_le(code + ip);
                ip += 4;
            } else {
                i32_val = (iron_i8)code[ip++];
            }
            val2 = iron_stack_pop(stack);
            val1 = iron_stack_pop(stack);
            if ((iron_u64)val1.value.i64 != (iron_u64)val2.value.i64) {
                ip += i32_val;
            }
            break;
            
        case IRON_CEE_BGE:
        case IRON_CEE_BGE_S:
            if (opcode == IRON_CEE_BGE) {
                i32_val = iron_read_i32_le(code + ip);
                ip += 4;
            } else {
                i32_val = (iron_i8)code[ip++];
            }
            val2 = iron_stack_pop(stack);
            val1 = iron_stack_pop(stack);
            if (val1.value.i64 >= val2.value.i64) {
                ip += i32_val;
            }
            break;
            
        case IRON_CEE_BGT:
        case IRON_CEE_BGT_S:
            if (opcode == IRON_CEE_BGT) {
                i32_val = iron_read_i32_le(code + ip);
                ip += 4;
            } else {
                i32_val = (iron_i8)code[ip++];
            }
            val2 = iron_stack_pop(stack);
            val1 = iron_stack_pop(stack);
            if (val1.value.i64 > val2.value.i64) {
                ip += i32_val;
            }
            break;
            
        case IRON_CEE_BLE:
        case IRON_CEE_BLE_S:
            if (opcode == IRON_CEE_BLE) {
                i32_val = iron_read_i32_le(code + ip);
                ip += 4;
            } else {
                i32_val = (iron_i8)code[ip++];
            }
            val2 = iron_stack_pop(stack);
            val1 = iron_stack_pop(stack);
            if (val1.value.i64 <= val2.value.i64) {
                ip += i32_val;
            }
            break;
            
        case IRON_CEE_BLT:
        case IRON_CEE_BLT_S:
            if (opcode == IRON_CEE_BLT) {
                i32_val = iron_read_i32_le(code + ip);
                ip += 4;
            } else {
                i32_val = (iron_i8)code[ip++];
            }
            val2 = iron_stack_pop(stack);
            val1 = iron_stack_pop(stack);
            if (val1.value.i64 < val2.value.i64) {
                ip += i32_val;
            }
            break;
            
        case IRON_CEE_BGE_UN:
        case IRON_CEE_BGE_UN_S:
            if (opcode == IRON_CEE_BGE_UN) {
                i32_val = iron_read_i32_le(code + ip);
                ip += 4;
            } else {
                i32_val = (iron_i8)code[ip++];
            }
            val2 = iron_stack_pop(stack);
            val1 = iron_stack_pop(stack);
            if ((iron_u64)val1.value.i64 >= (iron_u64)val2.value.i64) {
                ip += i32_val;
            }
            break;
            
        case IRON_CEE_BGT_UN:
        case IRON_CEE_BGT_UN_S:
            if (opcode == IRON_CEE_BGT_UN) {
                i32_val = iron_read_i32_le(code + ip);
                ip += 4;
            } else {
                i32_val = (iron_i8)code[ip++];
            }
            val2 = iron_stack_pop(stack);
            val1 = iron_stack_pop(stack);
            if ((iron_u64)val1.value.i64 > (iron_u64)val2.value.i64) {
                ip += i32_val;
            }
            break;
            
        case IRON_CEE_BLE_UN:
        case IRON_CEE_BLE_UN_S:
            if (opcode == IRON_CEE_BLE_UN) {
                i32_val = iron_read_i32_le(code + ip);
                ip += 4;
            } else {
                i32_val = (iron_i8)code[ip++];
            }
            val2 = iron_stack_pop(stack);
            val1 = iron_stack_pop(stack);
            if ((iron_u64)val1.value.i64 <= (iron_u64)val2.value.i64) {
                ip += i32_val;
            }
            break;
            
        case IRON_CEE_BLT_UN:
        case IRON_CEE_BLT_UN_S:
            if (opcode == IRON_CEE_BLT_UN) {
                i32_val = iron_read_i32_le(code + ip);
                ip += 4;
            } else {
                i32_val = (iron_i8)code[ip++];
            }
            val2 = iron_stack_pop(stack);
            val1 = iron_stack_pop(stack);
            if ((iron_u64)val1.value.i64 < (iron_u64)val2.value.i64) {
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
            
        /* Array creation */
        case IRON_CEE_NEWARR:
            {
                iron_u32 type_token = iron_read_u32_le(code + ip);
                iron_stack_value_t len_val;
                iron_i32 length;
                iron_runtime_type_t *element_type = NULL;
                void *array_obj = NULL;
                
                ip += 4;
                
                /* Pop array length from stack */
                len_val = iron_stack_pop(stack);
                length = len_val.value.i32;
                IRON_TRACE_EXEC("newarr: type_token=0x%08X length=%d (val_type=%d)", 
                               type_token, length, len_val.type);
                
                if (length < 0) {
                    iron_throw_overflow(thread);
                    frame->ip = ip;
                    return IRON_INTERP_EXCEPTION;
                }
                
                /* Resolve element type from token */
                if (frame->method && frame->method->declaring_type &&
                    frame->method->declaring_type->module) {
                    element_type = iron_type_resolve_token(
                        frame->method->declaring_type->module, type_token);
                }
                
                if (!element_type && thread->exec_ctx->domain) {
                    /* Try to create a basic type based on token */
                    element_type = iron_domain_find_type(thread->exec_ctx->domain, "System.Object");
                }
                
                /* Allocate array using exec context's GC alloc */
                array_obj = iron_gc_alloc_array(thread->exec_ctx, element_type, (iron_u32)length);
                
                if (!array_obj) {
                    fprintf(stderr, "[ERROR] Failed to allocate array of length %d\n", length);
                    return IRON_INTERP_ERROR;
                }
                
                iron_stack_push_obj(stack, array_obj);
            }
            break;
            
        /* Array element access */
        case IRON_CEE_LDELEM_REF:
            {
                iron_i32 index = iron_stack_pop_i32(stack);
                void *arr = iron_stack_pop_obj(stack);
                iron_u32 arr_len;
                void **elements;
                
                if (!arr) {
                    iron_throw_null_reference(thread);
                    frame->ip = ip;
                    return IRON_INTERP_EXCEPTION;
                }
                
                arr_len = *((iron_u32 *)arr);
                if (index < 0 || (iron_u32)index >= arr_len) {
                    iron_throw_index_out_of_range(thread);
                    frame->ip = ip;
                    return IRON_INTERP_EXCEPTION;
                }
                
                elements = (void **)((iron_u8 *)arr + sizeof(iron_u32));
                iron_stack_push_obj(stack, elements[index]);
            }
            break;
            
        case IRON_CEE_STELEM_REF:
            {
                void *value = iron_stack_pop_obj(stack);
                iron_i32 index = iron_stack_pop_i32(stack);
                void *arr = iron_stack_pop_obj(stack);
                iron_u32 arr_len;
                void **elements;
                
                if (!arr) {
                    iron_throw_null_reference(thread);
                    frame->ip = ip;
                    return IRON_INTERP_EXCEPTION;
                }
                
                arr_len = *((iron_u32 *)arr);
                if (index < 0 || (iron_u32)index >= arr_len) {
                    iron_throw_index_out_of_range(thread);
                    frame->ip = ip;
                    return IRON_INTERP_EXCEPTION;
                }
                
                elements = (void **)((iron_u8 *)arr + sizeof(iron_u32));
                elements[index] = value;
            }
            break;
            
        case IRON_CEE_STELEM:
            /* Generic stelem - type token follows, store element based on type */
            {
                iron_u32 type_token = iron_read_u32_le(code + ip);
                iron_stack_value_t value_val = iron_stack_pop(stack);
                iron_stack_value_t index_val = iron_stack_pop(stack);
                iron_stack_value_t arr_val = iron_stack_pop(stack);
                iron_i32 index = index_val.value.i32;
                void *arr = (arr_val.type == IRON_VAL_OBJ) ? arr_val.value.obj : arr_val.value.ptr;
                iron_u32 arr_len;
                
                (void)type_token; /* Type token used for verification, not needed for basic impl */
                ip += 4;
                
                IRON_TRACE_EXEC("stelem: arr=%p index=%d value_type=%d", arr, index, value_val.type);
                
                if (!arr) {
                    iron_throw_null_reference(thread);
                    frame->ip = ip;
                    return IRON_INTERP_EXCEPTION;
                }
                
                arr_len = *((iron_u32 *)arr);
                IRON_TRACE_EXEC("stelem: arr_len=%u", arr_len);
                if (index < 0 || (iron_u32)index >= arr_len) {
                    iron_throw_index_out_of_range(thread);
                    frame->ip = ip;
                    return IRON_INTERP_EXCEPTION;
                }
                
                /* Store element - use pointer-sized slots for all elements */
                {
                    void **elements = (void **)((iron_u8 *)arr + sizeof(iron_u32));
                    if (value_val.type == IRON_VAL_OBJ) {
                        elements[index] = value_val.value.obj;
                    } else if (value_val.type == IRON_VAL_I32) {
                        /* Store i32 in pointer-sized slot */
                        elements[index] = (void*)(intptr_t)value_val.value.i32;
                    } else {
                        elements[index] = value_val.value.ptr;
                    }
                }
            }
            break;
            
        case IRON_CEE_LDELEMA:
            /* Load element address - type token follows */
            {
                iron_u32 type_token = iron_read_u32_le(code + ip);
                iron_stack_value_t index_val = iron_stack_pop(stack);
                iron_stack_value_t arr_val = iron_stack_pop(stack);
                iron_i32 index = index_val.value.i32;
                void *arr = (arr_val.type == IRON_VAL_OBJ) ? arr_val.value.obj : arr_val.value.ptr;
                iron_u32 arr_len;
                
                (void)type_token;
                ip += 4;
                
                if (!arr) {
                    iron_throw_null_reference(thread);
                    frame->ip = ip;
                    return IRON_INTERP_EXCEPTION;
                }
                
                arr_len = *((iron_u32 *)arr);
                if (index < 0 || (iron_u32)index >= arr_len) {
                    iron_throw_index_out_of_range(thread);
                    frame->ip = ip;
                    return IRON_INTERP_EXCEPTION;
                }
                
                /* Push address of element (pointer-sized elements) */
                {
                    void **elements = (void **)((iron_u8 *)arr + sizeof(iron_u32));
                    iron_stack_value_t addr_val;
                    addr_val.type = IRON_VAL_PTR;
                    addr_val.value.ptr = &elements[index];
                    iron_stack_push(stack, addr_val);
                }
            }
            break;
            
        case IRON_CEE_LDELEM:
            /* Generic ldelem - type token follows, load element based on type */
            {
                iron_u32 type_token = iron_read_u32_le(code + ip);
                iron_stack_value_t index_val = iron_stack_pop(stack);
                iron_stack_value_t arr_val = iron_stack_pop(stack);
                iron_i32 index = index_val.value.i32;
                void *arr = (arr_val.type == IRON_VAL_OBJ) ? arr_val.value.obj : arr_val.value.ptr;
                iron_u32 arr_len;
                iron_u32 type_table = (type_token >> 24) & 0xFF;
                iron_bool is_value_type = IRON_FALSE;
                
                ip += 4;
                
                if (!arr) {
                    iron_throw_null_reference(thread);
                    frame->ip = ip;
                    return IRON_INTERP_EXCEPTION;
                }
                
                arr_len = *((iron_u32 *)arr);
                if (index < 0 || (iron_u32)index >= arr_len) {
                    iron_throw_index_out_of_range(thread);
                    frame->ip = ip;
                    return IRON_INTERP_EXCEPTION;
                }
                
                /* Check if element type is a value type based on type token */
                if (type_table == IRON_TABLE_TYPE_DEF || type_table == IRON_TABLE_TYPE_REF) {
                    /* Could be value or reference type - check signature */
                    /* For now, assume reference type for CLASS tokens */
                    is_value_type = IRON_FALSE;
                } else if (type_token == IRON_TYPE_I4 || type_token == IRON_TYPE_U4 ||
                           type_token == IRON_TYPE_I2 || type_token == IRON_TYPE_U2 ||
                           type_token == IRON_TYPE_I1 || type_token == IRON_TYPE_U1 ||
                           type_token == IRON_TYPE_I8 || type_token == IRON_TYPE_U8 ||
                           type_token == IRON_TYPE_R4 || type_token == IRON_TYPE_R8 ||
                           type_token == IRON_TYPE_BOOLEAN || type_token == IRON_TYPE_CHAR) {
                    is_value_type = IRON_TRUE;
                }
                
                /* Load element - all elements stored in pointer-sized slots */
                {
                    void **elements = (void **)((iron_u8 *)arr + sizeof(iron_u32));
                    if (is_value_type) {
                        iron_stack_push_i32(stack, (iron_i32)(intptr_t)elements[index]);
                    } else {
                        iron_stack_push_obj(stack, elements[index]);
                    }
                }
            }
            break;
            
        case IRON_CEE_LDLEN:
            {
                void *arr = iron_stack_pop_obj(stack);
                
                if (!arr) {
                    iron_throw_null_reference(thread);
                    frame->ip = ip;
                    return IRON_INTERP_EXCEPTION;
                }
                
                iron_stack_push_i32(stack, (iron_i32)*((iron_u32 *)arr));
            }
            break;
        
        /* Field access */
        case IRON_CEE_LDFLD:
            {
                iron_u32 field_token = iron_read_u32_le(code + ip);
                iron_stack_value_t obj_val;
                void *obj;
                iron_u32 field_offset;
                iron_assembly_t *asm_ = NULL;
                
                ip += 4;
                
                obj_val = iron_stack_pop(stack);
                obj = (obj_val.type == IRON_VAL_PTR) ? obj_val.value.ptr : obj_val.value.obj;
                
                if (!obj) {
                    iron_throw_null_reference(thread);
                    frame->ip = ip;
                    return IRON_INTERP_EXCEPTION;
                }
                
                /* Get assembly for field resolution */
                if (frame->method && frame->method->declaring_type && 
                    frame->method->declaring_type->module &&
                    frame->method->declaring_type->module->assembly) {
                    asm_ = frame->method->declaring_type->module->assembly;
                }
                
                /* Resolve field offset using metadata */
                field_offset = resolve_field_offset(asm_, field_token);
                
                /* Read field value - need to determine type from field signature */
                {
                    iron_u8 field_type = IRON_TYPE_OBJECT; /* Default to object */
                    
                    /* Try to get field type from metadata */
                    if (asm_) {
                        iron_u32 field_table = (field_token >> 24) & 0xFF;
                        if (field_table == IRON_TABLE_FIELD) {
                            iron_field_row_t field_row;
                            if (IRON_RESULT_OK(iron_metadata_read_row(&asm_->metadata, field_token, &field_row))) {
                                const iron_u8 *sig_data;
                                iron_u32 sig_size;
                                if (IRON_RESULT_OK(iron_metadata_get_blob(&asm_->metadata, field_row.signature, &sig_data, &sig_size))) {
                                    /* Field signature: 0x06 (FIELD) followed by type */
                                    if (sig_size >= 2 && sig_data[0] == 0x06) {
                                        field_type = sig_data[1];
                                    }
                                }
                            }
                        }
                        else if (field_table == IRON_TABLE_MEMBER_REF) {
                            /* MemberRef for field - get type from signature */
                            iron_member_ref_row_t member_ref;
                            if (IRON_RESULT_OK(iron_metadata_read_row(&asm_->metadata, field_token, &member_ref))) {
                                const iron_u8 *sig_data;
                                iron_u32 sig_size;
                                if (IRON_RESULT_OK(iron_metadata_get_blob(&asm_->metadata, member_ref.signature, &sig_data, &sig_size))) {
                                    /* Field signature: 0x06 (FIELD) followed by type */
                                    if (sig_size >= 2 && sig_data[0] == 0x06) {
                                        field_type = sig_data[1];
                                    }
                                }
                            }
                        }
                        IRON_TRACE_EXEC("ldfld: token=0x%08X field_type=0x%02X offset=%u obj=%p",
                                       field_token, field_type, field_offset, obj);
                    }
                    
                    /* Read based on field type */
                    if (field_type == IRON_TYPE_I4 || field_type == IRON_TYPE_U4 || 
                        field_type == IRON_TYPE_BOOLEAN || field_type == IRON_TYPE_CHAR) {
                        iron_i32 *field_ptr = (iron_i32 *)((iron_u8 *)obj + field_offset);
                        IRON_TRACE_EXEC("ldfld: reading i32 value=%d from %p", *field_ptr, (void*)field_ptr);
                        iron_stack_push_i32(stack, *field_ptr);
                    } else if (field_type == IRON_TYPE_I8 || field_type == IRON_TYPE_U8) {
                        iron_i64 *field_ptr = (iron_i64 *)((iron_u8 *)obj + field_offset);
                        iron_stack_push_i64(stack, *field_ptr);
                    } else if (field_type == IRON_TYPE_R4) {
                        iron_f32 *field_ptr = (iron_f32 *)((iron_u8 *)obj + field_offset);
                        iron_stack_push_f32(stack, *field_ptr);
                    } else if (field_type == IRON_TYPE_R8) {
                        iron_f64 *field_ptr = (iron_f64 *)((iron_u8 *)obj + field_offset);
                        iron_stack_push_f64(stack, *field_ptr);
                    } else if (field_type == IRON_TYPE_I1 || field_type == IRON_TYPE_U1) {
                        iron_i8 *field_ptr = (iron_i8 *)((iron_u8 *)obj + field_offset);
                        iron_stack_push_i32(stack, (iron_i32)*field_ptr);
                    } else if (field_type == IRON_TYPE_I2 || field_type == IRON_TYPE_U2) {
                        iron_i16 *field_ptr = (iron_i16 *)((iron_u8 *)obj + field_offset);
                        iron_stack_push_i32(stack, (iron_i32)*field_ptr);
                    } else {
                        /* Object reference (STRING, CLASS, OBJECT, SZARRAY, etc.) */
                        void **field_ptr = (void **)((iron_u8 *)obj + field_offset);
                        iron_stack_push_obj(stack, *field_ptr);
                    }
                }
            }
            break;
            
        case IRON_CEE_LDFLDA:
            /* Load field address */
            {
                iron_u32 field_token = iron_read_u32_le(code + ip);
                iron_stack_value_t obj_val;
                void *obj;
                iron_u32 field_offset;
                iron_assembly_t *asm_ = NULL;
                
                ip += 4;
                
                obj_val = iron_stack_pop(stack);
                obj = (obj_val.type == IRON_VAL_PTR) ? obj_val.value.ptr : obj_val.value.obj;
                
                if (!obj) {
                    iron_throw_null_reference(thread);
                    frame->ip = ip;
                    return IRON_INTERP_EXCEPTION;
                }
                
                /* Get assembly for field resolution */
                if (frame->method && frame->method->declaring_type && 
                    frame->method->declaring_type->module &&
                    frame->method->declaring_type->module->assembly) {
                    asm_ = frame->method->declaring_type->module->assembly;
                }
                
                field_offset = resolve_field_offset(asm_, field_token);
                
                /* Push address of field */
                {
                    iron_stack_value_t addr_val;
                    addr_val.type = IRON_VAL_PTR;
                    addr_val.value.ptr = (iron_u8 *)obj + field_offset;
                    iron_stack_push(stack, addr_val);
                }
            }
            break;
            
        case IRON_CEE_STFLD:
            {
                iron_u32 field_token = iron_read_u32_le(code + ip);
                iron_stack_value_t value_val;
                iron_stack_value_t obj_val;
                void *obj;
                iron_u32 field_offset;
                iron_assembly_t *asm_ = NULL;
                
                ip += 4;
                
                /* Pop value first, then object */
                value_val = iron_stack_pop(stack);
                obj_val = iron_stack_pop(stack);
                obj = (obj_val.type == IRON_VAL_PTR) ? obj_val.value.ptr : obj_val.value.obj;
                
                if (!obj) {
                    iron_throw_null_reference(thread);
                    frame->ip = ip;
                    return IRON_INTERP_EXCEPTION;
                }
                
                /* Get assembly for field resolution */
                if (frame->method && frame->method->declaring_type && 
                    frame->method->declaring_type->module &&
                    frame->method->declaring_type->module->assembly) {
                    asm_ = frame->method->declaring_type->module->assembly;
                }
                
                /* Resolve field offset using metadata */
                field_offset = resolve_field_offset(asm_, field_token);
                
                /* Store field value - need to determine type from field signature */
                {
                    iron_u8 field_type = IRON_TYPE_OBJECT; /* Default to object */
                    
                    /* Try to get field type from metadata */
                    if (asm_) {
                        iron_u32 field_table = (field_token >> 24) & 0xFF;
                        if (field_table == IRON_TABLE_FIELD) {
                            iron_field_row_t field_row;
                            if (IRON_RESULT_OK(iron_metadata_read_row(&asm_->metadata, field_token, &field_row))) {
                                const iron_u8 *sig_data;
                                iron_u32 sig_size;
                                if (IRON_RESULT_OK(iron_metadata_get_blob(&asm_->metadata, field_row.signature, &sig_data, &sig_size))) {
                                    /* Field signature: 0x06 (FIELD) followed by type */
                                    if (sig_size >= 2 && sig_data[0] == 0x06) {
                                        field_type = sig_data[1];
                                    }
                                }
                            }
                        }
                    }
                    
                    IRON_TRACE_EXEC("stfld: token=0x%08X field_type=0x%02X offset=%u value=%d obj=%p",
                                   field_token, field_type, field_offset, value_val.value.i32, obj);
                    
                    /* Store based on field type */
                    if (field_type == IRON_TYPE_I4 || field_type == IRON_TYPE_U4 || 
                        field_type == IRON_TYPE_BOOLEAN || field_type == IRON_TYPE_CHAR) {
                        iron_i32 *field_ptr = (iron_i32 *)((iron_u8 *)obj + field_offset);
                        *field_ptr = value_val.value.i32;
                    } else if (field_type == IRON_TYPE_I8 || field_type == IRON_TYPE_U8) {
                        iron_i64 *field_ptr = (iron_i64 *)((iron_u8 *)obj + field_offset);
                        *field_ptr = value_val.value.i64;
                    } else if (field_type == IRON_TYPE_R4) {
                        iron_f32 *field_ptr = (iron_f32 *)((iron_u8 *)obj + field_offset);
                        *field_ptr = value_val.value.f32;
                    } else if (field_type == IRON_TYPE_R8) {
                        iron_f64 *field_ptr = (iron_f64 *)((iron_u8 *)obj + field_offset);
                        *field_ptr = value_val.value.f64;
                    } else if (field_type == IRON_TYPE_I1 || field_type == IRON_TYPE_U1) {
                        iron_i8 *field_ptr = (iron_i8 *)((iron_u8 *)obj + field_offset);
                        *field_ptr = (iron_i8)value_val.value.i32;
                    } else if (field_type == IRON_TYPE_I2 || field_type == IRON_TYPE_U2) {
                        iron_i16 *field_ptr = (iron_i16 *)((iron_u8 *)obj + field_offset);
                        *field_ptr = (iron_i16)value_val.value.i32;
                    } else {
                        /* Object reference (STRING, CLASS, OBJECT, SZARRAY, etc.) */
                        void **field_ptr = (void **)((iron_u8 *)obj + field_offset);
                        *field_ptr = value_val.value.ptr;
                    }
                }
            }
            break;
            
        default:
            /* Unimplemented opcode */
            IRON_ERROR_EXEC("Unimplemented opcode: 0x%04X (%s) at IP=%u", 
                           opcode, 
                           opcode_info ? opcode_info->name : "unknown",
                           frame->ip);
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
    iron_exception_t *ex;
    iron_exec_context_t *ctx;
    
    if (!thread || !thread->exec_ctx) {
        iron_throw(thread, NULL);
        return;
    }
    
    ctx = thread->exec_ctx;
    
    /* Create NullReferenceException object */
    ex = (iron_exception_t *)iron_alloc(ctx->allocator, sizeof(iron_exception_t));
    if (ex) {
        memset(ex, 0, sizeof(iron_exception_t));
        ex->message = "Object reference not set to an instance of an object.";
        /* Try to find NullReferenceException type */
        if (ctx->domain) {
            ex->type = iron_domain_find_type(ctx->domain, "System.NullReferenceException");
        }
    }
    
    iron_throw(thread, ex);
}

void iron_throw_index_out_of_range(iron_thread_context_t *thread)
{
    iron_exception_t *ex;
    iron_exec_context_t *ctx;
    
    if (!thread || !thread->exec_ctx) {
        iron_throw(thread, NULL);
        return;
    }
    
    ctx = thread->exec_ctx;
    ex = (iron_exception_t *)iron_alloc(ctx->allocator, sizeof(iron_exception_t));
    if (ex) {
        memset(ex, 0, sizeof(iron_exception_t));
        ex->message = "Index was outside the bounds of the array.";
        if (ctx->domain) {
            ex->type = iron_domain_find_type(ctx->domain, "System.IndexOutOfRangeException");
        }
    }
    
    iron_throw(thread, ex);
}

void iron_throw_invalid_cast(iron_thread_context_t *thread)
{
    iron_exception_t *ex;
    iron_exec_context_t *ctx;
    
    if (!thread || !thread->exec_ctx) {
        iron_throw(thread, NULL);
        return;
    }
    
    ctx = thread->exec_ctx;
    ex = (iron_exception_t *)iron_alloc(ctx->allocator, sizeof(iron_exception_t));
    if (ex) {
        memset(ex, 0, sizeof(iron_exception_t));
        ex->message = "Specified cast is not valid.";
        if (ctx->domain) {
            ex->type = iron_domain_find_type(ctx->domain, "System.InvalidCastException");
        }
    }
    
    iron_throw(thread, ex);
}

void iron_throw_overflow(iron_thread_context_t *thread)
{
    iron_exception_t *ex;
    iron_exec_context_t *ctx;
    
    if (!thread || !thread->exec_ctx) {
        iron_throw(thread, NULL);
        return;
    }
    
    ctx = thread->exec_ctx;
    ex = (iron_exception_t *)iron_alloc(ctx->allocator, sizeof(iron_exception_t));
    if (ex) {
        memset(ex, 0, sizeof(iron_exception_t));
        ex->message = "Arithmetic operation resulted in an overflow.";
        if (ctx->domain) {
            ex->type = iron_domain_find_type(ctx->domain, "System.OverflowException");
        }
    }
    
    iron_throw(thread, ex);
}

void iron_throw_divide_by_zero(iron_thread_context_t *thread)
{
    iron_exception_t *ex;
    iron_exec_context_t *ctx;
    
    if (!thread || !thread->exec_ctx) {
        iron_throw(thread, NULL);
        return;
    }
    
    ctx = thread->exec_ctx;
    ex = (iron_exception_t *)iron_alloc(ctx->allocator, sizeof(iron_exception_t));
    if (ex) {
        memset(ex, 0, sizeof(iron_exception_t));
        ex->message = "Attempted to divide by zero.";
        if (ctx->domain) {
            ex->type = iron_domain_find_type(ctx->domain, "System.DivideByZeroException");
        }
    }
    
    iron_throw(thread, ex);
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
        /* Create string[] array from args */
        void *string_array = NULL;
        
        if (args && argc > 0) {
            /* Allocate array: [length (4 bytes)] [string pointers...] */
            iron_size array_size = sizeof(iron_u32) + argc * sizeof(void*);
            string_array = iron_alloc(ctx->allocator, array_size);
            
            if (string_array) {
                int i;
                void **str_ptrs;
                
                /* Set length */
                *((iron_u32 *)string_array) = (iron_u32)argc;
                str_ptrs = (void **)((iron_u8 *)string_array + sizeof(iron_u32));
                
                /* Create string objects for each argument */
                for (i = 0; i < argc; i++) {
                    if (args[i]) {
                        /* Create managed string from UTF-8 */
                        iron_u32 len = 0;
                        iron_u32 j;
                        void *str_obj;
                        iron_u16 *chars;
                        
                        while (args[i][len]) len++;
                        
                        str_obj = iron_alloc(ctx->allocator, 
                                             sizeof(iron_u32) + len * sizeof(iron_u16));
                        if (str_obj) {
                            *((iron_u32 *)str_obj) = len;
                            chars = (iron_u16 *)((iron_u8 *)str_obj + sizeof(iron_u32));
                            for (j = 0; j < len; j++) {
                                chars[j] = (iron_u16)(unsigned char)args[i][j];
                            }
                            str_ptrs[i] = str_obj;
                        } else {
                            str_ptrs[i] = NULL;
                        }
                    } else {
                        str_ptrs[i] = NULL;
                    }
                }
            }
        } else {
            /* Create empty string array */
            string_array = iron_alloc(ctx->allocator, sizeof(iron_u32));
            if (string_array) {
                *((iron_u32 *)string_array) = 0;
            }
        }
        
        method_args[0].type = IRON_VAL_OBJ;
        method_args[0].value.obj = string_array;
        
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

/* Build signature string from method signature blob using metadata for type resolution */
static void build_signature_string_with_metadata(
    const iron_metadata_t *meta,
    const iron_u8 *sig_data, 
    iron_u32 sig_size, 
    char *out_sig, 
    iron_u32 out_size)
{
    iron_u32 param_count = 0;
    iron_u32 pos = 1; /* Skip calling convention */
    iron_u32 i;
    iron_u32 written = 0;
    
    out_sig[0] = '\0';
    
    if (!sig_data || sig_size < 2) return;
    
    /* Read param count (compressed integer) */
    if (sig_data[pos] < 0x80) {
        param_count = sig_data[pos++];
    } else if (sig_data[pos] < 0xC0 && pos + 1 < sig_size) {
        param_count = ((sig_data[pos] & 0x3F) << 8) | sig_data[pos + 1];
        pos += 2;
    } else {
        return;
    }
    
    /* Skip return type - need to handle all type encodings */
    if (pos < sig_size) {
        iron_u8 ret_type = sig_data[pos++];
        /* Handle modifiers (BYREF, PINNED, etc.) */
        while ((ret_type == 0x10 || ret_type == 0x0F || ret_type == 0x45 || ret_type == 0x1D) && pos < sig_size) {
            ret_type = sig_data[pos++];
        }
        /* Skip class/valuetype token if present */
        if ((ret_type == 0x11 || ret_type == 0x12) && pos < sig_size) {
            /* Skip compressed TypeDefOrRef token */
            if (sig_data[pos] < 0x80) pos++;
            else if (sig_data[pos] < 0xC0) pos += 2;
            else pos += 4;
        }
        /* Handle generic inst */
        if (ret_type == 0x15 && pos < sig_size) {
            /* Skip GENERICINST encoding - complex, just skip for now */
            pos = sig_size; /* Bail out */
            return;
        }
    }
    
    /* Build parameter signature */
    for (i = 0; i < param_count && pos < sig_size; i++) {
        iron_u8 param_type = sig_data[pos++];
        const char *type_name = NULL;
        char type_buf[256];
        
        /* Handle modifiers */
        while ((param_type == 0x10 || param_type == 0x0F || param_type == 0x45) && pos < sig_size) {
            param_type = sig_data[pos++];
        }
        
        /* Handle SZARRAY */
        if (param_type == 0x1D && pos < sig_size) {
            param_type = sig_data[pos++];
        }
        
        /* Resolve type from element type using ECMA-335 encoding */
        switch (param_type) {
            case 0x01: type_name = "System.Void"; break;
            case 0x02: type_name = "System.Boolean"; break;
            case 0x03: type_name = "System.Char"; break;
            case 0x04: type_name = "System.SByte"; break;
            case 0x05: type_name = "System.Byte"; break;
            case 0x06: type_name = "System.Int16"; break;
            case 0x07: type_name = "System.UInt16"; break;
            case 0x08: type_name = "System.Int32"; break;
            case 0x09: type_name = "System.UInt32"; break;
            case 0x0A: type_name = "System.Int64"; break;
            case 0x0B: type_name = "System.UInt64"; break;
            case 0x0C: type_name = "System.Single"; break;
            case 0x0D: type_name = "System.Double"; break;
            case 0x0E: type_name = "System.String"; break;
            case 0x18: type_name = "System.IntPtr"; break;
            case 0x19: type_name = "System.UIntPtr"; break;
            case 0x1C: type_name = "System.Object"; break;
            case 0x11: /* VALUETYPE */
            case 0x12: /* CLASS */
                /* Resolve from TypeDefOrRef coded token */
                if (pos < sig_size && meta) {
                    iron_u32 coded_token = 0;
                    iron_u32 table_tag, row_idx;
                    
                    /* Read compressed token */
                    if (sig_data[pos] < 0x80) {
                        coded_token = sig_data[pos++];
                    } else if (sig_data[pos] < 0xC0 && pos + 1 < sig_size) {
                        coded_token = ((sig_data[pos] & 0x3F) << 8) | sig_data[pos + 1];
                        pos += 2;
                    } else if (pos + 3 < sig_size) {
                        coded_token = ((sig_data[pos] & 0x1F) << 24) | 
                                     (sig_data[pos+1] << 16) |
                                     (sig_data[pos+2] << 8) | 
                                     sig_data[pos+3];
                        pos += 4;
                    }
                    
                    /* Decode TypeDefOrRef: 2 bits tag, rest is row */
                    table_tag = coded_token & 0x03;
                    row_idx = coded_token >> 2;
                    
                    if (table_tag == 0 && row_idx > 0) {
                        /* TypeDef */
                        iron_type_def_row_t type_def;
                        iron_token_t type_token = (IRON_TABLE_TYPE_DEF << 24) | row_idx;
                        if (IRON_RESULT_OK(iron_metadata_read_row(meta, type_token, &type_def))) {
                            const char *ns = iron_metadata_get_string(meta, type_def.namespace_);
                            const char *name = iron_metadata_get_string(meta, type_def.name);
                            if (ns && ns[0]) {
                                snprintf(type_buf, sizeof(type_buf), "%s.%s", ns, name);
                            } else {
                                snprintf(type_buf, sizeof(type_buf), "%s", name ? name : "?");
                            }
                            type_name = type_buf;
                        }
                    } else if (table_tag == 1 && row_idx > 0) {
                        /* TypeRef */
                        iron_type_ref_row_t type_ref;
                        iron_token_t type_token = (IRON_TABLE_TYPE_REF << 24) | row_idx;
                        if (IRON_RESULT_OK(iron_metadata_read_row(meta, type_token, &type_ref))) {
                            const char *ns = iron_metadata_get_string(meta, type_ref.namespace_);
                            const char *name = iron_metadata_get_string(meta, type_ref.name);
                            if (ns && ns[0]) {
                                snprintf(type_buf, sizeof(type_buf), "%s.%s", ns, name);
                            } else {
                                snprintf(type_buf, sizeof(type_buf), "%s", name ? name : "?");
                            }
                            type_name = type_buf;
                        }
                    }
                }
                if (!type_name) type_name = "System.Object";
                break;
            default:
                type_name = "System.Object";
                break;
        }
        
        if (type_name) {
            if (i > 0 && written < out_size - 1) {
                out_sig[written++] = ',';
            }
            while (*type_name && written < out_size - 1) {
                out_sig[written++] = *type_name++;
            }
        }
    }
    out_sig[written] = '\0';
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
    else if (table_id == IRON_TABLE_METHOD_SPEC) {
        /* Generic method instantiation (MethodSpec) */
        iron_method_spec_row_t method_spec;
        iron_result_t res;
        iron_u32 base_method_token;
        iron_u32 base_table;
        const iron_u8 *inst_data;
        iron_u32 inst_size;
        
        res = iron_metadata_read_row(&assembly->metadata, token, &method_spec);
        if (!IRON_RESULT_OK(res)) {
            return res;
        }
        
        /* Decode the base method (MethodDefOrRef coded index) */
        base_method_token = iron_metadata_decode_coded(&assembly->metadata,
                                                        IRON_CODED_METHOD_DEF_OR_REF,
                                                        method_spec.method);
        base_table = (base_method_token >> 24) & 0xFF;
        
        /* Parse instantiation blob to get type arguments */
        if (IRON_RESULT_OK(iron_metadata_get_blob(&assembly->metadata, method_spec.instantiation, &inst_data, &inst_size))) {
            /* MethodSpec instantiation format:
             * GENRICINST (0x0A) followed by:
             * - GenArgCount (compressed)
             * - Type arguments...
             */
            if (inst_size >= 2 && inst_data[0] == 0x0A) {
                iron_u32 gen_arg_count = inst_data[1];
                IRON_DEBUG_EXEC("MethodSpec: token=0x%08X base=0x%08X gen_args=%u", 
                               token, base_method_token, gen_arg_count);
            }
        }
        
        /* Resolve the base method */
        if (base_table == IRON_TABLE_METHOD_DEF) {
            *out_method = iron_resolve_method_token(assembly, base_method_token);
            if (*out_method) {
                return (iron_result_t)IRON_SUCCESS;
            }
        }
        else if (base_table == IRON_TABLE_MEMBER_REF) {
            /* Generic method on a type - resolve through MemberRef */
            return resolve_method_token(thread, base_method_token, out_method, out_assembly);
        }
        
        return IRON_ERROR(IRON_ERR_NOT_FOUND, "MethodSpec base method not found");
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
        else if (class_table == IRON_TABLE_TYPE_SPEC) {
            /* Reference to generic type instantiation (TypeSpec) */
            /* For generics, we resolve to the generic type definition's method */
            IRON_DEBUG_EXEC("TypeSpec: class_token=0x%08X method=%s", class_token, method_name);
            *out_method = iron_resolve_method_token(assembly, token);
            if (*out_method) {
                IRON_DEBUG_EXEC("TypeSpec resolved: method=%s", (*out_method)->name);
                return (iron_result_t)IRON_SUCCESS;
            }
            IRON_DEBUG_EXEC("TypeSpec: iron_resolve_method_token returned NULL");
            /* If iron_resolve_method_token failed, fall through to try other resolution */
        }
        
        /* Build full type name for internal call lookup */
        if (type_name && method_name) {
            char full_type_name[256];
            iron_internal_call_fn icall = NULL;
            const char *key_ptr;
            char key[600];
            iron_u32 param_count = 0;
            const iron_u8 *sig_data = NULL;
            iron_u32 sig_size = 0;
            
            if (type_namespace && type_namespace[0]) {
                snprintf(full_type_name, sizeof(full_type_name), "%s.%s", type_namespace, type_name);
            } else {
                snprintf(full_type_name, sizeof(full_type_name), "%s", type_name);
            }
            
            /* Get signature from blob and build signature string using metadata */
            if (member_ref.signature > 0) {
                res = iron_metadata_get_blob(&assembly->metadata, member_ref.signature,
                                              &sig_data, &sig_size);
                if (IRON_RESULT_OK(res)) {
                    char sig_str[512];
                    param_count = get_method_param_count_from_sig(sig_data, sig_size);
                    build_signature_string_with_metadata(&assembly->metadata, sig_data, sig_size, sig_str, sizeof(sig_str));
                    
                    /* Try with parsed signature first */
                    snprintf(key, sizeof(key), "%s::%s(%s)", full_type_name, method_name, sig_str);
                    IRON_DEBUG_EXEC("Looking up internal call: %s (sig_size=%u, bytes: %02X %02X %02X %02X)", 
                                   key, sig_size, 
                                   sig_size > 0 ? sig_data[0] : 0,
                                   sig_size > 1 ? sig_data[1] : 0,
                                   sig_size > 2 ? sig_data[2] : 0,
                                   sig_size > 3 ? sig_data[3] : 0);
                    key_ptr = iron_intern_cstr(&ctx->domain->interner, key);
                    if (!iron_hashmap_get(&ctx->internal_calls, &key_ptr, &icall)) {
                        /* Fallback: try with empty signature */
                        snprintf(key, sizeof(key), "%s::%s()", full_type_name, method_name);
                        IRON_DEBUG_EXEC("Fallback lookup: %s", key);
                        key_ptr = iron_intern_cstr(&ctx->domain->interner, key);
                        iron_hashmap_get(&ctx->internal_calls, &key_ptr, &icall);
                    }
                }
            } else {
                /* No signature - try empty */
                snprintf(key, sizeof(key), "%s::%s()", full_type_name, method_name);
                key_ptr = iron_intern_cstr(&ctx->domain->interner, key);
                iron_hashmap_get(&ctx->internal_calls, &key_ptr, &icall);
            }
            
            if (icall) {
                /* Create a temporary method structure for internal call */
                static iron_runtime_method_t temp_method;
                memset(&temp_method, 0, sizeof(temp_method));
                temp_method.name = method_name;
                temp_method.token = token;
                temp_method.is_internal_call = IRON_TRUE;
                /* Store as void* to avoid cast warning - will be cast back in call_method */
                temp_method.internal_call = (iron_method_invoke_fn)(void*)icall;
                temp_method.param_count = param_count;
                /* Check calling convention: 0x00 = static, 0x20 = instance */
                if (sig_data && sig_size > 0 && (sig_data[0] & 0x20) == 0) {
                    temp_method.attrs = 0x10; /* Static */
                }
                
                *out_method = &temp_method;
                return (iron_result_t)IRON_SUCCESS;
            } else {
                /* Log unresolved method with full details */
                IRON_WARN_EXEC("Unresolved method: %s::%s (token 0x%08X, params=%u)",
                              full_type_name, method_name, token, param_count);
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
        iron_u32 alloc_size = method->declaring_type->instance_size;
        
        /* Ensure minimum allocation size for fields (8 bytes per field, minimum 64 bytes) */
        if (alloc_size < 64) {
            alloc_size = 64;
        }
        if (method->declaring_type->field_count > 0) {
            iron_u32 field_size = method->declaring_type->field_count * 8;
            if (field_size > alloc_size) {
                alloc_size = field_size;
            }
        }
        
        new_obj = iron_gc_alloc_object(gc, method->declaring_type, alloc_size);
        if (!new_obj) {
            return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate object");
        }
        /* Zero-initialize the object */
        memset(new_obj, 0, alloc_size);
    }
    
    /* For instance methods (not static, not newobj), we need to include 'this' */
    /* The 'this' pointer is on the stack before the arguments */
    {
        iron_bool is_instance = !is_newobj && !(method->attrs & 0x10); /* 0x10 = static */
        iron_u32 total_args = param_count + (is_instance ? 1 : 0);
        
        /* Pop arguments from stack (in reverse order) */
        if (total_args > 0) {
            args = (iron_stack_value_t *)iron_alloc(ctx->allocator, 
                                                     total_args * sizeof(iron_stack_value_t));
            if (!args) {
                return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate args");
            }
            
            /* Pop args in reverse order (args first, then 'this' if instance) */
            for (i = total_args; i > 0; i--) {
                if (stack->size > 0) {
                    args[i - 1] = iron_stack_pop(stack);
                } else {
                    memset(&args[i - 1], 0, sizeof(iron_stack_value_t));
                }
            }
            param_count = total_args;
        }
    }
    
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
        iron_internal_call_fn icall = (iron_internal_call_fn)(void*)method->internal_call;
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
    } else {
        /* Check if method returns a value by checking if result was set */
        /* The result is set by iron_exec_method when the method returns a value */
        /* We use a special marker: result.type is set to IRON_VAL_NONE (0xFF) for void */
        if (result.type != 0xFF) {
            IRON_TRACE_EXEC("call_method: pushing result type=%d value=%d", result.type, result.value.i32);
            iron_stack_push(stack, result);
        }
    }
    
    return (iron_result_t)IRON_SUCCESS;
}

/* ============================================================================
 * GC Allocation Wrappers (exec.h API)
 * ============================================================================ */

void *iron_gc_alloc(iron_exec_context_t *ctx, iron_runtime_type_t *type)
{
    if (!ctx || !type) return NULL;
    return iron_gc_alloc_object(&ctx->gc, type, type->instance_size);
}

void *iron_gc_alloc_array(iron_exec_context_t *ctx,
                          iron_runtime_type_t *element_type,
                          iron_u32 length)
{
    iron_size element_size;
    
    if (!ctx) return NULL;
    
    if (element_type) {
        element_size = element_type->instance_size;
        if (element_size == 0) {
            element_size = sizeof(void*);
        }
    } else {
        element_size = sizeof(void*);
    }
    
    return iron_gc_alloc_array_raw(&ctx->gc, element_type, element_size, length);
}

void *iron_gc_alloc_string(iron_exec_context_t *ctx,
                           const iron_u16 *chars,
                           iron_u32 length)
{
    void *str;
    iron_u32 *len_ptr;
    iron_u16 *data_ptr;
    iron_size size;
    
    if (!ctx) return NULL;
    
    /* String layout: 4-byte length + UTF-16 chars */
    size = sizeof(iron_u32) + (length * sizeof(iron_u16));
    str = iron_gc_alloc_object(&ctx->gc, NULL, size);
    
    if (str) {
        len_ptr = (iron_u32 *)str;
        *len_ptr = length;
        data_ptr = (iron_u16 *)((iron_u8 *)str + sizeof(iron_u32));
        if (chars) {
            memcpy(data_ptr, chars, length * sizeof(iron_u16));
        }
    }
    
    return str;
}

void *iron_gc_box(iron_exec_context_t *ctx,
                  iron_runtime_type_t *type,
                  const void *value)
{
    void *boxed;
    
    if (!ctx || !type || !value) return NULL;
    
    boxed = iron_gc_alloc_object(&ctx->gc, type, type->instance_size);
    if (boxed) {
        memcpy(boxed, value, type->instance_size);
    }
    
    return boxed;
}

void *iron_gc_unbox(void *obj, iron_runtime_type_t *type)
{
    (void)type;
    /* For boxed value types, the object data IS the value */
    return obj;
}
