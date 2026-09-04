/*
 * IronNet CLR Interpreter
 * corlib/console.c - System.Console internal calls
 */

#include "iron/corlib.h"
#include <stdio.h>
#include <string.h>

/* ============================================================================
 * System.Console Internal Calls
 * ============================================================================ */

iron_result_t icall_Console_WriteLine_String(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    void *str_obj;
    iron_u32 len, i;
    iron_u16 *chars;
    
    (void)ctx;
    (void)result;
    
    if (arg_count < 1) {
        printf("\n");
        return IRON_SUCCESS;
    }
    
    str_obj = (args[0].type == IRON_VAL_PTR) ? args[0].value.ptr : args[0].value.obj;
    
    if (!str_obj) {
        printf("\n");
        return IRON_SUCCESS;
    }
    
    len = iron_string_get_length(str_obj);
    chars = iron_string_get_chars(str_obj);
    
    for (i = 0; i < len; i++) {
        putchar((chars[i] < 128) ? (char)chars[i] : '?');
    }
    putchar('\n');
    fflush(stdout);
    
    return IRON_SUCCESS;
}

iron_result_t icall_Console_WriteLine_Int32(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    (void)ctx;
    (void)result;
    
    if (arg_count < 1) {
        printf("\n");
        return IRON_SUCCESS;
    }
    
    printf("%d\n", args[0].value.i32);
    fflush(stdout);
    
    return IRON_SUCCESS;
}

iron_result_t icall_Console_Write_String(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    void *str_obj;
    char *utf8;
    
    (void)result;
    
    if (arg_count < 1) {
        return IRON_SUCCESS;
    }
    
    str_obj = (args[0].type == IRON_VAL_PTR) ? args[0].value.ptr : args[0].value.obj;
    if (!str_obj) {
        return IRON_SUCCESS;
    }
    
    utf8 = iron_string_to_utf8(ctx, str_obj);
    if (utf8) {
        printf("%s", utf8);
        fflush(stdout);
        iron_free(ctx->allocator, utf8, strlen(utf8) + 1);
    }
    
    return IRON_SUCCESS;
}

iron_result_t icall_Console_Write_Int32(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    (void)ctx;
    (void)result;
    
    if (arg_count < 1) {
        return IRON_SUCCESS;
    }
    
    printf("%d", args[0].value.i32);
    fflush(stdout);
    
    return IRON_SUCCESS;
}

iron_result_t icall_Console_ReadLine(
    iron_exec_context_t *ctx,
    iron_stack_value_t *args,
    iron_u32 arg_count,
    iron_stack_value_t *result)
{
    char buffer[4096];
    void *str_obj;
    
    (void)args;
    (void)arg_count;
    
    if (!result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "ReadLine requires result");
    }
    
    if (fgets(buffer, sizeof(buffer), stdin)) {
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
        }
        str_obj = iron_string_new_utf8(ctx, buffer);
        result->type = IRON_VAL_PTR;
        result->value.ptr = str_obj;
    } else {
        result->type = IRON_VAL_PTR;
        result->value.ptr = NULL;
    }
    
    return IRON_SUCCESS;
}
