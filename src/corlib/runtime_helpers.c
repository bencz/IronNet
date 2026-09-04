/*
 * IronNet CLR Interpreter
 * corlib/runtime_helpers.c - System.Runtime.CompilerServices.RuntimeHelpers internal calls
 */

#include "iron/corlib.h"
#include <string.h>

static iron_bool type_contains_managed_references(const iron_runtime_type_t *type, iron_u32 depth)
{
    iron_u32 field_index;

    if (!type || depth >= 64) {
        return IRON_TRUE;
    }

    if (iron_type_is_managed_reference(type)) {
        return IRON_TRUE;
    }

    if (type->kind != IRON_KIND_VALUETYPE) {
        return IRON_FALSE;
    }

    for (field_index = 0; field_index < type->field_count; field_index++) {
        const iron_runtime_field_t *field;

        field = type->fields[field_index];
        if (field && (field->attrs & IRON_FIELD_STATIC) == 0 && type_contains_managed_references(field->field_type, depth + 1)) {
            return IRON_TRUE;
        }
    }

    return IRON_FALSE;
}

static iron_bool initialize_value_from_little_endian(iron_u8 *destination,
                                                     const iron_u8 *source,
                                                     iron_size size,
                                                     const iron_runtime_type_t *type,
                                                     iron_u32 depth)
{
    iron_u32 field_index;

    if (!destination || !source || !type || depth >= 64) {
        return IRON_FALSE;
    }

    switch (type->element_type) {
        case IRON_TYPE_BOOLEAN:
        case IRON_TYPE_I1:
        case IRON_TYPE_U1:
            if (size != 1) {
                return IRON_FALSE;
            }
            destination[0] = source[0];
            return IRON_TRUE;
        case IRON_TYPE_CHAR:
        case IRON_TYPE_I2:
        case IRON_TYPE_U2: {
            iron_u16 value;

            if (size != sizeof(value)) {
                return IRON_FALSE;
            }
            value = iron_read_u16_le(source);
            memcpy(destination, &value, sizeof(value));
            return IRON_TRUE;
        }
        case IRON_TYPE_I4:
        case IRON_TYPE_U4: {
            iron_u32 value;

            if (size != sizeof(value)) {
                return IRON_FALSE;
            }
            value = iron_read_u32_le(source);
            memcpy(destination, &value, sizeof(value));
            return IRON_TRUE;
        }
        case IRON_TYPE_I8:
        case IRON_TYPE_U8: {
            iron_u64 value;

            if (size != sizeof(value)) {
                return IRON_FALSE;
            }
            value = iron_read_u64_le(source);
            memcpy(destination, &value, sizeof(value));
            return IRON_TRUE;
        }
        case IRON_TYPE_R4: {
            iron_f32 value;

            if (size != sizeof(value)) {
                return IRON_FALSE;
            }
            value = iron_read_f32_le(source);
            memcpy(destination, &value, sizeof(value));
            return IRON_TRUE;
        }
        case IRON_TYPE_R8: {
            iron_f64 value;

            if (size != sizeof(value)) {
                return IRON_FALSE;
            }
            value = iron_read_f64_le(source);
            memcpy(destination, &value, sizeof(value));
            return IRON_TRUE;
        }
        case IRON_TYPE_I:
        case IRON_TYPE_U:
            if (size == sizeof(iron_u32)) {
                iron_u32 value;

                value = iron_read_u32_le(source);
                memcpy(destination, &value, sizeof(value));
                return IRON_TRUE;
            }
            if (size == sizeof(iron_u64)) {
                iron_u64 value;

                value = iron_read_u64_le(source);
                memcpy(destination, &value, sizeof(value));
                return IRON_TRUE;
            }
            return IRON_FALSE;
        default:
            break;
    }

    if (type->kind != IRON_KIND_VALUETYPE || type->field_count == 0) {
        memcpy(destination, source, size);
        return IRON_TRUE;
    }

    memset(destination, 0, size);
    for (field_index = 0; field_index < type->field_count; field_index++) {
        const iron_runtime_field_t *field;
        iron_size field_size;

        field = type->fields[field_index];
        if (!field || (field->attrs & IRON_FIELD_STATIC) != 0) {
            continue;
        }

        field_size = iron_type_storage_size(field->field_type);
        if (!field->field_type || field_size == 0 || field->offset > size || field_size > size - field->offset ||
            !initialize_value_from_little_endian(destination + field->offset, source + field->offset, field_size, field->field_type, depth + 1)) {
            return IRON_FALSE;
        }
    }

    return IRON_TRUE;
}

iron_result_t icall_RuntimeHelpers_InitializeArray(iron_exec_context_t *ctx,
                                                   iron_stack_value_t *args,
                                                   iron_u32 arg_count,
                                                   iron_stack_value_t *result)
{
    void *array;
    iron_runtime_field_t *field;
    iron_runtime_type_t *element_type;
    iron_u8 *destination;
    iron_size element_size;
    iron_size total_size;
    iron_u32 length;
    iron_u32 index;

    (void)ctx;

    if (!args || arg_count < 2) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "RuntimeHelpers.InitializeArray requires an array and a field handle");
    }

    array = args[0].value.obj;
    field = (iron_runtime_field_t *)args[1].value.ptr;
    if (!array) {
        return IRON_ERROR(IRON_ERR_NULL_REFERENCE, "RuntimeHelpers.InitializeArray received a null array");
    }
    if (!field || !field->rva_data) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "RuntimeHelpers.InitializeArray requires a field with RVA data");
    }

    element_type = iron_array_get_element_type(array);
    element_size = iron_array_get_element_size(array);
    length = iron_array_get_length(array);
    if (!element_type || element_size == 0 || type_contains_managed_references(element_type, 0)) {
        return IRON_ERROR(IRON_ERR_INVALID_TYPE, "RuntimeHelpers.InitializeArray requires a blittable primitive or value type array");
    }
    if ((iron_size)length > ((iron_size)-1) / element_size) {
        return IRON_ERROR(IRON_ERR_BUFFER_OVERFLOW, "RuntimeHelpers.InitializeArray size overflow");
    }

    total_size = (iron_size)length * element_size;
    if (total_size > field->rva_size) {
        return IRON_ERROR(IRON_ERR_INVALID_METADATA, "Field RVA data is smaller than the destination array");
    }

    destination = (iron_u8 *)iron_array_get_data(array);
    for (index = 0; index < length; index++) {
        iron_size offset;

        offset = (iron_size)index * element_size;
        if (!initialize_value_from_little_endian(destination + offset, field->rva_data + offset, element_size, element_type, 0)) {
            return IRON_ERROR(IRON_ERR_INVALID_TYPE, "RuntimeHelpers.InitializeArray cannot translate the array element layout");
        }
    }

    if (result) {
        memset(result, 0, sizeof(*result));
        result->type = IRON_VAL_VOID;
    }
    return IRON_SUCCESS;
}
