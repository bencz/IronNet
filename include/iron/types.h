/*
 * IronNet CLR Interpreter
 * types.h - Core type definitions and CLI metadata types
 * 
 * Strict C99 compatible
 */

#ifndef IRON_TYPES_H
#define IRON_TYPES_H

#include "platform.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * String Types
 * ============================================================================ */

/* Non-owning string view */
typedef struct iron_string_view {
    const char *data;
    iron_size length;
} iron_string_view_t;

/* Macros for string views */
#define IRON_SV(s) { (s), sizeof(s) - 1 }
#define IRON_SV_NULL { NULL, 0 }
#define IRON_SV_FMT "%.*s"
#define IRON_SV_ARG(sv) (int)(sv).length, (sv).data

/* String view functions */
IRON_API iron_string_view_t iron_sv_from_cstr(const char *str);
IRON_API iron_string_view_t iron_sv_from_parts(const char *data, iron_size length);
IRON_API iron_bool iron_sv_equals(iron_string_view_t a, iron_string_view_t b);
IRON_API iron_bool iron_sv_starts_with(iron_string_view_t sv, iron_string_view_t prefix);
IRON_API iron_bool iron_sv_ends_with(iron_string_view_t sv, iron_string_view_t suffix);
IRON_API iron_i32 iron_sv_find(iron_string_view_t sv, char c);
IRON_API iron_string_view_t iron_sv_substr(iron_string_view_t sv, iron_size start, iron_size len);
IRON_API iron_string_view_t iron_sv_trim(iron_string_view_t sv);

/* ============================================================================
 * Error Handling
 * ============================================================================ */

typedef enum iron_error {
    IRON_OK = 0,
    
    /* Memory errors */
    IRON_ERR_OUT_OF_MEMORY,
    IRON_ERR_BUFFER_OVERFLOW,
    IRON_ERR_NULL_POINTER,
    
    /* I/O errors */
    IRON_ERR_FILE_NOT_FOUND,
    IRON_ERR_FILE_READ,
    IRON_ERR_FILE_WRITE,
    IRON_ERR_FILE_SEEK,
    
    /* PE/COFF errors */
    IRON_ERR_INVALID_PE,
    IRON_ERR_INVALID_DOS_HEADER,
    IRON_ERR_INVALID_PE_SIGNATURE,
    IRON_ERR_INVALID_OPTIONAL_HEADER,
    IRON_ERR_INVALID_SECTION,
    IRON_ERR_NOT_CLI_IMAGE,
    
    /* Metadata errors */
    IRON_ERR_INVALID_METADATA,
    IRON_ERR_INVALID_STREAM,
    IRON_ERR_INVALID_TABLE,
    IRON_ERR_INVALID_SIGNATURE,
    IRON_ERR_INVALID_TOKEN,
    IRON_ERR_INVALID_BLOB,
    IRON_ERR_INVALID_STRING,
    IRON_ERR_INVALID_GUID,
    
    /* Type system errors */
    IRON_ERR_TYPE_NOT_FOUND,
    IRON_ERR_METHOD_NOT_FOUND,
    IRON_ERR_FIELD_NOT_FOUND,
    IRON_ERR_INVALID_TYPE,
    IRON_ERR_INVALID_GENERIC_ARGS,
    IRON_ERR_CONSTRAINT_VIOLATION,
    
    /* Execution errors */
    IRON_ERR_INVALID_OPCODE,
    IRON_ERR_STACK_OVERFLOW,
    IRON_ERR_STACK_UNDERFLOW,
    IRON_ERR_NULL_REFERENCE,
    IRON_ERR_INVALID_CAST,
    IRON_ERR_INDEX_OUT_OF_RANGE,
    IRON_ERR_DIVIDE_BY_ZERO,
    IRON_ERR_ARITHMETIC_OVERFLOW,
    IRON_ERR_INVALID_PROGRAM,
    
    /* Threading errors */
    IRON_ERR_THREAD_CREATE,
    IRON_ERR_THREAD_JOIN,
    IRON_ERR_TIMEOUT,
    IRON_ERR_MUTEX_ERROR,
    IRON_ERR_DEADLOCK,
    
    /* Argument errors */
    IRON_ERR_INVALID_ARGUMENT,
    IRON_ERR_ARGUMENT_NULL,
    IRON_ERR_ARGUMENT_OUT_OF_RANGE,
    
    /* Lookup errors */
    IRON_ERR_NOT_FOUND,
    IRON_ERR_INVALID_STATE,
    
    /* Execution errors */
    IRON_ERR_EXECUTION,
    IRON_ERR_EXCEPTION,
    
    /* Internal errors */
    IRON_ERR_NOT_IMPLEMENTED,
    IRON_ERR_INTERNAL,
    
    IRON_ERR_COUNT
} iron_error_t;

/* Get error message string */
IRON_API const char *iron_error_str(iron_error_t err);

/* ============================================================================
 * Result Type
 * ============================================================================ */

typedef struct iron_result {
    iron_error_t error;
    const char *message;
    iron_u32 line;
    const char *file;
} iron_result_t;

#define IRON_RESULT_OK(r) ((r).error == IRON_OK)

/* Create a success result without relying on a shared mutable object. */
IRON_INLINE iron_result_t iron_result_ok(void)
{
    iron_result_t r;
    r.error = IRON_OK;
    r.message = NULL;
    r.line = 0;
    r.file = NULL;
    return r;
}

/* Macro for returning success */
#define IRON_SUCCESS iron_result_ok()

/* Create error result */
IRON_API iron_result_t iron_make_error(iron_error_t err, const char *msg, 
                                        const char *file, iron_u32 line);

#define IRON_ERROR(err, msg) iron_make_error((err), (msg), __FILE__, __LINE__)

/* ============================================================================
 * Metadata Token (ECMA-335 compliant)
 * ============================================================================ */

/* Token format: [table_type:8][row_index:24] */
typedef iron_u32 iron_token_t;

#define IRON_TOKEN_TABLE_MASK  0xFF000000UL
#define IRON_TOKEN_INDEX_MASK  0x00FFFFFFUL
#define IRON_TOKEN_TABLE_SHIFT 24

#define IRON_TOKEN_TABLE(t)    (((t) & IRON_TOKEN_TABLE_MASK) >> IRON_TOKEN_TABLE_SHIFT)
#define IRON_TOKEN_INDEX(t)    ((t) & IRON_TOKEN_INDEX_MASK)
#define IRON_MAKE_TOKEN(tbl, idx) \
    ((((iron_token_t)(tbl)) << IRON_TOKEN_TABLE_SHIFT) | ((idx) & IRON_TOKEN_INDEX_MASK))

#define IRON_TOKEN_NIL 0

/* ============================================================================
 * Metadata Table Types (ECMA-335 II.22)
 * ============================================================================ */

typedef enum iron_table_id {
    IRON_TABLE_MODULE                    = 0x00,
    IRON_TABLE_TYPE_REF                  = 0x01,
    IRON_TABLE_TYPE_DEF                  = 0x02,
    IRON_TABLE_FIELD_PTR                 = 0x03,
    IRON_TABLE_FIELD                     = 0x04,
    IRON_TABLE_METHOD_PTR                = 0x05,
    IRON_TABLE_METHOD_DEF                = 0x06,
    IRON_TABLE_PARAM_PTR                 = 0x07,
    IRON_TABLE_PARAM                     = 0x08,
    IRON_TABLE_INTERFACE_IMPL            = 0x09,
    IRON_TABLE_MEMBER_REF                = 0x0A,
    IRON_TABLE_CONSTANT                  = 0x0B,
    IRON_TABLE_CUSTOM_ATTRIBUTE          = 0x0C,
    IRON_TABLE_FIELD_MARSHAL             = 0x0D,
    IRON_TABLE_DECL_SECURITY             = 0x0E,
    IRON_TABLE_CLASS_LAYOUT              = 0x0F,
    IRON_TABLE_FIELD_LAYOUT              = 0x10,
    IRON_TABLE_STANDALONE_SIG            = 0x11,
    IRON_TABLE_EVENT_MAP                 = 0x12,
    IRON_TABLE_EVENT_PTR                 = 0x13,
    IRON_TABLE_EVENT                     = 0x14,
    IRON_TABLE_PROPERTY_MAP              = 0x15,
    IRON_TABLE_PROPERTY_PTR              = 0x16,
    IRON_TABLE_PROPERTY                  = 0x17,
    IRON_TABLE_METHOD_SEMANTICS          = 0x18,
    IRON_TABLE_METHOD_IMPL               = 0x19,
    IRON_TABLE_MODULE_REF                = 0x1A,
    IRON_TABLE_TYPE_SPEC                 = 0x1B,
    IRON_TABLE_IMPL_MAP                  = 0x1C,
    IRON_TABLE_FIELD_RVA                 = 0x1D,
    IRON_TABLE_ENC_LOG                   = 0x1E,
    IRON_TABLE_ENC_MAP                   = 0x1F,
    IRON_TABLE_ASSEMBLY                  = 0x20,
    IRON_TABLE_ASSEMBLY_PROCESSOR        = 0x21,
    IRON_TABLE_ASSEMBLY_OS               = 0x22,
    IRON_TABLE_ASSEMBLY_REF              = 0x23,
    IRON_TABLE_ASSEMBLY_REF_PROCESSOR    = 0x24,
    IRON_TABLE_ASSEMBLY_REF_OS           = 0x25,
    IRON_TABLE_FILE                      = 0x26,
    IRON_TABLE_EXPORTED_TYPE             = 0x27,
    IRON_TABLE_MANIFEST_RESOURCE         = 0x28,
    IRON_TABLE_NESTED_CLASS              = 0x29,
    IRON_TABLE_GENERIC_PARAM             = 0x2A,
    IRON_TABLE_METHOD_SPEC               = 0x2B,
    IRON_TABLE_GENERIC_PARAM_CONSTRAINT  = 0x2C,
    
    IRON_TABLE_COUNT                     = 0x2D,
    
    /* Special pseudo-tables for heap references */
    IRON_TABLE_STRING                    = 0x70,
    IRON_TABLE_USER_STRING               = 0x70
} iron_table_id_t;

/* ============================================================================
 * Element Types (ECMA-335 II.23.1.16)
 * ============================================================================ */

typedef enum iron_element_type {
    IRON_TYPE_END            = 0x00,
    IRON_TYPE_VOID           = 0x01,
    IRON_TYPE_BOOLEAN        = 0x02,
    IRON_TYPE_CHAR           = 0x03,
    IRON_TYPE_I1             = 0x04,
    IRON_TYPE_U1             = 0x05,
    IRON_TYPE_I2             = 0x06,
    IRON_TYPE_U2             = 0x07,
    IRON_TYPE_I4             = 0x08,
    IRON_TYPE_U4             = 0x09,
    IRON_TYPE_I8             = 0x0A,
    IRON_TYPE_U8             = 0x0B,
    IRON_TYPE_R4             = 0x0C,
    IRON_TYPE_R8             = 0x0D,
    IRON_TYPE_STRING         = 0x0E,
    IRON_TYPE_PTR            = 0x0F,
    IRON_TYPE_BYREF          = 0x10,
    IRON_TYPE_VALUETYPE      = 0x11,
    IRON_TYPE_CLASS          = 0x12,
    IRON_TYPE_VAR            = 0x13,  /* Generic type parameter */
    IRON_TYPE_ARRAY          = 0x14,
    IRON_TYPE_GENERICINST    = 0x15,
    IRON_TYPE_TYPEDBYREF     = 0x16,
    IRON_TYPE_I              = 0x18,  /* native int */
    IRON_TYPE_U              = 0x19,  /* native uint */
    IRON_TYPE_FNPTR          = 0x1B,
    IRON_TYPE_OBJECT         = 0x1C,
    IRON_TYPE_SZARRAY        = 0x1D,  /* Single-dimension array */
    IRON_TYPE_MVAR           = 0x1E,  /* Generic method parameter */
    IRON_TYPE_CMOD_REQD      = 0x1F,
    IRON_TYPE_CMOD_OPT       = 0x20,
    IRON_TYPE_INTERNAL       = 0x21,
    IRON_TYPE_MODIFIER       = 0x40,
    IRON_TYPE_SENTINEL       = 0x41,
    IRON_TYPE_PINNED         = 0x45
} iron_element_type_t;

/* Element type utilities */
IRON_API const char *iron_element_type_name(iron_element_type_t type);
IRON_API const char *iron_element_type_managed_name(iron_element_type_t type);
IRON_API iron_size iron_element_type_size(iron_element_type_t type);
IRON_API iron_bool iron_element_type_is_primitive(iron_element_type_t type);
IRON_API iron_bool iron_element_type_is_reference(iron_element_type_t type);

/* ============================================================================
 * Type Attributes (ECMA-335 II.23.1.15)
 * ============================================================================ */

typedef enum iron_type_attr {
    /* Visibility */
    IRON_TYPE_VIS_MASK           = 0x00000007,
    IRON_TYPE_NOT_PUBLIC         = 0x00000000,
    IRON_TYPE_PUBLIC             = 0x00000001,
    IRON_TYPE_NESTED_PUBLIC      = 0x00000002,
    IRON_TYPE_NESTED_PRIVATE     = 0x00000003,
    IRON_TYPE_NESTED_FAMILY      = 0x00000004,
    IRON_TYPE_NESTED_ASSEMBLY    = 0x00000005,
    IRON_TYPE_NESTED_FAM_AND_ASSEM = 0x00000006,
    IRON_TYPE_NESTED_FAM_OR_ASSEM  = 0x00000007,
    
    /* Layout */
    IRON_TYPE_LAYOUT_MASK        = 0x00000018,
    IRON_TYPE_AUTO_LAYOUT        = 0x00000000,
    IRON_TYPE_SEQUENTIAL_LAYOUT  = 0x00000008,
    IRON_TYPE_EXPLICIT_LAYOUT    = 0x00000010,
    
    /* Semantics */
    IRON_TYPE_CLASS_SEMANTICS_MASK = 0x00000020,
    IRON_TYPE_ATTR_CLASS         = 0x00000000,
    IRON_TYPE_ATTR_INTERFACE     = 0x00000020,
    
    /* Special */
    IRON_TYPE_ABSTRACT           = 0x00000080,
    IRON_TYPE_SEALED             = 0x00000100,
    IRON_TYPE_SPECIAL_NAME       = 0x00000400,
    
    /* Implementation */
    IRON_TYPE_IMPORT             = 0x00001000,
    IRON_TYPE_SERIALIZABLE       = 0x00002000,
    
    /* String format */
    IRON_TYPE_STRING_FORMAT_MASK = 0x00030000,
    IRON_TYPE_ANSI_CLASS         = 0x00000000,
    IRON_TYPE_UNICODE_CLASS      = 0x00010000,
    IRON_TYPE_AUTO_CLASS         = 0x00020000,
    IRON_TYPE_CUSTOM_FORMAT_CLASS = 0x00030000,
    
    /* Other */
    IRON_TYPE_BEFORE_FIELD_INIT  = 0x00100000,
    IRON_TYPE_FORWARDER          = 0x00200000,
    IRON_TYPE_RT_SPECIAL_NAME    = 0x00000800,
    IRON_TYPE_HAS_SECURITY       = 0x00040000
} iron_type_attr_t;

/* ============================================================================
 * Method Attributes (ECMA-335 II.23.1.10)
 * ============================================================================ */

typedef enum iron_method_attr {
    /* Member access */
    IRON_METHOD_ACCESS_MASK      = 0x0007,
    IRON_METHOD_COMPILER_CONTROLLED = 0x0000,
    IRON_METHOD_PRIVATE          = 0x0001,
    IRON_METHOD_FAM_AND_ASSEM    = 0x0002,
    IRON_METHOD_ASSEM            = 0x0003,
    IRON_METHOD_FAMILY           = 0x0004,
    IRON_METHOD_FAM_OR_ASSEM     = 0x0005,
    IRON_METHOD_PUBLIC           = 0x0006,
    
    /* Flags */
    IRON_METHOD_STATIC           = 0x0010,
    IRON_METHOD_FINAL            = 0x0020,
    IRON_METHOD_VIRTUAL          = 0x0040,
    IRON_METHOD_HIDE_BY_SIG      = 0x0080,
    
    /* Vtable layout */
    IRON_METHOD_VTABLE_MASK      = 0x0100,
    IRON_METHOD_REUSE_SLOT       = 0x0000,
    IRON_METHOD_NEW_SLOT         = 0x0100,
    
    /* Implementation */
    IRON_METHOD_CHECK_ACCESS     = 0x0200,
    IRON_METHOD_ABSTRACT         = 0x0400,
    IRON_METHOD_SPECIAL_NAME     = 0x0800,
    
    /* Interop */
    IRON_METHOD_PINVOKE_IMPL     = 0x2000,
    IRON_METHOD_UNMANAGED_EXPORT = 0x0008,
    
    /* Reserved */
    IRON_METHOD_RT_SPECIAL_NAME  = 0x1000,
    IRON_METHOD_HAS_SECURITY     = 0x4000,
    IRON_METHOD_REQUIRE_SEC_OBJ  = 0x8000
} iron_method_attr_t;

/* ============================================================================
 * Method Implementation Attributes (ECMA-335 II.23.1.11)
 * ============================================================================ */

typedef enum iron_method_impl_attr {
    /* Code type */
    IRON_IMPL_CODE_TYPE_MASK     = 0x0003,
    IRON_IMPL_IL                 = 0x0000,
    IRON_IMPL_NATIVE             = 0x0001,
    IRON_IMPL_OPTIL              = 0x0002,
    IRON_IMPL_RUNTIME            = 0x0003,
    
    /* Managed */
    IRON_IMPL_MANAGED_MASK       = 0x0004,
    IRON_IMPL_UNMANAGED          = 0x0004,
    IRON_IMPL_MANAGED            = 0x0000,
    
    /* Implementation */
    IRON_IMPL_FORWARD_REF        = 0x0010,
    IRON_IMPL_PRESERVE_SIG       = 0x0080,
    IRON_IMPL_INTERNAL_CALL      = 0x1000,
    IRON_IMPL_SYNCHRONIZED       = 0x0020,
    IRON_IMPL_NO_INLINING        = 0x0008,
    IRON_IMPL_AGGRESSIVE_INLINING = 0x0100,
    IRON_IMPL_NO_OPTIMIZATION    = 0x0040
} iron_method_impl_attr_t;

/* ============================================================================
 * Field Attributes (ECMA-335 II.23.1.5)
 * ============================================================================ */

typedef enum iron_field_attr {
    /* Access */
    IRON_FIELD_ACCESS_MASK       = 0x0007,
    IRON_FIELD_COMPILER_CONTROLLED = 0x0000,
    IRON_FIELD_PRIVATE           = 0x0001,
    IRON_FIELD_FAM_AND_ASSEM     = 0x0002,
    IRON_FIELD_ASSEMBLY          = 0x0003,
    IRON_FIELD_FAMILY            = 0x0004,
    IRON_FIELD_FAM_OR_ASSEM      = 0x0005,
    IRON_FIELD_PUBLIC            = 0x0006,
    
    /* Flags */
    IRON_FIELD_STATIC            = 0x0010,
    IRON_FIELD_INIT_ONLY         = 0x0020,
    IRON_FIELD_LITERAL           = 0x0040,
    IRON_FIELD_NOT_SERIALIZED    = 0x0080,
    IRON_FIELD_SPECIAL_NAME      = 0x0200,
    
    /* Interop */
    IRON_FIELD_PINVOKE_IMPL      = 0x2000,
    
    /* Reserved */
    IRON_FIELD_RT_SPECIAL_NAME   = 0x0400,
    IRON_FIELD_HAS_FIELD_MARSHAL = 0x1000,
    IRON_FIELD_HAS_DEFAULT       = 0x8000,
    IRON_FIELD_HAS_FIELD_RVA     = 0x0100
} iron_field_attr_t;

/* ============================================================================
 * Parameter Attributes (ECMA-335 II.23.1.13)
 * ============================================================================ */

typedef enum iron_param_attr {
    IRON_PARAM_IN                = 0x0001,
    IRON_PARAM_OUT               = 0x0002,
    IRON_PARAM_OPTIONAL          = 0x0010,
    IRON_PARAM_HAS_DEFAULT       = 0x1000,
    IRON_PARAM_HAS_FIELD_MARSHAL = 0x2000
} iron_param_attr_t;

/* ============================================================================
 * Generic Parameter Attributes (ECMA-335 II.23.1.7)
 * ============================================================================ */

typedef enum iron_generic_param_attr {
    /* Variance */
    IRON_GPARAM_VARIANCE_MASK    = 0x0003,
    IRON_GPARAM_NONE             = 0x0000,
    IRON_GPARAM_COVARIANT        = 0x0001,
    IRON_GPARAM_CONTRAVARIANT    = 0x0002,
    
    /* Special constraints */
    IRON_GPARAM_SPECIAL_MASK     = 0x001C,
    IRON_GPARAM_REFERENCE_TYPE   = 0x0004,
    IRON_GPARAM_NOT_NULLABLE_VALUE = 0x0008,
    IRON_GPARAM_DEFAULT_CTOR     = 0x0010
} iron_generic_param_attr_t;

/* ============================================================================
 * Calling Conventions (ECMA-335 II.23.2.1)
 * ============================================================================ */

typedef enum iron_call_conv {
    IRON_CALL_DEFAULT            = 0x00,
    IRON_CALL_C                  = 0x01,
    IRON_CALL_STDCALL            = 0x02,
    IRON_CALL_THISCALL           = 0x03,
    IRON_CALL_FASTCALL           = 0x04,
    IRON_CALL_VARARG             = 0x05,
    IRON_CALL_FIELD              = 0x06,
    IRON_CALL_LOCAL_SIG          = 0x07,
    IRON_CALL_PROPERTY           = 0x08,
    IRON_CALL_UNMANAGED          = 0x09,
    IRON_CALL_GENERIC_INST       = 0x0A,
    IRON_CALL_NATIVE_VARARG      = 0x0B,
    IRON_CALL_GENERIC            = 0x10,
    IRON_CALL_HAS_THIS           = 0x20,
    IRON_CALL_EXPLICIT_THIS      = 0x40
} iron_call_conv_t;

/* ============================================================================
 * Assembly Flags (ECMA-335 II.23.1.2)
 * ============================================================================ */

typedef enum iron_assembly_flags {
    IRON_ASM_PUBLIC_KEY          = 0x0001,
    IRON_ASM_RETARGETABLE        = 0x0100,
    IRON_ASM_DISABLE_JIT_OPTIMIZER = 0x4000,
    IRON_ASM_ENABLE_JIT_TRACKING = 0x8000
} iron_assembly_flags_t;

/* ============================================================================
 * CLI Value Union (for evaluation stack)
 * ============================================================================ */

typedef union iron_value {
    iron_i32 i32;
    iron_i64 i64;
    iron_f32 f32;
    iron_f64 f64;
    void *ptr;
    void *obj;  /* Object reference */
    struct {
        void *ptr;
        void *stack_slot;
    } byref;
    struct {
        void *ptr;
        void *type;
    } typedref;
} iron_value_t;

/* Value type tag */
typedef enum iron_value_type {
    IRON_VAL_I32,
    IRON_VAL_I64,
    IRON_VAL_F32,
    IRON_VAL_F64,
    IRON_VAL_PTR,
    IRON_VAL_METHOD_PTR,
    IRON_VAL_OBJ,
    IRON_VAL_BYREF,
    IRON_VAL_TYPEDREF,
    IRON_VAL_VALUETYPE,
    IRON_VAL_VOID = 0xFF
} iron_value_type_t;

/* Tagged value for evaluation stack */
typedef struct iron_stack_value {
    iron_value_t value;
    iron_value_type_t type;
} iron_stack_value_t;

#ifdef __cplusplus
}
#endif

#endif /* IRON_TYPES_H */
