/*
 * IronNet CLR Interpreter
 * valuetype.c - System.ValueType structural equality and hashing
 */

#include "iron/corlib.h"
#include "iron/runtime.h"
#include <math.h>
#include <stdint.h>
#include <string.h>

static iron_bool strings_equal(const void *left, const void *right)
{
    iron_u32 length;

    if (left == right) {
        return IRON_TRUE;
    }
    if (!left || !right) {
        return IRON_FALSE;
    }

    length = iron_string_get_length((void *)left);
    return length == iron_string_get_length((void *)right) &&
           memcmp(iron_string_get_chars((void *)left), iron_string_get_chars((void *)right), (iron_size)length * sizeof(iron_u16)) == 0;
}

static iron_bool primitive_values_equal(const void *left, const void *right, iron_element_type_t element_type)
{
    switch (element_type) {
        case IRON_TYPE_BOOLEAN:
        case IRON_TYPE_I1:
        case IRON_TYPE_U1:
            return *(const iron_u8 *)left == *(const iron_u8 *)right;

        case IRON_TYPE_CHAR:
        case IRON_TYPE_I2:
        case IRON_TYPE_U2: {
            iron_u16 left_value;
            iron_u16 right_value;

            memcpy(&left_value, left, sizeof(left_value));
            memcpy(&right_value, right, sizeof(right_value));
            return left_value == right_value;
        }

        case IRON_TYPE_I4:
        case IRON_TYPE_U4: {
            iron_u32 left_value;
            iron_u32 right_value;

            memcpy(&left_value, left, sizeof(left_value));
            memcpy(&right_value, right, sizeof(right_value));
            return left_value == right_value;
        }

        case IRON_TYPE_I8:
        case IRON_TYPE_U8: {
            iron_u64 left_value;
            iron_u64 right_value;

            memcpy(&left_value, left, sizeof(left_value));
            memcpy(&right_value, right, sizeof(right_value));
            return left_value == right_value;
        }

        case IRON_TYPE_R4: {
            iron_f32 left_value;
            iron_f32 right_value;

            memcpy(&left_value, left, sizeof(left_value));
            memcpy(&right_value, right, sizeof(right_value));
            return left_value == right_value || (isnan(left_value) && isnan(right_value));
        }

        case IRON_TYPE_R8: {
            iron_f64 left_value;
            iron_f64 right_value;

            memcpy(&left_value, left, sizeof(left_value));
            memcpy(&right_value, right, sizeof(right_value));
            return left_value == right_value || (isnan(left_value) && isnan(right_value));
        }

        case IRON_TYPE_I:
        case IRON_TYPE_U:
        case IRON_TYPE_PTR:
        case IRON_TYPE_FNPTR: {
            void *left_value;
            void *right_value;

            memcpy(&left_value, left, sizeof(left_value));
            memcpy(&right_value, right, sizeof(right_value));
            return left_value == right_value;
        }

        default:
            return IRON_FALSE;
    }
}

static iron_bool value_storage_equal(iron_runtime_type_t *type, const void *left, const void *right)
{
    iron_u32 instance_field_count;
    iron_u32 field_index;

    if (!type || !left || !right) {
        return left == right;
    }
    if (type->element_type != IRON_TYPE_END && type->element_type != IRON_TYPE_VALUETYPE) {
        return primitive_values_equal(left, right, type->element_type);
    }

    instance_field_count = 0;
    for (field_index = 0; field_index < type->field_count; field_index++) {
        iron_runtime_field_t *field;
        const iron_u8 *left_field;
        const iron_u8 *right_field;

        field = type->fields[field_index];
        if (!field || !field->field_type || (field->attrs & IRON_FIELD_STATIC) != 0) {
            continue;
        }

        instance_field_count++;
        left_field = (const iron_u8 *)left + field->offset;
        right_field = (const iron_u8 *)right + field->offset;
        if (iron_type_is_managed_reference(field->field_type)) {
            void *left_object;
            void *right_object;

            memcpy(&left_object, left_field, sizeof(left_object));
            memcpy(&right_object, right_field, sizeof(right_object));
            if (field->field_type->element_type == IRON_TYPE_STRING) {
                if (!strings_equal(left_object, right_object)) {
                    return IRON_FALSE;
                }
            } else if (left_object != right_object) {
                return IRON_FALSE;
            }
        } else if (!value_storage_equal(field->field_type, left_field, right_field)) {
            return IRON_FALSE;
        }
    }

    if (instance_field_count == 0) {
        return memcmp(left, right, iron_type_storage_size(type)) == 0;
    }

    return IRON_TRUE;
}

static iron_u32 hash_bytes(iron_u32 hash, const void *data, iron_size size)
{
    const iron_u8 *bytes;
    iron_size index;

    bytes = (const iron_u8 *)data;
    for (index = 0; index < size; index++) {
        hash ^= bytes[index];
        hash *= 16777619u;
    }

    return hash;
}

static iron_u32 hash_primitive_value(iron_runtime_type_t *type, const void *storage, iron_u32 hash)
{
    if (type->element_type == IRON_TYPE_R4) {
        iron_f32 value;
        iron_u32 bits;

        memcpy(&value, storage, sizeof(value));
        if (value == 0.0f) {
            bits = 0;
        } else if (isnan(value)) {
            bits = 0x7FC00000u;
        } else {
            memcpy(&bits, &value, sizeof(bits));
        }
        return hash_bytes(hash, &bits, sizeof(bits));
    }
    if (type->element_type == IRON_TYPE_R8) {
        iron_f64 value;
        iron_u64 bits;

        memcpy(&value, storage, sizeof(value));
        if (value == 0.0) {
            bits = 0;
        } else if (isnan(value)) {
            bits = UINT64_C(0x7FF8000000000000);
        } else {
            memcpy(&bits, &value, sizeof(bits));
        }
        return hash_bytes(hash, &bits, sizeof(bits));
    }

    return hash_bytes(hash, storage, iron_type_storage_size(type));
}

static iron_u32 hash_value_storage(iron_runtime_type_t *type, const void *storage, iron_u32 hash)
{
    iron_u32 instance_field_count;
    iron_u32 field_index;

    if (!type || !storage) {
        return hash_bytes(hash, &storage, sizeof(storage));
    }
    if (type->element_type != IRON_TYPE_END && type->element_type != IRON_TYPE_VALUETYPE) {
        return hash_primitive_value(type, storage, hash);
    }

    instance_field_count = 0;
    for (field_index = 0; field_index < type->field_count; field_index++) {
        iron_runtime_field_t *field;
        const iron_u8 *field_storage;

        field = type->fields[field_index];
        if (!field || !field->field_type || (field->attrs & IRON_FIELD_STATIC) != 0) {
            continue;
        }

        instance_field_count++;
        field_storage = (const iron_u8 *)storage + field->offset;
        if (iron_type_is_managed_reference(field->field_type)) {
            void *object;

            memcpy(&object, field_storage, sizeof(object));
            if (object && field->field_type->element_type == IRON_TYPE_STRING) {
                iron_u32 string_length;

                string_length = iron_string_get_length(object);
                hash = hash_bytes(hash, iron_string_get_chars(object), (iron_size)string_length * sizeof(iron_u16));
            } else {
                uintptr_t pointer_value;

                pointer_value = (uintptr_t)object;
                hash = hash_bytes(hash, &pointer_value, sizeof(pointer_value));
            }
        } else {
            hash = hash_value_storage(field->field_type, field_storage, hash);
        }
    }

    return instance_field_count == 0 ? hash_bytes(hash, storage, iron_type_storage_size(type)) : hash;
}

iron_result_t icall_ValueType_EqualsInternal(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    void *left;
    void *right;
    iron_runtime_type_t *left_type;

    if (!args || arg_count < 2 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "ValueType.EqualsInternal requires this, an object, and a result slot");
    }

    left = iron_corlib_object_argument(&args[0]);
    right = iron_corlib_object_argument(&args[1]);
    left_type = iron_managed_reference_get_type(ctx->domain, left);

    result->type = IRON_VAL_I32;
    result->value.i32 = left_type && right && iron_managed_reference_get_type(ctx->domain, right) == left_type && value_storage_equal(left_type, left, right) ? 1 : 0;
    return IRON_SUCCESS;
}

iron_result_t icall_ValueType_GetHashCodeInternal(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    void *value;
    iron_runtime_type_t *type;

    if (!args || arg_count < 1 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "ValueType.GetHashCodeInternal requires this and a result slot");
    }

    value = iron_corlib_object_argument(&args[0]);
    type = iron_managed_reference_get_type(ctx->domain, value);
    if (!type || (type->kind != IRON_KIND_VALUETYPE && type->kind != IRON_KIND_ENUM)) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "ValueType.GetHashCodeInternal requires a boxed value type");
    }

    result->type = IRON_VAL_I32;
    result->value.i32 = (iron_i32)hash_value_storage(type, value, 2166136261u);
    return IRON_SUCCESS;
}
