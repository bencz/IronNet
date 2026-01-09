/*
 * IronNet CLR Interpreter
 * metadata.h - CLI Metadata structures and reader (ECMA-335)
 * 
 * Handles reading metadata streams, tables, and signatures
 * 
 * Pure C89 compatible
 */

#ifndef IRON_METADATA_H
#define IRON_METADATA_H

#include "platform.h"
#include "types.h"
#include "memory.h"
#include "pe.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Metadata Header (ECMA-335 II.24.2.1)
 * ============================================================================ */

#define IRON_METADATA_SIGNATURE 0x424A5342  /* "BSJB" */

typedef struct iron_metadata_header {
    iron_u32 signature;
    iron_u16 major_version;
    iron_u16 minor_version;
    iron_u32 reserved;
    iron_u32 version_length;
    /* version string follows */
} iron_metadata_header_t;

/* ============================================================================
 * Stream Headers (ECMA-335 II.24.2.2)
 * ============================================================================ */

typedef struct iron_stream_header {
    iron_u32 offset;
    iron_u32 size;
    /* name follows (null-terminated, 4-byte aligned) */
} iron_stream_header_t;

/* Stream types */
typedef enum iron_stream_type {
    IRON_STREAM_TABLES,      /* #~ or #- */
    IRON_STREAM_STRINGS,     /* #Strings */
    IRON_STREAM_US,          /* #US (User Strings) */
    IRON_STREAM_GUID,        /* #GUID */
    IRON_STREAM_BLOB,        /* #Blob */
    IRON_STREAM_COUNT
} iron_stream_type_t;

/* ============================================================================
 * Tables Stream Header (ECMA-335 II.24.2.6)
 * ============================================================================ */

typedef struct iron_tables_header {
    iron_u32 reserved;
    iron_u8  major_version;
    iron_u8  minor_version;
    iron_u8  heap_sizes;      /* Bit flags for heap index sizes */
    iron_u8  reserved2;
    iron_u64 valid;           /* Bit vector of present tables */
    iron_u64 sorted;          /* Bit vector of sorted tables */
    /* Row counts follow for each valid table */
} iron_tables_header_t;

/* Heap size flags */
#define IRON_HEAP_STRING_LARGE 0x01
#define IRON_HEAP_GUID_LARGE   0x02
#define IRON_HEAP_BLOB_LARGE   0x04

/* ============================================================================
 * Coded Index Types (ECMA-335 II.24.2.6)
 * ============================================================================ */

typedef enum iron_coded_index {
    IRON_CODED_TYPE_DEF_OR_REF,
    IRON_CODED_HAS_CONSTANT,
    IRON_CODED_HAS_CUSTOM_ATTRIBUTE,
    IRON_CODED_HAS_FIELD_MARSHAL,
    IRON_CODED_HAS_DECL_SECURITY,
    IRON_CODED_MEMBER_REF_PARENT,
    IRON_CODED_HAS_SEMANTICS,
    IRON_CODED_METHOD_DEF_OR_REF,
    IRON_CODED_MEMBER_FORWARDED,
    IRON_CODED_IMPLEMENTATION,
    IRON_CODED_CUSTOM_ATTRIBUTE_TYPE,
    IRON_CODED_RESOLUTION_SCOPE,
    IRON_CODED_TYPE_OR_METHOD_DEF,
    IRON_CODED_COUNT
} iron_coded_index_t;

/* ============================================================================
 * Table Row Structures
 * ============================================================================ */

/* Module table (0x00) */
typedef struct iron_module_row {
    iron_u16 generation;
    iron_u32 name;           /* String heap index */
    iron_u32 mvid;           /* GUID heap index */
    iron_u32 enc_id;         /* GUID heap index */
    iron_u32 enc_base_id;    /* GUID heap index */
} iron_module_row_t;

/* TypeRef table (0x01) */
typedef struct iron_type_ref_row {
    iron_u32 resolution_scope;  /* Coded index */
    iron_u32 name;              /* String heap index */
    iron_u32 namespace_;        /* String heap index */
} iron_type_ref_row_t;

/* TypeDef table (0x02) */
typedef struct iron_type_def_row {
    iron_u32 flags;
    iron_u32 name;              /* String heap index */
    iron_u32 namespace_;        /* String heap index */
    iron_u32 extends;           /* Coded index (TypeDefOrRef) */
    iron_u32 field_list;        /* Index into Field table */
    iron_u32 method_list;       /* Index into MethodDef table */
} iron_type_def_row_t;

/* Field table (0x04) */
typedef struct iron_field_row {
    iron_u16 flags;
    iron_u32 name;              /* String heap index */
    iron_u32 signature;         /* Blob heap index */
} iron_field_row_t;

/* MethodDef table (0x06) */
typedef struct iron_method_def_row {
    iron_u32 rva;
    iron_u16 impl_flags;
    iron_u16 flags;
    iron_u32 name;              /* String heap index */
    iron_u32 signature;         /* Blob heap index */
    iron_u32 param_list;        /* Index into Param table */
} iron_method_def_row_t;

/* Param table (0x08) */
typedef struct iron_param_row {
    iron_u16 flags;
    iron_u16 sequence;
    iron_u32 name;              /* String heap index */
} iron_param_row_t;

/* InterfaceImpl table (0x09) */
typedef struct iron_interface_impl_row {
    iron_u32 class_;            /* Index into TypeDef */
    iron_u32 interface_;        /* Coded index (TypeDefOrRef) */
} iron_interface_impl_row_t;

/* MemberRef table (0x0A) */
typedef struct iron_member_ref_row {
    iron_u32 class_;            /* Coded index (MemberRefParent) */
    iron_u32 name;              /* String heap index */
    iron_u32 signature;         /* Blob heap index */
} iron_member_ref_row_t;

/* Constant table (0x0B) */
typedef struct iron_constant_row {
    iron_u8  type;
    iron_u8  padding;
    iron_u32 parent;            /* Coded index (HasConstant) */
    iron_u32 value;             /* Blob heap index */
} iron_constant_row_t;

/* CustomAttribute table (0x0C) */
typedef struct iron_custom_attribute_row {
    iron_u32 parent;            /* Coded index (HasCustomAttribute) */
    iron_u32 type;              /* Coded index (CustomAttributeType) */
    iron_u32 value;             /* Blob heap index */
} iron_custom_attribute_row_t;

/* ClassLayout table (0x0F) */
typedef struct iron_class_layout_row {
    iron_u16 packing_size;
    iron_u32 class_size;
    iron_u32 parent;            /* Index into TypeDef */
} iron_class_layout_row_t;

/* FieldLayout table (0x10) */
typedef struct iron_field_layout_row {
    iron_u32 offset;
    iron_u32 field;             /* Index into Field */
} iron_field_layout_row_t;

/* StandAloneSig table (0x11) */
typedef struct iron_standalone_sig_row {
    iron_u32 signature;         /* Blob heap index */
} iron_standalone_sig_row_t;

/* EventMap table (0x12) */
typedef struct iron_event_map_row {
    iron_u32 parent;            /* Index into TypeDef */
    iron_u32 event_list;        /* Index into Event */
} iron_event_map_row_t;

/* Event table (0x14) */
typedef struct iron_event_row {
    iron_u16 event_flags;
    iron_u32 name;              /* String heap index */
    iron_u32 event_type;        /* Coded index (TypeDefOrRef) */
} iron_event_row_t;

/* PropertyMap table (0x15) */
typedef struct iron_property_map_row {
    iron_u32 parent;            /* Index into TypeDef */
    iron_u32 property_list;     /* Index into Property */
} iron_property_map_row_t;

/* Property table (0x17) */
typedef struct iron_property_row {
    iron_u16 flags;
    iron_u32 name;              /* String heap index */
    iron_u32 type;              /* Blob heap index */
} iron_property_row_t;

/* MethodSemantics table (0x18) */
typedef struct iron_method_semantics_row {
    iron_u16 semantics;
    iron_u32 method;            /* Index into MethodDef */
    iron_u32 association;       /* Coded index (HasSemantics) */
} iron_method_semantics_row_t;

/* MethodImpl table (0x19) */
typedef struct iron_method_impl_row {
    iron_u32 class_;            /* Index into TypeDef */
    iron_u32 method_body;       /* Coded index (MethodDefOrRef) */
    iron_u32 method_declaration; /* Coded index (MethodDefOrRef) */
} iron_method_impl_row_t;

/* ModuleRef table (0x1A) */
typedef struct iron_module_ref_row {
    iron_u32 name;              /* String heap index */
} iron_module_ref_row_t;

/* TypeSpec table (0x1B) */
typedef struct iron_type_spec_row {
    iron_u32 signature;         /* Blob heap index */
} iron_type_spec_row_t;

/* ImplMap table (0x1C) */
typedef struct iron_impl_map_row {
    iron_u16 mapping_flags;
    iron_u32 member_forwarded;  /* Coded index (MemberForwarded) */
    iron_u32 import_name;       /* String heap index */
    iron_u32 import_scope;      /* Index into ModuleRef */
} iron_impl_map_row_t;

/* FieldRVA table (0x1D) */
typedef struct iron_field_rva_row {
    iron_u32 rva;
    iron_u32 field;             /* Index into Field */
} iron_field_rva_row_t;

/* Assembly table (0x20) */
typedef struct iron_assembly_row {
    iron_u32 hash_alg_id;
    iron_u16 major_version;
    iron_u16 minor_version;
    iron_u16 build_number;
    iron_u16 revision_number;
    iron_u32 flags;
    iron_u32 public_key;        /* Blob heap index */
    iron_u32 name;              /* String heap index */
    iron_u32 culture;           /* String heap index */
} iron_assembly_row_t;

/* AssemblyRef table (0x23) */
typedef struct iron_assembly_ref_row {
    iron_u16 major_version;
    iron_u16 minor_version;
    iron_u16 build_number;
    iron_u16 revision_number;
    iron_u32 flags;
    iron_u32 public_key_or_token; /* Blob heap index */
    iron_u32 name;              /* String heap index */
    iron_u32 culture;           /* String heap index */
    iron_u32 hash_value;        /* Blob heap index */
} iron_assembly_ref_row_t;

/* File table (0x26) */
typedef struct iron_file_row {
    iron_u32 flags;
    iron_u32 name;              /* String heap index */
    iron_u32 hash_value;        /* Blob heap index */
} iron_file_row_t;

/* ExportedType table (0x27) */
typedef struct iron_exported_type_row {
    iron_u32 flags;
    iron_u32 type_def_id;
    iron_u32 name;              /* String heap index */
    iron_u32 namespace_;        /* String heap index */
    iron_u32 implementation;    /* Coded index (Implementation) */
} iron_exported_type_row_t;

/* ManifestResource table (0x28) */
typedef struct iron_manifest_resource_row {
    iron_u32 offset;
    iron_u32 flags;
    iron_u32 name;              /* String heap index */
    iron_u32 implementation;    /* Coded index (Implementation) */
} iron_manifest_resource_row_t;

/* NestedClass table (0x29) */
typedef struct iron_nested_class_row {
    iron_u32 nested_class;      /* Index into TypeDef */
    iron_u32 enclosing_class;   /* Index into TypeDef */
} iron_nested_class_row_t;

/* GenericParam table (0x2A) */
typedef struct iron_generic_param_row {
    iron_u16 number;
    iron_u16 flags;
    iron_u32 owner;             /* Coded index (TypeOrMethodDef) */
    iron_u32 name;              /* String heap index */
} iron_generic_param_row_t;

/* MethodSpec table (0x2B) */
typedef struct iron_method_spec_row {
    iron_u32 method;            /* Coded index (MethodDefOrRef) */
    iron_u32 instantiation;     /* Blob heap index */
} iron_method_spec_row_t;

/* GenericParamConstraint table (0x2C) */
typedef struct iron_generic_param_constraint_row {
    iron_u32 owner;             /* Index into GenericParam */
    iron_u32 constraint;        /* Coded index (TypeDefOrRef) */
} iron_generic_param_constraint_row_t;

/* ============================================================================
 * Metadata Table
 * ============================================================================ */

typedef struct iron_metadata_table {
    iron_u32 row_count;
    iron_u32 row_size;
    const iron_u8 *data;
} iron_metadata_table_t;

/* ============================================================================
 * Metadata Reader
 * ============================================================================ */

typedef struct iron_metadata {
    iron_allocator_t *allocator;
    const iron_pe_image_t *image;
    
    /* Metadata root */
    const iron_u8 *base;
    iron_size size;
    char version[256];
    
    /* Streams */
    struct {
        const iron_u8 *data;
        iron_size size;
    } streams[IRON_STREAM_COUNT];
    
    /* Tables */
    iron_metadata_table_t tables[IRON_TABLE_COUNT];
    
    /* Index sizes */
    iron_bool string_index_large;
    iron_bool guid_index_large;
    iron_bool blob_index_large;
    iron_u8 coded_index_sizes[IRON_CODED_COUNT];
    iron_u8 table_index_sizes[IRON_TABLE_COUNT];
} iron_metadata_t;

/* ============================================================================
 * Metadata API
 * ============================================================================ */

/* Initialize metadata reader from PE image */
IRON_API iron_result_t iron_metadata_init(iron_metadata_t *meta,
                                           const iron_pe_image_t *image,
                                           iron_allocator_t *alloc);

/* Free metadata reader */
IRON_API void iron_metadata_free(iron_metadata_t *meta);

/* Get string from #Strings heap */
IRON_API const char *iron_metadata_get_string(const iron_metadata_t *meta,
                                               iron_u32 index);

/* Get user string from #US heap */
IRON_API iron_result_t iron_metadata_get_user_string(const iron_metadata_t *meta,
                                                      iron_u32 index,
                                                      const iron_u16 **out_str,
                                                      iron_u32 *out_len);

/* Get GUID from #GUID heap */
IRON_API const iron_u8 *iron_metadata_get_guid(const iron_metadata_t *meta,
                                                iron_u32 index);

/* Get blob from #Blob heap */
IRON_API iron_result_t iron_metadata_get_blob(const iron_metadata_t *meta,
                                               iron_u32 index,
                                               const iron_u8 **out_data,
                                               iron_u32 *out_size);

/* Get table row count */
IRON_API iron_u32 iron_metadata_table_rows(const iron_metadata_t *meta,
                                            iron_table_id_t table);

/* Read table row */
IRON_API iron_result_t iron_metadata_read_row(const iron_metadata_t *meta,
                                               iron_token_t token,
                                               void *out_row);

/* Decode coded index */
IRON_API iron_token_t iron_metadata_decode_coded(const iron_metadata_t *meta,
                                                  iron_coded_index_t type,
                                                  iron_u32 coded);

/* ============================================================================
 * Signature Parsing
 * ============================================================================ */

/* Signature reader state */
typedef struct iron_sig_reader {
    const iron_u8 *data;
    iron_size size;
    iron_size pos;
} iron_sig_reader_t;

/* Initialize signature reader from blob */
IRON_API void iron_sig_init(iron_sig_reader_t *reader, 
                             const iron_u8 *data, iron_size size);

/* Read compressed unsigned integer */
IRON_API iron_result_t iron_sig_read_compressed_u32(iron_sig_reader_t *reader,
                                                     iron_u32 *out);

/* Read compressed signed integer */
IRON_API iron_result_t iron_sig_read_compressed_i32(iron_sig_reader_t *reader,
                                                     iron_i32 *out);

/* Read element type */
IRON_API iron_result_t iron_sig_read_element_type(iron_sig_reader_t *reader,
                                                   iron_element_type_t *out);

/* Read type def or ref encoded token */
IRON_API iron_result_t iron_sig_read_type_def_or_ref(iron_sig_reader_t *reader,
                                                      iron_token_t *out);

/* Check if more data available */
IRON_API iron_bool iron_sig_has_more(const iron_sig_reader_t *reader);

/* ============================================================================
 * Method Body Parsing (ECMA-335 II.25.4)
 * ============================================================================ */

/* Method header flags */
typedef enum iron_method_header_flags {
    IRON_METHOD_TINY_FORMAT  = 0x02,
    IRON_METHOD_FAT_FORMAT   = 0x03,
    IRON_METHOD_MORE_SECTS   = 0x08,
    IRON_METHOD_INIT_LOCALS  = 0x10
} iron_method_header_flags_t;

/* Exception clause flags */
typedef enum iron_exception_flags {
    IRON_EX_CLAUSE_EXCEPTION = 0x0000,
    IRON_EX_CLAUSE_FILTER    = 0x0001,
    IRON_EX_CLAUSE_FINALLY   = 0x0002,
    IRON_EX_CLAUSE_FAULT     = 0x0004,
    IRON_EX_CLAUSE_FAT       = 0x0040
} iron_exception_flags_t;

/* Exception clause */
typedef struct iron_exception_clause {
    iron_u32 flags;
    iron_u32 try_offset;
    iron_u32 try_length;
    iron_u32 handler_offset;
    iron_u32 handler_length;
    union {
        iron_token_t class_token;  /* For exception clause */
        iron_u32 filter_offset;    /* For filter clause */
    } u;
} iron_exception_clause_t;

/* Method body */
typedef struct iron_method_body {
    iron_bool is_fat;
    iron_u16 max_stack;
    iron_u32 code_size;
    iron_u32 local_var_sig_token;
    iron_bool init_locals;
    const iron_u8 *code;
    
    /* Exception handling */
    iron_u32 exception_count;
    iron_exception_clause_t *exceptions;
} iron_method_body_t;

/* Parse method body from RVA */
IRON_API iron_result_t iron_parse_method_body(const iron_pe_image_t *image,
                                               iron_u32 rva,
                                               iron_method_body_t *body,
                                               iron_allocator_t *alloc);

/* Free method body */
IRON_API void iron_free_method_body(iron_method_body_t *body,
                                     iron_allocator_t *alloc);

#ifdef __cplusplus
}
#endif

#endif /* IRON_METADATA_H */
