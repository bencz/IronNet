/*
 * IronNet CLR Interpreter
 * debug.c - Debug and logging system implementation
 */

#include "iron/debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

/* strcasecmp for portability */
static int iron_strcasecmp(const char *s1, const char *s2)
{
    while (*s1 && *s2) {
        int c1 = (*s1 >= 'A' && *s1 <= 'Z') ? *s1 + 32 : *s1;
        int c2 = (*s2 >= 'A' && *s2 <= 'Z') ? *s2 + 32 : *s2;
        if (c1 != c2) return c1 - c2;
        s1++;
        s2++;
    }
    return *s1 - *s2;
}

/* ============================================================================
 * Global State
 * ============================================================================ */

static iron_debug_config_t g_debug_config = IRON_DEBUG_CONFIG_DEFAULT;
static iron_bool g_debug_initialized = IRON_FALSE;

/* ANSI color codes */
#define ANSI_RESET   "\033[0m"
#define ANSI_RED     "\033[31m"
#define ANSI_GREEN   "\033[32m"
#define ANSI_YELLOW  "\033[33m"
#define ANSI_BLUE    "\033[34m"
#define ANSI_MAGENTA "\033[35m"
#define ANSI_CYAN    "\033[36m"
#define ANSI_WHITE   "\033[37m"
#define ANSI_BOLD    "\033[1m"
#define ANSI_DIM     "\033[2m"

/* ============================================================================
 * Level and Component Names
 * ============================================================================ */

static const char *g_level_names[] = {
    "TRACE",
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
    "FATAL",
    "OFF"
};

static const char *g_level_colors[] = {
    ANSI_DIM ANSI_WHITE,    /* TRACE - dim */
    ANSI_CYAN,              /* DEBUG - cyan */
    ANSI_GREEN,             /* INFO - green */
    ANSI_YELLOW,            /* WARN - yellow */
    ANSI_RED,               /* ERROR - red */
    ANSI_BOLD ANSI_RED,     /* FATAL - bold red */
    ""                      /* OFF */
};

typedef struct {
    iron_log_component_t comp;
    const char *name;
    const char *short_name;
} component_info_t;

static const component_info_t g_component_info[] = {
    { IRON_LOG_COMP_CORE,     "CORE",     "CORE" },
    { IRON_LOG_COMP_GC,       "GC",       "GC  " },
    { IRON_LOG_COMP_EXEC,     "EXEC",     "EXEC" },
    { IRON_LOG_COMP_METADATA, "METADATA", "META" },
    { IRON_LOG_COMP_PE,       "PE",       "PE  " },
    { IRON_LOG_COMP_THREAD,   "THREAD",   "THRD" },
    { IRON_LOG_COMP_CORLIB,   "CORLIB",   "CLIB" },
    { IRON_LOG_COMP_JIT,      "JIT",      "JIT " },
    { IRON_LOG_COMP_INTEROP,  "INTEROP",  "INTR" },
    { 0, NULL, NULL }
};

/* ============================================================================
 * Initialization
 * ============================================================================ */

void iron_debug_init(void)
{
    if (g_debug_initialized) return;
    
    g_debug_config.output = stderr;
    g_debug_initialized = IRON_TRUE;
}

void iron_debug_shutdown(void)
{
    g_debug_initialized = IRON_FALSE;
}

/* ============================================================================
 * Configuration
 * ============================================================================ */

void iron_debug_set_config(const iron_debug_config_t *config)
{
    if (config) {
        g_debug_config = *config;
        if (!g_debug_config.output) {
            g_debug_config.output = stderr;
        }
    }
}

void iron_debug_get_config(iron_debug_config_t *config)
{
    if (config) {
        *config = g_debug_config;
    }
}

void iron_debug_enable(iron_bool enable)
{
    g_debug_config.enabled = enable;
}

iron_bool iron_debug_is_enabled(void)
{
    return g_debug_config.enabled;
}

void iron_debug_set_level(iron_log_level_t level)
{
    g_debug_config.level = level;
}

iron_log_level_t iron_debug_get_level(void)
{
    return g_debug_config.level;
}

void iron_debug_set_components(iron_u32 components)
{
    g_debug_config.components = components;
}

void iron_debug_enable_component(iron_log_component_t comp)
{
    g_debug_config.components |= (iron_u32)comp;
}

void iron_debug_disable_component(iron_log_component_t comp)
{
    g_debug_config.components &= ~(iron_u32)comp;
}

void iron_debug_set_output(FILE *output)
{
    g_debug_config.output = output ? output : stderr;
}

/* ============================================================================
 * Logging Check
 * ============================================================================ */

iron_bool iron_debug_should_log(iron_log_level_t level, iron_log_component_t comp)
{
    if (!g_debug_config.enabled) return IRON_FALSE;
    if (level < g_debug_config.level) return IRON_FALSE;
    if (!(g_debug_config.components & (iron_u32)comp)) return IRON_FALSE;
    return IRON_TRUE;
}

/* ============================================================================
 * Core Logging
 * ============================================================================ */

static void get_timestamp(char *buf, size_t size)
{
    iron_calendar_time_t local_time;

    if (!buf || size == 0) {
        return;
    }

    if (!iron_platform_get_local_time(&local_time)) {
        buf[0] = '\0';
        return;
    }

    snprintf(buf, size, "%02d:%02d:%02d", local_time.hour, local_time.minute, local_time.second);
}

static const char *get_component_short_name(iron_log_component_t comp)
{
    int i;
    for (i = 0; g_component_info[i].name != NULL; i++) {
        if (g_component_info[i].comp == comp) {
            return g_component_info[i].short_name;
        }
    }
    return "????";
}

void iron_debug_log(iron_log_level_t level,
                    iron_log_component_t comp,
                    const char *file,
                    int line,
                    const char *fmt, ...)
{
    va_list args;
    FILE *out;
    char timestamp[16];
    
    if (!iron_debug_should_log(level, comp)) return;
    
    /* Custom handler takes precedence */
    if (g_debug_config.custom_handler) {
        va_start(args, fmt);
        g_debug_config.custom_handler(level, comp, file, line, fmt, args);
        va_end(args);
        return;
    }
    
    out = g_debug_config.output ? g_debug_config.output : stderr;
    
    /* Start with color if enabled */
    if (g_debug_config.colorize && level < IRON_LOG_OFF) {
        fprintf(out, "%s", g_level_colors[level]);
    }
    
    /* Timestamp */
    if (g_debug_config.show_timestamp) {
        get_timestamp(timestamp, sizeof(timestamp));
        fprintf(out, "[%s] ", timestamp);
    }
    
    /* Level */
    fprintf(out, "%-5s ", g_level_names[level]);
    
    /* Component */
    if (g_debug_config.show_component) {
        fprintf(out, "[%s] ", get_component_short_name(comp));
    }
    
    /* File:line */
    if (g_debug_config.show_file_line && file) {
        fprintf(out, "%s:%d: ", file, line);
    }
    
    /* Message */
    va_start(args, fmt);
    vfprintf(out, fmt, args);
    va_end(args);
    
    /* Reset color and newline */
    if (g_debug_config.colorize) {
        fprintf(out, "%s", ANSI_RESET);
    }
    fprintf(out, "\n");
    
    /* Flush on errors */
    if (level >= IRON_LOG_ERROR) {
        fflush(out);
    }
}

/* ============================================================================
 * Execution Tracing
 * ============================================================================ */

void iron_debug_trace_opcode(iron_u32 ip, iron_u16 opcode, const char *opcode_name)
{
    IRON_TRACE_EXEC("IP=%04X opcode=0x%04X (%s)", ip, opcode, 
                    opcode_name ? opcode_name : "unknown");
}

void iron_debug_trace_method_enter(const char *type_name,
                                   const char *method_name,
                                   iron_u32 arg_count)
{
    IRON_DEBUG_EXEC(">>> ENTER %s.%s (args=%u)", 
                    type_name ? type_name : "?",
                    method_name ? method_name : "?",
                    arg_count);
}

void iron_debug_trace_method_exit(const char *type_name,
                                  const char *method_name,
                                  iron_bool has_return)
{
    IRON_DEBUG_EXEC("<<< EXIT  %s.%s %s",
                    type_name ? type_name : "?",
                    method_name ? method_name : "?",
                    has_return ? "(returns value)" : "");
}

void iron_debug_trace_gc_alloc(const char *type_name, 
                               iron_size size,
                               void *ptr)
{
    IRON_TRACE_GC("ALLOC %s size=%lu ptr=%p",
                  type_name ? type_name : "<unknown>",
                  (unsigned long)size, ptr);
}

void iron_debug_trace_gc_collect_start(void)
{
    IRON_DEBUG_GC("=== GC COLLECTION START ===");
}

void iron_debug_trace_gc_collect_end(iron_u32 freed_count, iron_size freed_bytes)
{
    IRON_DEBUG_GC("=== GC COLLECTION END: freed %u objects, %lu bytes ===",
                  freed_count, (unsigned long)freed_bytes);
}

void iron_debug_dump_stack(const char *label, 
                           const void *stack_data,
                           iron_u32 stack_size)
{
    iron_u32 i;
    const iron_u8 *data = (const iron_u8 *)stack_data;
    
    IRON_DEBUG_EXEC("Stack dump [%s] size=%u:", label ? label : "", stack_size);
    
    if (!data || stack_size == 0) {
        IRON_DEBUG_EXEC("  (empty)");
        return;
    }
    
    for (i = 0; i < stack_size && i < 64; i++) {
        if (i % 16 == 0) {
            fprintf(g_debug_config.output, "  %04X: ", i);
        }
        fprintf(g_debug_config.output, "%02X ", data[i]);
        if (i % 16 == 15) {
            fprintf(g_debug_config.output, "\n");
        }
    }
    if (stack_size % 16 != 0) {
        fprintf(g_debug_config.output, "\n");
    }
}

/* ============================================================================
 * Utility Functions
 * ============================================================================ */

const char *iron_debug_level_name(iron_log_level_t level)
{
    if (level >= 0 && level <= IRON_LOG_OFF) {
        return g_level_names[level];
    }
    return "UNKNOWN";
}

const char *iron_debug_component_name(iron_log_component_t comp)
{
    int i;
    for (i = 0; g_component_info[i].name != NULL; i++) {
        if (g_component_info[i].comp == comp) {
            return g_component_info[i].name;
        }
    }
    return "UNKNOWN";
}

iron_log_level_t iron_debug_parse_level(const char *str)
{
    if (!str) return IRON_LOG_INFO;
    
    if (strcmp(str, "trace") == 0 || strcmp(str, "TRACE") == 0 || strcmp(str, "0") == 0)
        return IRON_LOG_TRACE;
    if (strcmp(str, "debug") == 0 || strcmp(str, "DEBUG") == 0 || strcmp(str, "1") == 0)
        return IRON_LOG_DEBUG;
    if (strcmp(str, "info") == 0 || strcmp(str, "INFO") == 0 || strcmp(str, "2") == 0)
        return IRON_LOG_INFO;
    if (strcmp(str, "warn") == 0 || strcmp(str, "WARN") == 0 || strcmp(str, "3") == 0)
        return IRON_LOG_WARN;
    if (strcmp(str, "error") == 0 || strcmp(str, "ERROR") == 0 || strcmp(str, "4") == 0)
        return IRON_LOG_ERROR;
    if (strcmp(str, "fatal") == 0 || strcmp(str, "FATAL") == 0 || strcmp(str, "5") == 0)
        return IRON_LOG_FATAL;
    if (strcmp(str, "off") == 0 || strcmp(str, "OFF") == 0 || strcmp(str, "6") == 0)
        return IRON_LOG_OFF;
    
    return IRON_LOG_INFO;
}

iron_u32 iron_debug_parse_components(const char *str)
{
    iron_u32 result = 0;
    char buf[256];
    char *token;
    char *separator;
    char *end;
    size_t length;
    int i;
    
    if (!str) {
        return (iron_u32)IRON_LOG_COMP_ALL;
    }
    
    if (strcmp(str, "all") == 0 || strcmp(str, "ALL") == 0) {
        return (iron_u32)IRON_LOG_COMP_ALL;
    }
    
    length = strlen(str);
    if (length >= sizeof(buf)) {
        length = sizeof(buf) - 1;
    }

    memcpy(buf, str, length);
    buf[length] = '\0';
    
    token = buf;
    while (*token != '\0') {
        separator = strchr(token, ',');
        if (separator) {
            *separator = '\0';
        }

        /* Skip whitespace */
        while (*token == ' ' || *token == '\t') {
            token++;
        }

        end = token + strlen(token);
        while (end > token && (end[-1] == ' ' || end[-1] == '\t')) {
            end--;
        }
        *end = '\0';
        
        for (i = 0; g_component_info[i].name != NULL; i++) {
            if (iron_strcasecmp(token, g_component_info[i].name) == 0) {
                result |= (iron_u32)g_component_info[i].comp;
                break;
            }
        }
        
        if (!separator) {
            break;
        }

        token = separator + 1;
    }
    
    return result ? result : (iron_u32)IRON_LOG_COMP_ALL;
}

void iron_debug_hex_dump(const void *data, iron_size size, iron_u32 bytes_per_line)
{
    const iron_u8 *ptr = (const iron_u8 *)data;
    iron_size i, j;
    FILE *out = g_debug_config.output ? g_debug_config.output : stderr;
    
    if (!data || size == 0) return;
    if (bytes_per_line == 0) bytes_per_line = 16;
    
    for (i = 0; i < size; i += bytes_per_line) {
        /* Address */
        fprintf(out, "  %08lX: ", (unsigned long)i);
        
        /* Hex bytes */
        for (j = 0; j < bytes_per_line; j++) {
            if (i + j < size) {
                fprintf(out, "%02X ", ptr[i + j]);
            } else {
                fprintf(out, "   ");
            }
            if (j == 7) fprintf(out, " ");
        }
        
        fprintf(out, " |");
        
        /* ASCII */
        for (j = 0; j < bytes_per_line && i + j < size; j++) {
            unsigned char c = ptr[i + j];
            fprintf(out, "%c", (c >= 32 && c < 127) ? c : '.');
        }
        
        fprintf(out, "|\n");
    }
}
