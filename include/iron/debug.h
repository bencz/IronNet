/*
 * IronNet CLR Interpreter
 * debug.h - Debug and logging system
 * 
 * Provides:
 * - Multiple log levels (TRACE, DEBUG, INFO, WARN, ERROR, FATAL)
 * - Component-based filtering
 * - Runtime enable/disable
 * - File and line information
 * 
 * Pure C89 compatible
 */

#ifndef IRON_DEBUG_H
#define IRON_DEBUG_H

#include "platform.h"
#include "types.h"
#include <stdio.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Log Levels
 * ============================================================================ */

typedef enum iron_log_level {
    IRON_LOG_TRACE = 0,    /* Very detailed tracing */
    IRON_LOG_DEBUG = 1,    /* Debug information */
    IRON_LOG_INFO  = 2,    /* General information */
    IRON_LOG_WARN  = 3,    /* Warnings */
    IRON_LOG_ERROR = 4,    /* Errors */
    IRON_LOG_FATAL = 5,    /* Fatal errors */
    IRON_LOG_OFF   = 6     /* Logging disabled */
} iron_log_level_t;

/* ============================================================================
 * Log Components (for filtering)
 * ============================================================================ */

typedef enum iron_log_component {
    IRON_LOG_COMP_CORE     = (1 << 0),   /* Core runtime */
    IRON_LOG_COMP_GC       = (1 << 1),   /* Garbage collector */
    IRON_LOG_COMP_EXEC     = (1 << 2),   /* Execution engine */
    IRON_LOG_COMP_METADATA = (1 << 3),   /* Metadata parsing */
    IRON_LOG_COMP_PE       = (1 << 4),   /* PE/COFF loading */
    IRON_LOG_COMP_THREAD   = (1 << 5),   /* Threading */
    IRON_LOG_COMP_CORLIB   = (1 << 6),   /* Corlib internal calls */
    IRON_LOG_COMP_JIT      = (1 << 7),   /* JIT (future) */
    IRON_LOG_COMP_INTEROP  = (1 << 8),   /* P/Invoke interop */
    IRON_LOG_COMP_ALL      = 0xFFFFFFFF  /* All components */
} iron_log_component_t;

/* ============================================================================
 * Debug Configuration
 * ============================================================================ */

typedef struct iron_debug_config {
    iron_log_level_t level;           /* Minimum log level */
    iron_u32 components;              /* Enabled components (bitmask) */
    iron_bool enabled;                /* Global enable/disable */
    iron_bool show_timestamp;         /* Show timestamps */
    iron_bool show_file_line;         /* Show file:line */
    iron_bool show_component;         /* Show component name */
    iron_bool colorize;               /* Use ANSI colors */
    FILE *output;                     /* Output stream (default: stderr) */
    void (*custom_handler)(iron_log_level_t level, 
                          iron_log_component_t comp,
                          const char *file, int line,
                          const char *fmt, va_list args);
} iron_debug_config_t;

/* Default configuration */
#define IRON_DEBUG_CONFIG_DEFAULT { \
    IRON_LOG_INFO,         /* level */ \
    IRON_LOG_COMP_ALL,     /* components */ \
    IRON_FALSE,            /* enabled */ \
    IRON_TRUE,             /* show_timestamp */ \
    IRON_TRUE,             /* show_file_line */ \
    IRON_TRUE,             /* show_component */ \
    IRON_TRUE,             /* colorize */ \
    NULL,                  /* output (set to stderr in init) */ \
    NULL                   /* custom_handler */ \
}

/* ============================================================================
 * Debug API
 * ============================================================================ */

/* Initialize debug system */
IRON_API void iron_debug_init(void);

/* Shutdown debug system */
IRON_API void iron_debug_shutdown(void);

/* Configure debug system */
IRON_API void iron_debug_set_config(const iron_debug_config_t *config);
IRON_API void iron_debug_get_config(iron_debug_config_t *config);

/* Enable/disable */
IRON_API void iron_debug_enable(iron_bool enable);
IRON_API iron_bool iron_debug_is_enabled(void);

/* Set log level */
IRON_API void iron_debug_set_level(iron_log_level_t level);
IRON_API iron_log_level_t iron_debug_get_level(void);

/* Set components */
IRON_API void iron_debug_set_components(iron_u32 components);
IRON_API void iron_debug_enable_component(iron_log_component_t comp);
IRON_API void iron_debug_disable_component(iron_log_component_t comp);

/* Set output stream */
IRON_API void iron_debug_set_output(FILE *output);

/* Check if logging is active for level/component */
IRON_API iron_bool iron_debug_should_log(iron_log_level_t level, 
                                          iron_log_component_t comp);

/* Core logging function */
IRON_API void iron_debug_log(iron_log_level_t level,
                              iron_log_component_t comp,
                              const char *file,
                              int line,
                              const char *fmt, ...);

/* ============================================================================
 * Convenience Macros
 * ============================================================================ */

/* Get just the filename from path */
#if defined(_WIN32) || defined(_WIN64)
    #define IRON_FILENAME (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#else
    #define IRON_FILENAME (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

/* Log macros with component */
#define IRON_LOG(level, comp, ...) \
    do { \
        if (iron_debug_should_log(level, comp)) { \
            iron_debug_log(level, comp, IRON_FILENAME, __LINE__, __VA_ARGS__); \
        } \
    } while(0)

/* Component-specific trace macros */
#define IRON_TRACE_CORE(...)     IRON_LOG(IRON_LOG_TRACE, IRON_LOG_COMP_CORE, __VA_ARGS__)
#define IRON_TRACE_GC(...)       IRON_LOG(IRON_LOG_TRACE, IRON_LOG_COMP_GC, __VA_ARGS__)
#define IRON_TRACE_EXEC(...)     IRON_LOG(IRON_LOG_TRACE, IRON_LOG_COMP_EXEC, __VA_ARGS__)
#define IRON_TRACE_META(...)     IRON_LOG(IRON_LOG_TRACE, IRON_LOG_COMP_METADATA, __VA_ARGS__)
#define IRON_TRACE_PE(...)       IRON_LOG(IRON_LOG_TRACE, IRON_LOG_COMP_PE, __VA_ARGS__)
#define IRON_TRACE_THREAD(...)   IRON_LOG(IRON_LOG_TRACE, IRON_LOG_COMP_THREAD, __VA_ARGS__)
#define IRON_TRACE_CORLIB(...)   IRON_LOG(IRON_LOG_TRACE, IRON_LOG_COMP_CORLIB, __VA_ARGS__)

/* Component-specific debug macros */
#define IRON_DEBUG_CORE(...)     IRON_LOG(IRON_LOG_DEBUG, IRON_LOG_COMP_CORE, __VA_ARGS__)
#define IRON_DEBUG_GC(...)       IRON_LOG(IRON_LOG_DEBUG, IRON_LOG_COMP_GC, __VA_ARGS__)
#define IRON_DEBUG_EXEC(...)     IRON_LOG(IRON_LOG_DEBUG, IRON_LOG_COMP_EXEC, __VA_ARGS__)
#define IRON_DEBUG_META(...)     IRON_LOG(IRON_LOG_DEBUG, IRON_LOG_COMP_METADATA, __VA_ARGS__)
#define IRON_DEBUG_PE(...)       IRON_LOG(IRON_LOG_DEBUG, IRON_LOG_COMP_PE, __VA_ARGS__)
#define IRON_DEBUG_THREAD(...)   IRON_LOG(IRON_LOG_DEBUG, IRON_LOG_COMP_THREAD, __VA_ARGS__)
#define IRON_DEBUG_CORLIB(...)   IRON_LOG(IRON_LOG_DEBUG, IRON_LOG_COMP_CORLIB, __VA_ARGS__)

/* Component-specific info macros */
#define IRON_INFO_CORE(...)      IRON_LOG(IRON_LOG_INFO, IRON_LOG_COMP_CORE, __VA_ARGS__)
#define IRON_INFO_GC(...)        IRON_LOG(IRON_LOG_INFO, IRON_LOG_COMP_GC, __VA_ARGS__)
#define IRON_INFO_EXEC(...)      IRON_LOG(IRON_LOG_INFO, IRON_LOG_COMP_EXEC, __VA_ARGS__)
#define IRON_INFO_META(...)      IRON_LOG(IRON_LOG_INFO, IRON_LOG_COMP_METADATA, __VA_ARGS__)
#define IRON_INFO_PE(...)        IRON_LOG(IRON_LOG_INFO, IRON_LOG_COMP_PE, __VA_ARGS__)
#define IRON_INFO_THREAD(...)    IRON_LOG(IRON_LOG_INFO, IRON_LOG_COMP_THREAD, __VA_ARGS__)
#define IRON_INFO_CORLIB(...)    IRON_LOG(IRON_LOG_INFO, IRON_LOG_COMP_CORLIB, __VA_ARGS__)

/* Component-specific warn macros */
#define IRON_WARN_CORE(...)      IRON_LOG(IRON_LOG_WARN, IRON_LOG_COMP_CORE, __VA_ARGS__)
#define IRON_WARN_GC(...)        IRON_LOG(IRON_LOG_WARN, IRON_LOG_COMP_GC, __VA_ARGS__)
#define IRON_WARN_EXEC(...)      IRON_LOG(IRON_LOG_WARN, IRON_LOG_COMP_EXEC, __VA_ARGS__)
#define IRON_WARN_META(...)      IRON_LOG(IRON_LOG_WARN, IRON_LOG_COMP_METADATA, __VA_ARGS__)
#define IRON_WARN_PE(...)        IRON_LOG(IRON_LOG_WARN, IRON_LOG_COMP_PE, __VA_ARGS__)
#define IRON_WARN_THREAD(...)    IRON_LOG(IRON_LOG_WARN, IRON_LOG_COMP_THREAD, __VA_ARGS__)
#define IRON_WARN_CORLIB(...)    IRON_LOG(IRON_LOG_WARN, IRON_LOG_COMP_CORLIB, __VA_ARGS__)

/* Component-specific error macros */
#define IRON_ERROR_CORE(...)     IRON_LOG(IRON_LOG_ERROR, IRON_LOG_COMP_CORE, __VA_ARGS__)
#define IRON_ERROR_GC(...)       IRON_LOG(IRON_LOG_ERROR, IRON_LOG_COMP_GC, __VA_ARGS__)
#define IRON_ERROR_EXEC(...)     IRON_LOG(IRON_LOG_ERROR, IRON_LOG_COMP_EXEC, __VA_ARGS__)
#define IRON_ERROR_META(...)     IRON_LOG(IRON_LOG_ERROR, IRON_LOG_COMP_METADATA, __VA_ARGS__)
#define IRON_ERROR_PE(...)       IRON_LOG(IRON_LOG_ERROR, IRON_LOG_COMP_PE, __VA_ARGS__)
#define IRON_ERROR_THREAD(...)   IRON_LOG(IRON_LOG_ERROR, IRON_LOG_COMP_THREAD, __VA_ARGS__)
#define IRON_ERROR_CORLIB(...)   IRON_LOG(IRON_LOG_ERROR, IRON_LOG_COMP_CORLIB, __VA_ARGS__)

/* Generic level macros (use CORE component) */
#define IRON_TRACE(...)  IRON_TRACE_CORE(__VA_ARGS__)
#define IRON_DEBUG(...)  IRON_DEBUG_CORE(__VA_ARGS__)
#define IRON_INFO(...)   IRON_INFO_CORE(__VA_ARGS__)
#define IRON_WARN(...)   IRON_WARN_CORE(__VA_ARGS__)
#define IRON_ERR(...)    IRON_ERROR_CORE(__VA_ARGS__)
#define IRON_FATAL(...)  IRON_LOG(IRON_LOG_FATAL, IRON_LOG_COMP_CORE, __VA_ARGS__)

/* ============================================================================
 * Execution Tracing
 * ============================================================================ */

/* Opcode execution trace */
IRON_API void iron_debug_trace_opcode(iron_u32 ip, iron_u16 opcode, 
                                       const char *opcode_name);

/* Method call trace */
IRON_API void iron_debug_trace_method_enter(const char *type_name,
                                             const char *method_name,
                                             iron_u32 arg_count);
IRON_API void iron_debug_trace_method_exit(const char *type_name,
                                            const char *method_name,
                                            iron_bool has_return);

/* GC trace */
IRON_API void iron_debug_trace_gc_alloc(const char *type_name, 
                                         iron_size size,
                                         void *ptr);
IRON_API void iron_debug_trace_gc_collect_start(void);
IRON_API void iron_debug_trace_gc_collect_end(iron_u32 freed_count,
                                               iron_size freed_bytes);

/* Stack trace */
IRON_API void iron_debug_dump_stack(const char *label, 
                                     const void *stack_data,
                                     iron_u32 stack_size);

/* ============================================================================
 * Utility Functions
 * ============================================================================ */

/* Get level name */
IRON_API const char *iron_debug_level_name(iron_log_level_t level);

/* Get component name */
IRON_API const char *iron_debug_component_name(iron_log_component_t comp);

/* Parse level from string */
IRON_API iron_log_level_t iron_debug_parse_level(const char *str);

/* Parse components from string (comma-separated) */
IRON_API iron_u32 iron_debug_parse_components(const char *str);

/* Hex dump utility */
IRON_API void iron_debug_hex_dump(const void *data, iron_size size,
                                   iron_u32 bytes_per_line);

#ifdef __cplusplus
}
#endif

#endif /* IRON_DEBUG_H */
