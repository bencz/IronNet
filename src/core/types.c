/*
 * IronNet CLR Interpreter
 * types.c - Core type utilities and error handling
 */

#include "iron/types.h"
#include <stdio.h>
#include <stdarg.h>

/* ============================================================================
 * Error Messages
 * ============================================================================ */

static const char *g_error_messages[] = {
    "Success",
    
    /* Memory errors */
    "Out of memory",
    "Buffer overflow",
    "Null pointer",
    
    /* I/O errors */
    "File not found",
    "File read error",
    "File write error",
    "File seek error",
    
    /* PE/COFF errors */
    "Invalid PE file",
    "Invalid DOS header",
    "Invalid PE signature",
    "Invalid optional header",
    "Invalid section",
    "Not a CLI image",
    
    /* Metadata errors */
    "Invalid metadata",
    "Invalid stream",
    "Invalid table",
    "Invalid signature",
    "Invalid token",
    "Invalid blob",
    "Invalid string",
    "Invalid GUID",
    
    /* Type system errors */
    "Type not found",
    "Method not found",
    "Field not found",
    "Invalid type",
    "Invalid generic arguments",
    "Constraint violation",
    
    /* Execution errors */
    "Invalid opcode",
    "Stack overflow",
    "Stack underflow",
    "Null reference exception",
    "Invalid cast exception",
    "Array rank exception",
    "Index out of range exception",
    "Divide by zero exception",
    "Arithmetic overflow exception",
    "Invalid program",
    
    /* Threading errors */
    "Thread creation failed",
    "Thread join failed",
    "Operation timed out",
    "Mutex error",
    "Deadlock detected",
    
    /* Argument errors */
    "Invalid argument",
    "Argument cannot be null",
    "Argument is outside the valid range",
    
    /* Lookup errors */
    "Not found",
    "Invalid state",
    
    /* Execution errors */
    "Execution error",
    "Unhandled exception",
    
    /* Internal errors */
    "Not implemented",
    "Internal error"
};

IRON_STATIC_ASSERT(
    sizeof(g_error_messages) / sizeof(g_error_messages[0]) == IRON_ERR_COUNT,
    error_message_count_mismatch
);

const char *iron_error_str(iron_error_t err)
{
    if (err >= 0 && err < IRON_ERR_COUNT) {
        return g_error_messages[err];
    }
    return "Unknown error";
}

iron_result_t iron_make_error(iron_error_t err, const char *msg,
                              const char *file, iron_u32 line)
{
    iron_result_t result;
    result.error = err;
    result.message = msg ? msg : iron_error_str(err);
    result.file = file;
    result.line = line;
    return result;
}

/* ============================================================================
 * String View Utilities
 * ============================================================================ */

iron_string_view_t iron_sv_from_cstr(const char *s)
{
    iron_string_view_t sv;
    sv.data = s;
    sv.length = 0;
    if (s) {
        while (s[sv.length]) sv.length++;
    }
    return sv;
}

iron_bool iron_sv_equals(iron_string_view_t a, iron_string_view_t b)
{
    iron_size i;
    if (a.length != b.length) return IRON_FALSE;
    for (i = 0; i < a.length; i++) {
        if (a.data[i] != b.data[i]) return IRON_FALSE;
    }
    return IRON_TRUE;
}

iron_bool iron_sv_equals_cstr(iron_string_view_t sv, const char *s)
{
    iron_size i;
    if (!s) return sv.data == NULL;
    for (i = 0; i < sv.length; i++) {
        if (s[i] == '\0' || sv.data[i] != s[i]) return IRON_FALSE;
    }
    return s[sv.length] == '\0';
}

iron_bool iron_sv_starts_with(iron_string_view_t sv, iron_string_view_t prefix)
{
    iron_size i;
    if (prefix.length > sv.length) return IRON_FALSE;
    for (i = 0; i < prefix.length; i++) {
        if (sv.data[i] != prefix.data[i]) return IRON_FALSE;
    }
    return IRON_TRUE;
}

iron_bool iron_sv_ends_with(iron_string_view_t sv, iron_string_view_t suffix)
{
    iron_size i, offset;
    if (suffix.length > sv.length) return IRON_FALSE;
    offset = sv.length - suffix.length;
    for (i = 0; i < suffix.length; i++) {
        if (sv.data[offset + i] != suffix.data[i]) return IRON_FALSE;
    }
    return IRON_TRUE;
}

iron_string_view_t iron_sv_substr(iron_string_view_t sv, iron_size start, iron_size len)
{
    iron_string_view_t result;
    if (start >= sv.length) {
        result.data = sv.data + sv.length;
        result.length = 0;
    } else {
        result.data = sv.data + start;
        result.length = len;
        if (start + len > sv.length) {
            result.length = sv.length - start;
        }
    }
    return result;
}

iron_i32 iron_sv_find(iron_string_view_t sv, char c)
{
    iron_size i;
    for (i = 0; i < sv.length; i++) {
        if (sv.data[i] == c) return (iron_i32)i;
    }
    return -1;
}

iron_i64 iron_sv_rfind(iron_string_view_t sv, char c)
{
    iron_size i;
    for (i = sv.length; i > 0; i--) {
        if (sv.data[i - 1] == c) return (iron_i64)(i - 1);
    }
    return -1;
}

iron_string_view_t iron_sv_trim(iron_string_view_t sv)
{
    iron_string_view_t result = sv;
    
    /* Trim leading whitespace */
    while (result.length > 0 && 
           (result.data[0] == ' ' || result.data[0] == '\t' ||
            result.data[0] == '\n' || result.data[0] == '\r')) {
        result.data++;
        result.length--;
    }
    
    /* Trim trailing whitespace */
    while (result.length > 0 &&
           (result.data[result.length - 1] == ' ' || 
            result.data[result.length - 1] == '\t' ||
            result.data[result.length - 1] == '\n' || 
            result.data[result.length - 1] == '\r')) {
        result.length--;
    }
    
    return result;
}

/* ============================================================================
 * Value Type Utilities
 * ============================================================================ */

const char *iron_value_type_name(iron_value_type_t type)
{
    switch (type) {
        case IRON_VAL_I32:      return "int32";
        case IRON_VAL_I64:      return "int64";
        case IRON_VAL_F32:      return "float32";
        case IRON_VAL_F64:      return "float64";
        case IRON_VAL_PTR:      return "native int";
        case IRON_VAL_OBJ:      return "object ref";
        case IRON_VAL_BYREF:    return "byref";
        case IRON_VAL_VALUETYPE: return "valuetype";
        case IRON_VAL_VOID:     return "void";
        default:                return "unknown";
    }
}

const char *iron_element_type_name(iron_element_type_t type)
{
    switch (type) {
        case IRON_TYPE_END:         return "end";
        case IRON_TYPE_VOID:        return "void";
        case IRON_TYPE_BOOLEAN:     return "bool";
        case IRON_TYPE_CHAR:        return "char";
        case IRON_TYPE_I1:          return "int8";
        case IRON_TYPE_U1:          return "uint8";
        case IRON_TYPE_I2:          return "int16";
        case IRON_TYPE_U2:          return "uint16";
        case IRON_TYPE_I4:          return "int32";
        case IRON_TYPE_U4:          return "uint32";
        case IRON_TYPE_I8:          return "int64";
        case IRON_TYPE_U8:          return "uint64";
        case IRON_TYPE_R4:          return "float32";
        case IRON_TYPE_R8:          return "float64";
        case IRON_TYPE_STRING:      return "string";
        case IRON_TYPE_PTR:         return "ptr";
        case IRON_TYPE_BYREF:       return "byref";
        case IRON_TYPE_VALUETYPE:   return "valuetype";
        case IRON_TYPE_CLASS:       return "class";
        case IRON_TYPE_VAR:         return "var";
        case IRON_TYPE_ARRAY:       return "array";
        case IRON_TYPE_GENERICINST: return "genericinst";
        case IRON_TYPE_TYPEDBYREF:  return "typedref";
        case IRON_TYPE_I:           return "native int";
        case IRON_TYPE_U:           return "native uint";
        case IRON_TYPE_FNPTR:       return "fnptr";
        case IRON_TYPE_OBJECT:      return "object";
        case IRON_TYPE_SZARRAY:     return "szarray";
        case IRON_TYPE_MVAR:        return "mvar";
        case IRON_TYPE_CMOD_REQD:   return "cmod_reqd";
        case IRON_TYPE_CMOD_OPT:    return "cmod_opt";
        case IRON_TYPE_INTERNAL:    return "internal";
        case IRON_TYPE_MODIFIER:    return "modifier";
        case IRON_TYPE_SENTINEL:    return "sentinel";
        case IRON_TYPE_PINNED:      return "pinned";
        default:                    return "unknown";
    }
}

const char *iron_element_type_managed_name(iron_element_type_t type)
{
    switch (type) {
        case IRON_TYPE_VOID: return "System.Void";
        case IRON_TYPE_BOOLEAN: return "System.Boolean";
        case IRON_TYPE_CHAR: return "System.Char";
        case IRON_TYPE_I1: return "System.SByte";
        case IRON_TYPE_U1: return "System.Byte";
        case IRON_TYPE_I2: return "System.Int16";
        case IRON_TYPE_U2: return "System.UInt16";
        case IRON_TYPE_I4: return "System.Int32";
        case IRON_TYPE_U4: return "System.UInt32";
        case IRON_TYPE_I8: return "System.Int64";
        case IRON_TYPE_U8: return "System.UInt64";
        case IRON_TYPE_R4: return "System.Single";
        case IRON_TYPE_R8: return "System.Double";
        case IRON_TYPE_STRING: return "System.String";
        case IRON_TYPE_OBJECT: return "System.Object";
        case IRON_TYPE_TYPEDBYREF: return "System.TypedReference";
        case IRON_TYPE_I: return "System.IntPtr";
        case IRON_TYPE_U: return "System.UIntPtr";
        default: return NULL;
    }
}

iron_size iron_element_type_size(iron_element_type_t type)
{
    switch (type) {
        case IRON_TYPE_VOID:        return 0;
        case IRON_TYPE_BOOLEAN:     return 1;
        case IRON_TYPE_I1:          return 1;
        case IRON_TYPE_U1:          return 1;
        case IRON_TYPE_CHAR:        return 2;
        case IRON_TYPE_I2:          return 2;
        case IRON_TYPE_U2:          return 2;
        case IRON_TYPE_I4:          return 4;
        case IRON_TYPE_U4:          return 4;
        case IRON_TYPE_R4:          return 4;
        case IRON_TYPE_I8:          return 8;
        case IRON_TYPE_U8:          return 8;
        case IRON_TYPE_R8:          return 8;
        case IRON_TYPE_I:           return IRON_PTR_SIZE;
        case IRON_TYPE_U:           return IRON_PTR_SIZE;
        case IRON_TYPE_PTR:         return IRON_PTR_SIZE;
        case IRON_TYPE_BYREF:       return IRON_PTR_SIZE;
        case IRON_TYPE_STRING:      return IRON_PTR_SIZE;
        case IRON_TYPE_CLASS:       return IRON_PTR_SIZE;
        case IRON_TYPE_OBJECT:      return IRON_PTR_SIZE;
        case IRON_TYPE_SZARRAY:     return IRON_PTR_SIZE;
        case IRON_TYPE_ARRAY:       return IRON_PTR_SIZE;
        case IRON_TYPE_FNPTR:       return IRON_PTR_SIZE;
        default:                    return 0; /* Unknown/variable size */
    }
}

iron_bool iron_element_type_is_primitive(iron_element_type_t type)
{
    switch (type) {
        case IRON_TYPE_BOOLEAN:
        case IRON_TYPE_CHAR:
        case IRON_TYPE_I1:
        case IRON_TYPE_U1:
        case IRON_TYPE_I2:
        case IRON_TYPE_U2:
        case IRON_TYPE_I4:
        case IRON_TYPE_U4:
        case IRON_TYPE_I8:
        case IRON_TYPE_U8:
        case IRON_TYPE_R4:
        case IRON_TYPE_R8:
        case IRON_TYPE_I:
        case IRON_TYPE_U:
            return IRON_TRUE;
        default:
            return IRON_FALSE;
    }
}

iron_bool iron_element_type_is_reference(iron_element_type_t type)
{
    switch (type) {
        case IRON_TYPE_STRING:
        case IRON_TYPE_CLASS:
        case IRON_TYPE_OBJECT:
        case IRON_TYPE_SZARRAY:
        case IRON_TYPE_ARRAY:
            return IRON_TRUE;
        default:
            return IRON_FALSE;
    }
}

/* ============================================================================
 * Token Utilities
 * ============================================================================ */

const char *iron_table_name(iron_table_id_t table)
{
    switch (table) {
        case IRON_TABLE_MODULE:                   return "Module";
        case IRON_TABLE_TYPE_REF:                 return "TypeRef";
        case IRON_TABLE_TYPE_DEF:                 return "TypeDef";
        case IRON_TABLE_FIELD_PTR:                return "FieldPtr";
        case IRON_TABLE_FIELD:                    return "Field";
        case IRON_TABLE_METHOD_PTR:               return "MethodPtr";
        case IRON_TABLE_METHOD_DEF:               return "MethodDef";
        case IRON_TABLE_PARAM_PTR:                return "ParamPtr";
        case IRON_TABLE_PARAM:                    return "Param";
        case IRON_TABLE_INTERFACE_IMPL:           return "InterfaceImpl";
        case IRON_TABLE_MEMBER_REF:               return "MemberRef";
        case IRON_TABLE_CONSTANT:                 return "Constant";
        case IRON_TABLE_CUSTOM_ATTRIBUTE:         return "CustomAttribute";
        case IRON_TABLE_FIELD_MARSHAL:            return "FieldMarshal";
        case IRON_TABLE_DECL_SECURITY:            return "DeclSecurity";
        case IRON_TABLE_CLASS_LAYOUT:             return "ClassLayout";
        case IRON_TABLE_FIELD_LAYOUT:             return "FieldLayout";
        case IRON_TABLE_STANDALONE_SIG:           return "StandAloneSig";
        case IRON_TABLE_EVENT_MAP:                return "EventMap";
        case IRON_TABLE_EVENT_PTR:                return "EventPtr";
        case IRON_TABLE_EVENT:                    return "Event";
        case IRON_TABLE_PROPERTY_MAP:             return "PropertyMap";
        case IRON_TABLE_PROPERTY_PTR:             return "PropertyPtr";
        case IRON_TABLE_PROPERTY:                 return "Property";
        case IRON_TABLE_METHOD_SEMANTICS:         return "MethodSemantics";
        case IRON_TABLE_METHOD_IMPL:              return "MethodImpl";
        case IRON_TABLE_MODULE_REF:               return "ModuleRef";
        case IRON_TABLE_TYPE_SPEC:                return "TypeSpec";
        case IRON_TABLE_IMPL_MAP:                 return "ImplMap";
        case IRON_TABLE_FIELD_RVA:                return "FieldRVA";
        case IRON_TABLE_ENC_LOG:                  return "ENCLog";
        case IRON_TABLE_ENC_MAP:                  return "ENCMap";
        case IRON_TABLE_ASSEMBLY:                 return "Assembly";
        case IRON_TABLE_ASSEMBLY_PROCESSOR:       return "AssemblyProcessor";
        case IRON_TABLE_ASSEMBLY_OS:              return "AssemblyOS";
        case IRON_TABLE_ASSEMBLY_REF:             return "AssemblyRef";
        case IRON_TABLE_ASSEMBLY_REF_PROCESSOR:   return "AssemblyRefProcessor";
        case IRON_TABLE_ASSEMBLY_REF_OS:          return "AssemblyRefOS";
        case IRON_TABLE_FILE:                     return "File";
        case IRON_TABLE_EXPORTED_TYPE:            return "ExportedType";
        case IRON_TABLE_MANIFEST_RESOURCE:        return "ManifestResource";
        case IRON_TABLE_NESTED_CLASS:             return "NestedClass";
        case IRON_TABLE_GENERIC_PARAM:            return "GenericParam";
        case IRON_TABLE_METHOD_SPEC:              return "MethodSpec";
        case IRON_TABLE_GENERIC_PARAM_CONSTRAINT: return "GenericParamConstraint";
        default:                                  return "Unknown";
    }
}

void iron_token_to_string(iron_token_t token, char *buf, iron_size buf_size)
{
    iron_table_id_t table = (iron_table_id_t)IRON_TOKEN_TABLE(token);
    iron_u32 index = IRON_TOKEN_INDEX(token);
    
    snprintf(buf, buf_size, "%s[0x%06X]", iron_table_name(table), index);
}
