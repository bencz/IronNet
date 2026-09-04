/*
 * IronNet CLR Interpreter
 * pe.h - PE/COFF file format structures and reader
 * 
 * Supports reading Windows PE files and CLI assemblies
 * 
 * Strict C99 compatible
 */

#ifndef IRON_PE_H
#define IRON_PE_H

#include "platform.h"
#include "types.h"
#include "memory.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * DOS Header
 * ============================================================================ */

#define IRON_DOS_SIGNATURE 0x5A4D  /* "MZ" */

typedef struct iron_dos_header {
    iron_u16 e_magic;       /* Magic number (MZ) */
    iron_u16 e_cblp;        /* Bytes on last page */
    iron_u16 e_cp;          /* Pages in file */
    iron_u16 e_crlc;        /* Relocations */
    iron_u16 e_cparhdr;     /* Size of header in paragraphs */
    iron_u16 e_minalloc;    /* Minimum extra paragraphs */
    iron_u16 e_maxalloc;    /* Maximum extra paragraphs */
    iron_u16 e_ss;          /* Initial SS value */
    iron_u16 e_sp;          /* Initial SP value */
    iron_u16 e_csum;        /* Checksum */
    iron_u16 e_ip;          /* Initial IP value */
    iron_u16 e_cs;          /* Initial CS value */
    iron_u16 e_lfarlc;      /* File address of relocation table */
    iron_u16 e_ovno;        /* Overlay number */
    iron_u16 e_res[4];      /* Reserved */
    iron_u16 e_oemid;       /* OEM identifier */
    iron_u16 e_oeminfo;     /* OEM information */
    iron_u16 e_res2[10];    /* Reserved */
    iron_u32 e_lfanew;      /* File address of PE header */
} iron_dos_header_t;

/* ============================================================================
 * PE Header
 * ============================================================================ */

#define IRON_PE_SIGNATURE 0x00004550  /* "PE\0\0" */

/* Machine types */
typedef enum iron_machine_type {
    IRON_MACHINE_UNKNOWN   = 0x0000,
    IRON_MACHINE_I386      = 0x014C,
    IRON_MACHINE_R3000     = 0x0162,
    IRON_MACHINE_R4000     = 0x0166,
    IRON_MACHINE_R10000    = 0x0168,
    IRON_MACHINE_WCEMIPSV2 = 0x0169,
    IRON_MACHINE_ALPHA     = 0x0184,
    IRON_MACHINE_SH3       = 0x01A2,
    IRON_MACHINE_SH3DSP    = 0x01A3,
    IRON_MACHINE_SH3E      = 0x01A4,
    IRON_MACHINE_SH4       = 0x01A6,
    IRON_MACHINE_SH5       = 0x01A8,
    IRON_MACHINE_ARM       = 0x01C0,
    IRON_MACHINE_THUMB     = 0x01C2,
    IRON_MACHINE_ARMNT     = 0x01C4,
    IRON_MACHINE_AM33      = 0x01D3,
    IRON_MACHINE_POWERPC   = 0x01F0,
    IRON_MACHINE_POWERPCFP = 0x01F1,
    IRON_MACHINE_IA64      = 0x0200,
    IRON_MACHINE_MIPS16    = 0x0266,
    IRON_MACHINE_ALPHA64   = 0x0284,
    IRON_MACHINE_MIPSFPU   = 0x0366,
    IRON_MACHINE_MIPSFPU16 = 0x0466,
    IRON_MACHINE_TRICORE   = 0x0520,
    IRON_MACHINE_CEF       = 0x0CEF,
    IRON_MACHINE_EBC       = 0x0EBC,
    IRON_MACHINE_AMD64     = 0x8664,
    IRON_MACHINE_M32R      = 0x9041,
    IRON_MACHINE_ARM64     = 0xAA64,
    IRON_MACHINE_CEE       = 0xC0EE
} iron_machine_type_t;

/* PE characteristics */
typedef enum iron_pe_characteristics {
    IRON_PE_RELOCS_STRIPPED         = 0x0001,
    IRON_PE_EXECUTABLE_IMAGE        = 0x0002,
    IRON_PE_LINE_NUMS_STRIPPED      = 0x0004,
    IRON_PE_LOCAL_SYMS_STRIPPED     = 0x0008,
    IRON_PE_AGGRESSIVE_WS_TRIM      = 0x0010,
    IRON_PE_LARGE_ADDRESS_AWARE     = 0x0020,
    IRON_PE_BYTES_REVERSED_LO       = 0x0080,
    IRON_PE_32BIT_MACHINE           = 0x0100,
    IRON_PE_DEBUG_STRIPPED          = 0x0200,
    IRON_PE_REMOVABLE_RUN_FROM_SWAP = 0x0400,
    IRON_PE_NET_RUN_FROM_SWAP       = 0x0800,
    IRON_PE_SYSTEM                  = 0x1000,
    IRON_PE_DLL                     = 0x2000,
    IRON_PE_UP_SYSTEM_ONLY          = 0x4000,
    IRON_PE_BYTES_REVERSED_HI       = 0x8000
} iron_pe_characteristics_t;

/* COFF file header */
typedef struct iron_coff_header {
    iron_u16 machine;
    iron_u16 number_of_sections;
    iron_u32 time_date_stamp;
    iron_u32 pointer_to_symbol_table;
    iron_u32 number_of_symbols;
    iron_u16 size_of_optional_header;
    iron_u16 characteristics;
} iron_coff_header_t;

/* ============================================================================
 * Optional Header
 * ============================================================================ */

#define IRON_PE32_MAGIC  0x010B
#define IRON_PE32P_MAGIC 0x020B  /* PE32+ (64-bit) */

/* Subsystem types */
typedef enum iron_subsystem {
    IRON_SUBSYS_UNKNOWN                  = 0,
    IRON_SUBSYS_NATIVE                   = 1,
    IRON_SUBSYS_WINDOWS_GUI              = 2,
    IRON_SUBSYS_WINDOWS_CUI              = 3,
    IRON_SUBSYS_OS2_CUI                  = 5,
    IRON_SUBSYS_POSIX_CUI                = 7,
    IRON_SUBSYS_NATIVE_WINDOWS           = 8,
    IRON_SUBSYS_WINDOWS_CE_GUI           = 9,
    IRON_SUBSYS_EFI_APPLICATION          = 10,
    IRON_SUBSYS_EFI_BOOT_SERVICE_DRIVER  = 11,
    IRON_SUBSYS_EFI_RUNTIME_DRIVER       = 12,
    IRON_SUBSYS_EFI_ROM                  = 13,
    IRON_SUBSYS_XBOX                     = 14,
    IRON_SUBSYS_WINDOWS_BOOT_APPLICATION = 16
} iron_subsystem_t;

/* DLL characteristics */
typedef enum iron_dll_characteristics {
    IRON_DLL_HIGH_ENTROPY_VA       = 0x0020,
    IRON_DLL_DYNAMIC_BASE          = 0x0040,
    IRON_DLL_FORCE_INTEGRITY       = 0x0080,
    IRON_DLL_NX_COMPAT             = 0x0100,
    IRON_DLL_NO_ISOLATION          = 0x0200,
    IRON_DLL_NO_SEH                = 0x0400,
    IRON_DLL_NO_BIND               = 0x0800,
    IRON_DLL_APPCONTAINER          = 0x1000,
    IRON_DLL_WDM_DRIVER            = 0x2000,
    IRON_DLL_GUARD_CF              = 0x4000,
    IRON_DLL_TERMINAL_SERVER_AWARE = 0x8000
} iron_dll_characteristics_t;

/* Data directory indices */
typedef enum iron_data_dir_index {
    IRON_DIR_EXPORT         = 0,
    IRON_DIR_IMPORT         = 1,
    IRON_DIR_RESOURCE       = 2,
    IRON_DIR_EXCEPTION      = 3,
    IRON_DIR_SECURITY       = 4,
    IRON_DIR_BASERELOC      = 5,
    IRON_DIR_DEBUG          = 6,
    IRON_DIR_ARCHITECTURE   = 7,
    IRON_DIR_GLOBALPTR      = 8,
    IRON_DIR_TLS            = 9,
    IRON_DIR_LOAD_CONFIG    = 10,
    IRON_DIR_BOUND_IMPORT   = 11,
    IRON_DIR_IAT            = 12,
    IRON_DIR_DELAY_IMPORT   = 13,
    IRON_DIR_CLI_HEADER     = 14,  /* CLR Runtime Header */
    IRON_DIR_RESERVED       = 15,
    IRON_DIR_COUNT          = 16
} iron_data_dir_index_t;

/* Data directory entry */
typedef struct iron_data_directory {
    iron_u32 virtual_address;
    iron_u32 size;
} iron_data_directory_t;

/* PE32 Optional Header */
typedef struct iron_optional_header32 {
    iron_u16 magic;
    iron_u8  major_linker_version;
    iron_u8  minor_linker_version;
    iron_u32 size_of_code;
    iron_u32 size_of_initialized_data;
    iron_u32 size_of_uninitialized_data;
    iron_u32 address_of_entry_point;
    iron_u32 base_of_code;
    iron_u32 base_of_data;
    iron_u32 image_base;
    iron_u32 section_alignment;
    iron_u32 file_alignment;
    iron_u16 major_os_version;
    iron_u16 minor_os_version;
    iron_u16 major_image_version;
    iron_u16 minor_image_version;
    iron_u16 major_subsystem_version;
    iron_u16 minor_subsystem_version;
    iron_u32 win32_version_value;
    iron_u32 size_of_image;
    iron_u32 size_of_headers;
    iron_u32 checksum;
    iron_u16 subsystem;
    iron_u16 dll_characteristics;
    iron_u32 size_of_stack_reserve;
    iron_u32 size_of_stack_commit;
    iron_u32 size_of_heap_reserve;
    iron_u32 size_of_heap_commit;
    iron_u32 loader_flags;
    iron_u32 number_of_rva_and_sizes;
    iron_data_directory_t data_directories[IRON_DIR_COUNT];
} iron_optional_header32_t;

/* PE32+ Optional Header (64-bit) */
typedef struct iron_optional_header64 {
    iron_u16 magic;
    iron_u8  major_linker_version;
    iron_u8  minor_linker_version;
    iron_u32 size_of_code;
    iron_u32 size_of_initialized_data;
    iron_u32 size_of_uninitialized_data;
    iron_u32 address_of_entry_point;
    iron_u32 base_of_code;
    iron_u64 image_base;
    iron_u32 section_alignment;
    iron_u32 file_alignment;
    iron_u16 major_os_version;
    iron_u16 minor_os_version;
    iron_u16 major_image_version;
    iron_u16 minor_image_version;
    iron_u16 major_subsystem_version;
    iron_u16 minor_subsystem_version;
    iron_u32 win32_version_value;
    iron_u32 size_of_image;
    iron_u32 size_of_headers;
    iron_u32 checksum;
    iron_u16 subsystem;
    iron_u16 dll_characteristics;
    iron_u64 size_of_stack_reserve;
    iron_u64 size_of_stack_commit;
    iron_u64 size_of_heap_reserve;
    iron_u64 size_of_heap_commit;
    iron_u32 loader_flags;
    iron_u32 number_of_rva_and_sizes;
    iron_data_directory_t data_directories[IRON_DIR_COUNT];
} iron_optional_header64_t;

/* ============================================================================
 * Section Header
 * ============================================================================ */

#define IRON_SECTION_NAME_SIZE 8

/* Section characteristics */
typedef enum iron_section_characteristics {
    IRON_SCN_TYPE_NO_PAD            = 0x00000008,
    IRON_SCN_CNT_CODE               = 0x00000020,
    IRON_SCN_CNT_INITIALIZED_DATA   = 0x00000040,
    IRON_SCN_CNT_UNINITIALIZED_DATA = 0x00000080,
    IRON_SCN_LNK_OTHER              = 0x00000100,
    IRON_SCN_LNK_INFO               = 0x00000200,
    IRON_SCN_LNK_REMOVE             = 0x00000800,
    IRON_SCN_LNK_COMDAT             = 0x00001000,
    IRON_SCN_GPREL                  = 0x00008000,
    IRON_SCN_MEM_PURGEABLE          = 0x00020000,
    IRON_SCN_MEM_16BIT              = 0x00020000,
    IRON_SCN_MEM_LOCKED             = 0x00040000,
    IRON_SCN_MEM_PRELOAD            = 0x00080000,
    IRON_SCN_ALIGN_1BYTES           = 0x00100000,
    IRON_SCN_ALIGN_2BYTES           = 0x00200000,
    IRON_SCN_ALIGN_4BYTES           = 0x00300000,
    IRON_SCN_ALIGN_8BYTES           = 0x00400000,
    IRON_SCN_ALIGN_16BYTES          = 0x00500000,
    IRON_SCN_ALIGN_32BYTES          = 0x00600000,
    IRON_SCN_ALIGN_64BYTES          = 0x00700000,
    IRON_SCN_ALIGN_128BYTES         = 0x00800000,
    IRON_SCN_ALIGN_256BYTES         = 0x00900000,
    IRON_SCN_ALIGN_512BYTES         = 0x00A00000,
    IRON_SCN_ALIGN_1024BYTES        = 0x00B00000,
    IRON_SCN_ALIGN_2048BYTES        = 0x00C00000,
    IRON_SCN_ALIGN_4096BYTES        = 0x00D00000,
    IRON_SCN_ALIGN_8192BYTES        = 0x00E00000,
    IRON_SCN_LNK_NRELOC_OVFL        = 0x01000000,
    IRON_SCN_MEM_DISCARDABLE        = 0x02000000,
    IRON_SCN_MEM_NOT_CACHED         = 0x04000000,
    IRON_SCN_MEM_NOT_PAGED          = 0x08000000,
    IRON_SCN_MEM_SHARED             = 0x10000000,
    IRON_SCN_MEM_EXECUTE            = 0x20000000,
    IRON_SCN_MEM_READ               = 0x40000000,
    IRON_SCN_MEM_WRITE              = 0x80000000
} iron_section_characteristics_t;

typedef struct iron_section_header {
    char name[IRON_SECTION_NAME_SIZE];
    iron_u32 virtual_size;
    iron_u32 virtual_address;
    iron_u32 size_of_raw_data;
    iron_u32 pointer_to_raw_data;
    iron_u32 pointer_to_relocations;
    iron_u32 pointer_to_line_numbers;
    iron_u16 number_of_relocations;
    iron_u16 number_of_line_numbers;
    iron_u32 characteristics;
} iron_section_header_t;

/* ============================================================================
 * CLI Header (ECMA-335 II.25.3.3)
 * ============================================================================ */

#define IRON_CLI_HEADER_SIZE 72

/* CLI flags */
typedef enum iron_cli_flags {
    IRON_CLI_IL_ONLY             = 0x00000001,
    IRON_CLI_32BIT_REQUIRED      = 0x00000002,
    IRON_CLI_IL_LIBRARY          = 0x00000004,
    IRON_CLI_STRONG_NAME_SIGNED  = 0x00000008,
    IRON_CLI_NATIVE_ENTRY_POINT  = 0x00000010,
    IRON_CLI_TRACK_DEBUG_DATA    = 0x00010000,
    IRON_CLI_32BIT_PREFERRED     = 0x00020000
} iron_cli_flags_t;

typedef struct iron_cli_header {
    iron_u32 cb;                          /* Size of header */
    iron_u16 major_runtime_version;
    iron_u16 minor_runtime_version;
    iron_data_directory_t metadata;       /* Metadata directory */
    iron_u32 flags;
    iron_u32 entry_point_token;           /* Entry point method token or RVA */
    iron_data_directory_t resources;      /* Resources directory */
    iron_data_directory_t strong_name_signature;
    iron_data_directory_t code_manager_table;
    iron_data_directory_t vtable_fixups;
    iron_data_directory_t export_address_table_jumps;
    iron_data_directory_t managed_native_header;
} iron_cli_header_t;

/* ============================================================================
 * PE Image Reader
 * ============================================================================ */

/* Loaded section */
typedef struct iron_pe_section {
    char name[IRON_SECTION_NAME_SIZE + 1];
    iron_u32 virtual_address;
    iron_u32 virtual_size;
    iron_u32 file_offset;
    iron_u32 file_size;
    iron_u32 characteristics;
    const iron_u8 *data;
} iron_pe_section_t;

/* PE Image structure */
typedef struct iron_pe_image {
    iron_allocator_t *allocator;
    
    /* Raw file data */
    const iron_u8 *data;
    iron_size data_size;
    iron_bool owns_data;
    
    /* Headers */
    iron_dos_header_t dos_header;
    iron_coff_header_t coff_header;
    iron_bool is_pe32plus;
    
    /* Optional header (union for 32/64 bit) */
    union {
        iron_optional_header32_t pe32;
        iron_optional_header64_t pe64;
    } optional_header;
    
    /* Sections */
    iron_pe_section_t *sections;
    iron_u16 section_count;
    
    /* CLI header (if present) */
    iron_bool has_cli_header;
    iron_cli_header_t cli_header;
    
    /* Convenience pointers */
    iron_u64 image_base;
    iron_u32 entry_point_rva;
} iron_pe_image_t;

/* ============================================================================
 * PE Image API
 * ============================================================================ */

/* Load PE image from memory */
IRON_API iron_result_t iron_pe_load(iron_pe_image_t *image,
                                     const iron_u8 *data,
                                     iron_size size,
                                     iron_bool copy_data,
                                     iron_allocator_t *alloc);

/* Load PE image from file */
IRON_API iron_result_t iron_pe_load_file(iron_pe_image_t *image,
                                          const char *filename,
                                          iron_allocator_t *alloc);

/* Free PE image */
IRON_API void iron_pe_free(iron_pe_image_t *image);

/* Check if image is a CLI assembly */
IRON_API iron_bool iron_pe_is_cli(const iron_pe_image_t *image);

/* Convert RVA to file offset */
IRON_API iron_u32 iron_pe_rva_to_offset(const iron_pe_image_t *image, 
                                         iron_u32 rva);

/* Get pointer to data at RVA */
IRON_API const iron_u8 *iron_pe_rva_to_ptr(const iron_pe_image_t *image,
                                            iron_u32 rva);

/* Find section by name */
IRON_API const iron_pe_section_t *iron_pe_find_section(const iron_pe_image_t *image,
                                                        const char *name);

/* Find section containing RVA */
IRON_API const iron_pe_section_t *iron_pe_section_from_rva(const iron_pe_image_t *image,
                                                            iron_u32 rva);

/* Get data directory */
IRON_API const iron_data_directory_t *iron_pe_get_directory(const iron_pe_image_t *image,
                                                             iron_data_dir_index_t index);

#ifdef __cplusplus
}
#endif

#endif /* IRON_PE_H */
