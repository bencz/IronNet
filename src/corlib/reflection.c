/*
 * IronNet CLR Interpreter
 * corlib/reflection.c - Managed assembly reflection internal calls
 */

#include "iron/corlib.h"
#include "iron/runtime.h"
#include <stdio.h>
#include <string.h>

typedef struct sha1_state {
    iron_u32 words[5];
    iron_u64 byte_count;
    iron_u8 block[64];
    iron_u32 block_size;
} sha1_state_t;

static iron_u32 rotate_left(iron_u32 value, iron_u32 count)
{
    return (value << count) | (value >> (32 - count));
}

static void sha1_process_block(sha1_state_t *state, const iron_u8 *block)
{
    iron_u32 schedule[80];
    iron_u32 a;
    iron_u32 b;
    iron_u32 c;
    iron_u32 d;
    iron_u32 e;
    iron_u32 index;

    for (index = 0; index < 16; index++) {
        schedule[index] = ((iron_u32)block[index * 4] << 24) |
                          ((iron_u32)block[index * 4 + 1] << 16) |
                          ((iron_u32)block[index * 4 + 2] << 8) |
                          (iron_u32)block[index * 4 + 3];
    }
    for (index = 16; index < 80; index++) {
        schedule[index] = rotate_left(schedule[index - 3] ^ schedule[index - 8] ^ schedule[index - 14] ^ schedule[index - 16], 1);
    }

    a = state->words[0];
    b = state->words[1];
    c = state->words[2];
    d = state->words[3];
    e = state->words[4];

    for (index = 0; index < 80; index++) {
        iron_u32 function;
        iron_u32 constant;
        iron_u32 temporary;

        if (index < 20) {
            function = (b & c) | ((~b) & d);
            constant = 0x5A827999U;
        } else if (index < 40) {
            function = b ^ c ^ d;
            constant = 0x6ED9EBA1U;
        } else if (index < 60) {
            function = (b & c) | (b & d) | (c & d);
            constant = 0x8F1BBCDCU;
        } else {
            function = b ^ c ^ d;
            constant = 0xCA62C1D6U;
        }

        temporary = rotate_left(a, 5) + function + e + constant + schedule[index];
        e = d;
        d = c;
        c = rotate_left(b, 30);
        b = a;
        a = temporary;
    }

    state->words[0] += a;
    state->words[1] += b;
    state->words[2] += c;
    state->words[3] += d;
    state->words[4] += e;
}

static void sha1_initialize(sha1_state_t *state)
{
    memset(state, 0, sizeof(*state));
    state->words[0] = 0x67452301U;
    state->words[1] = 0xEFCDAB89U;
    state->words[2] = 0x98BADCFEU;
    state->words[3] = 0x10325476U;
    state->words[4] = 0xC3D2E1F0U;
}

static void sha1_update(sha1_state_t *state, const iron_u8 *data, iron_u32 size)
{
    iron_u32 copied;

    state->byte_count += size;
    while (size != 0) {
        copied = 64 - state->block_size;
        if (copied > size) {
            copied = size;
        }

        memcpy(state->block + state->block_size, data, copied);
        state->block_size += copied;
        data += copied;
        size -= copied;

        if (state->block_size == 64) {
            sha1_process_block(state, state->block);
            state->block_size = 0;
        }
    }
}

static void sha1_finalize(sha1_state_t *state, iron_u8 digest[20])
{
    iron_u64 bit_count;
    iron_u32 index;

    bit_count = state->byte_count * 8;
    state->block[state->block_size++] = 0x80;
    if (state->block_size > 56) {
        memset(state->block + state->block_size, 0, 64 - state->block_size);
        sha1_process_block(state, state->block);
        state->block_size = 0;
    }

    memset(state->block + state->block_size, 0, 56 - state->block_size);
    for (index = 0; index < 8; index++) {
        state->block[63 - index] = (iron_u8)(bit_count >> (index * 8));
    }
    sha1_process_block(state, state->block);

    for (index = 0; index < 5; index++) {
        digest[index * 4] = (iron_u8)(state->words[index] >> 24);
        digest[index * 4 + 1] = (iron_u8)(state->words[index] >> 16);
        digest[index * 4 + 2] = (iron_u8)(state->words[index] >> 8);
        digest[index * 4 + 3] = (iron_u8)state->words[index];
    }
}

static iron_assembly_t *assembly_from_managed_object(void *managed_object)
{
    iron_runtime_field_t *handle_field;
    iron_runtime_type_t *managed_type;
    iron_assembly_t *assembly;

    if (!managed_object) {
        return NULL;
    }

    managed_type = iron_gc_get_type(managed_object);
    handle_field = iron_type_find_instance_field(managed_type, "_handle");
    if (!handle_field) {
        return NULL;
    }

    assembly = NULL;
    memcpy(&assembly, (const iron_u8 *)managed_object + handle_field->offset, sizeof(assembly));
    return assembly;
}

static iron_result_t get_assembly_argument(iron_stack_value_t *args, iron_u32 arg_count, iron_assembly_t **assembly)
{
    void *managed_object;

    if (!args || arg_count == 0 || !assembly) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Assembly reflection requires an instance");
    }

    managed_object = iron_corlib_object_argument(&args[0]);
    *assembly = assembly_from_managed_object(managed_object);
    return *assembly ? IRON_SUCCESS : IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Invalid RuntimeAssembly instance");
}

static void *get_managed_assembly(iron_exec_context_t *ctx, iron_assembly_t *assembly)
{
    iron_runtime_field_t *handle_field;
    iron_runtime_type_t *managed_type;
    void *managed_object;

    if (!ctx || !assembly) {
        return NULL;
    }
    if (assembly->managed_object) {
        return assembly->managed_object;
    }

    managed_type = iron_domain_find_type(ctx->domain, "System.Reflection.RuntimeAssembly");
    if (!managed_type || !IRON_RESULT_OK(iron_type_compute_layout(managed_type))) {
        return NULL;
    }

    handle_field = iron_type_find_instance_field(managed_type, "_handle");
    if (!handle_field || handle_field->size < sizeof(assembly)) {
        return NULL;
    }

    managed_object = iron_gc_alloc_object(&ctx->gc, managed_type, managed_type->instance_size);
    if (!managed_object) {
        return NULL;
    }

    memcpy((iron_u8 *)managed_object + handle_field->offset, &assembly, sizeof(assembly));
    assembly->managed_object = managed_object;
    return managed_object;
}

static iron_assembly_t *frame_assembly(iron_stack_frame_t *frame)
{
    if (!frame || !frame->method || !frame->method->declaring_type || !frame->method->declaring_type->module) {
        return NULL;
    }

    return frame->method->declaring_type->module->assembly;
}

static iron_result_t return_managed_assembly(iron_exec_context_t *ctx, iron_assembly_t *assembly, iron_stack_value_t *result)
{
    if (!result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Assembly reflection requires a result slot");
    }

    result->type = IRON_VAL_OBJ;
    result->value.obj = get_managed_assembly(ctx, assembly);
    if (assembly && !result->value.obj) {
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate a RuntimeAssembly object");
    }

    return IRON_SUCCESS;
}

static iron_bool ascii_names_equal(const char *left, const char *right, iron_bool ignore_case)
{
    unsigned char left_character;
    unsigned char right_character;

    if (!left || !right) {
        return left == right;
    }

    while (*left && *right) {
        left_character = (unsigned char)*left++;
        right_character = (unsigned char)*right++;
        if (ignore_case) {
            if (left_character >= 'A' && left_character <= 'Z') {
                left_character = (unsigned char)(left_character + ('a' - 'A'));
            }
            if (right_character >= 'A' && right_character <= 'Z') {
                right_character = (unsigned char)(right_character + ('a' - 'A'));
            }
        }
        if (left_character != right_character) {
            return IRON_FALSE;
        }
    }

    return *left == *right;
}

static void format_public_key_token(const iron_assembly_t *assembly, char token[17])
{
    static const char hex[] = "0123456789abcdef";
    sha1_state_t state;
    iron_u8 digest[20];
    iron_u32 index;

    if (!assembly->public_key || assembly->public_key_size == 0) {
        memcpy(token, "null", 5);
        return;
    }

    sha1_initialize(&state);
    sha1_update(&state, assembly->public_key, assembly->public_key_size);
    sha1_finalize(&state, digest);
    for (index = 0; index < 8; index++) {
        iron_u8 value;

        value = digest[19 - index];
        token[index * 2] = hex[value >> 4];
        token[index * 2 + 1] = hex[value & 0x0F];
    }
    token[16] = '\0';
}

iron_result_t icall_Assembly_GetExecutingAssembly(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_thread_context_t *thread;

    (void)args;
    (void)arg_count;
    thread = iron_exec_get_current_thread(ctx);
    return return_managed_assembly(ctx, thread ? frame_assembly(thread->current_frame) : NULL, result);
}

iron_result_t icall_Assembly_GetCallingAssembly(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_stack_frame_t *frame;
    iron_thread_context_t *thread;

    (void)args;
    (void)arg_count;
    thread = iron_exec_get_current_thread(ctx);
    frame = thread ? thread->current_frame : NULL;
    return return_managed_assembly(ctx, frame ? frame_assembly(frame->prev) : NULL, result);
}

iron_result_t icall_Assembly_GetEntryAssembly(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    (void)args;
    (void)arg_count;
    return return_managed_assembly(ctx, ctx ? ctx->entry_assembly : NULL, result);
}

iron_result_t icall_Assembly_GetAssembly(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_type_t *type;

    if (!args || arg_count < 1) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Assembly.GetAssembly requires a Type");
    }

    type = (iron_runtime_type_t *)iron_corlib_object_argument(&args[0]);
    if (!type) {
        return IRON_ERROR(IRON_ERR_NULL_REFERENCE, "Assembly.GetAssembly received a null Type");
    }

    return return_managed_assembly(ctx, type->module ? type->module->assembly : NULL, result);
}

iron_result_t icall_RuntimeAssembly_get_FullName(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_assembly_t *assembly;
    iron_result_t argument_result;
    char public_key_token[17];
    char full_name[512];
    const char *culture;
    int written;

    argument_result = get_assembly_argument(args, arg_count, &assembly);
    if (!IRON_RESULT_OK(argument_result) || !result) {
        return !IRON_RESULT_OK(argument_result) ? argument_result : IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Assembly.FullName requires a result slot");
    }

    format_public_key_token(assembly, public_key_token);
    culture = assembly->culture && assembly->culture[0] ? assembly->culture : "neutral";
    written = snprintf(full_name,
                       sizeof(full_name),
                       "%s, Version=%u.%u.%u.%u, Culture=%s, PublicKeyToken=%s",
                       assembly->name ? assembly->name : "",
                       assembly->major_version,
                       assembly->minor_version,
                       assembly->build_number,
                       assembly->revision_number,
                       culture,
                       public_key_token);
    if (written < 0 || (iron_size)written >= sizeof(full_name)) {
        return IRON_ERROR(IRON_ERR_BUFFER_OVERFLOW, "Assembly.FullName exceeded its formatting buffer");
    }

    result->type = IRON_VAL_OBJ;
    result->value.obj = iron_string_new_utf8(ctx, full_name);
    return result->value.obj ? IRON_SUCCESS : IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate Assembly.FullName");
}

iron_result_t icall_RuntimeAssembly_get_Location(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_assembly_t *assembly;
    iron_result_t argument_result;

    argument_result = get_assembly_argument(args, arg_count, &assembly);
    if (!IRON_RESULT_OK(argument_result) || !result) {
        return !IRON_RESULT_OK(argument_result) ? argument_result : IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Assembly.Location requires a result slot");
    }

    result->type = IRON_VAL_OBJ;
    result->value.obj = iron_string_new_utf8(ctx, assembly->location ? assembly->location : "");
    return result->value.obj ? IRON_SUCCESS : IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate Assembly.Location");
}

iron_result_t icall_RuntimeAssembly_GetTypes(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_assembly_t *assembly;
    iron_result_t argument_result;
    iron_u32 metadata_count;
    iron_u32 output_count;
    iron_u32 index;
    void *array;
    void **elements;

    argument_result = get_assembly_argument(args, arg_count, &assembly);
    if (!IRON_RESULT_OK(argument_result) || !result) {
        return !IRON_RESULT_OK(argument_result) ? argument_result : IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Assembly.GetTypes requires a result slot");
    }

    metadata_count = iron_metadata_table_rows(&assembly->metadata, IRON_TABLE_TYPE_DEF);
    output_count = 0;
    for (index = 0; index < metadata_count; index++) {
        iron_runtime_type_t *type;

        type = iron_type_resolve_token(assembly->module, IRON_MAKE_TOKEN(IRON_TABLE_TYPE_DEF, index + 1));
        if (type && (!type->name || strcmp(type->name, "<Module>") != 0)) {
            output_count++;
        }
    }

    array = iron_gc_alloc_array(ctx, ctx->domain->type_type, output_count);
    if (!array) {
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate the reflected Type array");
    }

    elements = (void **)iron_array_get_data(array);
    output_count = 0;
    for (index = 0; index < metadata_count; index++) {
        iron_runtime_type_t *type;

        type = iron_type_resolve_token(assembly->module, IRON_MAKE_TOKEN(IRON_TABLE_TYPE_DEF, index + 1));
        if (type && (!type->name || strcmp(type->name, "<Module>") != 0)) {
            elements[output_count++] = type;
        }
    }

    result->type = IRON_VAL_OBJ;
    result->value.obj = array;
    return IRON_SUCCESS;
}

iron_result_t icall_RuntimeAssembly_GetTypeCore(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_assembly_t *assembly;
    iron_result_t argument_result;
    char *requested_name;
    iron_bool ignore_case;
    iron_u32 metadata_count;
    iron_u32 index;

    if (!args || arg_count < 3 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Assembly.GetTypeCore requires a name, comparison mode, and result slot");
    }

    argument_result = get_assembly_argument(args, arg_count, &assembly);
    if (!IRON_RESULT_OK(argument_result)) {
        return argument_result;
    }

    requested_name = iron_string_to_utf8(ctx, iron_corlib_object_argument(&args[1]));
    if (!requested_name) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Assembly.GetTypeCore received an invalid name");
    }

    ignore_case = args[2].value.i32 != 0;
    result->type = IRON_VAL_OBJ;
    result->value.obj = NULL;
    metadata_count = iron_metadata_table_rows(&assembly->metadata, IRON_TABLE_TYPE_DEF);
    for (index = 0; index < metadata_count; index++) {
        iron_runtime_type_t *type;

        type = iron_type_resolve_token(assembly->module, IRON_MAKE_TOKEN(IRON_TABLE_TYPE_DEF, index + 1));
        if (type && ascii_names_equal(type->full_name, requested_name, ignore_case)) {
            result->value.obj = type;
            break;
        }
    }

    iron_free(ctx->allocator, requested_name, strlen(requested_name) + 1);
    return IRON_SUCCESS;
}

enum {
    REFLECTION_BIND_IGNORE_CASE = 0x01,
    REFLECTION_BIND_DECLARED_ONLY = 0x02,
    REFLECTION_BIND_INSTANCE = 0x04,
    REFLECTION_BIND_STATIC = 0x08,
    REFLECTION_BIND_PUBLIC = 0x10,
    REFLECTION_BIND_NON_PUBLIC = 0x20,
    REFLECTION_BIND_FLATTEN_HIERARCHY = 0x40
};

static void *reflection_descriptor_from_object(void *managed_object)
{
    iron_runtime_type_t *managed_type;
    iron_runtime_field_t *handle_field;
    void *descriptor;

    if (!managed_object) {
        return NULL;
    }

    managed_type = iron_gc_get_type(managed_object);
    handle_field = iron_type_find_instance_field(managed_type, "_handle");
    if (!handle_field || handle_field->size < sizeof(descriptor)) {
        return NULL;
    }

    descriptor = NULL;
    memcpy(&descriptor, (const iron_u8 *)managed_object + handle_field->offset, sizeof(descriptor));
    return descriptor;
}

static void *create_reflection_object(iron_exec_context_t *ctx, const char *managed_type_name, void *descriptor)
{
    iron_runtime_type_t *managed_type;
    iron_runtime_field_t *handle_field;
    void *managed_object;

    if (!ctx || !managed_type_name || !descriptor) {
        return NULL;
    }

    managed_type = iron_domain_find_type(ctx->domain, managed_type_name);
    if (!managed_type || !IRON_RESULT_OK(iron_type_compute_layout(managed_type))) {
        return NULL;
    }

    handle_field = iron_type_find_instance_field(managed_type, "_handle");
    if (!handle_field || handle_field->size < sizeof(descriptor)) {
        return NULL;
    }

    managed_object = iron_gc_alloc_object(&ctx->gc, managed_type, managed_type->instance_size);
    if (!managed_object) {
        return NULL;
    }

    memcpy((iron_u8 *)managed_object + handle_field->offset, &descriptor, sizeof(descriptor));
    return managed_object;
}

void *iron_reflection_create_method_info(iron_exec_context_t *ctx, iron_runtime_method_t *method)
{
    const char *managed_type_name;

    managed_type_name = method && method->kind == IRON_METHOD_CONSTRUCTOR ? "System.Reflection.RuntimeConstructorInfo" : "System.Reflection.RuntimeMethodInfo";
    return create_reflection_object(ctx, managed_type_name, method);
}

static void *create_field_info(iron_exec_context_t *ctx, iron_runtime_field_t *field)
{
    return create_reflection_object(ctx, "System.Reflection.RuntimeFieldInfo", field);
}

static void *create_property_info(iron_exec_context_t *ctx, iron_runtime_property_t *property)
{
    return create_reflection_object(ctx, "System.Reflection.RuntimePropertyInfo", property);
}

static void *create_event_info(iron_exec_context_t *ctx, iron_runtime_event_t *event)
{
    return create_reflection_object(ctx, "System.Reflection.RuntimeEventInfo", event);
}

static void *create_parameter_info(iron_exec_context_t *ctx, iron_runtime_param_t *parameter)
{
    return create_reflection_object(ctx, "System.Reflection.RuntimeParameterInfo", parameter);
}

static void set_reference_array_element(void *array, iron_u32 index, void *value)
{
    void **elements;

    elements = (void **)iron_array_get_data(array);
    elements[index] = value;
    iron_gc_write_barrier(array, &elements[index], value);
}

static iron_bool member_visibility_matches(iron_u16 attributes, iron_u32 binding_flags)
{
    iron_bool is_public;

    is_public = (attributes & IRON_METHOD_ACCESS_MASK) == IRON_METHOD_PUBLIC;
    return is_public ? (binding_flags & REFLECTION_BIND_PUBLIC) != 0 : (binding_flags & REFLECTION_BIND_NON_PUBLIC) != 0;
}

static iron_bool member_scope_matches(iron_u16 attributes, iron_u32 binding_flags, iron_bool inherited)
{
    iron_bool is_static;

    is_static = (attributes & IRON_METHOD_STATIC) != 0;
    if (is_static) {
        if ((binding_flags & REFLECTION_BIND_STATIC) == 0) {
            return IRON_FALSE;
        }
        if (inherited && (binding_flags & REFLECTION_BIND_FLATTEN_HIERARCHY) == 0) {
            return IRON_FALSE;
        }
    } else if ((binding_flags & REFLECTION_BIND_INSTANCE) == 0) {
        return IRON_FALSE;
    }

    if (inherited && (attributes & IRON_METHOD_ACCESS_MASK) == IRON_METHOD_PRIVATE) {
        return IRON_FALSE;
    }
    return IRON_TRUE;
}

static iron_bool method_signatures_equal(iron_runtime_method_t *left, iron_runtime_method_t *right)
{
    iron_u32 parameter_index;

    if (!left || !right || strcmp(left->name, right->name) != 0 || left->param_count != right->param_count ||
        ((left->attrs ^ right->attrs) & IRON_METHOD_STATIC) != 0) {
        return IRON_FALSE;
    }
    if (!IRON_RESULT_OK(iron_method_load_signature(left)) || !IRON_RESULT_OK(iron_method_load_signature(right))) {
        return IRON_FALSE;
    }

    for (parameter_index = 0; parameter_index < left->param_count; parameter_index++) {
        if (left->params[parameter_index]->param_type != right->params[parameter_index]->param_type) {
            return IRON_FALSE;
        }
    }
    return IRON_TRUE;
}

static iron_bool method_is_hidden(iron_runtime_method_t **methods, iron_u32 method_count, iron_runtime_method_t *candidate)
{
    iron_u32 method_index;

    for (method_index = 0; method_index < method_count; method_index++) {
        if (method_signatures_equal(methods[method_index], candidate)) {
            return IRON_TRUE;
        }
    }
    return IRON_FALSE;
}

static iron_result_t collect_methods(iron_exec_context_t *ctx,
                                     iron_runtime_type_t *type,
                                     iron_u32 binding_flags,
                                     iron_bool constructors,
                                     iron_stack_value_t *result)
{
    iron_runtime_type_t *current;
    iron_runtime_method_t **methods;
    iron_runtime_type_t *array_element_type;
    iron_u32 maximum_count;
    iron_u32 method_count;
    iron_u32 method_index;
    iron_bool inherited;
    void *array;

    maximum_count = 0;
    for (current = type; current; current = constructors || (binding_flags & REFLECTION_BIND_DECLARED_ONLY) != 0 ? NULL : current->base_type) {
        maximum_count += current->method_count;
    }

    methods = NULL;
    if (maximum_count != 0) {
        methods = (iron_runtime_method_t **)iron_alloc(ctx->allocator, (iron_size)maximum_count * sizeof(iron_runtime_method_t *));
        if (!methods) {
            return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate the reflection method collection");
        }
    }

    method_count = 0;
    inherited = IRON_FALSE;
    for (current = type; current; current = constructors || (binding_flags & REFLECTION_BIND_DECLARED_ONLY) != 0 ? NULL : current->base_type) {
        for (method_index = 0; method_index < current->method_count; method_index++) {
            iron_runtime_method_t *method;
            iron_bool is_constructor;

            method = current->methods[method_index];
            if (!method || !method->name) {
                continue;
            }

            is_constructor = method->kind == IRON_METHOD_CONSTRUCTOR || strcmp(method->name, ".ctor") == 0;
            if (is_constructor != constructors || strcmp(method->name, ".cctor") == 0 || !member_visibility_matches(method->attrs, binding_flags) ||
                !member_scope_matches(method->attrs, binding_flags, inherited) || (!constructors && inherited && method_is_hidden(methods, method_count, method))) {
                continue;
            }

            methods[method_count++] = method;
        }
        inherited = IRON_TRUE;
    }

    array_element_type = iron_domain_find_type(ctx->domain, constructors ? "System.Reflection.ConstructorInfo" : "System.Reflection.MethodInfo");
    array = array_element_type ? iron_gc_alloc_array(ctx, array_element_type, method_count) : NULL;
    if (!array) {
        if (methods) {
            iron_free(ctx->allocator, methods, (iron_size)maximum_count * sizeof(iron_runtime_method_t *));
        }
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate the reflection method array");
    }

    for (method_index = 0; method_index < method_count; method_index++) {
        void *method_info;

        method_info = iron_reflection_create_method_info(ctx, methods[method_index]);
        if (!method_info) {
            if (methods) {
                iron_free(ctx->allocator, methods, (iron_size)maximum_count * sizeof(iron_runtime_method_t *));
            }
            return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate a reflected method object");
        }
        set_reference_array_element(array, method_index, method_info);
    }

    if (methods) {
        iron_free(ctx->allocator, methods, (iron_size)maximum_count * sizeof(iron_runtime_method_t *));
    }
    result->type = IRON_VAL_OBJ;
    result->value.obj = array;
    return IRON_SUCCESS;
}

iron_result_t icall_Type_GetMethodsCore(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_type_t *type;

    if (!args || arg_count < 2 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Type.GetMethods requires binding flags and a result slot");
    }

    type = (iron_runtime_type_t *)iron_corlib_object_argument(&args[0]);
    if (!type) {
        return IRON_ERROR(IRON_ERR_NULL_REFERENCE, "Type.GetMethods requires an instance");
    }
    return collect_methods(ctx, type, (iron_u32)args[1].value.i32, IRON_FALSE, result);
}

iron_result_t icall_Type_GetConstructors(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_type_t *type;

    if (!args || arg_count < 2 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Type.GetConstructors requires binding flags and a result slot");
    }

    type = (iron_runtime_type_t *)iron_corlib_object_argument(&args[0]);
    if (!type) {
        return IRON_ERROR(IRON_ERR_NULL_REFERENCE, "Type.GetConstructors requires an instance");
    }
    return collect_methods(ctx, type, (iron_u32)args[1].value.i32 | REFLECTION_BIND_DECLARED_ONLY, IRON_TRUE, result);
}

static iron_bool field_is_hidden(iron_runtime_field_t **fields, iron_u32 field_count, iron_runtime_field_t *candidate)
{
    iron_u32 field_index;

    for (field_index = 0; field_index < field_count; field_index++) {
        if (fields[field_index]->name && candidate->name && strcmp(fields[field_index]->name, candidate->name) == 0) {
            return IRON_TRUE;
        }
    }
    return IRON_FALSE;
}

iron_result_t icall_Type_GetFields(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_type_t *type;
    iron_runtime_type_t *current;
    iron_runtime_field_t **fields;
    iron_runtime_type_t *array_element_type;
    iron_u32 binding_flags;
    iron_u32 maximum_count;
    iron_u32 field_count;
    iron_u32 field_index;
    iron_bool inherited;
    void *array;

    if (!args || arg_count < 2 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Type.GetFields requires binding flags and a result slot");
    }

    type = (iron_runtime_type_t *)iron_corlib_object_argument(&args[0]);
    if (!type) {
        return IRON_ERROR(IRON_ERR_NULL_REFERENCE, "Type.GetFields requires an instance");
    }

    binding_flags = (iron_u32)args[1].value.i32;
    maximum_count = 0;
    for (current = type; current; current = (binding_flags & REFLECTION_BIND_DECLARED_ONLY) != 0 ? NULL : current->base_type) {
        maximum_count += current->field_count;
    }

    fields = NULL;
    if (maximum_count != 0) {
        fields = (iron_runtime_field_t **)iron_alloc(ctx->allocator, (iron_size)maximum_count * sizeof(iron_runtime_field_t *));
        if (!fields) {
            return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate the reflection field collection");
        }
    }

    field_count = 0;
    inherited = IRON_FALSE;
    for (current = type; current; current = (binding_flags & REFLECTION_BIND_DECLARED_ONLY) != 0 ? NULL : current->base_type) {
        for (field_index = 0; field_index < current->field_count; field_index++) {
            iron_runtime_field_t *field;

            field = current->fields[field_index];
            if (!field || !field->name || !member_visibility_matches(field->attrs, binding_flags) || !member_scope_matches(field->attrs, binding_flags, inherited) ||
                (inherited && field_is_hidden(fields, field_count, field))) {
                continue;
            }
            fields[field_count++] = field;
        }
        inherited = IRON_TRUE;
    }

    array_element_type = iron_domain_find_type(ctx->domain, "System.Reflection.FieldInfo");
    array = array_element_type ? iron_gc_alloc_array(ctx, array_element_type, field_count) : NULL;
    if (!array) {
        if (fields) {
            iron_free(ctx->allocator, fields, (iron_size)maximum_count * sizeof(iron_runtime_field_t *));
        }
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate the reflection field array");
    }

    for (field_index = 0; field_index < field_count; field_index++) {
        void *field_info;

        field_info = create_field_info(ctx, fields[field_index]);
        if (!field_info) {
            if (fields) {
                iron_free(ctx->allocator, fields, (iron_size)maximum_count * sizeof(iron_runtime_field_t *));
            }
            return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate a reflected field object");
        }
        set_reference_array_element(array, field_index, field_info);
    }

    if (fields) {
        iron_free(ctx->allocator, fields, (iron_size)maximum_count * sizeof(iron_runtime_field_t *));
    }
    result->type = IRON_VAL_OBJ;
    result->value.obj = array;
    return IRON_SUCCESS;
}

static iron_bool property_binding_matches(iron_runtime_property_t *property, iron_u32 binding_flags, iron_bool inherited)
{
    iron_runtime_method_t *accessors[2];
    iron_u32 accessor_index;

    accessors[0] = property->getter;
    accessors[1] = property->setter;
    for (accessor_index = 0; accessor_index < 2; accessor_index++) {
        iron_runtime_method_t *accessor;

        accessor = accessors[accessor_index];
        if (accessor && member_visibility_matches(accessor->attrs, binding_flags) && member_scope_matches(accessor->attrs, binding_flags, inherited)) {
            return IRON_TRUE;
        }
    }
    return IRON_FALSE;
}

static iron_bool property_signatures_equal(iron_runtime_property_t *left, iron_runtime_property_t *right)
{
    iron_u32 parameter_index;

    if (!left || !right || !left->name || !right->name || strcmp(left->name, right->name) != 0 ||
        !IRON_RESULT_OK(iron_property_load_signature(left)) || !IRON_RESULT_OK(iron_property_load_signature(right)) ||
        left->property_type != right->property_type || left->index_param_count != right->index_param_count) {
        return IRON_FALSE;
    }

    for (parameter_index = 0; parameter_index < left->index_param_count; parameter_index++) {
        if (left->index_params[parameter_index]->param_type != right->index_params[parameter_index]->param_type) {
            return IRON_FALSE;
        }
    }
    return IRON_TRUE;
}

static iron_bool property_is_hidden(iron_runtime_property_t **properties, iron_u32 property_count, iron_runtime_property_t *candidate)
{
    iron_u32 property_index;

    for (property_index = 0; property_index < property_count; property_index++) {
        if (property_signatures_equal(properties[property_index], candidate)) {
            return IRON_TRUE;
        }
    }
    return IRON_FALSE;
}

iron_result_t icall_Type_GetProperties(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_type_t *type;
    iron_runtime_type_t *current;
    iron_runtime_property_t **properties;
    iron_runtime_type_t *array_element_type;
    iron_u32 binding_flags;
    iron_u32 maximum_count;
    iron_u32 property_count;
    iron_u32 property_index;
    iron_bool inherited;
    void *array;

    if (!args || arg_count < 2 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Type.GetProperties requires binding flags and a result slot");
    }
    type = (iron_runtime_type_t *)iron_corlib_object_argument(&args[0]);
    if (!type) {
        return IRON_ERROR(IRON_ERR_NULL_REFERENCE, "Type.GetProperties requires an instance");
    }

    binding_flags = (iron_u32)args[1].value.i32;
    maximum_count = 0;
    for (current = type; current; current = (binding_flags & REFLECTION_BIND_DECLARED_ONLY) != 0 ? NULL : current->base_type) {
        maximum_count += current->property_count;
    }

    properties = NULL;
    if (maximum_count != 0) {
        properties = (iron_runtime_property_t **)iron_alloc(ctx->allocator, (iron_size)maximum_count * sizeof(iron_runtime_property_t *));
        if (!properties) {
            return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate the reflection property collection");
        }
    }

    property_count = 0;
    inherited = IRON_FALSE;
    for (current = type; current; current = (binding_flags & REFLECTION_BIND_DECLARED_ONLY) != 0 ? NULL : current->base_type) {
        for (property_index = 0; property_index < current->property_count; property_index++) {
            iron_runtime_property_t *property;

            property = current->properties[property_index];
            if (!property || !property_binding_matches(property, binding_flags, inherited) ||
                (inherited && property_is_hidden(properties, property_count, property))) {
                continue;
            }
            properties[property_count++] = property;
        }
        inherited = IRON_TRUE;
    }

    array_element_type = iron_domain_find_type(ctx->domain, "System.Reflection.PropertyInfo");
    array = array_element_type ? iron_gc_alloc_array(ctx, array_element_type, property_count) : NULL;
    if (!array) {
        if (properties) {
            iron_free(ctx->allocator, properties, (iron_size)maximum_count * sizeof(iron_runtime_property_t *));
        }
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate the reflection property array");
    }

    for (property_index = 0; property_index < property_count; property_index++) {
        void *property_info;

        property_info = create_property_info(ctx, properties[property_index]);
        if (!property_info) {
            if (properties) {
                iron_free(ctx->allocator, properties, (iron_size)maximum_count * sizeof(iron_runtime_property_t *));
            }
            return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate a reflected property object");
        }
        set_reference_array_element(array, property_index, property_info);
    }

    if (properties) {
        iron_free(ctx->allocator, properties, (iron_size)maximum_count * sizeof(iron_runtime_property_t *));
    }
    result->type = IRON_VAL_OBJ;
    result->value.obj = array;
    return IRON_SUCCESS;
}

static iron_bool event_binding_matches(iron_runtime_event_t *event, iron_u32 binding_flags, iron_bool inherited)
{
    iron_runtime_method_t *accessors[3];
    iron_u32 accessor_index;

    accessors[0] = event->add_method;
    accessors[1] = event->remove_method;
    accessors[2] = event->raise_method;
    for (accessor_index = 0; accessor_index < 3; accessor_index++) {
        iron_runtime_method_t *accessor;

        accessor = accessors[accessor_index];
        if (accessor && member_visibility_matches(accessor->attrs, binding_flags) && member_scope_matches(accessor->attrs, binding_flags, inherited)) {
            return IRON_TRUE;
        }
    }
    return IRON_FALSE;
}

static iron_bool event_is_hidden(iron_runtime_event_t **events, iron_u32 event_count, iron_runtime_event_t *candidate)
{
    iron_u32 event_index;

    for (event_index = 0; event_index < event_count; event_index++) {
        if (events[event_index]->name && candidate->name && strcmp(events[event_index]->name, candidate->name) == 0 && events[event_index]->event_type == candidate->event_type) {
            return IRON_TRUE;
        }
    }
    return IRON_FALSE;
}

iron_result_t icall_Type_GetEvents(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_type_t *type;
    iron_runtime_type_t *current;
    iron_runtime_event_t **events;
    iron_runtime_type_t *array_element_type;
    iron_u32 binding_flags;
    iron_u32 maximum_count;
    iron_u32 event_count;
    iron_u32 event_index;
    iron_bool inherited;
    void *array;

    if (!args || arg_count < 2 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Type.GetEvents requires binding flags and a result slot");
    }
    type = (iron_runtime_type_t *)iron_corlib_object_argument(&args[0]);
    if (!type) {
        return IRON_ERROR(IRON_ERR_NULL_REFERENCE, "Type.GetEvents requires an instance");
    }

    binding_flags = (iron_u32)args[1].value.i32;
    maximum_count = 0;
    for (current = type; current; current = (binding_flags & REFLECTION_BIND_DECLARED_ONLY) != 0 ? NULL : current->base_type) {
        maximum_count += current->event_count;
    }

    events = NULL;
    if (maximum_count != 0) {
        events = (iron_runtime_event_t **)iron_alloc(ctx->allocator, (iron_size)maximum_count * sizeof(iron_runtime_event_t *));
        if (!events) {
            return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate the reflection event collection");
        }
    }

    event_count = 0;
    inherited = IRON_FALSE;
    for (current = type; current; current = (binding_flags & REFLECTION_BIND_DECLARED_ONLY) != 0 ? NULL : current->base_type) {
        for (event_index = 0; event_index < current->event_count; event_index++) {
            iron_runtime_event_t *event;

            event = current->events[event_index];
            if (!event || !event_binding_matches(event, binding_flags, inherited) || (inherited && event_is_hidden(events, event_count, event))) {
                continue;
            }
            events[event_count++] = event;
        }
        inherited = IRON_TRUE;
    }

    array_element_type = iron_domain_find_type(ctx->domain, "System.Reflection.EventInfo");
    array = array_element_type ? iron_gc_alloc_array(ctx, array_element_type, event_count) : NULL;
    if (!array) {
        if (events) {
            iron_free(ctx->allocator, events, (iron_size)maximum_count * sizeof(iron_runtime_event_t *));
        }
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate the reflection event array");
    }

    for (event_index = 0; event_index < event_count; event_index++) {
        void *event_info;

        event_info = create_event_info(ctx, events[event_index]);
        if (!event_info) {
            if (events) {
                iron_free(ctx->allocator, events, (iron_size)maximum_count * sizeof(iron_runtime_event_t *));
            }
            return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate a reflected event object");
        }
        set_reference_array_element(array, event_index, event_info);
    }

    if (events) {
        iron_free(ctx->allocator, events, (iron_size)maximum_count * sizeof(iron_runtime_event_t *));
    }
    result->type = IRON_VAL_OBJ;
    result->value.obj = array;
    return IRON_SUCCESS;
}

iron_result_t icall_Type_GetNestedTypes(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_type_t *type;
    iron_u32 binding_flags;
    iron_u32 nested_count;
    iron_u32 nested_index;
    void *array;

    if (!args || arg_count < 2 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Type.GetNestedTypes requires binding flags and a result slot");
    }
    type = (iron_runtime_type_t *)iron_corlib_object_argument(&args[0]);
    if (!type) {
        return IRON_ERROR(IRON_ERR_NULL_REFERENCE, "Type.GetNestedTypes requires an instance");
    }

    binding_flags = (iron_u32)args[1].value.i32;
    nested_count = 0;
    for (nested_index = 0; nested_index < type->nested_type_count; nested_index++) {
        iron_runtime_type_t *nested_type;
        iron_bool is_public;

        nested_type = type->nested_types[nested_index];
        is_public = nested_type && (nested_type->attrs & IRON_TYPE_VIS_MASK) == IRON_TYPE_NESTED_PUBLIC;
        if (nested_type && (is_public ? (binding_flags & REFLECTION_BIND_PUBLIC) != 0 : (binding_flags & REFLECTION_BIND_NON_PUBLIC) != 0)) {
            nested_count++;
        }
    }

    array = iron_gc_alloc_array(ctx, ctx->domain->type_type, nested_count);
    if (!array) {
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate the nested Type array");
    }

    nested_count = 0;
    for (nested_index = 0; nested_index < type->nested_type_count; nested_index++) {
        iron_runtime_type_t *nested_type;
        iron_bool is_public;

        nested_type = type->nested_types[nested_index];
        is_public = nested_type && (nested_type->attrs & IRON_TYPE_VIS_MASK) == IRON_TYPE_NESTED_PUBLIC;
        if (nested_type && (is_public ? (binding_flags & REFLECTION_BIND_PUBLIC) != 0 : (binding_flags & REFLECTION_BIND_NON_PUBLIC) != 0)) {
            set_reference_array_element(array, nested_count++, nested_type);
        }
    }

    result->type = IRON_VAL_OBJ;
    result->value.obj = array;
    return IRON_SUCCESS;
}

iron_result_t icall_Type_IsAssignableFrom(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_type_t *target;
    iron_runtime_type_t *source;

    (void)ctx;
    if (!args || arg_count < 2 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Type.IsAssignableFrom requires another Type and a result slot");
    }

    target = (iron_runtime_type_t *)iron_corlib_object_argument(&args[0]);
    source = (iron_runtime_type_t *)iron_corlib_object_argument(&args[1]);
    result->type = IRON_VAL_I32;
    result->value.i32 = target && source && iron_type_is_assignable_to(source, target) ? 1 : 0;
    return target ? IRON_SUCCESS : IRON_ERROR(IRON_ERR_NULL_REFERENCE, "Type.IsAssignableFrom requires an instance");
}

iron_result_t icall_Type_IsInstanceOfType(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_type_t *target;
    void *object;

    if (!args || arg_count < 2 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Type.IsInstanceOfType requires an object and a result slot");
    }

    target = (iron_runtime_type_t *)iron_corlib_object_argument(&args[0]);
    object = iron_corlib_object_argument(&args[1]);
    result->type = IRON_VAL_I32;
    result->value.i32 = target && object && iron_managed_reference_is_assignable(ctx->domain, object, target) ? 1 : 0;
    return target ? IRON_SUCCESS : IRON_ERROR(IRON_ERR_NULL_REFERENCE, "Type.IsInstanceOfType requires an instance");
}

static iron_runtime_method_t *get_method_info_argument(iron_stack_value_t *args, iron_u32 arg_count)
{
    return args && arg_count != 0 ? (iron_runtime_method_t *)reflection_descriptor_from_object(iron_corlib_object_argument(&args[0])) : NULL;
}

static iron_runtime_field_t *get_field_info_argument(iron_stack_value_t *args, iron_u32 arg_count)
{
    return args && arg_count != 0 ? (iron_runtime_field_t *)reflection_descriptor_from_object(iron_corlib_object_argument(&args[0])) : NULL;
}

static iron_runtime_param_t *get_parameter_info_argument(iron_stack_value_t *args, iron_u32 arg_count)
{
    return args && arg_count != 0 ? (iron_runtime_param_t *)reflection_descriptor_from_object(iron_corlib_object_argument(&args[0])) : NULL;
}

static iron_runtime_property_t *get_property_info_argument(iron_stack_value_t *args, iron_u32 arg_count)
{
    return args && arg_count != 0 ? (iron_runtime_property_t *)reflection_descriptor_from_object(iron_corlib_object_argument(&args[0])) : NULL;
}

static iron_runtime_event_t *get_event_info_argument(iron_stack_value_t *args, iron_u32 arg_count)
{
    return args && arg_count != 0 ? (iron_runtime_event_t *)reflection_descriptor_from_object(iron_corlib_object_argument(&args[0])) : NULL;
}

iron_result_t icall_RuntimeMethodInfo_get_Name(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_method_t *method;

    method = get_method_info_argument(args, arg_count);
    if (!method || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "MethodInfo.Name requires a valid instance and result slot");
    }
    result->type = IRON_VAL_OBJ;
    result->value.obj = iron_string_new_utf8(ctx, method->name ? method->name : "");
    return result->value.obj ? IRON_SUCCESS : IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate MethodInfo.Name");
}

iron_result_t icall_RuntimeMethodInfo_get_DeclaringType(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_method_t *method;

    (void)ctx;
    method = get_method_info_argument(args, arg_count);
    if (!method || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "MethodInfo.DeclaringType requires a valid instance and result slot");
    }
    result->type = IRON_VAL_OBJ;
    result->value.obj = method->declaring_type;
    return IRON_SUCCESS;
}

iron_result_t icall_RuntimeMethodInfo_get_Attributes(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_method_t *method;

    (void)ctx;
    method = get_method_info_argument(args, arg_count);
    if (!method || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "MethodInfo.Attributes requires a valid instance and result slot");
    }
    result->type = IRON_VAL_I32;
    result->value.i32 = (iron_i32)method->attrs;
    return IRON_SUCCESS;
}

iron_result_t icall_RuntimeMethodInfo_get_ReturnType(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_method_t *method;
    iron_result_t load_result;

    (void)ctx;
    method = get_method_info_argument(args, arg_count);
    if (!method || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "MethodInfo.ReturnType requires a valid instance and result slot");
    }
    load_result = iron_method_load_signature(method);
    if (!IRON_RESULT_OK(load_result)) {
        return load_result;
    }
    result->type = IRON_VAL_OBJ;
    result->value.obj = method->return_type;
    return IRON_SUCCESS;
}

iron_result_t icall_RuntimeMethodInfo_get_ContainsGenericParameters(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_method_t *method;
    iron_u32 argument_index;
    iron_bool contains_generic_parameters;

    (void)ctx;
    method = get_method_info_argument(args, arg_count);
    if (!method || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "MethodBase.ContainsGenericParameters requires a valid instance and result slot");
    }

    contains_generic_parameters = method->is_generic_definition || iron_type_contains_generic_parameters(method->declaring_type);
    for (argument_index = 0; !contains_generic_parameters && argument_index < method->generic_arg_count; argument_index++) {
        contains_generic_parameters = iron_type_contains_generic_parameters(method->generic_args[argument_index]);
    }

    result->type = IRON_VAL_I32;
    result->value.i32 = contains_generic_parameters ? 1 : 0;
    return IRON_SUCCESS;
}

iron_result_t icall_RuntimeMethodInfo_get_IsGenericMethod(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_method_t *method;

    (void)ctx;
    method = get_method_info_argument(args, arg_count);
    if (!method || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "MethodInfo.IsGenericMethod requires a valid instance and result slot");
    }

    result->type = IRON_VAL_I32;
    result->value.i32 = method->is_generic_definition || method->is_generic_instance ? 1 : 0;
    return IRON_SUCCESS;
}

iron_result_t icall_RuntimeMethodInfo_get_IsGenericMethodDefinition(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_method_t *method;

    (void)ctx;
    method = get_method_info_argument(args, arg_count);
    if (!method || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "MethodInfo.IsGenericMethodDefinition requires a valid instance and result slot");
    }

    result->type = IRON_VAL_I32;
    result->value.i32 = method->is_generic_definition ? 1 : 0;
    return IRON_SUCCESS;
}

iron_result_t icall_RuntimeMethodInfo_GetGenericArguments(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_method_t *method;
    iron_runtime_type_t **generic_arguments;
    iron_u32 generic_argument_count;

    method = get_method_info_argument(args, arg_count);
    if (!method || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "MethodInfo.GetGenericArguments requires a valid instance and result slot");
    }

    generic_arguments = method->is_generic_instance ? method->generic_args : method->generic_params;
    generic_argument_count = method->is_generic_instance ? method->generic_arg_count : method->generic_param_count;
    return iron_corlib_return_type_array(ctx, generic_arguments, generic_argument_count, result);
}

iron_result_t icall_RuntimeMethodInfo_GetGenericMethodDefinition(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_method_t *method;
    iron_runtime_method_t *definition;

    method = get_method_info_argument(args, arg_count);
    if (!method || !result || (!method->is_generic_definition && !method->is_generic_instance)) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "MethodInfo.GetGenericMethodDefinition requires a generic method");
    }

    definition = method->is_generic_instance ? method->generic_definition : method;
    result->type = IRON_VAL_OBJ;
    result->value.obj = iron_reflection_create_method_info(ctx, definition);
    return result->value.obj ? IRON_SUCCESS : IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate a generic method definition wrapper");
}

iron_result_t icall_RuntimeMethodInfo_MakeGenericMethod(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_method_t *definition;
    iron_runtime_method_t *constructed_method;
    void *argument_array;
    void **argument_objects;
    iron_runtime_type_t **type_arguments;
    iron_u32 argument_count;
    iron_u32 argument_index;
    iron_result_t validation_result;

    definition = get_method_info_argument(args, arg_count);
    argument_array = args && arg_count >= 2 ? iron_corlib_object_argument(&args[1]) : NULL;
    if (!definition || !definition->is_generic_definition || !argument_array || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "MethodInfo.MakeGenericMethod requires a generic method definition and a non-null argument array");
    }

    argument_count = iron_array_get_length(argument_array);
    if (argument_count != definition->generic_param_count) {
        return IRON_ERROR(IRON_ERR_INVALID_GENERIC_ARGS, "Generic method argument count does not match the definition");
    }

    argument_objects = (void **)iron_array_get_data(argument_array);
    type_arguments = (iron_runtime_type_t **)iron_alloc(ctx->allocator, (iron_size)argument_count * sizeof(iron_runtime_type_t *));
    if (!type_arguments) {
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate generic method argument storage");
    }

    for (argument_index = 0; argument_index < argument_count; argument_index++) {
        if (!iron_domain_is_type_descriptor(ctx->domain, argument_objects[argument_index])) {
            iron_free(ctx->allocator, type_arguments, (iron_size)argument_count * sizeof(iron_runtime_type_t *));
            return IRON_ERROR(IRON_ERR_INVALID_GENERIC_ARGS, "Generic method arguments must contain valid Type values");
        }
        type_arguments[argument_index] = (iron_runtime_type_t *)argument_objects[argument_index];
    }

    validation_result = iron_method_validate_generic_arguments(definition, type_arguments, argument_count);
    if (!IRON_RESULT_OK(validation_result)) {
        iron_free(ctx->allocator, type_arguments, (iron_size)argument_count * sizeof(iron_runtime_type_t *));
        return validation_result;
    }

    constructed_method = iron_method_make_generic(ctx->domain, definition, type_arguments, argument_count);
    iron_free(ctx->allocator, type_arguments, (iron_size)argument_count * sizeof(iron_runtime_type_t *));
    if (!constructed_method) {
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to construct a generic method");
    }

    result->type = IRON_VAL_OBJ;
    result->value.obj = iron_reflection_create_method_info(ctx, constructed_method);
    return result->value.obj ? IRON_SUCCESS : IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate a constructed generic method wrapper");
}

iron_result_t icall_RuntimeMethodInfo_GetParameters(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_method_t *method;
    iron_runtime_type_t *parameter_info_type;
    iron_result_t load_result;
    void *array;
    iron_u32 parameter_index;

    method = get_method_info_argument(args, arg_count);
    if (!method || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "MethodInfo.GetParameters requires a valid instance and result slot");
    }
    load_result = iron_method_load_signature(method);
    if (!IRON_RESULT_OK(load_result)) {
        return load_result;
    }

    parameter_info_type = iron_domain_find_type(ctx->domain, "System.Reflection.ParameterInfo");
    array = parameter_info_type ? iron_gc_alloc_array(ctx, parameter_info_type, method->param_count) : NULL;
    if (!array) {
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate the reflected parameter array");
    }

    for (parameter_index = 0; parameter_index < method->param_count; parameter_index++) {
        void *parameter_info;

        parameter_info = create_parameter_info(ctx, method->params[parameter_index]);
        if (!parameter_info) {
            return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate a reflected parameter object");
        }
        set_reference_array_element(array, parameter_index, parameter_info);
    }

    result->type = IRON_VAL_OBJ;
    result->value.obj = array;
    return IRON_SUCCESS;
}

static iron_result_t managed_object_to_stack_value(iron_exec_context_t *ctx,
                                                   void *object,
                                                   iron_runtime_type_t *expected_type,
                                                   iron_bool allow_null_value_type,
                                                   iron_stack_value_t *value)
{
    iron_runtime_type_t *object_type;

    if (!ctx || !expected_type || !value) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Invalid reflected argument conversion");
    }

    memset(value, 0, sizeof(*value));
    if (iron_type_is_managed_reference(expected_type)) {
        if (object) {
            object_type = iron_managed_reference_get_type(ctx->domain, object);
            if (!object_type || !iron_type_is_assignable_to(object_type, expected_type)) {
                return IRON_ERROR(IRON_ERR_INVALID_CAST, "Reflected argument is not assignable to the parameter type");
            }
        }

        value->type = IRON_VAL_OBJ;
        value->value.obj = object;
        return IRON_SUCCESS;
    }

    if (!object) {
        if (!allow_null_value_type || !iron_stack_value_init_default(ctx, value, expected_type)) {
            return IRON_ERROR(IRON_ERR_INVALID_CAST, "A null object cannot be converted to the requested value type");
        }
        return IRON_SUCCESS;
    }

    object_type = iron_managed_reference_get_type(ctx->domain, object);
    if (object_type != expected_type) {
        return IRON_ERROR(IRON_ERR_INVALID_CAST, "Boxed reflected argument has the wrong value type");
    }
    if (!iron_stack_value_load_from_storage(ctx, value, object, expected_type)) {
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to materialize a reflected value argument");
    }
    return IRON_SUCCESS;
}

static void free_reflection_invoke_buffers(iron_exec_context_t *ctx,
                                           iron_stack_value_t *invoke_args,
                                           iron_u32 invoke_arg_count,
                                           void **byref_storage,
                                           iron_runtime_type_t **byref_types,
                                           iron_gc_handle_t **byref_handles,
                                           iron_u32 parameter_count)
{
    iron_u32 parameter_index;

    if (byref_storage) {
        for (parameter_index = 0; parameter_index < parameter_count; parameter_index++) {
            if (byref_storage[parameter_index]) {
                if (byref_handles && byref_handles[parameter_index]) {
                    iron_gc_handle_free(&ctx->gc, byref_handles[parameter_index]);
                } else {
                    iron_free(ctx->allocator, byref_storage[parameter_index], iron_type_storage_size(byref_types[parameter_index]));
                }
            }
        }
        iron_free(ctx->allocator, byref_storage, (iron_size)parameter_count * sizeof(void *));
    }
    if (byref_types) {
        iron_free(ctx->allocator, byref_types, (iron_size)parameter_count * sizeof(iron_runtime_type_t *));
    }
    if (byref_handles) {
        iron_free(ctx->allocator, byref_handles, (iron_size)parameter_count * sizeof(iron_gc_handle_t *));
    }
    if (invoke_args) {
        iron_free(ctx->allocator, invoke_args, (iron_size)invoke_arg_count * sizeof(iron_stack_value_t));
    }
}

iron_result_t icall_RuntimeMethodInfo_Invoke(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_method_t *method;
    iron_runtime_method_t *implementation;
    iron_runtime_type_t *target_type;
    iron_stack_value_t *invoke_args;
    iron_stack_value_t invoke_result;
    iron_result_t operation_result;
    void *target;
    void *parameter_array;
    void **parameter_objects;
    void **byref_storage;
    iron_runtime_type_t **byref_types;
    iron_gc_handle_t **byref_handles;
    iron_u32 parameter_count;
    iron_u32 invoke_arg_count;
    iron_u32 first_parameter;
    iron_u32 parameter_index;
    iron_bool is_static;
    iron_bool is_constructor;

    if (!args || arg_count < 3 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "MethodInfo.Invoke requires a target, parameter array, and result slot");
    }

    method = get_method_info_argument(args, arg_count);
    if (!method) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Invalid RuntimeMethodInfo instance");
    }
    operation_result = iron_method_load_signature(method);
    if (!IRON_RESULT_OK(operation_result)) {
        return operation_result;
    }
    if (method->is_generic_definition) {
        return IRON_ERROR(IRON_ERR_INVALID_STATE, "An open generic method cannot be invoked");
    }

    target = iron_corlib_object_argument(&args[1]);
    parameter_array = iron_corlib_object_argument(&args[2]);
    parameter_count = parameter_array ? iron_array_get_length(parameter_array) : 0;
    if (parameter_count != method->param_count) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Reflection invocation parameter count does not match the method signature");
    }
    parameter_objects = parameter_array ? (void **)iron_array_get_data(parameter_array) : NULL;

    is_static = (method->attrs & IRON_METHOD_STATIC) != 0;
    is_constructor = method->kind == IRON_METHOD_CONSTRUCTOR || (method->name && strcmp(method->name, ".ctor") == 0);
    if (is_constructor && !target) {
        target = iron_gc_alloc_object(&ctx->gc, method->declaring_type, method->declaring_type->instance_size);
        if (!target) {
            return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate the reflected constructor target");
        }
    }
    if (!is_static) {
        if (!target) {
            return IRON_ERROR(IRON_ERR_NULL_REFERENCE, "An instance method requires a target object");
        }
        target_type = iron_managed_reference_get_type(ctx->domain, target);
        if (!target_type || !iron_type_is_assignable_to(target_type, method->declaring_type)) {
            return IRON_ERROR(IRON_ERR_INVALID_CAST, "Reflection invocation target has the wrong type");
        }
    }

    first_parameter = is_static ? 0 : 1;
    invoke_arg_count = method->param_count + first_parameter;
    invoke_args = NULL;
    byref_storage = NULL;
    byref_types = NULL;
    byref_handles = NULL;
    if (invoke_arg_count != 0) {
        invoke_args = (iron_stack_value_t *)iron_alloc(ctx->allocator, (iron_size)invoke_arg_count * sizeof(iron_stack_value_t));
        if (!invoke_args) {
            return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate reflection invocation arguments");
        }
        memset(invoke_args, 0, (iron_size)invoke_arg_count * sizeof(iron_stack_value_t));
    }
    if (method->param_count != 0) {
        byref_storage = (void **)iron_alloc(ctx->allocator, (iron_size)method->param_count * sizeof(void *));
        if (byref_storage) {
            memset(byref_storage, 0, (iron_size)method->param_count * sizeof(void *));
        }
        byref_types = (iron_runtime_type_t **)iron_alloc(ctx->allocator, (iron_size)method->param_count * sizeof(iron_runtime_type_t *));
        if (byref_types) {
            memset(byref_types, 0, (iron_size)method->param_count * sizeof(iron_runtime_type_t *));
        }
        byref_handles = (iron_gc_handle_t **)iron_alloc(ctx->allocator, (iron_size)method->param_count * sizeof(iron_gc_handle_t *));
        if (byref_handles) {
            memset(byref_handles, 0, (iron_size)method->param_count * sizeof(iron_gc_handle_t *));
        }
        if (!byref_storage || !byref_types || !byref_handles) {
            free_reflection_invoke_buffers(ctx, invoke_args, invoke_arg_count, byref_storage, byref_types, byref_handles, method->param_count);
            return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate reflection by-reference argument metadata");
        }
    }

    if (!is_static) {
        invoke_args[0].type = IRON_VAL_OBJ;
        invoke_args[0].value.obj = target;
    }

    for (parameter_index = 0; parameter_index < method->param_count; parameter_index++) {
        iron_runtime_type_t *parameter_type;

        parameter_type = method->params[parameter_index]->param_type;
        if (parameter_type->kind == IRON_KIND_BYREF && parameter_type->element) {
            iron_stack_value_t initial_value;
            iron_size storage_size;

            byref_types[parameter_index] = parameter_type->element;
            storage_size = iron_type_storage_size(parameter_type->element);
            operation_result = managed_object_to_stack_value(ctx, parameter_objects[parameter_index], parameter_type->element, IRON_TRUE, &initial_value);
            if (!IRON_RESULT_OK(operation_result)) {
                free_reflection_invoke_buffers(ctx, invoke_args, invoke_arg_count, byref_storage, byref_types, byref_handles, method->param_count);
                return operation_result;
            }

            if (iron_type_is_managed_reference(parameter_type->element)) {
                byref_handles[parameter_index] = iron_gc_handle_alloc(&ctx->gc, initial_value.value.obj, IRON_GC_HANDLE_NORMAL);
                if (!byref_handles[parameter_index]) {
                    free_reflection_invoke_buffers(ctx, invoke_args, invoke_arg_count, byref_storage, byref_types, byref_handles, method->param_count);
                    return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to root a reflection by-reference argument");
                }
                byref_storage[parameter_index] = &byref_handles[parameter_index]->target;
            } else {
                byref_storage[parameter_index] = iron_alloc(ctx->allocator, storage_size);
                if (!byref_storage[parameter_index]) {
                    free_reflection_invoke_buffers(ctx, invoke_args, invoke_arg_count, byref_storage, byref_types, byref_handles, method->param_count);
                    return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate reflection by-reference argument storage");
                }
                memset(byref_storage[parameter_index], 0, storage_size);
                if (!iron_stack_value_store_to_storage(NULL, byref_storage[parameter_index], parameter_type->element, &initial_value)) {
                    free_reflection_invoke_buffers(ctx, invoke_args, invoke_arg_count, byref_storage, byref_types, byref_handles, method->param_count);
                    return IRON_ERROR(IRON_ERR_INVALID_CAST, "Failed to initialize a reflection by-reference argument");
                }
            }

            invoke_args[first_parameter + parameter_index].type = IRON_VAL_BYREF;
            invoke_args[first_parameter + parameter_index].value.byref.ptr = byref_storage[parameter_index];
        } else {
            operation_result = managed_object_to_stack_value(ctx, parameter_objects[parameter_index], parameter_type, IRON_FALSE, &invoke_args[first_parameter + parameter_index]);
            if (!IRON_RESULT_OK(operation_result)) {
                free_reflection_invoke_buffers(ctx, invoke_args, invoke_arg_count, byref_storage, byref_types, byref_handles, method->param_count);
                return operation_result;
            }
        }
    }

    implementation = method;
    if (!is_static && !is_constructor && (method->attrs & IRON_METHOD_VIRTUAL) != 0) {
        iron_runtime_method_t *resolved_method;

        resolved_method = iron_exec_resolve_virtual(iron_managed_reference_get_type(ctx->domain, target), method);
        if (resolved_method) {
            implementation = resolved_method;
        }
    }

    memset(&invoke_result, 0, sizeof(invoke_result));
    invoke_result.type = IRON_VAL_VOID;
    operation_result = iron_exec_method(ctx, implementation, invoke_args, invoke_arg_count, &invoke_result);
    if (!IRON_RESULT_OK(operation_result)) {
        free_reflection_invoke_buffers(ctx, invoke_args, invoke_arg_count, byref_storage, byref_types, byref_handles, method->param_count);
        return operation_result;
    }

    for (parameter_index = 0; parameter_index < method->param_count; parameter_index++) {
        if (byref_storage[parameter_index]) {
            iron_stack_value_t updated_value;
            void *boxed_value;

            if (!iron_stack_value_load_from_storage(ctx, &updated_value, byref_storage[parameter_index], byref_types[parameter_index])) {
                free_reflection_invoke_buffers(ctx, invoke_args, invoke_arg_count, byref_storage, byref_types, byref_handles, method->param_count);
                return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to materialize a reflection by-reference result");
            }
            boxed_value = iron_stack_value_box(ctx, byref_types[parameter_index], &updated_value);
            if (!boxed_value && !iron_type_is_managed_reference(byref_types[parameter_index])) {
                free_reflection_invoke_buffers(ctx, invoke_args, invoke_arg_count, byref_storage, byref_types, byref_handles, method->param_count);
                return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to box a reflection by-reference result");
            }
            set_reference_array_element(parameter_array, parameter_index, boxed_value);
        }
    }

    result->type = IRON_VAL_OBJ;
    if (is_constructor) {
        result->value.obj = invoke_result.type == IRON_VAL_OBJ && invoke_result.value.obj ? invoke_result.value.obj : target;
    } else {
        result->value.obj = iron_stack_value_box(ctx, method->return_type, &invoke_result);
        if (method->return_type->element_type != IRON_TYPE_VOID && !result->value.obj && !iron_type_is_managed_reference(method->return_type)) {
            free_reflection_invoke_buffers(ctx, invoke_args, invoke_arg_count, byref_storage, byref_types, byref_handles, method->param_count);
            return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to box a reflection invocation result");
        }
    }

    free_reflection_invoke_buffers(ctx, invoke_args, invoke_arg_count, byref_storage, byref_types, byref_handles, method->param_count);
    return IRON_SUCCESS;
}

iron_result_t icall_RuntimeFieldInfo_get_Name(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_field_t *field;

    field = get_field_info_argument(args, arg_count);
    if (!field || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "FieldInfo.Name requires a valid instance and result slot");
    }
    result->type = IRON_VAL_OBJ;
    result->value.obj = iron_string_new_utf8(ctx, field->name ? field->name : "");
    return result->value.obj ? IRON_SUCCESS : IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate FieldInfo.Name");
}

iron_result_t icall_RuntimeFieldInfo_get_DeclaringType(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_field_t *field;

    (void)ctx;
    field = get_field_info_argument(args, arg_count);
    if (!field || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "FieldInfo.DeclaringType requires a valid instance and result slot");
    }
    result->type = IRON_VAL_OBJ;
    result->value.obj = field->declaring_type;
    return IRON_SUCCESS;
}

iron_result_t icall_RuntimeFieldInfo_get_FieldType(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_field_t *field;

    (void)ctx;
    field = get_field_info_argument(args, arg_count);
    if (!field || !field->field_type || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "FieldInfo.FieldType requires a valid instance and result slot");
    }
    result->type = IRON_VAL_OBJ;
    result->value.obj = field->field_type;
    return IRON_SUCCESS;
}

iron_result_t icall_RuntimeFieldInfo_get_Attributes(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_field_t *field;

    (void)ctx;
    field = get_field_info_argument(args, arg_count);
    if (!field || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "FieldInfo.Attributes requires a valid instance and result slot");
    }
    result->type = IRON_VAL_I32;
    result->value.i32 = (iron_i32)field->attrs;
    return IRON_SUCCESS;
}

static iron_result_t get_reflected_field_address(iron_exec_context_t *ctx, iron_runtime_field_t *field, void *target, void **address)
{
    if ((field->attrs & IRON_FIELD_STATIC) != 0) {
        iron_result_t initialization_result;

        initialization_result = iron_type_init_static(field->declaring_type, ctx);
        if (!IRON_RESULT_OK(initialization_result)) {
            return initialization_result;
        }
        *address = iron_field_get_address(field, NULL);
    } else {
        iron_runtime_type_t *target_type;

        if (!target) {
            return IRON_ERROR(IRON_ERR_NULL_REFERENCE, "An instance field requires a target object");
        }
        target_type = iron_managed_reference_get_type(ctx->domain, target);
        if (!target_type || !iron_type_is_assignable_to(target_type, field->declaring_type)) {
            return IRON_ERROR(IRON_ERR_INVALID_CAST, "Reflection field target has the wrong type");
        }
        *address = iron_field_get_address(field, target);
    }

    return *address ? IRON_SUCCESS : IRON_ERROR(IRON_ERR_INVALID_STATE, "Cannot resolve reflected field storage");
}

static iron_result_t create_metadata_string(iron_exec_context_t *ctx,
                                            const iron_u8 *data,
                                            iron_u32 data_size,
                                            void **result)
{
    iron_u16 *characters;
    iron_u32 character_count;
    iron_u32 character_index;

    if (!ctx || !result || (data_size != 0 && !data) || (data_size % sizeof(iron_u16)) != 0) {
        return IRON_ERROR(IRON_ERR_INVALID_SIGNATURE, "Invalid UTF-16 metadata constant");
    }

    character_count = data_size / sizeof(iron_u16);
    characters = NULL;
    if (character_count != 0) {
        characters = (iron_u16 *)iron_alloc(ctx->allocator, (iron_size)character_count * sizeof(iron_u16));
        if (!characters) {
            return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to decode a UTF-16 metadata constant");
        }
        for (character_index = 0; character_index < character_count; character_index++) {
            characters[character_index] = iron_read_u16_le(data + (iron_size)character_index * sizeof(iron_u16));
        }
    }

    *result = iron_string_new_utf16(ctx, characters, character_count);
    if (characters) {
        iron_free(ctx->allocator, characters, (iron_size)character_count * sizeof(iron_u16));
    }
    return *result ? IRON_SUCCESS : IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate a metadata constant string");
}

iron_result_t icall_RuntimeFieldInfo_GetValue(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_field_t *field;
    iron_stack_value_t field_value;
    iron_result_t address_result;
    void *address;

    if (!args || arg_count < 2 || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "FieldInfo.GetValue requires a target and result slot");
    }
    field = get_field_info_argument(args, arg_count);
    if (!field || !field->field_type) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Invalid RuntimeFieldInfo instance");
    }
    if ((field->attrs & IRON_FIELD_LITERAL) != 0) {
        if (!field->has_constant) {
            return IRON_ERROR(IRON_ERR_INVALID_STATE, "Literal field does not have constant metadata");
        }
        result->type = IRON_VAL_OBJ;
        result->value.obj = NULL;
        if (field->constant_type == IRON_TYPE_CLASS || field->constant_type == IRON_TYPE_OBJECT) {
            return IRON_SUCCESS;
        }
        if (field->constant_type == IRON_TYPE_STRING) {
            return create_metadata_string(ctx, field->constant_data, field->constant_data_size, &result->value.obj);
        }

        memset(&field_value, 0, sizeof(field_value));
        field_value.type = field->constant_type == IRON_TYPE_I8 || field->constant_type == IRON_TYPE_U8 ? IRON_VAL_I64 :
                           field->constant_type == IRON_TYPE_R4 ? IRON_VAL_F32 : field->constant_type == IRON_TYPE_R8 ? IRON_VAL_F64 : IRON_VAL_I32;
        field_value.value = field->constant_value;
        result->value.obj = iron_stack_value_box(ctx, field->field_type, &field_value);
        return result->value.obj ? IRON_SUCCESS : IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to box a literal field value");
    }

    address_result = get_reflected_field_address(ctx, field, iron_corlib_object_argument(&args[1]), &address);
    if (!IRON_RESULT_OK(address_result)) {
        return address_result;
    }
    if (!iron_stack_value_load_from_storage(ctx, &field_value, address, field->field_type)) {
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to materialize a reflected field value");
    }

    result->type = IRON_VAL_OBJ;
    result->value.obj = iron_stack_value_box(ctx, field->field_type, &field_value);
    return result->value.obj || iron_type_is_managed_reference(field->field_type) ? IRON_SUCCESS : IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to box a reflected field value");
}

iron_result_t icall_RuntimeFieldInfo_SetValue(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_field_t *field;
    iron_stack_value_t field_value;
    iron_result_t operation_result;
    void *target;
    void *address;
    void *value_object;

    (void)result;
    if (!args || arg_count < 3) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "FieldInfo.SetValue requires a target and value");
    }
    field = get_field_info_argument(args, arg_count);
    if (!field || !field->field_type) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "Invalid RuntimeFieldInfo instance");
    }
    if ((field->attrs & (IRON_FIELD_LITERAL | IRON_FIELD_INIT_ONLY)) != 0) {
        return IRON_ERROR(IRON_ERR_INVALID_STATE, "Literal and init-only fields cannot be changed through reflection");
    }

    target = iron_corlib_object_argument(&args[1]);
    value_object = iron_corlib_object_argument(&args[2]);
    operation_result = get_reflected_field_address(ctx, field, target, &address);
    if (!IRON_RESULT_OK(operation_result)) {
        return operation_result;
    }
    operation_result = managed_object_to_stack_value(ctx, value_object, field->field_type, IRON_TRUE, &field_value);
    if (!IRON_RESULT_OK(operation_result)) {
        return operation_result;
    }
    if (!iron_stack_value_store_to_storage((field->attrs & IRON_FIELD_STATIC) != 0 ? NULL : target, address, field->field_type, &field_value)) {
        return IRON_ERROR(IRON_ERR_INVALID_CAST, "Failed to store a reflected field value");
    }
    return IRON_SUCCESS;
}

iron_result_t icall_RuntimeParameterInfo_get_Name(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_param_t *parameter;

    parameter = get_parameter_info_argument(args, arg_count);
    if (!parameter || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "ParameterInfo.Name requires a valid instance and result slot");
    }
    result->type = IRON_VAL_OBJ;
    result->value.obj = iron_string_new_utf8(ctx, parameter->name ? parameter->name : "");
    return result->value.obj ? IRON_SUCCESS : IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate ParameterInfo.Name");
}

iron_result_t icall_RuntimeParameterInfo_get_ParameterType(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_param_t *parameter;

    (void)ctx;
    parameter = get_parameter_info_argument(args, arg_count);
    if (!parameter || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "ParameterInfo.ParameterType requires a valid instance and result slot");
    }
    result->type = IRON_VAL_OBJ;
    result->value.obj = parameter->param_type;
    return IRON_SUCCESS;
}

iron_result_t icall_RuntimeParameterInfo_get_Position(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_param_t *parameter;

    (void)ctx;
    parameter = get_parameter_info_argument(args, arg_count);
    if (!parameter || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "ParameterInfo.Position requires a valid instance and result slot");
    }
    result->type = IRON_VAL_I32;
    result->value.i32 = (iron_i32)parameter->sequence - 1;
    return IRON_SUCCESS;
}

iron_result_t icall_RuntimeParameterInfo_get_IsOptional(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_param_t *parameter;

    (void)ctx;
    parameter = get_parameter_info_argument(args, arg_count);
    if (!parameter || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "ParameterInfo.IsOptional requires a valid instance and result slot");
    }
    result->type = IRON_VAL_I32;
    result->value.i32 = (parameter->attrs & IRON_PARAM_OPTIONAL) != 0 ? 1 : 0;
    return IRON_SUCCESS;
}

iron_result_t icall_RuntimeParameterInfo_get_DefaultValue(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_param_t *parameter;
    iron_stack_value_t default_value;

    parameter = get_parameter_info_argument(args, arg_count);
    if (!parameter || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "ParameterInfo.DefaultValue requires a valid instance and result slot");
    }

    result->type = IRON_VAL_OBJ;
    result->value.obj = NULL;
    if (!parameter->has_default || parameter->default_type == IRON_TYPE_CLASS || parameter->default_type == IRON_TYPE_OBJECT) {
        return IRON_SUCCESS;
    }
    if (parameter->default_type == IRON_TYPE_STRING) {
        return create_metadata_string(ctx, parameter->default_data, parameter->default_data_size, &result->value.obj);
    }

    memset(&default_value, 0, sizeof(default_value));
    default_value.type = parameter->default_type == IRON_TYPE_I8 || parameter->default_type == IRON_TYPE_U8 ? IRON_VAL_I64 :
                         parameter->default_type == IRON_TYPE_R4 ? IRON_VAL_F32 : parameter->default_type == IRON_TYPE_R8 ? IRON_VAL_F64 : IRON_VAL_I32;
    default_value.value = parameter->default_value;
    result->value.obj = iron_stack_value_box(ctx, parameter->param_type, &default_value);
    return result->value.obj ? IRON_SUCCESS : IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to box a parameter default value");
}

iron_result_t icall_RuntimePropertyInfo_get_Name(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_property_t *property;

    property = get_property_info_argument(args, arg_count);
    if (!property || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "PropertyInfo.Name requires a valid instance and result slot");
    }
    result->type = IRON_VAL_OBJ;
    result->value.obj = iron_string_new_utf8(ctx, property->name ? property->name : "");
    return result->value.obj ? IRON_SUCCESS : IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate PropertyInfo.Name");
}

iron_result_t icall_RuntimePropertyInfo_get_DeclaringType(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_property_t *property;

    (void)ctx;
    property = get_property_info_argument(args, arg_count);
    if (!property || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "PropertyInfo.DeclaringType requires a valid instance and result slot");
    }
    result->type = IRON_VAL_OBJ;
    result->value.obj = property->declaring_type;
    return IRON_SUCCESS;
}

iron_result_t icall_RuntimePropertyInfo_get_PropertyType(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_property_t *property;
    iron_result_t load_result;

    (void)ctx;
    property = get_property_info_argument(args, arg_count);
    if (!property || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "PropertyInfo.PropertyType requires a valid instance and result slot");
    }
    load_result = iron_property_load_signature(property);
    if (!IRON_RESULT_OK(load_result)) {
        return load_result;
    }
    result->type = IRON_VAL_OBJ;
    result->value.obj = property->property_type;
    return IRON_SUCCESS;
}

iron_result_t icall_RuntimePropertyInfo_get_CanRead(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_property_t *property;

    (void)ctx;
    property = get_property_info_argument(args, arg_count);
    if (!property || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "PropertyInfo.CanRead requires a valid instance and result slot");
    }
    result->type = IRON_VAL_I32;
    result->value.i32 = property->getter ? 1 : 0;
    return IRON_SUCCESS;
}

iron_result_t icall_RuntimePropertyInfo_get_CanWrite(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_property_t *property;

    (void)ctx;
    property = get_property_info_argument(args, arg_count);
    if (!property || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "PropertyInfo.CanWrite requires a valid instance and result slot");
    }
    result->type = IRON_VAL_I32;
    result->value.i32 = property->setter ? 1 : 0;
    return IRON_SUCCESS;
}

static iron_result_t return_property_accessor(iron_exec_context_t *ctx,
                                              iron_stack_value_t *args,
                                              iron_u32 arg_count,
                                              iron_stack_value_t *result,
                                              iron_bool getter)
{
    iron_runtime_property_t *property;
    iron_runtime_method_t *accessor;

    property = get_property_info_argument(args, arg_count);
    if (!property || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "PropertyInfo accessor lookup requires a valid instance and result slot");
    }

    accessor = getter ? property->getter : property->setter;
    result->type = IRON_VAL_OBJ;
    result->value.obj = accessor ? iron_reflection_create_method_info(ctx, accessor) : NULL;
    return !accessor || result->value.obj ? IRON_SUCCESS : IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate a reflected property accessor");
}

iron_result_t icall_RuntimePropertyInfo_GetGetMethod(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    return return_property_accessor(ctx, args, arg_count, result, IRON_TRUE);
}

iron_result_t icall_RuntimePropertyInfo_GetSetMethod(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    return return_property_accessor(ctx, args, arg_count, result, IRON_FALSE);
}

iron_result_t icall_RuntimePropertyInfo_GetIndexParameters(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_property_t *property;
    iron_runtime_type_t *parameter_info_type;
    iron_result_t load_result;
    iron_u32 parameter_index;
    void *array;

    property = get_property_info_argument(args, arg_count);
    if (!property || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "PropertyInfo.GetIndexParameters requires a valid instance and result slot");
    }
    load_result = iron_property_load_signature(property);
    if (!IRON_RESULT_OK(load_result)) {
        return load_result;
    }

    parameter_info_type = iron_domain_find_type(ctx->domain, "System.Reflection.ParameterInfo");
    array = parameter_info_type ? iron_gc_alloc_array(ctx, parameter_info_type, property->index_param_count) : NULL;
    if (!array) {
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate the property index parameter array");
    }

    for (parameter_index = 0; parameter_index < property->index_param_count; parameter_index++) {
        void *parameter_info;

        parameter_info = create_parameter_info(ctx, property->index_params[parameter_index]);
        if (!parameter_info) {
            return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate a property index parameter object");
        }
        set_reference_array_element(array, parameter_index, parameter_info);
    }

    result->type = IRON_VAL_OBJ;
    result->value.obj = array;
    return IRON_SUCCESS;
}

iron_result_t icall_RuntimeEventInfo_get_Name(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_event_t *event;

    event = get_event_info_argument(args, arg_count);
    if (!event || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "EventInfo.Name requires a valid instance and result slot");
    }
    result->type = IRON_VAL_OBJ;
    result->value.obj = iron_string_new_utf8(ctx, event->name ? event->name : "");
    return result->value.obj ? IRON_SUCCESS : IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate EventInfo.Name");
}

iron_result_t icall_RuntimeEventInfo_get_DeclaringType(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_event_t *event;

    (void)ctx;
    event = get_event_info_argument(args, arg_count);
    if (!event || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "EventInfo.DeclaringType requires a valid instance and result slot");
    }
    result->type = IRON_VAL_OBJ;
    result->value.obj = event->declaring_type;
    return IRON_SUCCESS;
}

iron_result_t icall_RuntimeEventInfo_get_Attributes(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_event_t *event;

    (void)ctx;
    event = get_event_info_argument(args, arg_count);
    if (!event || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "EventInfo.Attributes requires a valid instance and result slot");
    }
    result->type = IRON_VAL_I32;
    result->value.i32 = (iron_i32)event->attrs;
    return IRON_SUCCESS;
}

iron_result_t icall_RuntimeEventInfo_get_EventHandlerType(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    iron_runtime_event_t *event;

    (void)ctx;
    event = get_event_info_argument(args, arg_count);
    if (!event || !event->event_type || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "EventInfo.EventHandlerType requires a valid instance and result slot");
    }
    result->type = IRON_VAL_OBJ;
    result->value.obj = event->event_type;
    return IRON_SUCCESS;
}

static iron_result_t return_event_accessor(iron_exec_context_t *ctx,
                                           iron_stack_value_t *args,
                                           iron_u32 arg_count,
                                           iron_stack_value_t *result,
                                           iron_method_kind_t accessor_kind)
{
    iron_runtime_event_t *event;
    iron_runtime_method_t *accessor;

    event = get_event_info_argument(args, arg_count);
    if (!event || !result) {
        return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "EventInfo accessor lookup requires a valid instance and result slot");
    }

    accessor = accessor_kind == IRON_METHOD_EVENT_ADD ? event->add_method :
               accessor_kind == IRON_METHOD_EVENT_REMOVE ? event->remove_method : event->raise_method;
    result->type = IRON_VAL_OBJ;
    result->value.obj = accessor ? iron_reflection_create_method_info(ctx, accessor) : NULL;
    return !accessor || result->value.obj ? IRON_SUCCESS : IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate a reflected event accessor");
}

iron_result_t icall_RuntimeEventInfo_GetAddMethod(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    return return_event_accessor(ctx, args, arg_count, result, IRON_METHOD_EVENT_ADD);
}

iron_result_t icall_RuntimeEventInfo_GetRemoveMethod(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    return return_event_accessor(ctx, args, arg_count, result, IRON_METHOD_EVENT_REMOVE);
}

iron_result_t icall_RuntimeEventInfo_GetRaiseMethod(iron_exec_context_t *ctx, iron_stack_value_t *args, iron_u32 arg_count, iron_stack_value_t *result)
{
    return return_event_accessor(ctx, args, arg_count, result, IRON_METHOD_EVENT_RAISE);
}
