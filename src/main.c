/*
 * IronNet CLR Interpreter
 * main.c - Command-line interface
 * 
 * Usage: ironnet [options] <assembly.exe> [args...]
 */

#include "iron/iron.h"
#include "iron/corlib.h"
#include "iron/disasm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * Version and Help
 * ============================================================================ */

#define IRONNET_VERSION_MAJOR 0
#define IRONNET_VERSION_MINOR 1
#define IRONNET_VERSION_PATCH 0

static void print_version(void)
{
    printf("IronNet CLI Interpreter v%d.%d.%d\n",
           IRONNET_VERSION_MAJOR, IRONNET_VERSION_MINOR, IRONNET_VERSION_PATCH);
    printf("Pure C89 implementation of ECMA-335 CLI\n");
}

static void print_usage(const char *program)
{
    printf("Usage: %s [options] <assembly.exe> [args...]\n\n", program);
    printf("Options:\n");
    printf("  -h, --help              Show this help message\n");
    printf("  -v, --version           Show version information\n");
    printf("  -L <path>               Add assembly search path\n");
    printf("  --corlib <path>         Path to corlib.dll (default: ./corlib/corlib.dll)\n");
    printf("  --debug                 Enable debug output (level: INFO)\n");
    printf("  --debug-level <level>   Set debug level (trace,debug,info,warn,error,fatal)\n");
    printf("  --debug-components <c>  Enable specific components (comma-separated)\n");
    printf("                          Components: core,gc,exec,metadata,pe,thread,corlib,all\n");
    printf("  --no-color              Disable colored output\n");
    printf("  --trace                 Trace IL execution (shortcut for --debug-level trace)\n");
    printf("  --gc-threshold <n>      Set GC threshold in bytes\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s HelloWorld.exe\n", program);
    printf("  %s -L ./libs MyApp.exe arg1 arg2\n", program);
    printf("  %s --debug Program.exe\n", program);
    printf("  %s --debug-level trace --debug-components exec,gc Program.exe\n", program);
}

/* ============================================================================
 * Configuration
 * ============================================================================ */

typedef struct ironnet_config {
    const char *assembly_path;
    const char *corlib_path;
    const char **search_paths;
    int search_path_count;
    int search_path_capacity;
    const char **assembly_args;
    int assembly_argc;
    iron_bool debug_mode;
    iron_bool trace_mode;
    iron_bool disasm_mode;
    iron_log_level_t debug_level;
    iron_u32 debug_components;
    iron_bool no_color;
    iron_size gc_threshold;
} ironnet_config_t;

static void config_init(ironnet_config_t *config)
{
    memset(config, 0, sizeof(*config));
    config->corlib_path = "corlib/corlib.dll";
    config->gc_threshold = 1024 * 1024; /* 1MB default */
    config->debug_level = IRON_LOG_INFO;
    config->debug_components = IRON_LOG_COMP_ALL;
}

static void config_add_search_path(ironnet_config_t *config, const char *path)
{
    if (config->search_path_count >= config->search_path_capacity) {
        int new_cap = config->search_path_capacity == 0 ? 4 : config->search_path_capacity * 2;
        const char **new_paths = (const char **)realloc(
            (void *)config->search_paths, 
            new_cap * sizeof(const char *));
        if (!new_paths) return;
        config->search_paths = new_paths;
        config->search_path_capacity = new_cap;
    }
    config->search_paths[config->search_path_count++] = path;
}

static void config_cleanup(ironnet_config_t *config)
{
    if (config->search_paths) {
        free((void *)config->search_paths);
    }
}

/* ============================================================================
 * Argument Parsing
 * ============================================================================ */

static int parse_args(int argc, char **argv, ironnet_config_t *config)
{
    int i;
    
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return -1;
        }
        else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            print_version();
            return -1;
        }
        else if (strcmp(argv[i], "-L") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: -L requires a path argument\n");
                return 1;
            }
            config_add_search_path(config, argv[++i]);
        }
        else if (strcmp(argv[i], "--corlib") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: --corlib requires a path argument\n");
                return 1;
            }
            config->corlib_path = argv[++i];
        }
        else if (strcmp(argv[i], "--debug") == 0) {
            config->debug_mode = IRON_TRUE;
        }
        else if (strcmp(argv[i], "--debug-level") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: --debug-level requires a level argument\n");
                return 1;
            }
            config->debug_mode = IRON_TRUE;
            config->debug_level = iron_debug_parse_level(argv[++i]);
        }
        else if (strcmp(argv[i], "--debug-components") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: --debug-components requires a component list\n");
                return 1;
            }
            config->debug_mode = IRON_TRUE;
            config->debug_components = iron_debug_parse_components(argv[++i]);
        }
        else if (strcmp(argv[i], "--no-color") == 0) {
            config->no_color = IRON_TRUE;
        }
        else if (strcmp(argv[i], "--trace") == 0) {
            config->debug_mode = IRON_TRUE;
            config->trace_mode = IRON_TRUE;
            config->debug_level = IRON_LOG_TRACE;
        }
        else if (strcmp(argv[i], "--disasm") == 0) {
            config->disasm_mode = IRON_TRUE;
        }
        else if (strcmp(argv[i], "--gc-threshold") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: --gc-threshold requires a number\n");
                return 1;
            }
            config->gc_threshold = (iron_size)atol(argv[++i]);
        }
        else if (argv[i][0] == '-') {
            fprintf(stderr, "Error: Unknown option '%s'\n", argv[i]);
            return 1;
        }
        else {
            /* First non-option is the assembly */
            config->assembly_path = argv[i];
            /* Rest are arguments to the assembly */
            config->assembly_args = (const char **)&argv[i + 1];
            config->assembly_argc = argc - i - 1;
            break;
        }
    }
    
    if (!config->assembly_path) {
        fprintf(stderr, "Error: No assembly specified\n");
        print_usage(argv[0]);
        return 1;
    }
    
    return 0;
}

/* ============================================================================
 * Debug Callbacks
 * ============================================================================ */

static void on_method_enter(iron_exec_context_t *ctx, iron_runtime_method_t *method)
{
    (void)ctx;
    if (method && method->name) {
        printf("[TRACE] Enter: %s.%s\n", 
               method->declaring_type ? method->declaring_type->name : "?",
               method->name);
    }
}

static void on_method_exit(iron_exec_context_t *ctx, iron_runtime_method_t *method)
{
    (void)ctx;
    if (method && method->name) {
        printf("[TRACE] Exit: %s.%s\n",
               method->declaring_type ? method->declaring_type->name : "?",
               method->name);
    }
}

static void on_exception(iron_exec_context_t *ctx, iron_exception_t *ex)
{
    (void)ctx;
    if (ex) {
        printf("[EXCEPTION] %s: %p\n",
               ex->type ? ex->type->name : "Exception",
               ex->message);
    }
}

/* ============================================================================
 * Main Entry Point
 * ============================================================================ */

int main(int argc, char **argv)
{
    ironnet_config_t config;
    iron_result_t result;
    iron_domain_t *domain = NULL;
    iron_exec_context_t *ctx = NULL;
    iron_assembly_t *corlib = NULL;
    iron_assembly_t *main_assembly = NULL;
    iron_runtime_method_t *entry_point = NULL;
    iron_domain_config_t domain_config;
    int exit_code = 0;
    int i;
    
    /* Parse command line */
    config_init(&config);
    i = parse_args(argc, argv, &config);
    if (i != 0) {
        config_cleanup(&config);
        return i < 0 ? 0 : i;
    }
    
    /* Initialize debug system */
    iron_debug_init();
    if (config.debug_mode) {
        iron_debug_enable(IRON_TRUE);
        iron_debug_set_level(config.debug_level);
        iron_debug_set_components(config.debug_components);
        if (config.no_color) {
            iron_debug_config_t dbg_cfg;
            iron_debug_get_config(&dbg_cfg);
            dbg_cfg.colorize = IRON_FALSE;
            iron_debug_set_config(&dbg_cfg);
        }
    }
    
    /* Initialize runtime */
    result = iron_init();
    if (!IRON_RESULT_OK(result)) {
        fprintf(stderr, "Error: Failed to initialize runtime: %s\n", result.message);
        config_cleanup(&config);
        return 1;
    }
    
    IRON_INFO_CORE("IronNet initialized");
    IRON_INFO_CORE("Assembly: %s", config.assembly_path);
    IRON_INFO_CORE("Corlib: %s", config.corlib_path);
    IRON_DEBUG_CORE("Debug level: %s", iron_debug_level_name(config.debug_level));
    
    /* Create domain */
    memset(&domain_config, 0, sizeof(domain_config));
    domain_config.name = "IronNet";
    domain_config.base_path = ".";
    
    result = iron_domain_create(&domain, &domain_config, NULL);
    if (!IRON_RESULT_OK(result)) {
        fprintf(stderr, "Error: Failed to create domain: %s\n", result.message);
        goto cleanup;
    }
    
    /* Add search paths */
    for (i = 0; i < config.search_path_count; i++) {
        iron_domain_add_search_path(domain, config.search_paths[i]);
        IRON_DEBUG_CORE("Added search path: %s", config.search_paths[i]);
    }
    
    /* Create execution context */
    result = iron_exec_create(&ctx, domain);
    if (!IRON_RESULT_OK(result)) {
        IRON_ERROR_CORE("Failed to create execution context: %s", result.message);
        goto cleanup;
    }
    
    /* Set up debug hooks if requested */
    if (config.trace_mode) {
        ctx->on_method_enter = on_method_enter;
        ctx->on_method_exit = on_method_exit;
        ctx->on_exception = on_exception;
    }
    
    /* Register internal calls */
    result = iron_register_corlib(ctx);
    if (!IRON_RESULT_OK(result)) {
        IRON_ERROR_CORLIB("Failed to register corlib: %s", result.message);
        goto cleanup;
    }
    IRON_DEBUG_CORLIB("Internal calls registered");
    
    /* Load corlib */
    IRON_INFO_CORE("Loading corlib from: %s", config.corlib_path);
    
    result = iron_domain_load_assembly(domain, config.corlib_path, &corlib);
    if (!IRON_RESULT_OK(result)) {
        IRON_WARN_CORE("Failed to load corlib from '%s': %s", config.corlib_path, result.message);
        IRON_WARN_CORE("Continuing without corlib (internal calls only)");
    } else {
        IRON_INFO_CORE("Corlib loaded successfully");
    }
    
    /* Load main assembly */
    IRON_INFO_CORE("Loading assembly: %s", config.assembly_path);
    
    result = iron_domain_load_assembly(domain, config.assembly_path, &main_assembly);
    if (!IRON_RESULT_OK(result)) {
        IRON_ERROR_CORE("Failed to load assembly '%s': %s", config.assembly_path, result.message);
        exit_code = 1;
        goto cleanup;
    }
    IRON_INFO_CORE("Assembly loaded successfully");
    
    /* Disassemble if requested */
    if (config.disasm_mode) {
        if (corlib) {
            printf("\n*** CORLIB ***\n");
            iron_disasm_assembly(corlib);
        }
        printf("\n*** MAIN ASSEMBLY ***\n");
        iron_disasm_assembly(main_assembly);
    }
    
    /* Find entry point */
    entry_point = iron_assembly_get_entry_point(main_assembly);
    if (!entry_point) {
        IRON_ERROR_CORE("Assembly has no entry point");
        exit_code = 1;
        goto cleanup;
    }
    
    IRON_INFO_EXEC("Entry point: %s.%s",
           entry_point->declaring_type ? entry_point->declaring_type->name : "?",
           entry_point->name ? entry_point->name : "Main");
    
    /* Execute */
    IRON_INFO_EXEC("Starting execution...");
    IRON_INFO_EXEC("========================================");
    
    result = iron_exec_entry_point(ctx, entry_point, 
                                   config.assembly_args, config.assembly_argc,
                                   &exit_code);
    
    IRON_INFO_EXEC("========================================");
    
    if (!IRON_RESULT_OK(result)) {
        IRON_ERROR_EXEC("Execution failed: %s", result.message);
        exit_code = 1;
    } else {
        IRON_INFO_EXEC("Execution completed with exit code: %d", exit_code);
    }
    
cleanup:
    IRON_DEBUG_CORE("Cleaning up...");
    if (ctx) {
        iron_exec_destroy(ctx);
    }
    if (domain) {
        iron_domain_destroy(domain);
    }
    iron_shutdown();
    iron_debug_shutdown();
    config_cleanup(&config);
    
    return exit_code;
}
