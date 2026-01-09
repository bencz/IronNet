/*
 * IronNet CLR Interpreter
 * main.c - Command-line interface
 * 
 * Usage: ironnet [options] <assembly.exe> [args...]
 */

#include "iron/iron.h"
#include "iron/corlib.h"
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
    printf("  -h, --help          Show this help message\n");
    printf("  -v, --version       Show version information\n");
    printf("  -L <path>           Add assembly search path\n");
    printf("  --corlib <path>     Path to corlib.dll (default: ./corlib/corlib.dll)\n");
    printf("  --debug             Enable debug output\n");
    printf("  --trace             Trace IL execution\n");
    printf("  --gc-threshold <n>  Set GC threshold in bytes\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s HelloWorld.exe\n", program);
    printf("  %s -L ./libs MyApp.exe arg1 arg2\n", program);
    printf("  %s --corlib /path/to/corlib.dll Program.exe\n", program);
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
    iron_size gc_threshold;
} ironnet_config_t;

static void config_init(ironnet_config_t *config)
{
    memset(config, 0, sizeof(*config));
    config->corlib_path = "corlib/corlib.dll";
    config->gc_threshold = 1024 * 1024; /* 1MB default */
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
        else if (strcmp(argv[i], "--trace") == 0) {
            config->trace_mode = IRON_TRUE;
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
        printf("[EXCEPTION] %s: %s\n",
               ex->type ? ex->type->name : "Exception",
               ex->message ? ex->message : "(no message)");
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
    
    /* Initialize runtime */
    result = iron_init();
    if (!IRON_RESULT_OK(result)) {
        fprintf(stderr, "Error: Failed to initialize runtime: %s\n", result.message);
        config_cleanup(&config);
        return 1;
    }
    
    if (config.debug_mode) {
        printf("[DEBUG] IronNet initialized\n");
        printf("[DEBUG] Assembly: %s\n", config.assembly_path);
        printf("[DEBUG] Corlib: %s\n", config.corlib_path);
    }
    
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
        if (config.debug_mode) {
            printf("[DEBUG] Added search path: %s\n", config.search_paths[i]);
        }
    }
    
    /* Create execution context */
    result = iron_exec_create(&ctx, domain);
    if (!IRON_RESULT_OK(result)) {
        fprintf(stderr, "Error: Failed to create execution context: %s\n", result.message);
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
        fprintf(stderr, "Error: Failed to register corlib: %s\n", result.message);
        goto cleanup;
    }
    
    /* Load corlib */
    if (config.debug_mode) {
        printf("[DEBUG] Loading corlib from: %s\n", config.corlib_path);
    }
    
    result = iron_domain_load_assembly(domain, config.corlib_path, &corlib);
    if (!IRON_RESULT_OK(result)) {
        fprintf(stderr, "Warning: Failed to load corlib from '%s': %s\n", 
                config.corlib_path, result.message);
        fprintf(stderr, "         Continuing without corlib (internal calls only)\n");
        /* Continue anyway - we have internal call implementations */
    } else {
        if (config.debug_mode) {
            printf("[DEBUG] Corlib loaded successfully\n");
        }
    }
    
    /* Load main assembly */
    if (config.debug_mode) {
        printf("[DEBUG] Loading assembly: %s\n", config.assembly_path);
    }
    
    result = iron_domain_load_assembly(domain, config.assembly_path, &main_assembly);
    if (!IRON_RESULT_OK(result)) {
        fprintf(stderr, "Error: Failed to load assembly '%s': %s\n", 
                config.assembly_path, result.message);
        exit_code = 1;
        goto cleanup;
    }
    
    if (config.debug_mode) {
        printf("[DEBUG] Assembly loaded successfully\n");
    }
    
    /* Find entry point */
    entry_point = iron_assembly_get_entry_point(main_assembly);
    if (!entry_point) {
        fprintf(stderr, "Error: Assembly has no entry point\n");
        exit_code = 1;
        goto cleanup;
    }
    
    if (config.debug_mode) {
        printf("[DEBUG] Entry point: %s.%s\n",
               entry_point->declaring_type ? entry_point->declaring_type->name : "?",
               entry_point->name ? entry_point->name : "Main");
    }
    
    /* Execute */
    if (config.debug_mode) {
        printf("[DEBUG] Starting execution...\n");
        printf("========================================\n");
    }
    
    result = iron_exec_entry_point(ctx, entry_point, 
                                   config.assembly_args, config.assembly_argc,
                                   &exit_code);
    
    if (config.debug_mode) {
        printf("========================================\n");
    }
    
    if (!IRON_RESULT_OK(result)) {
        fprintf(stderr, "Error: Execution failed: %s\n", result.message);
        exit_code = 1;
    } else if (config.debug_mode) {
        printf("[DEBUG] Execution completed with exit code: %d\n", exit_code);
    }
    
cleanup:
    if (ctx) {
        iron_exec_destroy(ctx);
    }
    if (domain) {
        iron_domain_destroy(domain);
    }
    iron_shutdown();
    config_cleanup(&config);
    
    return exit_code;
}
