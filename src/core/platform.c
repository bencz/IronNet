/*
 * IronNet CLR Interpreter
 * platform.c - Platform detection and endianness utilities
 */

#include "iron/platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(IRON_OS_WINDOWS)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
#elif defined(IRON_OS_POSIX)
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

iron_bool iron_platform_get_local_time(iron_calendar_time_t *result)
{
    time_t now;
    struct tm local_time;

    if (!result) {
        return IRON_FALSE;
    }

    now = time(NULL);
    if (now == (time_t)-1) {
        return IRON_FALSE;
    }

#if defined(IRON_OS_WINDOWS)
    if (localtime_s(&local_time, &now) != 0) {
        return IRON_FALSE;
    }
#elif defined(IRON_OS_POSIX)
    if (!localtime_r(&now, &local_time)) {
        return IRON_FALSE;
    }
#else
    {
        struct tm *local_time_ptr;

        local_time_ptr = localtime(&now);
        if (!local_time_ptr) {
            return IRON_FALSE;
        }

        memcpy(&local_time, local_time_ptr, sizeof(local_time));
    }
#endif

    result->year = (iron_i32)local_time.tm_year + 1900;
    result->month = (iron_i32)local_time.tm_mon + 1;
    result->day = (iron_i32)local_time.tm_mday;
    result->hour = (iron_i32)local_time.tm_hour;
    result->minute = (iron_i32)local_time.tm_min;
    result->second = (iron_i32)local_time.tm_sec;
    return IRON_TRUE;
}

static iron_bool calendar_to_ticks(iron_i32 year,
                                   iron_i32 month,
                                   iron_i32 day,
                                   iron_i32 hour,
                                   iron_i32 minute,
                                   iron_i32 second,
                                   iron_i32 nanosecond,
                                   iron_i64 *result)
{
    static const iron_i32 days_to_month_365[] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 };
    static const iron_i32 days_to_month_366[] = { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 };
    const iron_i32 *days_to_month;
    iron_i32 previous_year;
    iron_i64 days;

    if (!result || year < 1 || year > 9999 || month < 1 || month > 12 || day < 1 || hour < 0 || hour > 23 || minute < 0 || minute > 59 ||
        second < 0 || second > 59 || nanosecond < 0 || nanosecond > 999999999) {
        return IRON_FALSE;
    }

    days_to_month = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) ? days_to_month_366 : days_to_month_365;
    if (day > days_to_month[month] - days_to_month[month - 1]) {
        return IRON_FALSE;
    }

    previous_year = year - 1;
    days = (iron_i64)previous_year * 365 + previous_year / 4 - previous_year / 100 + previous_year / 400 + days_to_month[month - 1] + day - 1;
    *result = days * 864000000000LL + (iron_i64)hour * 36000000000LL + (iron_i64)minute * 600000000LL +
              (iron_i64)second * 10000000LL + nanosecond / 100;
    return IRON_TRUE;
}

iron_bool iron_platform_get_utc_ticks(iron_i64 *result)
{
    if (!result) {
        return IRON_FALSE;
    }

#if defined(IRON_OS_WINDOWS)
    FILETIME file_time;
    ULARGE_INTEGER ticks_since_1601;

    GetSystemTimeAsFileTime(&file_time);
    ticks_since_1601.LowPart = file_time.dwLowDateTime;
    ticks_since_1601.HighPart = file_time.dwHighDateTime;
    *result = (iron_i64)ticks_since_1601.QuadPart + 504911232000000000LL;
    return IRON_TRUE;
#elif defined(IRON_OS_POSIX)
    struct timespec timestamp;

    if (clock_gettime(CLOCK_REALTIME, &timestamp) != 0) {
        return IRON_FALSE;
    }

    *result = 621355968000000000LL + (iron_i64)timestamp.tv_sec * 10000000LL + (iron_i64)timestamp.tv_nsec / 100;
    return IRON_TRUE;
#else
    return IRON_FALSE;
#endif
}

iron_bool iron_platform_get_local_ticks(iron_i64 *result)
{
#if defined(IRON_OS_WINDOWS)
    SYSTEMTIME local_time;

    if (!result) {
        return IRON_FALSE;
    }

    GetLocalTime(&local_time);
    return calendar_to_ticks((iron_i32)local_time.wYear,
                             (iron_i32)local_time.wMonth,
                             (iron_i32)local_time.wDay,
                             (iron_i32)local_time.wHour,
                             (iron_i32)local_time.wMinute,
                             (iron_i32)local_time.wSecond,
                             (iron_i32)local_time.wMilliseconds * 1000000,
                             result);
#elif defined(IRON_OS_POSIX)
    struct timespec timestamp;
    struct tm local_time;

    if (!result || clock_gettime(CLOCK_REALTIME, &timestamp) != 0 || !localtime_r(&timestamp.tv_sec, &local_time)) {
        return IRON_FALSE;
    }

    return calendar_to_ticks((iron_i32)local_time.tm_year + 1900,
                             (iron_i32)local_time.tm_mon + 1,
                             (iron_i32)local_time.tm_mday,
                             (iron_i32)local_time.tm_hour,
                             (iron_i32)local_time.tm_min,
                             (iron_i32)local_time.tm_sec,
                             (iron_i32)timestamp.tv_nsec,
                             result);
#else
    (void)result;
    return IRON_FALSE;
#endif
}

iron_u64 iron_platform_monotonic_milliseconds(void)
{
#if defined(IRON_OS_WINDOWS)
    return (iron_u64)GetTickCount64();
#elif defined(IRON_OS_POSIX)
    struct timespec timestamp;

    if (clock_gettime(CLOCK_MONOTONIC, &timestamp) != 0) {
        return 0;
    }
    return (iron_u64)timestamp.tv_sec * 1000u + (iron_u64)timestamp.tv_nsec / 1000000u;
#else
    clock_t ticks;

    ticks = clock();
    if (ticks == (clock_t)-1) {
        return 0;
    }
    return (iron_u64)ticks * 1000u / (iron_u64)CLOCKS_PER_SEC;
#endif
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
