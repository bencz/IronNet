/*
 * IronNet CLR Interpreter
 * corlib/value.c - Shared internal-call argument representation helpers
 */

#include "iron/corlib.h"

void *iron_corlib_object_argument(const iron_stack_value_t *argument)
{
    if (!argument) {
        return NULL;
    }

    switch (argument->type) {
        case IRON_VAL_PTR:
            return argument->value.ptr;

        case IRON_VAL_BYREF:
            return argument->value.byref.ptr;

        case IRON_VAL_OBJ:
        case IRON_VAL_VALUETYPE:
            return argument->value.obj;

        default:
            return NULL;
    }
}

const void *iron_corlib_value_argument_data(const iron_stack_value_t *argument)
{
    void *storage;

    if (!argument) {
        return NULL;
    }

    storage = iron_corlib_object_argument(argument);
    if (storage) {
        return storage;
    }

    switch (argument->type) {
        case IRON_VAL_I32:
        case IRON_VAL_I64:
        case IRON_VAL_F32:
        case IRON_VAL_F64:
            return &argument->value;

        default:
            return NULL;
    }
}
