/*
 * IronNet CLR Interpreter
 * disasm.h - Disassembly utilities
 */

#ifndef IRON_DISASM_H
#define IRON_DISASM_H

#include "iron/types.h"
#include "iron/runtime.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Disassemble an entire assembly (metadata tables + IL code) */
IRON_API void iron_disasm_assembly(iron_assembly_t *assembly);

/* Disassemble IL code */
IRON_API void iron_disasm_il_code(iron_assembly_t *assembly, const iron_u8 *code, iron_u32 code_size);

/* Print a MemberRef row */
IRON_API void iron_disasm_memberref(iron_assembly_t *assembly, iron_u32 row_index);

#ifdef __cplusplus
}
#endif

#endif /* IRON_DISASM_H */
