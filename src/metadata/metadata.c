/*
 * IronNet CLR Interpreter
 * metadata.c - CLI Metadata reader implementation
 */

#include "iron/metadata.h"
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * Coded Index Tables (ECMA-335 II.24.2.6)
 * ============================================================================ */

/* Tables for each coded index type */
static const iron_table_id_t g_type_def_or_ref_tables[] = {
    IRON_TABLE_TYPE_DEF, IRON_TABLE_TYPE_REF, IRON_TABLE_TYPE_SPEC
};

static const iron_table_id_t g_has_constant_tables[] = {
    IRON_TABLE_FIELD, IRON_TABLE_PARAM, IRON_TABLE_PROPERTY
};

static const iron_table_id_t g_has_custom_attribute_tables[] = {
    IRON_TABLE_METHOD_DEF, IRON_TABLE_FIELD, IRON_TABLE_TYPE_REF,
    IRON_TABLE_TYPE_DEF, IRON_TABLE_PARAM, IRON_TABLE_INTERFACE_IMPL,
    IRON_TABLE_MEMBER_REF, IRON_TABLE_MODULE, /* Permission */ 0xFF,
    IRON_TABLE_PROPERTY, IRON_TABLE_EVENT, IRON_TABLE_STANDALONE_SIG,
    IRON_TABLE_MODULE_REF, IRON_TABLE_TYPE_SPEC, IRON_TABLE_ASSEMBLY,
    IRON_TABLE_ASSEMBLY_REF, IRON_TABLE_FILE, IRON_TABLE_EXPORTED_TYPE,
    IRON_TABLE_MANIFEST_RESOURCE, IRON_TABLE_GENERIC_PARAM,
    IRON_TABLE_GENERIC_PARAM_CONSTRAINT, IRON_TABLE_METHOD_SPEC
};

static const iron_table_id_t g_has_field_marshal_tables[] = {
    IRON_TABLE_FIELD, IRON_TABLE_PARAM
};

static const iron_table_id_t g_has_decl_security_tables[] = {
    IRON_TABLE_TYPE_DEF, IRON_TABLE_METHOD_DEF, IRON_TABLE_ASSEMBLY
};

static const iron_table_id_t g_member_ref_parent_tables[] = {
    IRON_TABLE_TYPE_DEF, IRON_TABLE_TYPE_REF, IRON_TABLE_MODULE_REF,
    IRON_TABLE_METHOD_DEF, IRON_TABLE_TYPE_SPEC
};

static const iron_table_id_t g_has_semantics_tables[] = {
    IRON_TABLE_EVENT, IRON_TABLE_PROPERTY
};

static const iron_table_id_t g_method_def_or_ref_tables[] = {
    IRON_TABLE_METHOD_DEF, IRON_TABLE_MEMBER_REF
};

static const iron_table_id_t g_member_forwarded_tables[] = {
    IRON_TABLE_FIELD, IRON_TABLE_METHOD_DEF
};

static const iron_table_id_t g_implementation_tables[] = {
    IRON_TABLE_FILE, IRON_TABLE_ASSEMBLY_REF, IRON_TABLE_EXPORTED_TYPE
};

static const iron_table_id_t g_custom_attribute_type_tables[] = {
    0xFF, 0xFF, IRON_TABLE_METHOD_DEF, IRON_TABLE_MEMBER_REF, 0xFF
};

static const iron_table_id_t g_resolution_scope_tables[] = {
    IRON_TABLE_MODULE, IRON_TABLE_MODULE_REF, IRON_TABLE_ASSEMBLY_REF,
    IRON_TABLE_TYPE_REF
};

static const iron_table_id_t g_type_or_method_def_tables[] = {
    IRON_TABLE_TYPE_DEF, IRON_TABLE_METHOD_DEF
};

/* Coded index info */
typedef struct {
    const iron_table_id_t *tables;
    iron_u8 table_count;
    iron_u8 tag_bits;
} coded_index_info_t;

static const coded_index_info_t g_coded_index_info[] = {
    { g_type_def_or_ref_tables, 3, 2 },
    { g_has_constant_tables, 3, 2 },
    { g_has_custom_attribute_tables, 22, 5 },
    { g_has_field_marshal_tables, 2, 1 },
    { g_has_decl_security_tables, 3, 2 },
    { g_member_ref_parent_tables, 5, 3 },
    { g_has_semantics_tables, 2, 1 },
    { g_method_def_or_ref_tables, 2, 1 },
    { g_member_forwarded_tables, 2, 1 },
    { g_implementation_tables, 3, 2 },
    { g_custom_attribute_type_tables, 5, 3 },
    { g_resolution_scope_tables, 4, 2 },
    { g_type_or_method_def_tables, 2, 1 }
};

/* ============================================================================
 * Internal Helpers
 * ============================================================================ */

static iron_u8 compute_coded_index_size(const iron_metadata_t *meta,
                                         iron_coded_index_t type)
{
    const coded_index_info_t *info = &g_coded_index_info[type];
    iron_u32 max_rows = 0;
    iron_u8 i;
    
    for (i = 0; i < info->table_count; i++) {
        iron_table_id_t table = info->tables[i];
        if (table != 0xFF && meta->tables[table].row_count > max_rows) {
            max_rows = meta->tables[table].row_count;
        }
    }
    
    /* If max_rows << tag_bits fits in 16 bits, use 2 bytes, else 4 */
    if ((max_rows << info->tag_bits) < 0x10000) {
        return 2;
    }
    return 4;
}

static iron_u8 compute_table_index_size(const iron_metadata_t *meta,
                                         iron_table_id_t table)
{
    if (meta->tables[table].row_count < 0x10000) {
        return 2;
    }
    return 4;
}

static iron_u32 read_index(const iron_u8 **ptr, iron_u8 size)
{
    iron_u32 value;
    if (size == 2) {
        value = iron_read_u16_le(*ptr);
        *ptr += 2;
    } else {
        value = iron_read_u32_le(*ptr);
        *ptr += 4;
    }
    return value;
}

/* ============================================================================
 * Stream Parsing
 * ============================================================================ */

static iron_result_t parse_streams(iron_metadata_t *meta, const iron_u8 *base,
                                    iron_size size, iron_u16 stream_count)
{
    const iron_u8 *ptr = base;
    iron_u16 i;
    
    for (i = 0; i < stream_count; i++) {
        iron_u32 offset, stream_size;
        const char *name;
        iron_size name_len;
        iron_stream_type_t type;
        
        if (ptr + 8 > base + size) {
            return IRON_ERROR(IRON_ERR_INVALID_STREAM, "Stream header truncated");
        }
        
        offset = iron_read_u32_le(ptr);
        stream_size = iron_read_u32_le(ptr + 4);
        ptr += 8;
        
        name = (const char *)ptr;
        name_len = strlen(name);
        
        /* Align to 4 bytes */
        ptr += (name_len + 4) & ~3;
        
        /* Identify stream type */
        if (strcmp(name, "#~") == 0 || strcmp(name, "#-") == 0) {
            type = IRON_STREAM_TABLES;
        } else if (strcmp(name, "#Strings") == 0) {
            type = IRON_STREAM_STRINGS;
        } else if (strcmp(name, "#US") == 0) {
            type = IRON_STREAM_US;
        } else if (strcmp(name, "#GUID") == 0) {
            type = IRON_STREAM_GUID;
        } else if (strcmp(name, "#Blob") == 0) {
            type = IRON_STREAM_BLOB;
        } else {
            continue; /* Unknown stream, skip */
        }
        
        meta->streams[type].data = meta->base + offset;
        meta->streams[type].size = stream_size;
    }
    
    return (iron_result_t)IRON_SUCCESS;
}

/* ============================================================================
 * Tables Parsing
 * ============================================================================ */

/* Row sizes for each table (computed dynamically based on heap sizes) */
static iron_u32 compute_row_size(const iron_metadata_t *meta, iron_table_id_t table)
{
    iron_u32 size = 0;
    iron_u8 str_size = meta->string_index_large ? 4 : 2;
    iron_u8 guid_size = meta->guid_index_large ? 4 : 2;
    iron_u8 blob_size = meta->blob_index_large ? 4 : 2;
    
    switch (table) {
        case IRON_TABLE_MODULE:
            size = 2 + str_size + guid_size * 3;
            break;
        case IRON_TABLE_TYPE_REF:
            size = meta->coded_index_sizes[IRON_CODED_RESOLUTION_SCOPE] +
                   str_size * 2;
            break;
        case IRON_TABLE_TYPE_DEF:
            size = 4 + str_size * 2 +
                   meta->coded_index_sizes[IRON_CODED_TYPE_DEF_OR_REF] +
                   meta->table_index_sizes[IRON_TABLE_FIELD] +
                   meta->table_index_sizes[IRON_TABLE_METHOD_DEF];
            break;
        case IRON_TABLE_FIELD:
            size = 2 + str_size + blob_size;
            break;
        case IRON_TABLE_METHOD_DEF:
            size = 4 + 2 + 2 + str_size + blob_size +
                   meta->table_index_sizes[IRON_TABLE_PARAM];
            break;
        case IRON_TABLE_PARAM:
            size = 2 + 2 + str_size;
            break;
        case IRON_TABLE_INTERFACE_IMPL:
            size = meta->table_index_sizes[IRON_TABLE_TYPE_DEF] +
                   meta->coded_index_sizes[IRON_CODED_TYPE_DEF_OR_REF];
            break;
        case IRON_TABLE_MEMBER_REF:
            size = meta->coded_index_sizes[IRON_CODED_MEMBER_REF_PARENT] +
                   str_size + blob_size;
            break;
        case IRON_TABLE_CONSTANT:
            size = 2 + meta->coded_index_sizes[IRON_CODED_HAS_CONSTANT] + blob_size;
            break;
        case IRON_TABLE_CUSTOM_ATTRIBUTE:
            size = meta->coded_index_sizes[IRON_CODED_HAS_CUSTOM_ATTRIBUTE] +
                   meta->coded_index_sizes[IRON_CODED_CUSTOM_ATTRIBUTE_TYPE] +
                   blob_size;
            break;
        case IRON_TABLE_FIELD_MARSHAL:
            size = meta->coded_index_sizes[IRON_CODED_HAS_FIELD_MARSHAL] + blob_size;
            break;
        case IRON_TABLE_DECL_SECURITY:
            size = 2 + meta->coded_index_sizes[IRON_CODED_HAS_DECL_SECURITY] + blob_size;
            break;
        case IRON_TABLE_CLASS_LAYOUT:
            size = 2 + 4 + meta->table_index_sizes[IRON_TABLE_TYPE_DEF];
            break;
        case IRON_TABLE_FIELD_LAYOUT:
            size = 4 + meta->table_index_sizes[IRON_TABLE_FIELD];
            break;
        case IRON_TABLE_STANDALONE_SIG:
            size = blob_size;
            break;
        case IRON_TABLE_EVENT_MAP:
            size = meta->table_index_sizes[IRON_TABLE_TYPE_DEF] +
                   meta->table_index_sizes[IRON_TABLE_EVENT];
            break;
        case IRON_TABLE_EVENT:
            size = 2 + str_size + meta->coded_index_sizes[IRON_CODED_TYPE_DEF_OR_REF];
            break;
        case IRON_TABLE_PROPERTY_MAP:
            size = meta->table_index_sizes[IRON_TABLE_TYPE_DEF] +
                   meta->table_index_sizes[IRON_TABLE_PROPERTY];
            break;
        case IRON_TABLE_PROPERTY:
            size = 2 + str_size + blob_size;
            break;
        case IRON_TABLE_METHOD_SEMANTICS:
            size = 2 + meta->table_index_sizes[IRON_TABLE_METHOD_DEF] +
                   meta->coded_index_sizes[IRON_CODED_HAS_SEMANTICS];
            break;
        case IRON_TABLE_METHOD_IMPL:
            size = meta->table_index_sizes[IRON_TABLE_TYPE_DEF] +
                   meta->coded_index_sizes[IRON_CODED_METHOD_DEF_OR_REF] * 2;
            break;
        case IRON_TABLE_MODULE_REF:
            size = str_size;
            break;
        case IRON_TABLE_TYPE_SPEC:
            size = blob_size;
            break;
        case IRON_TABLE_IMPL_MAP:
            size = 2 + meta->coded_index_sizes[IRON_CODED_MEMBER_FORWARDED] +
                   str_size + meta->table_index_sizes[IRON_TABLE_MODULE_REF];
            break;
        case IRON_TABLE_FIELD_RVA:
            size = 4 + meta->table_index_sizes[IRON_TABLE_FIELD];
            break;
        case IRON_TABLE_ASSEMBLY:
            size = 4 + 2 * 4 + 4 + blob_size + str_size * 2;
            break;
        case IRON_TABLE_ASSEMBLY_REF:
            size = 2 * 4 + 4 + blob_size * 2 + str_size * 2;
            break;
        case IRON_TABLE_FILE:
            size = 4 + str_size + blob_size;
            break;
        case IRON_TABLE_EXPORTED_TYPE:
            size = 4 + 4 + str_size * 2 +
                   meta->coded_index_sizes[IRON_CODED_IMPLEMENTATION];
            break;
        case IRON_TABLE_MANIFEST_RESOURCE:
            size = 4 + 4 + str_size +
                   meta->coded_index_sizes[IRON_CODED_IMPLEMENTATION];
            break;
        case IRON_TABLE_NESTED_CLASS:
            size = meta->table_index_sizes[IRON_TABLE_TYPE_DEF] * 2;
            break;
        case IRON_TABLE_GENERIC_PARAM:
            size = 2 + 2 + meta->coded_index_sizes[IRON_CODED_TYPE_OR_METHOD_DEF] +
                   str_size;
            break;
        case IRON_TABLE_METHOD_SPEC:
            size = meta->coded_index_sizes[IRON_CODED_METHOD_DEF_OR_REF] + blob_size;
            break;
        case IRON_TABLE_GENERIC_PARAM_CONSTRAINT:
            size = meta->table_index_sizes[IRON_TABLE_GENERIC_PARAM] +
                   meta->coded_index_sizes[IRON_CODED_TYPE_DEF_OR_REF];
            break;
        default:
            size = 0;
            break;
    }
    
    return size;
}

static iron_result_t parse_tables_stream(iron_metadata_t *meta)
{
    const iron_u8 *ptr;
    iron_tables_header_t header;
    iron_u8 i;
    iron_u64 valid;
    const iron_u8 *row_data;
    
    if (!meta->streams[IRON_STREAM_TABLES].data) {
        return IRON_ERROR(IRON_ERR_INVALID_STREAM, "No tables stream");
    }
    
    ptr = meta->streams[IRON_STREAM_TABLES].data;
    
    /* Read tables header */
    header.reserved = iron_read_u32_le(ptr);
    header.major_version = ptr[4];
    header.minor_version = ptr[5];
    header.heap_sizes = ptr[6];
    header.reserved2 = ptr[7];
    header.valid = iron_read_u64_le(ptr + 8);
    header.sorted = iron_read_u64_le(ptr + 16);
    
    ptr += 24;
    
    /* Set heap index sizes */
    meta->string_index_large = (header.heap_sizes & IRON_HEAP_STRING_LARGE) != 0;
    meta->guid_index_large = (header.heap_sizes & IRON_HEAP_GUID_LARGE) != 0;
    meta->blob_index_large = (header.heap_sizes & IRON_HEAP_BLOB_LARGE) != 0;
    
    /* Read row counts for valid tables */
    valid = header.valid;
    for (i = 0; i < IRON_TABLE_COUNT; i++) {
        if (valid & ((iron_u64)1 << i)) {
            meta->tables[i].row_count = iron_read_u32_le(ptr);
            ptr += 4;
        } else {
            meta->tables[i].row_count = 0;
        }
    }
    
    /* Compute index sizes */
    for (i = 0; i < IRON_TABLE_COUNT; i++) {
        meta->table_index_sizes[i] = compute_table_index_size(meta, (iron_table_id_t)i);
    }
    for (i = 0; i < IRON_CODED_COUNT; i++) {
        meta->coded_index_sizes[i] = compute_coded_index_size(meta, (iron_coded_index_t)i);
    }
    
    /* Compute row sizes and set data pointers */
    row_data = ptr;
    for (i = 0; i < IRON_TABLE_COUNT; i++) {
        if (meta->tables[i].row_count > 0) {
            meta->tables[i].row_size = compute_row_size(meta, (iron_table_id_t)i);
            meta->tables[i].data = row_data;
            row_data += meta->tables[i].row_count * meta->tables[i].row_size;
        }
    }
    
    return (iron_result_t)IRON_SUCCESS;
}

/* ============================================================================
 * Public API
 * ============================================================================ */

iron_result_t iron_metadata_init(iron_metadata_t *meta,
                                  const iron_pe_image_t *image,
                                  iron_allocator_t *alloc)
{
    iron_result_t result;
    const iron_u8 *metadata_ptr;
    iron_metadata_header_t header;
    iron_u16 stream_count;
    const iron_u8 *stream_headers;
    
    if (!meta || !image) {
        return IRON_ERROR(IRON_ERR_NULL_POINTER, "Invalid parameters");
    }
    
    if (!image->has_cli_header) {
        return IRON_ERROR(IRON_ERR_NOT_CLI_IMAGE, "Not a CLI image");
    }
    
    memset(meta, 0, sizeof(iron_metadata_t));
    meta->allocator = alloc ? alloc : iron_system_allocator();
    meta->image = image;
    
    /* Get metadata root */
    metadata_ptr = iron_pe_rva_to_ptr(image, image->cli_header.metadata.virtual_address);
    if (!metadata_ptr) {
        return IRON_ERROR(IRON_ERR_INVALID_METADATA, "Cannot locate metadata");
    }
    
    meta->base = metadata_ptr;
    meta->size = image->cli_header.metadata.size;
    
    /* Read metadata header */
    header.signature = iron_read_u32_le(metadata_ptr);
    if (header.signature != IRON_METADATA_SIGNATURE) {
        return IRON_ERROR(IRON_ERR_INVALID_METADATA, "Invalid metadata signature");
    }
    
    header.major_version = iron_read_u16_le(metadata_ptr + 4);
    header.minor_version = iron_read_u16_le(metadata_ptr + 6);
    header.reserved = iron_read_u32_le(metadata_ptr + 8);
    header.version_length = iron_read_u32_le(metadata_ptr + 12);
    
    /* Copy version string */
    if (header.version_length > 0 && header.version_length < sizeof(meta->version)) {
        memcpy(meta->version, metadata_ptr + 16, header.version_length);
        meta->version[header.version_length] = '\0';
    }
    
    /* Skip to stream count (after version string, aligned to 4 bytes) */
    stream_headers = metadata_ptr + 16 + ((header.version_length + 3) & ~3);
    
    /* Read flags (2 bytes) and stream count (2 bytes) */
    stream_count = iron_read_u16_le(stream_headers + 2);
    stream_headers += 4;
    
    /* Parse streams */
    result = parse_streams(meta, stream_headers, 
                          meta->size - (stream_headers - metadata_ptr),
                          stream_count);
    if (!IRON_RESULT_OK(result)) {
        return result;
    }
    
    /* Parse tables stream */
    result = parse_tables_stream(meta);
    if (!IRON_RESULT_OK(result)) {
        return result;
    }
    
    return (iron_result_t)IRON_SUCCESS;
}

void iron_metadata_free(iron_metadata_t *meta)
{
    if (!meta) return;
    memset(meta, 0, sizeof(iron_metadata_t));
}

const char *iron_metadata_get_string(const iron_metadata_t *meta, iron_u32 index)
{
    if (!meta || !meta->streams[IRON_STREAM_STRINGS].data) return NULL;
    if (index >= meta->streams[IRON_STREAM_STRINGS].size) return NULL;
    return (const char *)(meta->streams[IRON_STREAM_STRINGS].data + index);
}

iron_result_t iron_metadata_get_user_string(const iron_metadata_t *meta,
                                             iron_u32 index,
                                             const iron_u16 **out_str,
                                             iron_u32 *out_len)
{
    const iron_u8 *ptr;
    iron_u32 blob_len;
    
    if (!meta || !out_str || !out_len) {
        return IRON_ERROR(IRON_ERR_NULL_POINTER, "Invalid parameters");
    }
    
    if (!meta->streams[IRON_STREAM_US].data || 
        index >= meta->streams[IRON_STREAM_US].size) {
        return IRON_ERROR(IRON_ERR_INVALID_STRING, "Invalid user string index");
    }
    
    ptr = meta->streams[IRON_STREAM_US].data + index;
    
    /* Read compressed length */
    if (*ptr < 0x80) {
        blob_len = *ptr++;
    } else if (*ptr < 0xC0) {
        blob_len = ((*ptr & 0x3F) << 8) | ptr[1];
        ptr += 2;
    } else {
        blob_len = ((*ptr & 0x1F) << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
        ptr += 4;
    }
    
    /* User strings have a trailing byte, actual char count is (len-1)/2 */
    *out_str = (const iron_u16 *)ptr;
    *out_len = (blob_len > 0) ? (blob_len - 1) / 2 : 0;
    
    return (iron_result_t)IRON_SUCCESS;
}

const iron_u8 *iron_metadata_get_guid(const iron_metadata_t *meta, iron_u32 index)
{
    if (!meta || !meta->streams[IRON_STREAM_GUID].data) return NULL;
    if (index == 0) return NULL; /* GUID indices are 1-based */
    index--; /* Convert to 0-based */
    if (index * 16 >= meta->streams[IRON_STREAM_GUID].size) return NULL;
    return meta->streams[IRON_STREAM_GUID].data + index * 16;
}

iron_result_t iron_metadata_get_blob(const iron_metadata_t *meta,
                                      iron_u32 index,
                                      const iron_u8 **out_data,
                                      iron_u32 *out_size)
{
    const iron_u8 *ptr;
    iron_u32 blob_len;
    
    if (!meta || !out_data || !out_size) {
        return IRON_ERROR(IRON_ERR_NULL_POINTER, "Invalid parameters");
    }
    
    if (!meta->streams[IRON_STREAM_BLOB].data ||
        index >= meta->streams[IRON_STREAM_BLOB].size) {
        return IRON_ERROR(IRON_ERR_INVALID_BLOB, "Invalid blob index");
    }
    
    ptr = meta->streams[IRON_STREAM_BLOB].data + index;
    
    /* Read compressed length */
    if (*ptr < 0x80) {
        blob_len = *ptr++;
    } else if (*ptr < 0xC0) {
        blob_len = ((*ptr & 0x3F) << 8) | ptr[1];
        ptr += 2;
    } else {
        blob_len = ((*ptr & 0x1F) << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
        ptr += 4;
    }
    
    *out_data = ptr;
    *out_size = blob_len;
    
    return (iron_result_t)IRON_SUCCESS;
}

iron_u32 iron_metadata_table_rows(const iron_metadata_t *meta, iron_table_id_t table)
{
    if (!meta || table >= IRON_TABLE_COUNT) return 0;
    return meta->tables[table].row_count;
}

iron_token_t iron_metadata_decode_coded(const iron_metadata_t *meta,
                                         iron_coded_index_t type,
                                         iron_u32 coded)
{
    const coded_index_info_t *info;
    iron_u32 tag, index;
    iron_table_id_t table;
    
    if (!meta || type >= IRON_CODED_COUNT) return IRON_TOKEN_NIL;
    
    info = &g_coded_index_info[type];
    tag = coded & ((1 << info->tag_bits) - 1);
    index = coded >> info->tag_bits;
    
    if (tag >= info->table_count) return IRON_TOKEN_NIL;
    table = info->tables[tag];
    if (table == 0xFF) return IRON_TOKEN_NIL;
    
    return IRON_MAKE_TOKEN(table, index);
}

/* ============================================================================
 * Row Reading
 * ============================================================================ */

iron_result_t iron_metadata_read_row(const iron_metadata_t *meta,
                                      iron_token_t token,
                                      void *out_row)
{
    iron_table_id_t table;
    iron_u32 index;
    const iron_u8 *ptr;
    iron_u8 str_size, guid_size, blob_size;
    
    if (!meta || !out_row) {
        return IRON_ERROR(IRON_ERR_NULL_POINTER, "Invalid parameters");
    }
    
    table = (iron_table_id_t)IRON_TOKEN_TABLE(token);
    index = IRON_TOKEN_INDEX(token);
    
    if (table >= IRON_TABLE_COUNT || index == 0 || 
        index > meta->tables[table].row_count) {
        return IRON_ERROR(IRON_ERR_INVALID_TOKEN, "Invalid token");
    }
    
    ptr = meta->tables[table].data + (index - 1) * meta->tables[table].row_size;
    
    str_size = meta->string_index_large ? 4 : 2;
    guid_size = meta->guid_index_large ? 4 : 2;
    blob_size = meta->blob_index_large ? 4 : 2;
    
    switch (table) {
        case IRON_TABLE_MODULE: {
            iron_module_row_t *row = (iron_module_row_t *)out_row;
            row->generation = iron_read_u16_le(ptr); ptr += 2;
            row->name = read_index(&ptr, str_size);
            row->mvid = read_index(&ptr, guid_size);
            row->enc_id = read_index(&ptr, guid_size);
            row->enc_base_id = read_index(&ptr, guid_size);
            break;
        }
        case IRON_TABLE_TYPE_REF: {
            iron_type_ref_row_t *row = (iron_type_ref_row_t *)out_row;
            row->resolution_scope = read_index(&ptr, 
                meta->coded_index_sizes[IRON_CODED_RESOLUTION_SCOPE]);
            row->name = read_index(&ptr, str_size);
            row->namespace_ = read_index(&ptr, str_size);
            break;
        }
        case IRON_TABLE_TYPE_DEF: {
            iron_type_def_row_t *row = (iron_type_def_row_t *)out_row;
            row->flags = iron_read_u32_le(ptr); ptr += 4;
            row->name = read_index(&ptr, str_size);
            row->namespace_ = read_index(&ptr, str_size);
            row->extends = read_index(&ptr, 
                meta->coded_index_sizes[IRON_CODED_TYPE_DEF_OR_REF]);
            row->field_list = read_index(&ptr, 
                meta->table_index_sizes[IRON_TABLE_FIELD]);
            row->method_list = read_index(&ptr, 
                meta->table_index_sizes[IRON_TABLE_METHOD_DEF]);
            break;
        }
        case IRON_TABLE_FIELD: {
            iron_field_row_t *row = (iron_field_row_t *)out_row;
            row->flags = iron_read_u16_le(ptr); ptr += 2;
            row->name = read_index(&ptr, str_size);
            row->signature = read_index(&ptr, blob_size);
            break;
        }
        case IRON_TABLE_METHOD_DEF: {
            iron_method_def_row_t *row = (iron_method_def_row_t *)out_row;
            row->rva = iron_read_u32_le(ptr); ptr += 4;
            row->impl_flags = iron_read_u16_le(ptr); ptr += 2;
            row->flags = iron_read_u16_le(ptr); ptr += 2;
            row->name = read_index(&ptr, str_size);
            row->signature = read_index(&ptr, blob_size);
            row->param_list = read_index(&ptr, 
                meta->table_index_sizes[IRON_TABLE_PARAM]);
            break;
        }
        case IRON_TABLE_PARAM: {
            iron_param_row_t *row = (iron_param_row_t *)out_row;
            row->flags = iron_read_u16_le(ptr); ptr += 2;
            row->sequence = iron_read_u16_le(ptr); ptr += 2;
            row->name = read_index(&ptr, str_size);
            break;
        }
        case IRON_TABLE_ASSEMBLY: {
            iron_assembly_row_t *row = (iron_assembly_row_t *)out_row;
            row->hash_alg_id = iron_read_u32_le(ptr); ptr += 4;
            row->major_version = iron_read_u16_le(ptr); ptr += 2;
            row->minor_version = iron_read_u16_le(ptr); ptr += 2;
            row->build_number = iron_read_u16_le(ptr); ptr += 2;
            row->revision_number = iron_read_u16_le(ptr); ptr += 2;
            row->flags = iron_read_u32_le(ptr); ptr += 4;
            row->public_key = read_index(&ptr, blob_size);
            row->name = read_index(&ptr, str_size);
            row->culture = read_index(&ptr, str_size);
            break;
        }
        case IRON_TABLE_ASSEMBLY_REF: {
            iron_assembly_ref_row_t *row = (iron_assembly_ref_row_t *)out_row;
            row->major_version = iron_read_u16_le(ptr); ptr += 2;
            row->minor_version = iron_read_u16_le(ptr); ptr += 2;
            row->build_number = iron_read_u16_le(ptr); ptr += 2;
            row->revision_number = iron_read_u16_le(ptr); ptr += 2;
            row->flags = iron_read_u32_le(ptr); ptr += 4;
            row->public_key_or_token = read_index(&ptr, blob_size);
            row->name = read_index(&ptr, str_size);
            row->culture = read_index(&ptr, str_size);
            row->hash_value = read_index(&ptr, blob_size);
            break;
        }
        case IRON_TABLE_GENERIC_PARAM: {
            iron_generic_param_row_t *row = (iron_generic_param_row_t *)out_row;
            row->number = iron_read_u16_le(ptr); ptr += 2;
            row->flags = iron_read_u16_le(ptr); ptr += 2;
            row->owner = read_index(&ptr, 
                meta->coded_index_sizes[IRON_CODED_TYPE_OR_METHOD_DEF]);
            row->name = read_index(&ptr, str_size);
            break;
        }
        case IRON_TABLE_MEMBER_REF: {
            iron_member_ref_row_t *row = (iron_member_ref_row_t *)out_row;
            row->class_ = read_index(&ptr, 
                meta->coded_index_sizes[IRON_CODED_MEMBER_REF_PARENT]);
            row->name = read_index(&ptr, str_size);
            row->signature = read_index(&ptr, blob_size);
            break;
        }
        case IRON_TABLE_STANDALONE_SIG: {
            iron_standalone_sig_row_t *row = (iron_standalone_sig_row_t *)out_row;
            row->signature = read_index(&ptr, blob_size);
            break;
        }
        case IRON_TABLE_TYPE_SPEC: {
            iron_type_spec_row_t *row = (iron_type_spec_row_t *)out_row;
            row->signature = read_index(&ptr, blob_size);
            break;
        }
        case IRON_TABLE_METHOD_SPEC: {
            iron_method_spec_row_t *row = (iron_method_spec_row_t *)out_row;
            row->method = read_index(&ptr, 
                meta->coded_index_sizes[IRON_CODED_METHOD_DEF_OR_REF]);
            row->instantiation = read_index(&ptr, blob_size);
            break;
        }
        default:
            return IRON_ERROR(IRON_ERR_NOT_IMPLEMENTED, "Table not implemented");
    }
    
    return (iron_result_t)IRON_SUCCESS;
}

/* ============================================================================
 * Signature Parsing
 * ============================================================================ */

void iron_sig_init(iron_sig_reader_t *reader, const iron_u8 *data, iron_size size)
{
    reader->data = data;
    reader->size = size;
    reader->pos = 0;
}

iron_result_t iron_sig_read_compressed_u32(iron_sig_reader_t *reader, iron_u32 *out)
{
    iron_u8 b;
    
    if (reader->pos >= reader->size) {
        return IRON_ERROR(IRON_ERR_INVALID_SIGNATURE, "Unexpected end of signature");
    }
    
    b = reader->data[reader->pos++];
    
    if ((b & 0x80) == 0) {
        *out = b;
    } else if ((b & 0xC0) == 0x80) {
        if (reader->pos >= reader->size) {
            return IRON_ERROR(IRON_ERR_INVALID_SIGNATURE, "Truncated compressed int");
        }
        *out = ((b & 0x3F) << 8) | reader->data[reader->pos++];
    } else if ((b & 0xE0) == 0xC0) {
        if (reader->pos + 2 >= reader->size) {
            return IRON_ERROR(IRON_ERR_INVALID_SIGNATURE, "Truncated compressed int");
        }
        *out = ((b & 0x1F) << 24) | 
               (reader->data[reader->pos] << 16) |
               (reader->data[reader->pos + 1] << 8) |
               reader->data[reader->pos + 2];
        reader->pos += 3;
    } else {
        return IRON_ERROR(IRON_ERR_INVALID_SIGNATURE, "Invalid compressed int");
    }
    
    return (iron_result_t)IRON_SUCCESS;
}

iron_result_t iron_sig_read_compressed_i32(iron_sig_reader_t *reader, iron_i32 *out)
{
    iron_u32 raw;
    iron_result_t result = iron_sig_read_compressed_u32(reader, &raw);
    if (!IRON_RESULT_OK(result)) return result;
    
    /* Decode signed value */
    if (raw & 1) {
        *out = -((iron_i32)(raw >> 1)) - 1;
    } else {
        *out = (iron_i32)(raw >> 1);
    }
    
    return (iron_result_t)IRON_SUCCESS;
}

iron_result_t iron_sig_read_element_type(iron_sig_reader_t *reader,
                                          iron_element_type_t *out)
{
    if (reader->pos >= reader->size) {
        return IRON_ERROR(IRON_ERR_INVALID_SIGNATURE, "Unexpected end of signature");
    }
    
    *out = (iron_element_type_t)reader->data[reader->pos++];
    return (iron_result_t)IRON_SUCCESS;
}

iron_result_t iron_sig_read_type_def_or_ref(iron_sig_reader_t *reader,
                                             iron_token_t *out)
{
    iron_u32 coded;
    iron_result_t result = iron_sig_read_compressed_u32(reader, &coded);
    if (!IRON_RESULT_OK(result)) return result;
    
    /* Decode TypeDefOrRefOrSpecEncoded */
    iron_u32 tag = coded & 0x03;
    iron_u32 index = coded >> 2;
    
    switch (tag) {
        case 0: *out = IRON_MAKE_TOKEN(IRON_TABLE_TYPE_DEF, index); break;
        case 1: *out = IRON_MAKE_TOKEN(IRON_TABLE_TYPE_REF, index); break;
        case 2: *out = IRON_MAKE_TOKEN(IRON_TABLE_TYPE_SPEC, index); break;
        default: return IRON_ERROR(IRON_ERR_INVALID_SIGNATURE, "Invalid type tag");
    }
    
    return (iron_result_t)IRON_SUCCESS;
}

iron_bool iron_sig_has_more(const iron_sig_reader_t *reader)
{
    return reader->pos < reader->size;
}

/* ============================================================================
 * Method Body Parsing
 * ============================================================================ */

iron_result_t iron_parse_method_body(const iron_pe_image_t *image,
                                      iron_u32 rva,
                                      iron_method_body_t *body,
                                      iron_allocator_t *alloc)
{
    const iron_u8 *ptr;
    iron_u8 header_byte;
    
    if (!image || !body) {
        return IRON_ERROR(IRON_ERR_NULL_POINTER, "Invalid parameters");
    }
    
    memset(body, 0, sizeof(iron_method_body_t));
    
    if (rva == 0) {
        return (iron_result_t)IRON_SUCCESS; /* Abstract/extern method */
    }
    
    ptr = iron_pe_rva_to_ptr(image, rva);
    if (!ptr) {
        return IRON_ERROR(IRON_ERR_INVALID_PROGRAM, "Invalid method RVA");
    }
    
    header_byte = *ptr;
    
    if ((header_byte & 0x03) == IRON_METHOD_TINY_FORMAT) {
        /* Tiny format: 1 byte header */
        body->is_fat = IRON_FALSE;
        body->max_stack = 8;
        body->code_size = header_byte >> 2;
        body->local_var_sig_token = 0;
        body->init_locals = IRON_FALSE;
        body->code = ptr + 1;
        body->exception_count = 0;
        body->exceptions = NULL;
    } else if ((header_byte & 0x03) == IRON_METHOD_FAT_FORMAT) {
        /* Fat format: 12 byte header */
        iron_u16 flags_size;
        iron_u32 header_size;
        
        body->is_fat = IRON_TRUE;
        
        flags_size = iron_read_u16_le(ptr);
        header_size = (flags_size >> 12) * 4;
        
        body->init_locals = (flags_size & IRON_METHOD_INIT_LOCALS) != 0;
        body->max_stack = iron_read_u16_le(ptr + 2);
        body->code_size = iron_read_u32_le(ptr + 4);
        body->local_var_sig_token = iron_read_u32_le(ptr + 8);
        body->code = ptr + header_size;
        
        /* Check for exception handlers */
        if (flags_size & IRON_METHOD_MORE_SECTS) {
            const iron_u8 *sect_ptr = body->code + body->code_size;
            iron_u8 sect_flags;
            iron_u32 sect_size;
            iron_u32 clause_count;
            iron_u32 i;
            
            /* Align to 4 bytes */
            sect_ptr = (const iron_u8 *)(((iron_size)sect_ptr + 3) & ~3);
            
            sect_flags = *sect_ptr;
            
            if (sect_flags & IRON_EX_CLAUSE_FAT) {
                /* Fat exception section */
                sect_size = iron_read_u32_le(sect_ptr) >> 8;
                clause_count = (sect_size - 4) / 24;
                sect_ptr += 4;
                
                body->exception_count = clause_count;
                body->exceptions = (iron_exception_clause_t *)iron_alloc(
                    alloc ? alloc : iron_system_allocator(),
                    clause_count * sizeof(iron_exception_clause_t));
                
                if (!body->exceptions) {
                    return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate exceptions");
                }
                
                for (i = 0; i < clause_count; i++) {
                    body->exceptions[i].flags = iron_read_u32_le(sect_ptr);
                    body->exceptions[i].try_offset = iron_read_u32_le(sect_ptr + 4);
                    body->exceptions[i].try_length = iron_read_u32_le(sect_ptr + 8);
                    body->exceptions[i].handler_offset = iron_read_u32_le(sect_ptr + 12);
                    body->exceptions[i].handler_length = iron_read_u32_le(sect_ptr + 16);
                    body->exceptions[i].u.class_token = iron_read_u32_le(sect_ptr + 20);
                    sect_ptr += 24;
                }
            } else {
                /* Small exception section */
                sect_size = sect_ptr[1];
                clause_count = (sect_size - 4) / 12;
                sect_ptr += 4;
                
                body->exception_count = clause_count;
                body->exceptions = (iron_exception_clause_t *)iron_alloc(
                    alloc ? alloc : iron_system_allocator(),
                    clause_count * sizeof(iron_exception_clause_t));
                
                if (!body->exceptions) {
                    return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate exceptions");
                }
                
                for (i = 0; i < clause_count; i++) {
                    body->exceptions[i].flags = iron_read_u16_le(sect_ptr);
                    body->exceptions[i].try_offset = iron_read_u16_le(sect_ptr + 2);
                    body->exceptions[i].try_length = sect_ptr[4];
                    body->exceptions[i].handler_offset = iron_read_u16_le(sect_ptr + 5);
                    body->exceptions[i].handler_length = sect_ptr[7];
                    body->exceptions[i].u.class_token = iron_read_u32_le(sect_ptr + 8);
                    sect_ptr += 12;
                }
            }
        }
    } else {
        return IRON_ERROR(IRON_ERR_INVALID_PROGRAM, "Invalid method header");
    }
    
    return (iron_result_t)IRON_SUCCESS;
}

void iron_free_method_body(iron_method_body_t *body, iron_allocator_t *alloc)
{
    if (!body) return;
    
    if (body->exceptions) {
        iron_free(alloc ? alloc : iron_system_allocator(), body->exceptions,
                  body->exception_count * sizeof(iron_exception_clause_t));
        body->exceptions = NULL;
    }
    body->exception_count = 0;
}
