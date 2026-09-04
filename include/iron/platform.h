/*
 * IronNet CLR Interpreter
 * platform.h - Platform detection, endianness, and portability macros
 * 
 * Supports:
 *   - Architectures: x86, x64, ARM, ARM64, MIPS, PPC, RISC-V, etc.
 *   - Word sizes: 32 and 64 bits
 *   - Endianness: Little-endian, Big-endian
 *   - OS: Windows, Linux, macOS, AIX, BSD, bare-metal
 * 
 * Strict C99 compatible
 */

#ifndef IRON_PLATFORM_H
#define IRON_PLATFORM_H

/* Enable POSIX features before any system includes */
#if !defined(_WIN32) && !defined(_WIN64)
    #ifndef _POSIX_C_SOURCE
        #define _POSIX_C_SOURCE 200809L
    #endif
    #ifndef _XOPEN_SOURCE
        #define _XOPEN_SOURCE 700
    #endif
#endif

/* ============================================================================
 * C99 fixed-width and pointer-sized types
 * ============================================================================ */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef int8_t    iron_i8;
typedef uint8_t   iron_u8;
typedef int16_t   iron_i16;
typedef uint16_t  iron_u16;
typedef int32_t   iron_i32;
typedef uint32_t  iron_u32;
typedef int64_t   iron_i64;
typedef uint64_t  iron_u64;
typedef size_t    iron_size;
typedef ptrdiff_t iron_ptrdiff;

/* Floating point types */
typedef float  iron_f32;
typedef double iron_f64;

typedef bool iron_bool;
#define IRON_TRUE  true
#define IRON_FALSE false

/* NULL definition */
#ifndef NULL
    #define NULL ((void*)0)
#endif

/* ============================================================================
 * Architecture Detection
 * ============================================================================ */

/* Word size detection is based on the actual C pointer representation. */
#if UINTPTR_MAX == UINT64_MAX
    #define IRON_WORD_SIZE 64
    #define IRON_ARCH_64BIT 1
#elif UINTPTR_MAX == UINT32_MAX
    #define IRON_WORD_SIZE 32
    #define IRON_ARCH_32BIT 1
#else
    #error "IronNet requires a 32-bit or 64-bit C implementation"
#endif

/* Pointer size in bytes */
#define IRON_PTR_SIZE (IRON_WORD_SIZE / 8)

/* Specific architecture detection */
#if defined(__x86_64__) || defined(_M_X64) || defined(__amd64__)
    #define IRON_ARCH_X64 1
    #define IRON_ARCH_NAME "x86_64"
#elif defined(__i386__) || defined(_M_IX86) || defined(__i686__)
    #define IRON_ARCH_X86 1
    #define IRON_ARCH_NAME "x86"
#elif defined(__aarch64__) || defined(_M_ARM64)
    #define IRON_ARCH_ARM64 1
    #define IRON_ARCH_NAME "arm64"
#elif defined(__arm__) || defined(_M_ARM)
    #define IRON_ARCH_ARM 1
    #define IRON_ARCH_NAME "arm"
#elif defined(__powerpc64__) || defined(__ppc64__)
    #define IRON_ARCH_PPC64 1
    #define IRON_ARCH_NAME "ppc64"
#elif defined(__powerpc__) || defined(__ppc__)
    #define IRON_ARCH_PPC 1
    #define IRON_ARCH_NAME "ppc"
#elif defined(__mips64)
    #define IRON_ARCH_MIPS64 1
    #define IRON_ARCH_NAME "mips64"
#elif defined(__mips__)
    #define IRON_ARCH_MIPS 1
    #define IRON_ARCH_NAME "mips"
#elif defined(__riscv)
    #if __riscv_xlen == 64
        #define IRON_ARCH_RISCV64 1
        #define IRON_ARCH_NAME "riscv64"
    #else
        #define IRON_ARCH_RISCV32 1
        #define IRON_ARCH_NAME "riscv32"
    #endif
#elif defined(__s390x__)
    #define IRON_ARCH_S390X 1
    #define IRON_ARCH_NAME "s390x"
#elif defined(__s390__)
    #define IRON_ARCH_S390 1
    #define IRON_ARCH_NAME "s390"
#elif defined(__sparc__)
    #define IRON_ARCH_SPARC 1
    #define IRON_ARCH_NAME "sparc"
#elif defined(__wasm__)
    #define IRON_ARCH_WASM 1
    #define IRON_ARCH_NAME "wasm"
#else
    #define IRON_ARCH_UNKNOWN 1
    #define IRON_ARCH_NAME "unknown"
#endif

/* ============================================================================
 * Operating System Detection
 * ============================================================================ */

#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
    #define IRON_OS_WINDOWS 1
    #define IRON_OS_NAME "windows"
#elif defined(__APPLE__) && defined(__MACH__)
    #define IRON_OS_MACOS 1
    #define IRON_OS_POSIX 1
    #define IRON_OS_NAME "macos"
#elif defined(__ANDROID__)
    #define IRON_OS_ANDROID 1
    #define IRON_OS_POSIX 1
    #define IRON_OS_NAME "android"
#elif defined(__EMSCRIPTEN__)
    #define IRON_OS_EMSCRIPTEN 1
    #define IRON_OS_POSIX 1
    #define IRON_OS_NAME "emscripten"
#elif defined(_AIX) || defined(__PASE__)
    #define IRON_OS_AIX 1
    #define IRON_OS_POSIX 1
    #define IRON_OS_NAME "aix"
#elif defined(__linux__)
    #define IRON_OS_LINUX 1
    #define IRON_OS_POSIX 1
    #define IRON_OS_NAME "linux"
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__DragonFly__)
    #define IRON_OS_BSD 1
    #define IRON_OS_POSIX 1
    #define IRON_OS_NAME "bsd"
#elif defined(__unix__)
    #define IRON_OS_UNIX 1
    #define IRON_OS_POSIX 1
    #define IRON_OS_NAME "unix"
#else
    #error "IronNet requires a supported Windows or POSIX platform backend"
#endif

/* ============================================================================
 * Endianness Detection and Byte Swapping
 * ============================================================================ */

typedef enum iron_endian {
    IRON_ENDIAN_LITTLE = 0,
    IRON_ENDIAN_BIG    = 1
} iron_endian_t;

/* Compile-time endianness detection */
#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && \
    defined(__ORDER_BIG_ENDIAN__)
    #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        #define IRON_LITTLE_ENDIAN 1
    #elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        #define IRON_BIG_ENDIAN 1
    #endif
#elif defined(__LITTLE_ENDIAN__) || defined(__ARMEL__) || defined(__AARCH64EL__) || defined(__MIPSEL__) || defined(_MIPSEL) || defined(__PPC64LE__)
    #define IRON_LITTLE_ENDIAN 1
#elif defined(_WIN32) || defined(IRON_ARCH_X86) || defined(IRON_ARCH_X64) || \
      defined(IRON_ARCH_ARM) || defined(IRON_ARCH_ARM64) || \
      defined(IRON_ARCH_RISCV32) || defined(IRON_ARCH_RISCV64) || \
      defined(IRON_ARCH_WASM)
    #define IRON_LITTLE_ENDIAN 1
#elif defined(__BIG_ENDIAN__) || defined(__ARMEB__) || defined(__THUMBEB__) || \
      defined(__AARCH64EB__) || defined(_MIPSEB) || defined(__MIPSEB) || \
      defined(IRON_ARCH_S390) || defined(IRON_ARCH_S390X) || \
      defined(IRON_ARCH_PPC) || defined(IRON_ARCH_PPC64)
    #define IRON_BIG_ENDIAN 1
#else
    /* Runtime detection needed */
    #define IRON_ENDIAN_RUNTIME 1
#endif

/* Runtime endianness detection function */
#ifdef __cplusplus
extern "C" {
#endif

iron_endian_t iron_detect_endian(void);

#ifdef __cplusplus
}
#endif

/* Portable byte-swap macros. */
#define IRON_BSWAP16(x) \
    ((iron_u16)(((iron_u16)(x) >> 8) | ((iron_u16)(x) << 8)))

#define IRON_BSWAP32(x) \
    ((iron_u32)( \
        (((iron_u32)(x) >> 24) & 0x000000FFUL) | \
        (((iron_u32)(x) >> 8)  & 0x0000FF00UL) | \
        (((iron_u32)(x) << 8)  & 0x00FF0000UL) | \
        (((iron_u32)(x) << 24) & 0xFF000000UL)  \
    ))

#ifndef IRON_NO_NATIVE_64BIT
#define IRON_BSWAP64(x) \
    ((iron_u64)( \
        (((iron_u64)(x) >> 56) & 0x00000000000000FFULL) | \
        (((iron_u64)(x) >> 40) & 0x000000000000FF00ULL) | \
        (((iron_u64)(x) >> 24) & 0x0000000000FF0000ULL) | \
        (((iron_u64)(x) >> 8)  & 0x00000000FF000000ULL) | \
        (((iron_u64)(x) << 8)  & 0x000000FF00000000ULL) | \
        (((iron_u64)(x) << 24) & 0x0000FF0000000000ULL) | \
        (((iron_u64)(x) << 40) & 0x00FF000000000000ULL) | \
        (((iron_u64)(x) << 56) & 0xFF00000000000000ULL)  \
    ))
#endif

/* Little-endian to host conversion */
#ifdef IRON_LITTLE_ENDIAN
    #define IRON_LE16(x) (x)
    #define IRON_LE32(x) (x)
    #define IRON_LE64(x) (x)
    #define IRON_BE16(x) IRON_BSWAP16(x)
    #define IRON_BE32(x) IRON_BSWAP32(x)
    #define IRON_BE64(x) IRON_BSWAP64(x)
#elif defined(IRON_BIG_ENDIAN)
    #define IRON_LE16(x) IRON_BSWAP16(x)
    #define IRON_LE32(x) IRON_BSWAP32(x)
    #define IRON_LE64(x) IRON_BSWAP64(x)
    #define IRON_BE16(x) (x)
    #define IRON_BE32(x) (x)
    #define IRON_BE64(x) (x)
#else
    /* Runtime conversion functions declared below */
    iron_u16 iron_le16(iron_u16 x);
    iron_u32 iron_le32(iron_u32 x);
    iron_u64 iron_le64(iron_u64 x);
    iron_u16 iron_be16(iron_u16 x);
    iron_u32 iron_be32(iron_u32 x);
    iron_u64 iron_be64(iron_u64 x);
    #define IRON_LE16(x) iron_le16(x)
    #define IRON_LE32(x) iron_le32(x)
    #define IRON_LE64(x) iron_le64(x)
    #define IRON_BE16(x) iron_be16(x)
    #define IRON_BE32(x) iron_be32(x)
    #define IRON_BE64(x) iron_be64(x)
#endif

/* ============================================================================
 * Unaligned Memory Access
 * ============================================================================ */

#ifdef __cplusplus
extern "C" {
#endif

/* Safe unaligned read functions (always work) */
iron_u16 iron_read_u16_le(const void *ptr);
iron_u32 iron_read_u32_le(const void *ptr);
iron_u64 iron_read_u64_le(const void *ptr);
iron_i8  iron_read_i8(const void *ptr);
iron_i16 iron_read_i16_le(const void *ptr);
iron_i32 iron_read_i32_le(const void *ptr);
iron_i64 iron_read_i64_le(const void *ptr);
iron_f32 iron_read_f32_le(const void *ptr);
iron_f64 iron_read_f64_le(const void *ptr);

/* Safe unaligned write functions */
void iron_write_u16_le(void *ptr, iron_u16 val);
void iron_write_u32_le(void *ptr, iron_u32 val);
void iron_write_u64_le(void *ptr, iron_u64 val);
void iron_write_i16_le(void *ptr, iron_i16 val);
void iron_write_i32_le(void *ptr, iron_i32 val);
void iron_write_i64_le(void *ptr, iron_i64 val);

#ifdef __cplusplus
}
#endif

/* ============================================================================
 * Compiler attributes
 * ============================================================================ */

#if defined(__GNUC__) || defined(__clang__)
    #define IRON_UNUSED       __attribute__((unused))
    #define IRON_PACKED       __attribute__((packed))
    #define IRON_ALIGNED(n)   __attribute__((aligned(n)))
    #define IRON_NORETURN     __attribute__((noreturn))
    #define IRON_PRINTF(f,a)  __attribute__((format(printf, f, a)))
    #define IRON_LIKELY(x)    __builtin_expect(!!(x), 1)
    #define IRON_UNLIKELY(x)  __builtin_expect(!!(x), 0)
    #define IRON_INLINE       static __inline__
    #define IRON_NOINLINE     __attribute__((noinline))
    #define IRON_RESTRICT     __restrict__
    #define IRON_THREAD_LOCAL __thread
#elif defined(_MSC_VER)
    #define IRON_UNUSED
    #define IRON_PACKED
    #define IRON_ALIGNED(n)   __declspec(align(n))
    #define IRON_NORETURN     __declspec(noreturn)
    #define IRON_PRINTF(f,a)
    #define IRON_LIKELY(x)    (x)
    #define IRON_UNLIKELY(x)  (x)
    #define IRON_INLINE       static __inline
    #define IRON_NOINLINE     __declspec(noinline)
    #define IRON_RESTRICT     __restrict
    #define IRON_THREAD_LOCAL __declspec(thread)
#else
    #define IRON_UNUSED
    #define IRON_PACKED
    #define IRON_ALIGNED(n)
    #define IRON_NORETURN
    #define IRON_PRINTF(f,a)
    #define IRON_LIKELY(x)    (x)
    #define IRON_UNLIKELY(x)  (x)
    #define IRON_INLINE       static
    #define IRON_NOINLINE
    #define IRON_RESTRICT
    #define IRON_THREAD_LOCAL
#endif

/* Export/Import for shared libraries */
#if defined(IRON_OS_WINDOWS)
    #ifdef IRON_BUILD_SHARED
        #define IRON_API __declspec(dllexport)
    #elif defined(IRON_USE_SHARED)
        #define IRON_API __declspec(dllimport)
    #else
        #define IRON_API
    #endif
#elif defined(__GNUC__) && __GNUC__ >= 4
    #define IRON_API __attribute__((visibility("default")))
#else
    #define IRON_API
#endif

/* ============================================================================
 * Assertions and Debug
 * ============================================================================ */

#ifdef IRON_DEBUG
    #include <assert.h>
    #define IRON_ASSERT(x) assert(x)
#else
    #define IRON_ASSERT(x) ((void)0)
#endif

/* Portable compile-time assertion. */
#define IRON_STATIC_ASSERT(cond, msg) \
    typedef char iron_static_assert_##msg[(cond) ? 1 : -1]

/* ============================================================================
 * Platform Information
 * ============================================================================ */

typedef struct iron_platform_info {
    const char *arch_name;
    const char *os_name;
    iron_endian_t endianness;
    iron_u32 word_size;
    iron_u32 ptr_size;
    iron_u32 page_size;
} iron_platform_info_t;

typedef struct iron_calendar_time {
    iron_i32 year;
    iron_i32 month;
    iron_i32 day;
    iron_i32 hour;
    iron_i32 minute;
    iron_i32 second;
} iron_calendar_time_t;

#ifdef __cplusplus
extern "C" {
#endif

IRON_API const iron_platform_info_t *iron_get_platform_info(void);
IRON_API void iron_print_platform_info(void);
IRON_API iron_bool iron_platform_get_local_time(iron_calendar_time_t *result);
IRON_API iron_bool iron_platform_get_utc_ticks(iron_i64 *result);
IRON_API iron_bool iron_platform_get_local_ticks(iron_i64 *result);
IRON_API iron_u64 iron_platform_monotonic_milliseconds(void);
IRON_API void *iron_platform_alloc_hglobal(iron_size size);
IRON_API void iron_platform_free_hglobal(void *memory);
IRON_API iron_i32 iron_platform_get_last_error(void);

#ifdef __cplusplus
}
#endif

#endif /* IRON_PLATFORM_H */
