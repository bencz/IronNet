/*
 * IronNet CLR Interpreter
 * iron.c - Main entry point and high-level API
 */

#include "iron/iron.h"
#include "iron/corlib.h"
#include <stdio.h>
#include <string.h>

/* ============================================================================
 * Global State
 * ============================================================================ */

static iron_bool g_initialized = IRON_FALSE;

/* ============================================================================
 * Initialization
 * ============================================================================ */

iron_result_t iron_init(void)
{
    if (g_initialized) {
        return IRON_SUCCESS;
    }
    
    /* Platform-specific initialization could go here */
    
    g_initialized = IRON_TRUE;
    return IRON_SUCCESS;
}

void iron_shutdown(void)
{
    if (!g_initialized) return;
    
    /* Cleanup global resources */
    
    g_initialized = IRON_FALSE;
}

const char *iron_version(void)
{
    return IRON_VERSION_STRING;
}

/* ============================================================================
 * High-Level Execution API
 * ============================================================================ */

iron_result_t iron_run_assembly(const char *path, int argc, const char **argv,
                                int *exit_code)
{
    iron_result_t result;
    iron_domain_t *domain = NULL;
    iron_assembly_t *assembly = NULL;
    iron_exec_context_t *ctx = NULL;
    iron_runtime_method_t *entry_point;
    iron_domain_config_t config;
    
    if (!path || !exit_code) {
        return IRON_ERROR(IRON_ERR_NULL_POINTER, "Invalid parameters");
    }
    
    *exit_code = 0;
    
    /* Initialize runtime if needed */
    result = iron_init();
    if (!IRON_RESULT_OK(result)) {
        return result;
    }
    
    /* Create domain */
    memset(&config, 0, sizeof(config));
    config.name = "DefaultDomain";
    
    result = iron_domain_create(&domain, &config, NULL);
    if (!IRON_RESULT_OK(result)) {
        return result;
    }
    
    /* Load assembly */
    result = iron_domain_load_assembly(domain, path, &assembly);
    if (!IRON_RESULT_OK(result)) {
        iron_domain_destroy(domain);
        return result;
    }
    
    /* Get entry point */
    entry_point = iron_assembly_get_entry_point(assembly);
    if (!entry_point) {
        iron_domain_destroy(domain);
        return IRON_ERROR(IRON_ERR_METHOD_NOT_FOUND, "No entry point found");
    }
    
    /* Create execution context */
    result = iron_exec_create(&ctx, domain);
    if (!IRON_RESULT_OK(result)) {
        iron_domain_destroy(domain);
        return result;
    }
    
    /* Register corlib internal calls */
    result = iron_register_corlib(ctx);
    if (!IRON_RESULT_OK(result)) {
        iron_exec_destroy(ctx);
        iron_domain_destroy(domain);
        return result;
    }
    
    result = iron_exec_entry_point(ctx, entry_point, argv, argc, exit_code);
    
    /* Cleanup */
    iron_exec_destroy(ctx);
    iron_domain_destroy(domain);
    
    return result;
}

iron_result_t iron_run_assembly_memory(const iron_u8 *data, iron_size size,
                                       int argc, const char **argv,
                                       int *exit_code)
{
    iron_result_t result;
    iron_domain_t *domain = NULL;
    iron_assembly_t *assembly = NULL;
    iron_exec_context_t *ctx = NULL;
    iron_runtime_method_t *entry_point;
    iron_domain_config_t config;
    
    if (!data || size == 0 || !exit_code) {
        return IRON_ERROR(IRON_ERR_NULL_POINTER, "Invalid parameters");
    }
    
    *exit_code = 0;
    
    /* Initialize runtime if needed */
    result = iron_init();
    if (!IRON_RESULT_OK(result)) {
        return result;
    }
    
    /* Create domain */
    memset(&config, 0, sizeof(config));
    config.name = "DefaultDomain";
    
    result = iron_domain_create(&domain, &config, NULL);
    if (!IRON_RESULT_OK(result)) {
        return result;
    }
    
    /* Load assembly from memory */
    result = iron_domain_load_assembly_memory(domain, data, size, &assembly);
    if (!IRON_RESULT_OK(result)) {
        iron_domain_destroy(domain);
        return result;
    }
    
    /* Get entry point */
    entry_point = iron_assembly_get_entry_point(assembly);
    if (!entry_point) {
        iron_domain_destroy(domain);
        return IRON_ERROR(IRON_ERR_METHOD_NOT_FOUND, "No entry point found");
    }
    
    /* Create execution context */
    result = iron_exec_create(&ctx, domain);
    if (!IRON_RESULT_OK(result)) {
        iron_domain_destroy(domain);
        return result;
    }
    
    /* Register corlib internal calls */
    result = iron_register_corlib(ctx);
    if (!IRON_RESULT_OK(result)) {
        iron_exec_destroy(ctx);
        iron_domain_destroy(domain);
        return result;
    }
    
    result = iron_exec_entry_point(ctx, entry_point, argv, argc, exit_code);
    
    /* Cleanup */
    iron_exec_destroy(ctx);
    iron_domain_destroy(domain);
    
    return result;
}

/* iron_register_corlib is now implemented in src/corlib/corlib.c */
