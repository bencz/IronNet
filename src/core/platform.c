/*
 * IronNet CLR Interpreter
 * platform.c - Platform detection and endianness utilities
 */

#include "iron/platform.h"
#include <stdio.h>
#include <stdlib.h>

#if defined(IRON_OS_POSIX)
#include <unistd.h>
#endif

/* ============================================================================
 * Runtime Endianness Detection
 * ============================================================================ */

iron_endian_t iron_detect_endian(void)
{
    const iron_u32 test = 0x01020304;
    const iron_u8 *bytes = (const iron_u8 *)&test;
    return (bytes[0] == 0x04) ? IRON_ENDIAN_LITTLE : IRON_ENDIAN_BIG;
}

/* Runtime endianness conversion (for platforms without compile-time detection) */
#ifdef IRON_ENDIAN_RUNTIME

static iron_endian_t g_native_endian = IRON_ENDIAN_LITTLE;
static iron_bool g_endian_detected = IRON_FALSE;

static void detect_endian_once(void)
{
    if (!g_endian_detected) {
        g_native_endian = iron_detect_endian();
        g_endian_detected = IRON_TRUE;
    }
}

iron_u16 iron_le16(iron_u16 x)
{
    detect_endian_once();
    return (g_native_endian == IRON_ENDIAN_LITTLE) ? x : IRON_BSWAP16(x);
}

iron_u32 iron_le32(iron_u32 x)
{
    detect_endian_once();
    return (g_native_endian == IRON_ENDIAN_LITTLE) ? x : IRON_BSWAP32(x);
}

iron_u64 iron_le64(iron_u64 x)
{
    detect_endian_once();
    return (g_native_endian == IRON_ENDIAN_LITTLE) ? x : IRON_BSWAP64(x);
}

iron_u16 iron_be16(iron_u16 x)
{
    detect_endian_once();
    return (g_native_endian == IRON_ENDIAN_BIG) ? x : IRON_BSWAP16(x);
}

iron_u32 iron_be32(iron_u32 x)
{
    detect_endian_once();
    return (g_native_endian == IRON_ENDIAN_BIG) ? x : IRON_BSWAP32(x);
}

iron_u64 iron_be64(iron_u64 x)
{
    detect_endian_once();
    return (g_native_endian == IRON_ENDIAN_BIG) ? x : IRON_BSWAP64(x);
}

#endif /* IRON_ENDIAN_RUNTIME */

/* ============================================================================
 * Unaligned Memory Access
 * ============================================================================ */

iron_u16 iron_read_u16_le(const void *ptr)
{
    const iron_u8 *p = (const iron_u8 *)ptr;
    return (iron_u16)p[0] | ((iron_u16)p[1] << 8);
}

iron_u32 iron_read_u32_le(const void *ptr)
{
    const iron_u8 *p = (const iron_u8 *)ptr;
    return (iron_u32)p[0] | 
           ((iron_u32)p[1] << 8) |
           ((iron_u32)p[2] << 16) | 
           ((iron_u32)p[3] << 24);
}

iron_u64 iron_read_u64_le(const void *ptr)
{
    const iron_u8 *p = (const iron_u8 *)ptr;
#ifndef IRON_NO_NATIVE_64BIT
    return (iron_u64)p[0] | 
           ((iron_u64)p[1] << 8) |
           ((iron_u64)p[2] << 16) | 
           ((iron_u64)p[3] << 24) |
           ((iron_u64)p[4] << 32) | 
           ((iron_u64)p[5] << 40) |
           ((iron_u64)p[6] << 48) | 
           ((iron_u64)p[7] << 56);
#else
    iron_u64 result;
    result.lo = iron_read_u32_le(p);
    result.hi = iron_read_u32_le(p + 4);
    return result;
#endif
}

iron_i8 iron_read_i8(const void *ptr)
{
    return *(const iron_i8 *)ptr;
}

iron_i16 iron_read_i16_le(const void *ptr)
{
    return (iron_i16)iron_read_u16_le(ptr);
}

iron_i32 iron_read_i32_le(const void *ptr)
{
    return (iron_i32)iron_read_u32_le(ptr);
}

iron_i64 iron_read_i64_le(const void *ptr)
{
#ifndef IRON_NO_NATIVE_64BIT
    return (iron_i64)iron_read_u64_le(ptr);
#else
    return iron_read_u64_le(ptr);
#endif
}

iron_f32 iron_read_f32_le(const void *ptr)
{
    union { iron_u32 u; iron_f32 f; } conv;
    conv.u = iron_read_u32_le(ptr);
    return conv.f;
}

iron_f64 iron_read_f64_le(const void *ptr)
{
    union { iron_u64 u; iron_f64 f; } conv;
    conv.u = iron_read_u64_le(ptr);
    return conv.f;
}

void iron_write_u16_le(void *ptr, iron_u16 val)
{
    iron_u8 *p = (iron_u8 *)ptr;
    p[0] = (iron_u8)(val & 0xFF);
    p[1] = (iron_u8)((val >> 8) & 0xFF);
}

void iron_write_u32_le(void *ptr, iron_u32 val)
{
    iron_u8 *p = (iron_u8 *)ptr;
    p[0] = (iron_u8)(val & 0xFF);
    p[1] = (iron_u8)((val >> 8) & 0xFF);
    p[2] = (iron_u8)((val >> 16) & 0xFF);
    p[3] = (iron_u8)((val >> 24) & 0xFF);
}

void iron_write_u64_le(void *ptr, iron_u64 val)
{
    iron_u8 *p = (iron_u8 *)ptr;
#ifndef IRON_NO_NATIVE_64BIT
    p[0] = (iron_u8)(val & 0xFF);
    p[1] = (iron_u8)((val >> 8) & 0xFF);
    p[2] = (iron_u8)((val >> 16) & 0xFF);
    p[3] = (iron_u8)((val >> 24) & 0xFF);
    p[4] = (iron_u8)((val >> 32) & 0xFF);
    p[5] = (iron_u8)((val >> 40) & 0xFF);
    p[6] = (iron_u8)((val >> 48) & 0xFF);
    p[7] = (iron_u8)((val >> 56) & 0xFF);
#else
    iron_write_u32_le(p, val.lo);
    iron_write_u32_le(p + 4, val.hi);
#endif
}

void iron_write_i16_le(void *ptr, iron_i16 val)
{
    iron_write_u16_le(ptr, (iron_u16)val);
}

void iron_write_i32_le(void *ptr, iron_i32 val)
{
    iron_write_u32_le(ptr, (iron_u32)val);
}

void iron_write_i64_le(void *ptr, iron_i64 val)
{
#ifndef IRON_NO_NATIVE_64BIT
    iron_write_u64_le(ptr, (iron_u64)val);
#else
    iron_write_u64_le(ptr, val);
#endif
}

/* ============================================================================
 * Platform Information
 * ============================================================================ */

static iron_platform_info_t g_platform_info = {
    IRON_ARCH_NAME,
    IRON_OS_NAME,
#ifdef IRON_LITTLE_ENDIAN
    IRON_ENDIAN_LITTLE,
#elif defined(IRON_BIG_ENDIAN)
    IRON_ENDIAN_BIG,
#else
    IRON_ENDIAN_LITTLE, /* Will be updated at runtime */
#endif
    IRON_WORD_SIZE,
    IRON_PTR_SIZE,
    4096  /* Default page size, updated at runtime */
};

static iron_bool g_platform_initialized = IRON_FALSE;

static void init_platform_info(void)
{
    if (g_platform_initialized) return;
    
#ifdef IRON_ENDIAN_RUNTIME
    g_platform_info.endianness = iron_detect_endian();
#endif
    
    /* Get actual page size */
#if defined(IRON_OS_WINDOWS)
    /* Will be set in Windows-specific code */
    g_platform_info.page_size = 4096;
#elif defined(IRON_OS_POSIX)
    {
        long page_size = sysconf(_SC_PAGESIZE);
        if (page_size > 0) {
            g_platform_info.page_size = (iron_u32)page_size;
        }
    }
#endif
    
    g_platform_initialized = IRON_TRUE;
}

const iron_platform_info_t *iron_get_platform_info(void)
{
    init_platform_info();
    return &g_platform_info;
}

void iron_print_platform_info(void)
{
    const iron_platform_info_t *info = iron_get_platform_info();
    
    printf("IronNet Platform Information:\n");
    printf("  Architecture: %s\n", info->arch_name);
    printf("  OS: %s\n", info->os_name);
    printf("  Endianness: %s\n", 
           info->endianness == IRON_ENDIAN_LITTLE ? "little-endian" : "big-endian");
    printf("  Word size: %u bits\n", info->word_size);
    printf("  Pointer size: %u bytes\n", info->ptr_size);
    printf("  Page size: %u bytes\n", info->page_size);
}
