/*
 * IronNet CLR Interpreter
 * pe.c - PE/COFF file format reader
 */

#include "iron/pe.h"
#include <stdio.h>
#include <string.h>

/* ============================================================================
 * Internal Helpers
 * ============================================================================ */

static iron_result_t read_dos_header(const iron_u8 *data, iron_size size,
                                      iron_dos_header_t *dos)
{
    if (size < sizeof(iron_dos_header_t)) {
        return IRON_ERROR(IRON_ERR_INVALID_DOS_HEADER, "File too small for DOS header");
    }
    
    dos->e_magic = iron_read_u16_le(data + 0);
    if (dos->e_magic != IRON_DOS_SIGNATURE) {
        return IRON_ERROR(IRON_ERR_INVALID_DOS_HEADER, "Invalid DOS signature");
    }
    
    dos->e_cblp = iron_read_u16_le(data + 2);
    dos->e_cp = iron_read_u16_le(data + 4);
    dos->e_crlc = iron_read_u16_le(data + 6);
    dos->e_cparhdr = iron_read_u16_le(data + 8);
    dos->e_minalloc = iron_read_u16_le(data + 10);
    dos->e_maxalloc = iron_read_u16_le(data + 12);
    dos->e_ss = iron_read_u16_le(data + 14);
    dos->e_sp = iron_read_u16_le(data + 16);
    dos->e_csum = iron_read_u16_le(data + 18);
    dos->e_ip = iron_read_u16_le(data + 20);
    dos->e_cs = iron_read_u16_le(data + 22);
    dos->e_lfarlc = iron_read_u16_le(data + 24);
    dos->e_ovno = iron_read_u16_le(data + 26);
    dos->e_oemid = iron_read_u16_le(data + 36);
    dos->e_oeminfo = iron_read_u16_le(data + 38);
    dos->e_lfanew = iron_read_u32_le(data + 60);
    
    return IRON_SUCCESS;
}

static iron_result_t read_coff_header(const iron_u8 *data, iron_size size,
                                       iron_u32 offset, iron_coff_header_t *coff)
{
    iron_u32 sig;
    
    if (offset + 4 + 20 > size) {
        return IRON_ERROR(IRON_ERR_INVALID_PE_SIGNATURE, "File too small for PE header");
    }
    
    sig = iron_read_u32_le(data + offset);
    if (sig != IRON_PE_SIGNATURE) {
        return IRON_ERROR(IRON_ERR_INVALID_PE_SIGNATURE, "Invalid PE signature");
    }
    
    offset += 4;
    
    coff->machine = iron_read_u16_le(data + offset);
    coff->number_of_sections = iron_read_u16_le(data + offset + 2);
    coff->time_date_stamp = iron_read_u32_le(data + offset + 4);
    coff->pointer_to_symbol_table = iron_read_u32_le(data + offset + 8);
    coff->number_of_symbols = iron_read_u32_le(data + offset + 12);
    coff->size_of_optional_header = iron_read_u16_le(data + offset + 16);
    coff->characteristics = iron_read_u16_le(data + offset + 18);
    
    return IRON_SUCCESS;
}

static iron_result_t read_optional_header32(const iron_u8 *data, iron_size size,
                                             iron_u32 offset,
                                             iron_optional_header32_t *opt)
{
    iron_u32 i;
    
    if (offset + 96 > size) {
        return IRON_ERROR(IRON_ERR_INVALID_OPTIONAL_HEADER, "File too small for optional header");
    }
    
    opt->magic = iron_read_u16_le(data + offset);
    opt->major_linker_version = data[offset + 2];
    opt->minor_linker_version = data[offset + 3];
    opt->size_of_code = iron_read_u32_le(data + offset + 4);
    opt->size_of_initialized_data = iron_read_u32_le(data + offset + 8);
    opt->size_of_uninitialized_data = iron_read_u32_le(data + offset + 12);
    opt->address_of_entry_point = iron_read_u32_le(data + offset + 16);
    opt->base_of_code = iron_read_u32_le(data + offset + 20);
    opt->base_of_data = iron_read_u32_le(data + offset + 24);
    opt->image_base = iron_read_u32_le(data + offset + 28);
    opt->section_alignment = iron_read_u32_le(data + offset + 32);
    opt->file_alignment = iron_read_u32_le(data + offset + 36);
    opt->major_os_version = iron_read_u16_le(data + offset + 40);
    opt->minor_os_version = iron_read_u16_le(data + offset + 42);
    opt->major_image_version = iron_read_u16_le(data + offset + 44);
    opt->minor_image_version = iron_read_u16_le(data + offset + 46);
    opt->major_subsystem_version = iron_read_u16_le(data + offset + 48);
    opt->minor_subsystem_version = iron_read_u16_le(data + offset + 50);
    opt->win32_version_value = iron_read_u32_le(data + offset + 52);
    opt->size_of_image = iron_read_u32_le(data + offset + 56);
    opt->size_of_headers = iron_read_u32_le(data + offset + 60);
    opt->checksum = iron_read_u32_le(data + offset + 64);
    opt->subsystem = iron_read_u16_le(data + offset + 68);
    opt->dll_characteristics = iron_read_u16_le(data + offset + 70);
    opt->size_of_stack_reserve = iron_read_u32_le(data + offset + 72);
    opt->size_of_stack_commit = iron_read_u32_le(data + offset + 76);
    opt->size_of_heap_reserve = iron_read_u32_le(data + offset + 80);
    opt->size_of_heap_commit = iron_read_u32_le(data + offset + 84);
    opt->loader_flags = iron_read_u32_le(data + offset + 88);
    opt->number_of_rva_and_sizes = iron_read_u32_le(data + offset + 92);
    
    offset += 96;
    
    /* Read data directories */
    for (i = 0; i < IRON_DIR_COUNT && i < opt->number_of_rva_and_sizes; i++) {
        if (offset + 8 > size) break;
        opt->data_directories[i].virtual_address = iron_read_u32_le(data + offset);
        opt->data_directories[i].size = iron_read_u32_le(data + offset + 4);
        offset += 8;
    }
    
    return IRON_SUCCESS;
}

static iron_result_t read_optional_header64(const iron_u8 *data, iron_size size,
                                             iron_u32 offset,
                                             iron_optional_header64_t *opt)
{
    iron_u32 i;
    
    if (offset + 112 > size) {
        return IRON_ERROR(IRON_ERR_INVALID_OPTIONAL_HEADER, "File too small for optional header");
    }
    
    opt->magic = iron_read_u16_le(data + offset);
    opt->major_linker_version = data[offset + 2];
    opt->minor_linker_version = data[offset + 3];
    opt->size_of_code = iron_read_u32_le(data + offset + 4);
    opt->size_of_initialized_data = iron_read_u32_le(data + offset + 8);
    opt->size_of_uninitialized_data = iron_read_u32_le(data + offset + 12);
    opt->address_of_entry_point = iron_read_u32_le(data + offset + 16);
    opt->base_of_code = iron_read_u32_le(data + offset + 20);
    opt->image_base = iron_read_u64_le(data + offset + 24);
    opt->section_alignment = iron_read_u32_le(data + offset + 32);
    opt->file_alignment = iron_read_u32_le(data + offset + 36);
    opt->major_os_version = iron_read_u16_le(data + offset + 40);
    opt->minor_os_version = iron_read_u16_le(data + offset + 42);
    opt->major_image_version = iron_read_u16_le(data + offset + 44);
    opt->minor_image_version = iron_read_u16_le(data + offset + 46);
    opt->major_subsystem_version = iron_read_u16_le(data + offset + 48);
    opt->minor_subsystem_version = iron_read_u16_le(data + offset + 50);
    opt->win32_version_value = iron_read_u32_le(data + offset + 52);
    opt->size_of_image = iron_read_u32_le(data + offset + 56);
    opt->size_of_headers = iron_read_u32_le(data + offset + 60);
    opt->checksum = iron_read_u32_le(data + offset + 64);
    opt->subsystem = iron_read_u16_le(data + offset + 68);
    opt->dll_characteristics = iron_read_u16_le(data + offset + 70);
    opt->size_of_stack_reserve = iron_read_u64_le(data + offset + 72);
    opt->size_of_stack_commit = iron_read_u64_le(data + offset + 80);
    opt->size_of_heap_reserve = iron_read_u64_le(data + offset + 88);
    opt->size_of_heap_commit = iron_read_u64_le(data + offset + 96);
    opt->loader_flags = iron_read_u32_le(data + offset + 104);
    opt->number_of_rva_and_sizes = iron_read_u32_le(data + offset + 108);
    
    offset += 112;
    
    /* Read data directories */
    for (i = 0; i < IRON_DIR_COUNT && i < opt->number_of_rva_and_sizes; i++) {
        if (offset + 8 > size) break;
        opt->data_directories[i].virtual_address = iron_read_u32_le(data + offset);
        opt->data_directories[i].size = iron_read_u32_le(data + offset + 4);
        offset += 8;
    }
    
    return IRON_SUCCESS;
}

static iron_result_t read_section_header(const iron_u8 *data, iron_size size,
                                          iron_u32 offset,
                                          iron_section_header_t *section)
{
    if (offset + 40 > size) {
        return IRON_ERROR(IRON_ERR_INVALID_SECTION, "File too small for section header");
    }
    
    memcpy(section->name, data + offset, IRON_SECTION_NAME_SIZE);
    section->virtual_size = iron_read_u32_le(data + offset + 8);
    section->virtual_address = iron_read_u32_le(data + offset + 12);
    section->size_of_raw_data = iron_read_u32_le(data + offset + 16);
    section->pointer_to_raw_data = iron_read_u32_le(data + offset + 20);
    section->pointer_to_relocations = iron_read_u32_le(data + offset + 24);
    section->pointer_to_line_numbers = iron_read_u32_le(data + offset + 28);
    section->number_of_relocations = iron_read_u16_le(data + offset + 32);
    section->number_of_line_numbers = iron_read_u16_le(data + offset + 34);
    section->characteristics = iron_read_u32_le(data + offset + 36);
    
    return IRON_SUCCESS;
}

static iron_result_t read_cli_header(const iron_u8 *data, iron_size size,
                                      iron_u32 offset, iron_cli_header_t *cli)
{
    if (offset + IRON_CLI_HEADER_SIZE > size) {
        return IRON_ERROR(IRON_ERR_NOT_CLI_IMAGE, "File too small for CLI header");
    }
    
    cli->cb = iron_read_u32_le(data + offset);
    cli->major_runtime_version = iron_read_u16_le(data + offset + 4);
    cli->minor_runtime_version = iron_read_u16_le(data + offset + 6);
    cli->metadata.virtual_address = iron_read_u32_le(data + offset + 8);
    cli->metadata.size = iron_read_u32_le(data + offset + 12);
    cli->flags = iron_read_u32_le(data + offset + 16);
    cli->entry_point_token = iron_read_u32_le(data + offset + 20);
    cli->resources.virtual_address = iron_read_u32_le(data + offset + 24);
    cli->resources.size = iron_read_u32_le(data + offset + 28);
    cli->strong_name_signature.virtual_address = iron_read_u32_le(data + offset + 32);
    cli->strong_name_signature.size = iron_read_u32_le(data + offset + 36);
    cli->code_manager_table.virtual_address = iron_read_u32_le(data + offset + 40);
    cli->code_manager_table.size = iron_read_u32_le(data + offset + 44);
    cli->vtable_fixups.virtual_address = iron_read_u32_le(data + offset + 48);
    cli->vtable_fixups.size = iron_read_u32_le(data + offset + 52);
    cli->export_address_table_jumps.virtual_address = iron_read_u32_le(data + offset + 56);
    cli->export_address_table_jumps.size = iron_read_u32_le(data + offset + 60);
    cli->managed_native_header.virtual_address = iron_read_u32_le(data + offset + 64);
    cli->managed_native_header.size = iron_read_u32_le(data + offset + 68);
    
    return IRON_SUCCESS;
}

/* ============================================================================
 * Public API
 * ============================================================================ */

iron_result_t iron_pe_load(iron_pe_image_t *image, const iron_u8 *data,
                           iron_size size, iron_bool copy_data,
                           iron_allocator_t *alloc)
{
    iron_result_t result;
    iron_u32 offset;
    iron_u16 magic;
    iron_u16 i;
    iron_section_header_t section_hdr = {0};
    const iron_data_directory_t *cli_dir;
    iron_u32 cli_offset;
    
    if (!image || !data || size == 0) {
        return IRON_ERROR(IRON_ERR_NULL_POINTER, "Invalid parameters");
    }
    
    memset(image, 0, sizeof(iron_pe_image_t));
    image->allocator = alloc ? alloc : iron_system_allocator();
    
    /* Copy or reference data */
    if (copy_data) {
        iron_u8 *copy = (iron_u8 *)iron_alloc(image->allocator, size);
        if (!copy) {
            return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate image data");
        }
        memcpy(copy, data, size);
        image->data = copy;
        image->owns_data = IRON_TRUE;
    } else {
        image->data = data;
        image->owns_data = IRON_FALSE;
    }
    image->data_size = size;
    
    /* Read DOS header */
    result = read_dos_header(image->data, size, &image->dos_header);
    if (!IRON_RESULT_OK(result)) {
        iron_pe_free(image);
        return result;
    }
    
    /* Read COFF header */
    result = read_coff_header(image->data, size, image->dos_header.e_lfanew,
                              &image->coff_header);
    if (!IRON_RESULT_OK(result)) {
        iron_pe_free(image);
        return result;
    }
    
    /* Read optional header */
    offset = image->dos_header.e_lfanew + 4 + 20;
    
    if (offset + 2 > size) {
        iron_pe_free(image);
        return IRON_ERROR(IRON_ERR_INVALID_OPTIONAL_HEADER, "File too small");
    }
    
    magic = iron_read_u16_le(image->data + offset);
    
    if (magic == IRON_PE32P_MAGIC) {
        image->is_pe32plus = IRON_TRUE;
        result = read_optional_header64(image->data, size, offset,
                                        &image->optional_header.pe64);
        image->image_base = image->optional_header.pe64.image_base;
        image->entry_point_rva = image->optional_header.pe64.address_of_entry_point;
    } else if (magic == IRON_PE32_MAGIC) {
        image->is_pe32plus = IRON_FALSE;
        result = read_optional_header32(image->data, size, offset,
                                        &image->optional_header.pe32);
        image->image_base = image->optional_header.pe32.image_base;
        image->entry_point_rva = image->optional_header.pe32.address_of_entry_point;
    } else {
        iron_pe_free(image);
        return IRON_ERROR(IRON_ERR_INVALID_OPTIONAL_HEADER, "Unknown PE format");
    }
    
    if (!IRON_RESULT_OK(result)) {
        iron_pe_free(image);
        return result;
    }
    
    /* Read section headers */
    offset = image->dos_header.e_lfanew + 4 + 20 + 
             image->coff_header.size_of_optional_header;
    
    image->section_count = image->coff_header.number_of_sections;
    image->sections = (iron_pe_section_t *)iron_alloc(image->allocator,
        image->section_count * sizeof(iron_pe_section_t));
    
    if (!image->sections) {
        iron_pe_free(image);
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate sections");
    }
    
    for (i = 0; i < image->section_count; i++) {
        result = read_section_header(image->data, size, offset, &section_hdr);
        if (!IRON_RESULT_OK(result)) {
            iron_pe_free(image);
            return result;
        }
        
        memcpy(image->sections[i].name, section_hdr.name, IRON_SECTION_NAME_SIZE);
        image->sections[i].name[IRON_SECTION_NAME_SIZE] = '\0';
        image->sections[i].virtual_address = section_hdr.virtual_address;
        image->sections[i].virtual_size = section_hdr.virtual_size;
        image->sections[i].file_offset = section_hdr.pointer_to_raw_data;
        image->sections[i].file_size = section_hdr.size_of_raw_data;
        image->sections[i].characteristics = section_hdr.characteristics;
        
        if (section_hdr.pointer_to_raw_data + section_hdr.size_of_raw_data <= size) {
            image->sections[i].data = image->data + section_hdr.pointer_to_raw_data;
        } else {
            image->sections[i].data = NULL;
        }
        
        offset += 40;
    }
    
    /* Check for CLI header */
    cli_dir = iron_pe_get_directory(image, IRON_DIR_CLI_HEADER);
    if (cli_dir && cli_dir->virtual_address != 0 && cli_dir->size >= IRON_CLI_HEADER_SIZE) {
        cli_offset = iron_pe_rva_to_offset(image, cli_dir->virtual_address);
        if (cli_offset != 0) {
            result = read_cli_header(image->data, size, cli_offset, &image->cli_header);
            if (IRON_RESULT_OK(result)) {
                image->has_cli_header = IRON_TRUE;
            }
        }
    }
    
    return IRON_SUCCESS;
}

iron_result_t iron_pe_load_file(iron_pe_image_t *image, const char *filename,
                                iron_allocator_t *alloc)
{
    FILE *f;
    iron_u8 *data;
    long file_size;
    iron_size read_size;
    iron_result_t result;
    
    if (!image || !filename) {
        return IRON_ERROR(IRON_ERR_NULL_POINTER, "Invalid parameters");
    }
    
    f = fopen(filename, "rb");
    if (!f) {
        return IRON_ERROR(IRON_ERR_FILE_NOT_FOUND, "Cannot open file");
    }
    
    /* Get file size */
    fseek(f, 0, SEEK_END);
    file_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    if (file_size <= 0) {
        fclose(f);
        return IRON_ERROR(IRON_ERR_FILE_READ, "Invalid file size");
    }
    
    /* Allocate buffer */
    if (!alloc) alloc = iron_system_allocator();
    data = (iron_u8 *)iron_alloc(alloc, (iron_size)file_size);
    if (!data) {
        fclose(f);
        return IRON_ERROR(IRON_ERR_OUT_OF_MEMORY, "Failed to allocate file buffer");
    }
    
    /* Read file */
    read_size = fread(data, 1, (iron_size)file_size, f);
    fclose(f);
    
    if (read_size != (iron_size)file_size) {
        iron_free(alloc, data, (iron_size)file_size);
        return IRON_ERROR(IRON_ERR_FILE_READ, "Failed to read file");
    }
    
    /* Load PE from memory (transfer ownership) */
    result = iron_pe_load(image, data, (iron_size)file_size, IRON_FALSE, alloc);
    if (IRON_RESULT_OK(result)) {
        image->owns_data = IRON_TRUE;
    } else {
        iron_free(alloc, data, (iron_size)file_size);
    }
    
    return result;
}

void iron_pe_free(iron_pe_image_t *image)
{
    if (!image) return;
    
    if (image->sections) {
        iron_free(image->allocator, image->sections,
                  image->section_count * sizeof(iron_pe_section_t));
        image->sections = NULL;
    }
    
    if (image->owns_data && image->data) {
        iron_free(image->allocator, (void *)image->data, image->data_size);
    }
    image->data = NULL;
    image->data_size = 0;
}

iron_bool iron_pe_is_cli(const iron_pe_image_t *image)
{
    return image && image->has_cli_header;
}

iron_u32 iron_pe_rva_to_offset(const iron_pe_image_t *image, iron_u32 rva)
{
    iron_u16 i;
    
    if (!image) return 0;
    
    for (i = 0; i < image->section_count; i++) {
        iron_u32 section_start = image->sections[i].virtual_address;
        iron_u32 section_end = section_start + image->sections[i].virtual_size;
        
        if (rva >= section_start && rva < section_end) {
            return image->sections[i].file_offset + (rva - section_start);
        }
    }
    
    return 0;
}

const iron_u8 *iron_pe_rva_to_ptr(const iron_pe_image_t *image, iron_u32 rva)
{
    iron_u32 offset;
    
    if (!image || !image->data) return NULL;
    
    offset = iron_pe_rva_to_offset(image, rva);
    if (offset == 0 || offset >= image->data_size) return NULL;
    
    return image->data + offset;
}

const iron_pe_section_t *iron_pe_find_section(const iron_pe_image_t *image,
                                               const char *name)
{
    iron_u16 i;
    
    if (!image || !name) return NULL;
    
    for (i = 0; i < image->section_count; i++) {
        if (strncmp(image->sections[i].name, name, IRON_SECTION_NAME_SIZE) == 0) {
            return &image->sections[i];
        }
    }
    
    return NULL;
}

const iron_pe_section_t *iron_pe_section_from_rva(const iron_pe_image_t *image,
                                                   iron_u32 rva)
{
    iron_u16 i;
    
    if (!image) return NULL;
    
    for (i = 0; i < image->section_count; i++) {
        iron_u32 section_start = image->sections[i].virtual_address;
        iron_u32 section_end = section_start + image->sections[i].virtual_size;
        
        if (rva >= section_start && rva < section_end) {
            return &image->sections[i];
        }
    }
    
    return NULL;
}

const iron_data_directory_t *iron_pe_get_directory(const iron_pe_image_t *image,
                                                    iron_data_dir_index_t index)
{
    iron_u32 directory_index;

    if (!image || index < 0 || index >= IRON_DIR_COUNT) {
        return NULL;
    }

    directory_index = (iron_u32)index;
    
    if (image->is_pe32plus) {
        if (directory_index >= image->optional_header.pe64.number_of_rva_and_sizes) {
            return NULL;
        }
        return &image->optional_header.pe64.data_directories[directory_index];
    } else {
        if (directory_index >= image->optional_header.pe32.number_of_rva_and_sizes) {
            return NULL;
        }
        return &image->optional_header.pe32.data_directories[directory_index];
    }
}
