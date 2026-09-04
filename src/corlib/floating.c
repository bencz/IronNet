/*
 * IronNet CLR Interpreter
 * corlib/floating.c - System.Single and System.Double internal calls
 */

#include "iron/corlib.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static iron_result_t format_floating_point(iron_exec_context_t *ctx, iron_f64 value, int precision, iron_stack_value_t *result)
{
    char buffer[64];
    char *decimal_separator;
    void *string;
    int written;

    if (!ctx || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Invalid floating-point ToString arguments");
    }

    if (isnan(value)) {
        iron_strcpy(buffer, "NaN");
    } else if (isinf(value)) {
        iron_strcpy(buffer, signbit(value) ? "-Infinity" : "Infinity");
    } else {
        written = snprintf(buffer, sizeof(buffer), "%.*g", precision, value);
        if (written < 0 || (iron_size)written >= sizeof(buffer)) {
            return IRON_ERROR(IRON_ERR_BUFFER_OVERFLOW, "Floating-point formatting exceeded its buffer");
        }

        decimal_separator = strchr(buffer, ',');
        if (decimal_separator) {
            *decimal_separator = '.';
        }
    }

    string = iron_string_new_utf8(ctx, buffer);
    if (!string) {
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate floating-point string");
    }

    memset(result, 0, sizeof(*result));
    result->type = IRON_VAL_OBJ;
    result->value.obj = string;
    return IRON_SUCCESS;
}

iron_result_t icall_Single_ToString(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    const void *value_data;
    iron_f32 value;

    if (!args || arg_count < 1 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Single.ToString requires 'this'");
    }

    value_data = iron_corlib_value_argument_data(&args[0]);
    if (!value_data) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Single.ToString received an invalid 'this' value");
    }

    memcpy(&value, value_data, sizeof(value));

    return format_floating_point(ctx, (iron_f64)value, 9, result);
}

iron_result_t icall_Double_ToString(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    const void *value_data;
    iron_f64 value;

    if (!args || arg_count < 1 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Double.ToString requires 'this'");
    }

    value_data = iron_corlib_value_argument_data(&args[0]);
    if (!value_data) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Double.ToString received an invalid 'this' value");
    }

    memcpy(&value, value_data, sizeof(value));

    return format_floating_point(ctx, value, 17, result);
}
