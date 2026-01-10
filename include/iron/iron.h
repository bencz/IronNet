/*
 * IronNet CLR Interpreter
 * iron.h - Main header file (includes all components)
 * 
 * Pure C89 compatible CLR interpreter supporting:
 * - Multi-platform (Windows, Linux, macOS, BSD, bare-metal)
 * - Multi-architecture (x86, x64, ARM, ARM64, MIPS, PPC, RISC-V)
 * - Multi-endianness (little-endian, big-endian)
 * - Word sizes: 16, 24, 31, 32, 64 bits
 * - Full CLI metadata and IL interpretation
 * - Generics support
 * - Multi-threading with Tasks
 * - Custom corlib with InternalCall support
 */

#ifndef IRON_H
#define IRON_H

/* Core headers */
#include "platform.h"
#include "types.h"
#include "memory.h"
#include "vtable.h"

/* Threading */
#include "thread.h"

/* PE/COFF and Metadata */
#include "pe.h"
#include "metadata.h"
#include "opcodes.h"

/* Runtime */
#include "runtime.h"
#include "exec.h"
#include "gc.h"
#include "debug.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Version Information
 * ============================================================================ */

#define IRON_VERSION_MAJOR 0
#define IRON_VERSION_MINOR 1
#define IRON_VERSION_PATCH 0
#define IRON_VERSION_STRING "0.1.0"

/* ============================================================================
 * Initialization and Shutdown
 * ============================================================================ */

/* Initialize the IronNet runtime */
IRON_API iron_result_t iron_init(void);

/* Shutdown the IronNet runtime */
IRON_API void iron_shutdown(void);

/* Get version string */
IRON_API const char *iron_version(void);

/* ============================================================================
 * High-Level API
 * ============================================================================ */

/* Execute an assembly's entry point */
IRON_API iron_result_t iron_run_assembly(const char *path, 
                                          int argc, 
                                          const char **argv,
                                          int *exit_code);

/* Execute from memory */
IRON_API iron_result_t iron_run_assembly_memory(const iron_u8 *data,
                                                 iron_size size,
                                                 int argc,
                                                 const char **argv,
                                                 int *exit_code);

/* ============================================================================
 * Corlib Registration
 * ============================================================================ */

/* Register the built-in corlib internal calls */
IRON_API iron_result_t iron_register_corlib(iron_exec_context_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* IRON_H */
