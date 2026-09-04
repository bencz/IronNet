/*
 * IronNet CLR Interpreter
 * corlib/gc.c - System.GC internal calls
 */

#include "iron/corlib.h"
#include "iron/gc.h"

/* ============================================================================
 * System.GC Internal Calls
 * ============================================================================ */

iron_result_t icall_GC_Collect(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    (void)args;
    (void)arg_count;

    if (!ctx) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "ctx is NULL");
    }

    iron_gc_collect(&ctx->gc);

    if (result) {
        result->type = IRON_VAL_VOID;
    }

    return IRON_SUCCESS;
}

iron_result_t icall_GC_GetTotalMemory(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    iron_gc_stats_t stats;

    if (!ctx || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "GetTotalMemory requires ctx and result");
    }

    /* If forceFullCollection is true, collect first */
    if (arg_count >= 1 && args[0].value.i32) {
        iron_gc_collect(&ctx->gc);
    }

    iron_gc_get_stats(&ctx->gc, &stats);

    result->type = IRON_VAL_I64;
    result->value.i64 = (iron_i64)stats.current_heap_size;

    return IRON_SUCCESS;
}

iron_result_t icall_GC_SuppressFinalize(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    void *obj;

    if (!ctx || arg_count < 1) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "SuppressFinalize requires object");
    }

    obj = args[0].value.obj;
    if (!obj) {
        return IRON_ERROR(IRON_ERR_NULL_REFERENCE, "ArgumentNullException");
    }

    if (!iron_domain_is_type_descriptor(ctx->domain, obj)) {
        iron_gc_suppress_finalize(obj);
    }

    if (result) {
        result->type = IRON_VAL_VOID;
    }

    return IRON_SUCCESS;
}

iron_result_t icall_GC_ReRegisterForFinalize(iron_exec_context_t *ctx,
                                             iron_stack_value_t *args,
                                             iron_u32 arg_count,
                                             iron_stack_value_t *result)
{
    void *obj;

    if (!ctx || !args || arg_count < 1) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "ReRegisterForFinalize requires object");
    }

    obj = args[0].value.obj;
    if (!obj) {
        return IRON_ERROR(IRON_ERR_NULL_REFERENCE, "ArgumentNullException");
    }

    if (!iron_domain_is_type_descriptor(ctx->domain, obj)) {
        iron_gc_reregister_finalize(obj);
    }
    if (result) {
        result->type = IRON_VAL_VOID;
    }

    return IRON_SUCCESS;
}
