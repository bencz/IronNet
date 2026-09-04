/*
 * IronNet CLR Interpreter
 * enum.c - System.Enum internal calls
 */

#include "iron/corlib.h"
#include "iron/runtime.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

static iron_bool is_integral_element_type(iron_element_type_t element_type)
{
    return element_type == IRON_TYPE_BOOLEAN || element_type == IRON_TYPE_CHAR || element_type == IRON_TYPE_I1 || element_type == IRON_TYPE_U1 ||
           element_type == IRON_TYPE_I2 || element_type == IRON_TYPE_U2 || element_type == IRON_TYPE_I4 || element_type == IRON_TYPE_U4 ||
           element_type == IRON_TYPE_I8 || element_type == IRON_TYPE_U8;
}

static iron_u64 storage_bits(const void *storage, iron_element_type_t element_type)
{
    switch (element_type) {
        case IRON_TYPE_BOOLEAN:
        case IRON_TYPE_I1:
        case IRON_TYPE_U1: {
            iron_u8 value;

            memcpy(&value, storage, sizeof(value));
            return value;
        }

        case IRON_TYPE_CHAR:
        case IRON_TYPE_I2:
        case IRON_TYPE_U2: {
            iron_u16 value;

            memcpy(&value, storage, sizeof(value));
            return value;
        }

        case IRON_TYPE_I4:
        case IRON_TYPE_U4: {
            iron_u32 value;

            memcpy(&value, storage, sizeof(value));
            return value;
        }

        case IRON_TYPE_I8:
        case IRON_TYPE_U8: {
            iron_u64 value;

            memcpy(&value, storage, sizeof(value));
            return value;
        }

        default:
            return 0;
    }
}

static iron_u64 constant_bits(const iron_runtime_field_t *field)
{
    switch (field->constant_type) {
        case IRON_TYPE_BOOLEAN:
        case IRON_TYPE_I1:
        case IRON_TYPE_U1:
            return (iron_u8)field->constant_value.i32;

        case IRON_TYPE_CHAR:
        case IRON_TYPE_I2:
        case IRON_TYPE_U2:
            return (iron_u16)field->constant_value.i32;

        case IRON_TYPE_I4:
        case IRON_TYPE_U4:
            return (iron_u32)field->constant_value.i32;

        case IRON_TYPE_I8:
        case IRON_TYPE_U8:
            return (iron_u64)field->constant_value.i64;

        default:
            return 0;
    }
}

static void write_storage_bits(void *storage, iron_element_type_t element_type, iron_u64 bits)
{
    switch (element_type) {
        case IRON_TYPE_BOOLEAN:
        case IRON_TYPE_I1:
        case IRON_TYPE_U1: {
            iron_u8 value;

            value = (iron_u8)bits;
            memcpy(storage, &value, sizeof(value));
            break;
        }

        case IRON_TYPE_CHAR:
        case IRON_TYPE_I2:
        case IRON_TYPE_U2: {
            iron_u16 value;

            value = (iron_u16)bits;
            memcpy(storage, &value, sizeof(value));
            break;
        }

        case IRON_TYPE_I4:
        case IRON_TYPE_U4: {
            iron_u32 value;

            value = (iron_u32)bits;
            memcpy(storage, &value, sizeof(value));
            break;
        }

        case IRON_TYPE_I8:
        case IRON_TYPE_U8: {
            iron_u64 value;

            value = bits;
            memcpy(storage, &value, sizeof(value));
            break;
        }

        default:
            break;
    }
}

static iron_runtime_type_t *require_enum_type(iron_stack_value_t *args, iron_u32 arg_count)
{
    iron_runtime_type_t *type;

    if (!args || arg_count == 0) {
        return NULL;
    }

    type = (iron_runtime_type_t *)iron_corlib_object_argument(&args[0]);
    return type && type->kind == IRON_KIND_ENUM && is_integral_element_type(type->element_type) ? type : NULL;
}

static iron_bool get_boxed_value_bits(iron_exec_context_t *ctx, iron_runtime_type_t *enum_type, void *value, iron_bool allow_string, iron_u64 *bits, char **name)
{
    iron_runtime_type_t *value_type;

    if (!ctx || !enum_type || !value || !bits || !name) {
        return IRON_FALSE;
    }

    *name = NULL;
    value_type = iron_managed_reference_get_type(ctx->domain, value);
    if (allow_string && value_type && value_type->element_type == IRON_TYPE_STRING) {
        *name = iron_string_to_utf8(ctx, value);
        return *name != NULL;
    }
    if (!value_type || (value_type != enum_type && value_type->element_type != enum_type->element_type)) {
        return IRON_FALSE;
    }

    *bits = storage_bits(value, enum_type->element_type);
    return IRON_TRUE;
}

static iron_u32 collect_enum_fields(iron_runtime_type_t *enum_type, iron_runtime_field_t **fields)
{
    iron_u32 count;
    iron_u32 field_index;

    count = 0;
    for (field_index = 0; field_index < enum_type->field_count; field_index++) {
        iron_runtime_field_t *field;

        field = enum_type->fields[field_index];
        if (field && (field->attrs & IRON_FIELD_STATIC) != 0 && (field->attrs & IRON_FIELD_LITERAL) != 0 && field->has_constant &&
            is_integral_element_type(field->constant_type)) {
            if (fields) {
                fields[count] = field;
            }
            count++;
        }
    }

    return count;
}

static void sort_enum_fields(iron_runtime_field_t **fields, iron_u32 count)
{
    iron_u32 index;

    for (index = 1; index < count; index++) {
        iron_runtime_field_t *field;
        iron_u64 field_value;
        iron_u32 position;

        field = fields[index];
        field_value = constant_bits(field);
        position = index;
        while (position > 0 && constant_bits(fields[position - 1]) > field_value) {
            fields[position] = fields[position - 1];
            position--;
        }
        fields[position] = field;
    }
}

static iron_runtime_field_t **create_sorted_enum_fields(iron_exec_context_t *ctx, iron_runtime_type_t *enum_type, iron_u32 *count)
{
    iron_runtime_field_t **fields;

    *count = collect_enum_fields(enum_type, NULL);
    if (*count == 0) {
        return NULL;
    }
    if ((iron_size)*count > (iron_size)-1 / sizeof(iron_runtime_field_t *)) {
        return NULL;
    }

    fields = (iron_runtime_field_t **)iron_alloc(ctx->allocator, (iron_size)*count * sizeof(iron_runtime_field_t *));
    if (!fields) {
        return NULL;
    }

    collect_enum_fields(enum_type, fields);
    sort_enum_fields(fields, *count);
    return fields;
}

static const char *find_enum_name(iron_runtime_type_t *enum_type, iron_u64 bits)
{
    iron_u32 field_index;

    for (field_index = 0; field_index < enum_type->field_count; field_index++) {
        iron_runtime_field_t *field;

        field = enum_type->fields[field_index];
        if (field && field->name && (field->attrs & IRON_FIELD_LITERAL) != 0 && field->has_constant && constant_bits(field) == bits) {
            return field->name;
        }
    }

    return NULL;
}

static iron_bool enum_has_flags_attribute(iron_runtime_type_t *enum_type)
{
    iron_assembly_t *assembly;
    iron_u32 attribute_count;
    iron_u32 attribute_index;

    if (!enum_type || !enum_type->module || !enum_type->module->assembly) {
        return IRON_FALSE;
    }

    assembly = enum_type->module->assembly;
    attribute_count = iron_metadata_table_rows(&assembly->metadata, IRON_TABLE_CUSTOM_ATTRIBUTE);
    for (attribute_index = 1; attribute_index <= attribute_count; attribute_index++) {
        iron_custom_attribute_row_t row;
        iron_token_t parent_token;
        iron_token_t constructor_token;
        iron_runtime_method_t *constructor;

        if (!IRON_RESULT_OK(iron_metadata_read_row(&assembly->metadata, IRON_MAKE_TOKEN(IRON_TABLE_CUSTOM_ATTRIBUTE, attribute_index), &row))) {
            continue;
        }

        parent_token = iron_metadata_decode_coded(&assembly->metadata, IRON_CODED_HAS_CUSTOM_ATTRIBUTE, row.parent);
        if (parent_token != enum_type->token) {
            continue;
        }

        constructor_token = iron_metadata_decode_coded(&assembly->metadata, IRON_CODED_CUSTOM_ATTRIBUTE_TYPE, row.type);
        constructor = iron_method_resolve_token(enum_type->module, constructor_token);
        if (constructor && constructor->declaring_type && constructor->declaring_type->full_name &&
            strcmp(constructor->declaring_type->full_name, "System.FlagsAttribute") == 0) {
            return IRON_TRUE;
        }
    }

    return IRON_FALSE;
}

static char *format_flags_value(iron_exec_context_t *ctx, iron_runtime_type_t *enum_type, iron_u64 bits)
{
    iron_runtime_field_t **fields;
    iron_u32 *selected;
    iron_u32 count;
    iron_u32 selected_count;
    iron_u64 remaining;
    iron_size text_length;
    char *text;
    char *cursor;
    iron_u32 index;

    if (!enum_has_flags_attribute(enum_type) || bits == 0) {
        return NULL;
    }

    fields = create_sorted_enum_fields(ctx, enum_type, &count);
    if (!fields || count == 0 || (iron_size)count > (iron_size)-1 / sizeof(iron_u32)) {
        if (fields) {
            iron_free(ctx->allocator, fields, (iron_size)count * sizeof(iron_runtime_field_t *));
        }
        return NULL;
    }

    selected = (iron_u32 *)iron_alloc(ctx->allocator, (iron_size)count * sizeof(iron_u32));
    if (!selected) {
        iron_free(ctx->allocator, fields, (iron_size)count * sizeof(iron_runtime_field_t *));
        return NULL;
    }

    selected_count = 0;
    remaining = bits;
    for (index = count; index > 0; index--) {
        iron_u64 field_value;

        field_value = constant_bits(fields[index - 1]);
        if (field_value != 0 && (remaining & field_value) == field_value) {
            selected[selected_count++] = index - 1;
            remaining &= ~field_value;
        }
    }

    if (remaining != 0 || selected_count == 0) {
        iron_free(ctx->allocator, selected, (iron_size)count * sizeof(iron_u32));
        iron_free(ctx->allocator, fields, (iron_size)count * sizeof(iron_runtime_field_t *));
        return NULL;
    }

    text_length = (iron_size)(selected_count - 1) * 2;
    for (index = 0; index < selected_count; index++) {
        iron_size name_length;

        name_length = strlen(fields[selected[index]]->name);
        if (text_length > (iron_size)-1 - name_length) {
            iron_free(ctx->allocator, selected, (iron_size)count * sizeof(iron_u32));
            iron_free(ctx->allocator, fields, (iron_size)count * sizeof(iron_runtime_field_t *));
            return NULL;
        }
        text_length += name_length;
    }
    if (text_length == (iron_size)-1) {
        iron_free(ctx->allocator, selected, (iron_size)count * sizeof(iron_u32));
        iron_free(ctx->allocator, fields, (iron_size)count * sizeof(iron_runtime_field_t *));
        return NULL;
    }

    text = (char *)iron_alloc(ctx->allocator, text_length + 1);
    if (!text) {
        iron_free(ctx->allocator, selected, (iron_size)count * sizeof(iron_u32));
        iron_free(ctx->allocator, fields, (iron_size)count * sizeof(iron_runtime_field_t *));
        return NULL;
    }

    cursor = text;
    for (index = selected_count; index > 0; index--) {
        const char *name;
        iron_size name_length;

        if (cursor != text) {
            *cursor++ = ',';
            *cursor++ = ' ';
        }
        name = fields[selected[index - 1]]->name;
        name_length = strlen(name);
        memcpy(cursor, name, name_length);
        cursor += name_length;
    }
    *cursor = '\0';

    iron_free(ctx->allocator, selected, (iron_size)count * sizeof(iron_u32));
    iron_free(ctx->allocator, fields, (iron_size)count * sizeof(iron_runtime_field_t *));
    return text;
}

static void format_enum_number(char *buffer, iron_size buffer_size, iron_element_type_t element_type, iron_u64 bits)
{
    switch (element_type) {
        case IRON_TYPE_I1:
            snprintf(buffer, buffer_size, "%d", (int)(iron_i8)bits);
            break;
        case IRON_TYPE_I2:
            snprintf(buffer, buffer_size, "%d", (int)(iron_i16)bits);
            break;
        case IRON_TYPE_I4:
            snprintf(buffer, buffer_size, "%" PRId32, (iron_i32)bits);
            break;
        case IRON_TYPE_I8:
            snprintf(buffer, buffer_size, "%" PRId64, (iron_i64)bits);
            break;
        case IRON_TYPE_U8:
            snprintf(buffer, buffer_size, "%" PRIu64, bits);
            break;
        default:
            snprintf(buffer, buffer_size, "%" PRIu32, (iron_u32)bits);
            break;
    }
}

iron_result_t icall_Enum_ToString(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    void *value;
    iron_runtime_type_t *enum_type;
    iron_u64 bits;
    const char *name;
    char *flags_name;
    char number[32];

    if (!args || arg_count < 1 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Enum.ToString requires this and a result slot");
    }

    value = iron_corlib_object_argument(&args[0]);
    enum_type = iron_managed_reference_get_type(ctx->domain, value);
    if (!enum_type || enum_type->kind != IRON_KIND_ENUM) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Enum.ToString requires a boxed enum value");
    }

    bits = storage_bits(value, enum_type->element_type);
    name = find_enum_name(enum_type, bits);
    flags_name = NULL;
    if (!name) {
        flags_name = format_flags_value(ctx, enum_type, bits);
        if (flags_name) {
            name = flags_name;
        } else {
            format_enum_number(number, sizeof(number), enum_type->element_type, bits);
            name = number;
        }
    }

    result->type = IRON_VAL_OBJ;
    result->value.obj = iron_string_new_utf8(ctx, name);
    if (flags_name) {
        iron_free(ctx->allocator, flags_name, strlen(flags_name) + 1);
    }
    return result->value.obj ? IRON_SUCCESS : IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate Enum.ToString result");
}

iron_result_t icall_Enum_Equals(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    void *left;
    void *right;
    iron_runtime_type_t *left_type;

    (void)ctx;

    if (!args || arg_count < 2 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Enum.Equals requires this, an object, and a result slot");
    }

    left = iron_corlib_object_argument(&args[0]);
    right = iron_corlib_object_argument(&args[1]);
    left_type = iron_managed_reference_get_type(ctx->domain, left);
    result->type = IRON_VAL_I32;
    result->value.i32 = left_type && right && iron_managed_reference_get_type(ctx->domain, right) == left_type && storage_bits(left, left_type->element_type) == storage_bits(right, left_type->element_type);
    return IRON_SUCCESS;
}

iron_result_t icall_Enum_GetHashCode(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    void *value;
    iron_runtime_type_t *enum_type;
    iron_u64 bits;

    (void)ctx;

    if (!args || arg_count < 1 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Enum.GetHashCode requires this and a result slot");
    }

    value = iron_corlib_object_argument(&args[0]);
    enum_type = iron_managed_reference_get_type(ctx->domain, value);
    if (!enum_type || enum_type->kind != IRON_KIND_ENUM) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Enum.GetHashCode requires a boxed enum value");
    }

    bits = storage_bits(value, enum_type->element_type);
    result->type = IRON_VAL_I32;
    result->value.i32 = (iron_i32)((iron_u32)bits ^ (iron_u32)(bits >> 32));
    return IRON_SUCCESS;
}

iron_result_t icall_Enum_IsDefinedCore(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_type_t *enum_type;
    void *value;
    iron_u64 bits;
    char *requested_name;
    iron_bool found;

    if (!args || arg_count < 2 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Enum.IsDefined requires two arguments and a result slot");
    }

    enum_type = require_enum_type(args, arg_count);
    value = iron_corlib_object_argument(&args[1]);
    if (!enum_type || !get_boxed_value_bits(ctx, enum_type, value, IRON_TRUE, &bits, &requested_name)) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Enum.IsDefined received a value with an incompatible type");
    }

    found = IRON_FALSE;
    if (requested_name) {
        iron_u32 field_index;

        for (field_index = 0; field_index < enum_type->field_count; field_index++) {
            iron_runtime_field_t *field;

            field = enum_type->fields[field_index];
            if (field && field->name && (field->attrs & IRON_FIELD_LITERAL) != 0 && strcmp(field->name, requested_name) == 0) {
                found = IRON_TRUE;
                break;
            }
        }
        iron_free(ctx->allocator, requested_name, strlen(requested_name) + 1);
    } else {
        found = find_enum_name(enum_type, bits) != NULL;
    }

    result->type = IRON_VAL_I32;
    result->value.i32 = found ? 1 : 0;
    return IRON_SUCCESS;
}

iron_result_t icall_Enum_GetNameCore(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_type_t *enum_type;
    iron_u64 bits;
    char *unused_name;
    const char *name;

    if (!args || arg_count < 2 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Enum.GetName requires two arguments and a result slot");
    }

    enum_type = require_enum_type(args, arg_count);
    if (!enum_type || !get_boxed_value_bits(ctx, enum_type, iron_corlib_object_argument(&args[1]), IRON_FALSE, &bits, &unused_name)) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Enum.GetName received a value with an incompatible type");
    }

    name = find_enum_name(enum_type, bits);
    result->type = IRON_VAL_OBJ;
    result->value.obj = name ? iron_string_new_utf8(ctx, name) : NULL;
    return name && !result->value.obj ? IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate Enum.GetName result") : IRON_SUCCESS;
}

iron_result_t icall_Enum_GetNamesCore(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_type_t *enum_type;
    iron_runtime_field_t **fields;
    iron_u32 count;
    void *array;
    void **elements;
    iron_gc_handle_t *array_handle;
    iron_u32 index;

    enum_type = require_enum_type(args, arg_count);
    if (!result || !enum_type) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Enum.GetNames requires an enum Type and a result slot");
    }

    fields = create_sorted_enum_fields(ctx, enum_type, &count);
    if (count != 0 && !fields) {
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate the sorted enum field list");
    }

    array = iron_gc_alloc_array_raw(&ctx->gc, ctx->domain->type_string, sizeof(void *), count);
    if (!array) {
        if (fields) {
            iron_free(ctx->allocator, fields, (iron_size)count * sizeof(iron_runtime_field_t *));
        }
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate the enum name array");
    }

    array_handle = iron_gc_handle_alloc(&ctx->gc, array, IRON_GC_HANDLE_NORMAL);
    if (!array_handle) {
        if (fields) {
            iron_free(ctx->allocator, fields, (iron_size)count * sizeof(iron_runtime_field_t *));
        }
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to protect the enum name array during construction");
    }

    elements = (void **)iron_array_get_data(array);
    for (index = 0; index < count; index++) {
        elements[index] = iron_string_new_utf8(ctx, fields[index]->name);
        if (!elements[index]) {
            iron_gc_handle_free(&ctx->gc, array_handle);
            iron_free(ctx->allocator, fields, (iron_size)count * sizeof(iron_runtime_field_t *));
            return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate an enum name");
        }
        iron_gc_write_barrier(array, &elements[index], elements[index]);
    }

    iron_gc_handle_free(&ctx->gc, array_handle);
    if (fields) {
        iron_free(ctx->allocator, fields, (iron_size)count * sizeof(iron_runtime_field_t *));
    }

    result->type = IRON_VAL_OBJ;
    result->value.obj = array;
    return IRON_SUCCESS;
}

iron_result_t icall_Enum_GetValuesCore(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_type_t *enum_type;
    iron_runtime_field_t **fields;
    iron_u32 count;
    iron_size element_size;
    void *array;
    iron_u8 *elements;
    iron_u32 index;

    enum_type = require_enum_type(args, arg_count);
    if (!result || !enum_type) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Enum.GetValues requires an enum Type and a result slot");
    }

    fields = create_sorted_enum_fields(ctx, enum_type, &count);
    if (count != 0 && !fields) {
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate the sorted enum field list");
    }

    element_size = iron_type_storage_size(enum_type);
    array = iron_gc_alloc_array_raw(&ctx->gc, enum_type, element_size, count);
    if (!array) {
        if (fields) {
            iron_free(ctx->allocator, fields, (iron_size)count * sizeof(iron_runtime_field_t *));
        }
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate the enum value array");
    }

    elements = (iron_u8 *)iron_array_get_data(array);
    for (index = 0; index < count; index++) {
        write_storage_bits(elements + (iron_size)index * element_size, enum_type->element_type, constant_bits(fields[index]));
    }

    if (fields) {
        iron_free(ctx->allocator, fields, (iron_size)count * sizeof(iron_runtime_field_t *));
    }

    result->type = IRON_VAL_OBJ;
    result->value.obj = array;
    return IRON_SUCCESS;
}
