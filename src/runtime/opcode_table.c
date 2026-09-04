/*
 * IronNet CLR Interpreter
 * opcode_table.c - Opcode information table and utility functions
 *
 * This file contains:
 * - Complete opcode info tables (single-byte and two-byte)
 * - Opcode decode, name, operand size, branch/prefix query functions
 */

#include "iron/opcodes.h"

/* Macro for single-byte opcodes (size=1) */
#define OP1(op, nm, val, operand, pop, push, flow) \
    { op, nm, (iron_u16)(val), 1, operand, pop, push, flow }

/* Macro for two-byte opcodes (size=2) */
#define OP2(op, nm, val, operand, pop, push, flow) \
    { op, nm, (iron_u16)(val), 2, operand, pop, push, flow }

/* Unused opcode slot */
#define OP_UNUSED(val) \
    { 0, NULL, (iron_u16)(val), 1, IRON_OP_NONE, IRON_POP_0, IRON_PUSH_0, IRON_FLOW_NEXT }

/* Single-byte opcodes (0x00-0xFF) */
static const iron_opcode_info_t g_opcode_table[256] = {
    /* 0x00-0x0F */
    OP1(IRON_CEE_NOP,       "nop",       0x00, IRON_OP_NONE,         IRON_POP_0, IRON_PUSH_0, IRON_FLOW_NEXT),
    OP1(IRON_CEE_BREAK,     "break",     0x01, IRON_OP_NONE,         IRON_POP_0, IRON_PUSH_0, IRON_FLOW_BREAK),
    OP1(IRON_CEE_LDARG_0,   "ldarg.0",   0x02, IRON_OP_NONE,         IRON_POP_0, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_LDARG_1,   "ldarg.1",   0x03, IRON_OP_NONE,         IRON_POP_0, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_LDARG_2,   "ldarg.2",   0x04, IRON_OP_NONE,         IRON_POP_0, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_LDARG_3,   "ldarg.3",   0x05, IRON_OP_NONE,         IRON_POP_0, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_LDLOC_0,   "ldloc.0",   0x06, IRON_OP_NONE,         IRON_POP_0, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_LDLOC_1,   "ldloc.1",   0x07, IRON_OP_NONE,         IRON_POP_0, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_LDLOC_2,   "ldloc.2",   0x08, IRON_OP_NONE,         IRON_POP_0, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_LDLOC_3,   "ldloc.3",   0x09, IRON_OP_NONE,         IRON_POP_0, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_STLOC_0,   "stloc.0",   0x0A, IRON_OP_NONE,         IRON_POP_1, IRON_PUSH_0, IRON_FLOW_NEXT),
    OP1(IRON_CEE_STLOC_1,   "stloc.1",   0x0B, IRON_OP_NONE,         IRON_POP_1, IRON_PUSH_0, IRON_FLOW_NEXT),
    OP1(IRON_CEE_STLOC_2,   "stloc.2",   0x0C, IRON_OP_NONE,         IRON_POP_1, IRON_PUSH_0, IRON_FLOW_NEXT),
    OP1(IRON_CEE_STLOC_3,   "stloc.3",   0x0D, IRON_OP_NONE,         IRON_POP_1, IRON_PUSH_0, IRON_FLOW_NEXT),
    OP1(IRON_CEE_LDARG_S,   "ldarg.s",   0x0E, IRON_OP_SHORT_VAR,    IRON_POP_0, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_LDARGA_S,  "ldarga.s",  0x0F, IRON_OP_SHORT_VAR,    IRON_POP_0, IRON_PUSH_1, IRON_FLOW_NEXT),
    /* 0x10-0x1F */
    OP1(IRON_CEE_STARG_S,   "starg.s",   0x10, IRON_OP_SHORT_VAR,    IRON_POP_1, IRON_PUSH_0, IRON_FLOW_NEXT),
    OP1(IRON_CEE_LDLOC_S,   "ldloc.s",   0x11, IRON_OP_SHORT_VAR,    IRON_POP_0, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_LDLOCA_S,  "ldloca.s",  0x12, IRON_OP_SHORT_VAR,    IRON_POP_0, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_STLOC_S,   "stloc.s",   0x13, IRON_OP_SHORT_VAR,    IRON_POP_1, IRON_PUSH_0, IRON_FLOW_NEXT),
    OP1(IRON_CEE_LDNULL,    "ldnull",    0x14, IRON_OP_NONE,         IRON_POP_0, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_LDC_I4_M1, "ldc.i4.m1", 0x15, IRON_OP_NONE,         IRON_POP_0, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_LDC_I4_0,  "ldc.i4.0",  0x16, IRON_OP_NONE,         IRON_POP_0, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_LDC_I4_1,  "ldc.i4.1",  0x17, IRON_OP_NONE,         IRON_POP_0, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_LDC_I4_2,  "ldc.i4.2",  0x18, IRON_OP_NONE,         IRON_POP_0, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_LDC_I4_3,  "ldc.i4.3",  0x19, IRON_OP_NONE,         IRON_POP_0, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_LDC_I4_4,  "ldc.i4.4",  0x1A, IRON_OP_NONE,         IRON_POP_0, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_LDC_I4_5,  "ldc.i4.5",  0x1B, IRON_OP_NONE,         IRON_POP_0, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_LDC_I4_6,  "ldc.i4.6",  0x1C, IRON_OP_NONE,         IRON_POP_0, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_LDC_I4_7,  "ldc.i4.7",  0x1D, IRON_OP_NONE,         IRON_POP_0, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_LDC_I4_8,  "ldc.i4.8",  0x1E, IRON_OP_NONE,         IRON_POP_0, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_LDC_I4_S,  "ldc.i4.s",  0x1F, IRON_OP_I8,           IRON_POP_0, IRON_PUSH_1, IRON_FLOW_NEXT),
    /* 0x20-0x2F */
    OP1(IRON_CEE_LDC_I4,    "ldc.i4",    0x20, IRON_OP_I32,          IRON_POP_0, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_LDC_I8,    "ldc.i8",    0x21, IRON_OP_I64,          IRON_POP_0, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_LDC_R4,    "ldc.r4",    0x22, IRON_OP_F32,          IRON_POP_0, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_LDC_R8,    "ldc.r8",    0x23, IRON_OP_F64,          IRON_POP_0, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP_UNUSED(0x24),
    OP1(IRON_CEE_DUP,       "dup",       0x25, IRON_OP_NONE,         IRON_POP_1, IRON_PUSH_2, IRON_FLOW_NEXT),
    OP1(IRON_CEE_POP,       "pop",       0x26, IRON_OP_NONE,         IRON_POP_1, IRON_PUSH_0, IRON_FLOW_NEXT),
    OP1(IRON_CEE_JMP,       "jmp",       0x27, IRON_OP_TOKEN,        IRON_POP_0, IRON_PUSH_0, IRON_FLOW_CALL),
    OP1(IRON_CEE_CALL,      "call",      0x28, IRON_OP_TOKEN,        IRON_POP_VAR, IRON_PUSH_VAR, IRON_FLOW_CALL),
    OP1(IRON_CEE_CALLI,     "calli",     0x29, IRON_OP_TOKEN,        IRON_POP_VAR, IRON_PUSH_VAR, IRON_FLOW_CALL),
    OP1(IRON_CEE_RET,       "ret",       0x2A, IRON_OP_NONE,         IRON_POP_VAR, IRON_PUSH_0, IRON_FLOW_RETURN),
    OP1(IRON_CEE_BR_S,      "br.s",      0x2B, IRON_OP_SHORT_BRANCH, IRON_POP_0, IRON_PUSH_0, IRON_FLOW_BRANCH),
    OP1(IRON_CEE_BRFALSE_S, "brfalse.s", 0x2C, IRON_OP_SHORT_BRANCH, IRON_POP_1, IRON_PUSH_0, IRON_FLOW_COND_BRANCH),
    OP1(IRON_CEE_BRTRUE_S,  "brtrue.s",  0x2D, IRON_OP_SHORT_BRANCH, IRON_POP_1, IRON_PUSH_0, IRON_FLOW_COND_BRANCH),
    OP1(IRON_CEE_BEQ_S,     "beq.s",     0x2E, IRON_OP_SHORT_BRANCH, IRON_POP_2, IRON_PUSH_0, IRON_FLOW_COND_BRANCH),
    OP1(IRON_CEE_BGE_S,     "bge.s",     0x2F, IRON_OP_SHORT_BRANCH, IRON_POP_2, IRON_PUSH_0, IRON_FLOW_COND_BRANCH),
    /* 0x30-0x3F */
    OP1(IRON_CEE_BGT_S,     "bgt.s",     0x30, IRON_OP_SHORT_BRANCH, IRON_POP_2, IRON_PUSH_0, IRON_FLOW_COND_BRANCH),
    OP1(IRON_CEE_BLE_S,     "ble.s",     0x31, IRON_OP_SHORT_BRANCH, IRON_POP_2, IRON_PUSH_0, IRON_FLOW_COND_BRANCH),
    OP1(IRON_CEE_BLT_S,     "blt.s",     0x32, IRON_OP_SHORT_BRANCH, IRON_POP_2, IRON_PUSH_0, IRON_FLOW_COND_BRANCH),
    OP1(IRON_CEE_BNE_UN_S,  "bne.un.s",  0x33, IRON_OP_SHORT_BRANCH, IRON_POP_2, IRON_PUSH_0, IRON_FLOW_COND_BRANCH),
    OP1(IRON_CEE_BGE_UN_S,  "bge.un.s",  0x34, IRON_OP_SHORT_BRANCH, IRON_POP_2, IRON_PUSH_0, IRON_FLOW_COND_BRANCH),
    OP1(IRON_CEE_BGT_UN_S,  "bgt.un.s",  0x35, IRON_OP_SHORT_BRANCH, IRON_POP_2, IRON_PUSH_0, IRON_FLOW_COND_BRANCH),
    OP1(IRON_CEE_BLE_UN_S,  "ble.un.s",  0x36, IRON_OP_SHORT_BRANCH, IRON_POP_2, IRON_PUSH_0, IRON_FLOW_COND_BRANCH),
    OP1(IRON_CEE_BLT_UN_S,  "blt.un.s",  0x37, IRON_OP_SHORT_BRANCH, IRON_POP_2, IRON_PUSH_0, IRON_FLOW_COND_BRANCH),
    OP1(IRON_CEE_BR,        "br",        0x38, IRON_OP_BRANCH,       IRON_POP_0, IRON_PUSH_0, IRON_FLOW_BRANCH),
    OP1(IRON_CEE_BRFALSE,   "brfalse",   0x39, IRON_OP_BRANCH,       IRON_POP_1, IRON_PUSH_0, IRON_FLOW_COND_BRANCH),
    OP1(IRON_CEE_BRTRUE,    "brtrue",    0x3A, IRON_OP_BRANCH,       IRON_POP_1, IRON_PUSH_0, IRON_FLOW_COND_BRANCH),
    OP1(IRON_CEE_BEQ,       "beq",       0x3B, IRON_OP_BRANCH,       IRON_POP_2, IRON_PUSH_0, IRON_FLOW_COND_BRANCH),
    OP1(IRON_CEE_BGE,       "bge",       0x3C, IRON_OP_BRANCH,       IRON_POP_2, IRON_PUSH_0, IRON_FLOW_COND_BRANCH),
    OP1(IRON_CEE_BGT,       "bgt",       0x3D, IRON_OP_BRANCH,       IRON_POP_2, IRON_PUSH_0, IRON_FLOW_COND_BRANCH),
    OP1(IRON_CEE_BLE,       "ble",       0x3E, IRON_OP_BRANCH,       IRON_POP_2, IRON_PUSH_0, IRON_FLOW_COND_BRANCH),
    OP1(IRON_CEE_BLT,       "blt",       0x3F, IRON_OP_BRANCH,       IRON_POP_2, IRON_PUSH_0, IRON_FLOW_COND_BRANCH),
    /* 0x40-0x4F */
    OP1(IRON_CEE_BNE_UN,    "bne.un",    0x40, IRON_OP_BRANCH,       IRON_POP_2, IRON_PUSH_0, IRON_FLOW_COND_BRANCH),
    OP1(IRON_CEE_BGE_UN,    "bge.un",    0x41, IRON_OP_BRANCH,       IRON_POP_2, IRON_PUSH_0, IRON_FLOW_COND_BRANCH),
    OP1(IRON_CEE_BGT_UN,    "bgt.un",    0x42, IRON_OP_BRANCH,       IRON_POP_2, IRON_PUSH_0, IRON_FLOW_COND_BRANCH),
    OP1(IRON_CEE_BLE_UN,    "ble.un",    0x43, IRON_OP_BRANCH,       IRON_POP_2, IRON_PUSH_0, IRON_FLOW_COND_BRANCH),
    OP1(IRON_CEE_BLT_UN,    "blt.un",    0x44, IRON_OP_BRANCH,       IRON_POP_2, IRON_PUSH_0, IRON_FLOW_COND_BRANCH),
    OP1(IRON_CEE_SWITCH,    "switch",    0x45, IRON_OP_SWITCH,       IRON_POP_1, IRON_PUSH_0, IRON_FLOW_COND_BRANCH),
    OP1(IRON_CEE_LDIND_I1,  "ldind.i1",  0x46, IRON_OP_NONE,         IRON_POP_1, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_LDIND_U1,  "ldind.u1",  0x47, IRON_OP_NONE,         IRON_POP_1, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_LDIND_I2,  "ldind.i2",  0x48, IRON_OP_NONE,         IRON_POP_1, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_LDIND_U2,  "ldind.u2",  0x49, IRON_OP_NONE,         IRON_POP_1, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_LDIND_I4,  "ldind.i4",  0x4A, IRON_OP_NONE,         IRON_POP_1, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_LDIND_U4,  "ldind.u4",  0x4B, IRON_OP_NONE,         IRON_POP_1, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_LDIND_I8,  "ldind.i8",  0x4C, IRON_OP_NONE,         IRON_POP_1, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_LDIND_I,   "ldind.i",   0x4D, IRON_OP_NONE,         IRON_POP_1, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_LDIND_R4,  "ldind.r4",  0x4E, IRON_OP_NONE,         IRON_POP_1, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_LDIND_R8,  "ldind.r8",  0x4F, IRON_OP_NONE,         IRON_POP_1, IRON_PUSH_1, IRON_FLOW_NEXT),
    /* 0x50-0x5F */
    OP1(IRON_CEE_LDIND_REF, "ldind.ref", 0x50, IRON_OP_NONE,         IRON_POP_1, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_STIND_REF, "stind.ref", 0x51, IRON_OP_NONE,         IRON_POP_2, IRON_PUSH_0, IRON_FLOW_NEXT),
    OP1(IRON_CEE_STIND_I1,  "stind.i1",  0x52, IRON_OP_NONE,         IRON_POP_2, IRON_PUSH_0, IRON_FLOW_NEXT),
    OP1(IRON_CEE_STIND_I2,  "stind.i2",  0x53, IRON_OP_NONE,         IRON_POP_2, IRON_PUSH_0, IRON_FLOW_NEXT),
    OP1(IRON_CEE_STIND_I4,  "stind.i4",  0x54, IRON_OP_NONE,         IRON_POP_2, IRON_PUSH_0, IRON_FLOW_NEXT),
    OP1(IRON_CEE_STIND_I8,  "stind.i8",  0x55, IRON_OP_NONE,         IRON_POP_2, IRON_PUSH_0, IRON_FLOW_NEXT),
    OP1(IRON_CEE_STIND_R4,  "stind.r4",  0x56, IRON_OP_NONE,         IRON_POP_2, IRON_PUSH_0, IRON_FLOW_NEXT),
    OP1(IRON_CEE_STIND_R8,  "stind.r8",  0x57, IRON_OP_NONE,         IRON_POP_2, IRON_PUSH_0, IRON_FLOW_NEXT),
    OP1(IRON_CEE_ADD,       "add",       0x58, IRON_OP_NONE,         IRON_POP_2, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_SUB,       "sub",       0x59, IRON_OP_NONE,         IRON_POP_2, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_MUL,       "mul",       0x5A, IRON_OP_NONE,         IRON_POP_2, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_DIV,       "div",       0x5B, IRON_OP_NONE,         IRON_POP_2, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_DIV_UN,    "div.un",    0x5C, IRON_OP_NONE,         IRON_POP_2, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_REM,       "rem",       0x5D, IRON_OP_NONE,         IRON_POP_2, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_REM_UN,    "rem.un",    0x5E, IRON_OP_NONE,         IRON_POP_2, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_AND,       "and",       0x5F, IRON_OP_NONE,         IRON_POP_2, IRON_PUSH_1, IRON_FLOW_NEXT),
    /* 0x60-0x6F */
    OP1(IRON_CEE_OR,        "or",        0x60, IRON_OP_NONE,         IRON_POP_2, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_XOR,       "xor",       0x61, IRON_OP_NONE,         IRON_POP_2, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_SHL,       "shl",       0x62, IRON_OP_NONE,         IRON_POP_2, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_SHR,       "shr",       0x63, IRON_OP_NONE,         IRON_POP_2, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_SHR_UN,    "shr.un",    0x64, IRON_OP_NONE,         IRON_POP_2, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_NEG,       "neg",       0x65, IRON_OP_NONE,         IRON_POP_1, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_NOT,       "not",       0x66, IRON_OP_NONE,         IRON_POP_1, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_CONV_I1,   "conv.i1",   0x67, IRON_OP_NONE,         IRON_POP_1, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_CONV_I2,   "conv.i2",   0x68, IRON_OP_NONE,         IRON_POP_1, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_CONV_I4,   "conv.i4",   0x69, IRON_OP_NONE,         IRON_POP_1, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_CONV_I8,   "conv.i8",   0x6A, IRON_OP_NONE,         IRON_POP_1, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_CONV_R4,   "conv.r4",   0x6B, IRON_OP_NONE,         IRON_POP_1, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_CONV_R8,   "conv.r8",   0x6C, IRON_OP_NONE,         IRON_POP_1, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_CONV_U4,   "conv.u4",   0x6D, IRON_OP_NONE,         IRON_POP_1, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_CONV_U8,   "conv.u8",   0x6E, IRON_OP_NONE,         IRON_POP_1, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_CALLVIRT,  "callvirt",  0x6F, IRON_OP_TOKEN,        IRON_POP_VAR, IRON_PUSH_VAR, IRON_FLOW_CALL),
    /* 0x70-0x7F */
    OP1(IRON_CEE_CPOBJ,     "cpobj",     0x70, IRON_OP_TOKEN,        IRON_POP_2, IRON_PUSH_0, IRON_FLOW_NEXT),
    OP1(IRON_CEE_LDOBJ,     "ldobj",     0x71, IRON_OP_TOKEN,        IRON_POP_1, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_LDSTR,     "ldstr",     0x72, IRON_OP_STRING,       IRON_POP_0, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_NEWOBJ,    "newobj",    0x73, IRON_OP_TOKEN,        IRON_POP_VAR, IRON_PUSH_1, IRON_FLOW_CALL),
    OP1(IRON_CEE_CASTCLASS, "castclass", 0x74, IRON_OP_TOKEN,        IRON_POP_1, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_ISINST,    "isinst",    0x75, IRON_OP_TOKEN,        IRON_POP_1, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_CONV_R_UN, "conv.r.un", 0x76, IRON_OP_NONE,         IRON_POP_1, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP_UNUSED(0x77), OP_UNUSED(0x78),
    OP1(IRON_CEE_UNBOX,     "unbox",     0x79, IRON_OP_TOKEN,        IRON_POP_1, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_THROW,     "throw",     0x7A, IRON_OP_NONE,         IRON_POP_1, IRON_PUSH_0, IRON_FLOW_THROW),
    OP1(IRON_CEE_LDFLD,     "ldfld",     0x7B, IRON_OP_TOKEN,        IRON_POP_1, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_LDFLDA,    "ldflda",    0x7C, IRON_OP_TOKEN,        IRON_POP_1, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_STFLD,     "stfld",     0x7D, IRON_OP_TOKEN,        IRON_POP_2, IRON_PUSH_0, IRON_FLOW_NEXT),
    OP1(IRON_CEE_LDSFLD,    "ldsfld",    0x7E, IRON_OP_TOKEN,        IRON_POP_0, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_LDSFLDA,   "ldsflda",   0x7F, IRON_OP_TOKEN,        IRON_POP_0, IRON_PUSH_1, IRON_FLOW_NEXT),
    /* 0x80-0x8F */
    OP1(IRON_CEE_STSFLD,    "stsfld",    0x80, IRON_OP_TOKEN,        IRON_POP_1, IRON_PUSH_0, IRON_FLOW_NEXT),
    OP1(IRON_CEE_STOBJ,     "stobj",     0x81, IRON_OP_TOKEN,        IRON_POP_2, IRON_PUSH_0, IRON_FLOW_NEXT),
    OP1(IRON_CEE_CONV_OVF_I1_UN, "conv.ovf.i1.un", 0x82, IRON_OP_NONE, IRON_POP_1, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_CONV_OVF_I2_UN, "conv.ovf.i2.un", 0x83, IRON_OP_NONE, IRON_POP_1, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_CONV_OVF_I4_UN, "conv.ovf.i4.un", 0x84, IRON_OP_NONE, IRON_POP_1, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_CONV_OVF_I8_UN, "conv.ovf.i8.un", 0x85, IRON_OP_NONE, IRON_POP_1, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_CONV_OVF_U1_UN, "conv.ovf.u1.un", 0x86, IRON_OP_NONE, IRON_POP_1, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_CONV_OVF_U2_UN, "conv.ovf.u2.un", 0x87, IRON_OP_NONE, IRON_POP_1, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_CONV_OVF_U4_UN, "conv.ovf.u4.un", 0x88, IRON_OP_NONE, IRON_POP_1, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_CONV_OVF_U8_UN, "conv.ovf.u8.un", 0x89, IRON_OP_NONE, IRON_POP_1, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_CONV_OVF_I_UN,  "conv.ovf.i.un",  0x8A, IRON_OP_NONE, IRON_POP_1, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_CONV_OVF_U_UN,  "conv.ovf.u.un",  0x8B, IRON_OP_NONE, IRON_POP_1, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_BOX,       "box",       0x8C, IRON_OP_TOKEN,        IRON_POP_1, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_NEWARR,    "newarr",    0x8D, IRON_OP_TOKEN,        IRON_POP_1, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_LDLEN,     "ldlen",     0x8E, IRON_OP_NONE,         IRON_POP_1, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_LDELEMA,   "ldelema",   0x8F, IRON_OP_TOKEN,        IRON_POP_2, IRON_PUSH_1, IRON_FLOW_NEXT),
    /* 0x90-0x9F */
    OP1(IRON_CEE_LDELEM_I1, "ldelem.i1", 0x90, IRON_OP_NONE,         IRON_POP_2, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_LDELEM_U1, "ldelem.u1", 0x91, IRON_OP_NONE,         IRON_POP_2, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_LDELEM_I2, "ldelem.i2", 0x92, IRON_OP_NONE,         IRON_POP_2, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_LDELEM_U2, "ldelem.u2", 0x93, IRON_OP_NONE,         IRON_POP_2, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_LDELEM_I4, "ldelem.i4", 0x94, IRON_OP_NONE,         IRON_POP_2, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_LDELEM_U4, "ldelem.u4", 0x95, IRON_OP_NONE,         IRON_POP_2, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_LDELEM_I8, "ldelem.i8", 0x96, IRON_OP_NONE,         IRON_POP_2, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_LDELEM_I,  "ldelem.i",  0x97, IRON_OP_NONE,         IRON_POP_2, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_LDELEM_R4, "ldelem.r4", 0x98, IRON_OP_NONE,         IRON_POP_2, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_LDELEM_R8, "ldelem.r8", 0x99, IRON_OP_NONE,         IRON_POP_2, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_LDELEM_REF,"ldelem.ref",0x9A, IRON_OP_NONE,         IRON_POP_2, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_STELEM_I,  "stelem.i",  0x9B, IRON_OP_NONE,         IRON_POP_3, IRON_PUSH_0, IRON_FLOW_NEXT),
    OP1(IRON_CEE_STELEM_I1, "stelem.i1", 0x9C, IRON_OP_NONE,         IRON_POP_3, IRON_PUSH_0, IRON_FLOW_NEXT),
    OP1(IRON_CEE_STELEM_I2, "stelem.i2", 0x9D, IRON_OP_NONE,         IRON_POP_3, IRON_PUSH_0, IRON_FLOW_NEXT),
    OP1(IRON_CEE_STELEM_I4, "stelem.i4", 0x9E, IRON_OP_NONE,         IRON_POP_3, IRON_PUSH_0, IRON_FLOW_NEXT),
    OP1(IRON_CEE_STELEM_I8, "stelem.i8", 0x9F, IRON_OP_NONE,         IRON_POP_3, IRON_PUSH_0, IRON_FLOW_NEXT),
    /* 0xA0-0xAF */
    OP1(IRON_CEE_STELEM_R4, "stelem.r4", 0xA0, IRON_OP_NONE,         IRON_POP_3, IRON_PUSH_0, IRON_FLOW_NEXT),
    OP1(IRON_CEE_STELEM_R8, "stelem.r8", 0xA1, IRON_OP_NONE,         IRON_POP_3, IRON_PUSH_0, IRON_FLOW_NEXT),
    OP1(IRON_CEE_STELEM_REF,"stelem.ref",0xA2, IRON_OP_NONE,         IRON_POP_3, IRON_PUSH_0, IRON_FLOW_NEXT),
    OP1(IRON_CEE_LDELEM,    "ldelem",    0xA3, IRON_OP_TOKEN,        IRON_POP_2, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_STELEM,    "stelem",    0xA4, IRON_OP_TOKEN,        IRON_POP_3, IRON_PUSH_0, IRON_FLOW_NEXT),
    OP1(IRON_CEE_UNBOX_ANY, "unbox.any", 0xA5, IRON_OP_TOKEN,        IRON_POP_1, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP_UNUSED(0xA6), OP_UNUSED(0xA7), OP_UNUSED(0xA8), OP_UNUSED(0xA9),
    OP_UNUSED(0xAA), OP_UNUSED(0xAB), OP_UNUSED(0xAC), OP_UNUSED(0xAD),
    OP_UNUSED(0xAE), OP_UNUSED(0xAF),
    /* 0xB0-0xBF */
    OP_UNUSED(0xB0), OP_UNUSED(0xB1), OP_UNUSED(0xB2),
    OP1(IRON_CEE_CONV_OVF_I1, "conv.ovf.i1", 0xB3, IRON_OP_NONE, IRON_POP_1, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_CONV_OVF_U1, "conv.ovf.u1", 0xB4, IRON_OP_NONE, IRON_POP_1, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_CONV_OVF_I2, "conv.ovf.i2", 0xB5, IRON_OP_NONE, IRON_POP_1, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_CONV_OVF_U2, "conv.ovf.u2", 0xB6, IRON_OP_NONE, IRON_POP_1, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_CONV_OVF_I4, "conv.ovf.i4", 0xB7, IRON_OP_NONE, IRON_POP_1, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_CONV_OVF_U4, "conv.ovf.u4", 0xB8, IRON_OP_NONE, IRON_POP_1, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_CONV_OVF_I8, "conv.ovf.i8", 0xB9, IRON_OP_NONE, IRON_POP_1, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_CONV_OVF_U8, "conv.ovf.u8", 0xBA, IRON_OP_NONE, IRON_POP_1, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP_UNUSED(0xBB),
    OP_UNUSED(0xBC), OP_UNUSED(0xBD), OP_UNUSED(0xBE), OP_UNUSED(0xBF),
    /* 0xC0-0xCF */
    OP_UNUSED(0xC0), OP_UNUSED(0xC1),
    OP1(IRON_CEE_REFANYVAL, "refanyval", 0xC2, IRON_OP_TOKEN, IRON_POP_1, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_CKFINITE,  "ckfinite",  0xC3, IRON_OP_NONE,  IRON_POP_1, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP_UNUSED(0xC4), OP_UNUSED(0xC5),
    OP1(IRON_CEE_MKREFANY,  "mkrefany",  0xC6, IRON_OP_TOKEN, IRON_POP_1, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP_UNUSED(0xC7),
    OP_UNUSED(0xC8), OP_UNUSED(0xC9), OP_UNUSED(0xCA), OP_UNUSED(0xCB),
    OP_UNUSED(0xCC), OP_UNUSED(0xCD), OP_UNUSED(0xCE), OP_UNUSED(0xCF),
    /* 0xD0-0xDF */
    OP1(IRON_CEE_LDTOKEN,   "ldtoken",   0xD0, IRON_OP_TOKEN,        IRON_POP_0, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_CONV_U2,   "conv.u2",   0xD1, IRON_OP_NONE,         IRON_POP_1, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_CONV_U1,   "conv.u1",   0xD2, IRON_OP_NONE,         IRON_POP_1, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_CONV_I,    "conv.i",    0xD3, IRON_OP_NONE,         IRON_POP_1, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_CONV_OVF_I, "conv.ovf.i", 0xD4, IRON_OP_NONE, IRON_POP_1, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_CONV_OVF_U, "conv.ovf.u", 0xD5, IRON_OP_NONE, IRON_POP_1, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_ADD_OVF,    "add.ovf",    0xD6, IRON_OP_NONE, IRON_POP_2, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_ADD_OVF_UN, "add.ovf.un", 0xD7, IRON_OP_NONE, IRON_POP_2, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_MUL_OVF,    "mul.ovf",    0xD8, IRON_OP_NONE, IRON_POP_2, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_MUL_OVF_UN, "mul.ovf.un", 0xD9, IRON_OP_NONE, IRON_POP_2, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_SUB_OVF,    "sub.ovf",    0xDA, IRON_OP_NONE, IRON_POP_2, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_SUB_OVF_UN, "sub.ovf.un", 0xDB, IRON_OP_NONE, IRON_POP_2, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP1(IRON_CEE_ENDFINALLY,"endfinally",0xDC, IRON_OP_NONE,         IRON_POP_0, IRON_PUSH_0, IRON_FLOW_RETURN),
    OP1(IRON_CEE_LEAVE,     "leave",     0xDD, IRON_OP_BRANCH,       IRON_POP_0, IRON_PUSH_0, IRON_FLOW_BRANCH),
    OP1(IRON_CEE_LEAVE_S,   "leave.s",   0xDE, IRON_OP_SHORT_BRANCH, IRON_POP_0, IRON_PUSH_0, IRON_FLOW_BRANCH),
    OP1(IRON_CEE_STIND_I,   "stind.i",   0xDF, IRON_OP_NONE,         IRON_POP_2, IRON_PUSH_0, IRON_FLOW_NEXT),
    /* 0xE0-0xEF */
    OP1(IRON_CEE_CONV_U,    "conv.u",    0xE0, IRON_OP_NONE,         IRON_POP_1, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP_UNUSED(0xE1), OP_UNUSED(0xE2), OP_UNUSED(0xE3), OP_UNUSED(0xE4),
    OP_UNUSED(0xE5), OP_UNUSED(0xE6), OP_UNUSED(0xE7), OP_UNUSED(0xE8),
    OP_UNUSED(0xE9), OP_UNUSED(0xEA), OP_UNUSED(0xEB), OP_UNUSED(0xEC),
    OP_UNUSED(0xED), OP_UNUSED(0xEE), OP_UNUSED(0xEF),
    /* 0xF0-0xFF */
    OP_UNUSED(0xF0), OP_UNUSED(0xF1), OP_UNUSED(0xF2), OP_UNUSED(0xF3),
    OP_UNUSED(0xF4), OP_UNUSED(0xF5), OP_UNUSED(0xF6), OP_UNUSED(0xF7),
    OP_UNUSED(0xF8), OP_UNUSED(0xF9), OP_UNUSED(0xFA), OP_UNUSED(0xFB),
    OP_UNUSED(0xFC), OP_UNUSED(0xFD),
    OP1(0, "prefix", 0xFE, IRON_OP_NONE, IRON_POP_0, IRON_PUSH_0, IRON_FLOW_NEXT),
    OP_UNUSED(0xFF),
};

/* Two-byte opcodes (0xFE xx) - use OP2 for size=2 */
#define OP2_UNUSED(val) \
    { 0, NULL, (iron_u16)(0xFE00 | (val)), 2, IRON_OP_NONE, IRON_POP_0, IRON_PUSH_0, IRON_FLOW_NEXT }

static const iron_opcode_info_t g_opcode_table_fe[32] = {
    OP2(IRON_CEE_ARGLIST,   "arglist",   0xFE00, IRON_OP_NONE,  IRON_POP_0, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP2(IRON_CEE_CEQ,       "ceq",       0xFE01, IRON_OP_NONE,  IRON_POP_2, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP2(IRON_CEE_CGT,       "cgt",       0xFE02, IRON_OP_NONE,  IRON_POP_2, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP2(IRON_CEE_CGT_UN,    "cgt.un",    0xFE03, IRON_OP_NONE,  IRON_POP_2, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP2(IRON_CEE_CLT,       "clt",       0xFE04, IRON_OP_NONE,  IRON_POP_2, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP2(IRON_CEE_CLT_UN,    "clt.un",    0xFE05, IRON_OP_NONE,  IRON_POP_2, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP2(IRON_CEE_LDFTN,     "ldftn",     0xFE06, IRON_OP_TOKEN, IRON_POP_0, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP2(IRON_CEE_LDVIRTFTN, "ldvirtftn", 0xFE07, IRON_OP_TOKEN, IRON_POP_1, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP2_UNUSED(0x08),
    OP2(IRON_CEE_LDARG,     "ldarg",     0xFE09, IRON_OP_VAR,   IRON_POP_0, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP2(IRON_CEE_LDARGA,    "ldarga",    0xFE0A, IRON_OP_VAR,   IRON_POP_0, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP2(IRON_CEE_STARG,     "starg",     0xFE0B, IRON_OP_VAR,   IRON_POP_1, IRON_PUSH_0, IRON_FLOW_NEXT),
    OP2(IRON_CEE_LDLOC,     "ldloc",     0xFE0C, IRON_OP_VAR,   IRON_POP_0, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP2(IRON_CEE_LDLOCA,    "ldloca",    0xFE0D, IRON_OP_VAR,   IRON_POP_0, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP2(IRON_CEE_STLOC,     "stloc",     0xFE0E, IRON_OP_VAR,   IRON_POP_1, IRON_PUSH_0, IRON_FLOW_NEXT),
    OP2(IRON_CEE_LOCALLOC,  "localloc",  0xFE0F, IRON_OP_NONE,  IRON_POP_1, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP2_UNUSED(0x10),
    OP2(IRON_CEE_ENDFILTER, "endfilter", 0xFE11, IRON_OP_NONE,  IRON_POP_1, IRON_PUSH_0, IRON_FLOW_RETURN),
    OP2(IRON_CEE_UNALIGNED, "unaligned", 0xFE12, IRON_OP_U8,    IRON_POP_0, IRON_PUSH_0, IRON_FLOW_NEXT),
    OP2(IRON_CEE_VOLATILE,  "volatile",  0xFE13, IRON_OP_NONE,  IRON_POP_0, IRON_PUSH_0, IRON_FLOW_NEXT),
    OP2(IRON_CEE_TAIL,      "tail",      0xFE14, IRON_OP_NONE,  IRON_POP_0, IRON_PUSH_0, IRON_FLOW_NEXT),
    OP2(IRON_CEE_INITOBJ,   "initobj",   0xFE15, IRON_OP_TOKEN, IRON_POP_1, IRON_PUSH_0, IRON_FLOW_NEXT),
    OP2(IRON_CEE_CONSTRAINED,"constrained",0xFE16,IRON_OP_TOKEN,IRON_POP_0, IRON_PUSH_0, IRON_FLOW_NEXT),
    OP2(IRON_CEE_CPBLK,     "cpblk",     0xFE17, IRON_OP_NONE,  IRON_POP_3, IRON_PUSH_0, IRON_FLOW_NEXT),
    OP2(IRON_CEE_INITBLK,   "initblk",   0xFE18, IRON_OP_NONE,  IRON_POP_3, IRON_PUSH_0, IRON_FLOW_NEXT),
    OP2(IRON_CEE_NO,        "no",        0xFE19, IRON_OP_U8,    IRON_POP_0, IRON_PUSH_0, IRON_FLOW_NEXT),
    OP2(IRON_CEE_RETHROW,   "rethrow",   0xFE1A, IRON_OP_NONE,  IRON_POP_0, IRON_PUSH_0, IRON_FLOW_THROW),
    OP2_UNUSED(0x1B),
    OP2(IRON_CEE_SIZEOF,    "sizeof",    0xFE1C, IRON_OP_TOKEN, IRON_POP_0, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP2(IRON_CEE_REFANYTYPE,"refanytype",0xFE1D, IRON_OP_NONE,  IRON_POP_1, IRON_PUSH_1, IRON_FLOW_NEXT),
    OP2(IRON_CEE_READONLY,  "readonly",  0xFE1E, IRON_OP_NONE,  IRON_POP_0, IRON_PUSH_0, IRON_FLOW_NEXT),
    OP2_UNUSED(0x1F),
};

const iron_opcode_info_t *iron_opcode_info(iron_opcode_t opcode)
{
    iron_u32 idx = (iron_u32)opcode;

    if (idx < 256) {
        if (g_opcode_table[idx].name != NULL) {
            return &g_opcode_table[idx];
        }
    } else if (idx >= 0x100 && idx < 0x120) {
        idx -= 0x100;
        if (g_opcode_table_fe[idx].name != NULL) {
            return &g_opcode_table_fe[idx];
        }
    }
    return NULL;
}

/* ============================================================================
 * Opcode Utility Functions (consolidated from opcodes.c)
 * ============================================================================ */

iron_opcode_t iron_opcode_decode(const iron_u8 *code, iron_u32 *size)
{
    if (code[0] == 0xFE) {
        *size = 2;
        return (iron_opcode_t)(0x100 + code[1]);
    }
    *size = 1;
    return (iron_opcode_t)code[0];
}

const char *iron_opcode_name(iron_opcode_t opcode)
{
    const iron_opcode_info_t *info = iron_opcode_info(opcode);
    if (info && info->name) {
        return info->name;
    }
    return "unknown";
}

iron_u32 iron_operand_size(iron_operand_type_t type)
{
    switch (type) {
        case IRON_OP_NONE:         return 0;
        case IRON_OP_I8:           return 1;
        case IRON_OP_U8:           return 1;
        case IRON_OP_I16:          return 2;
        case IRON_OP_I32:          return 4;
        case IRON_OP_I64:          return 8;
        case IRON_OP_F32:          return 4;
        case IRON_OP_F64:          return 8;
        case IRON_OP_TOKEN:        return 4;
        case IRON_OP_STRING:       return 4;
        case IRON_OP_BRANCH:       return 4;
        case IRON_OP_SHORT_BRANCH: return 1;
        case IRON_OP_SWITCH:       return 0; /* Variable */
        case IRON_OP_VAR:          return 2;
        case IRON_OP_SHORT_VAR:    return 1;
        case IRON_OP_ARG:          return 2;
        case IRON_OP_SHORT_ARG:    return 1;
        default:                   return 0;
    }
}

iron_bool iron_opcode_is_branch(iron_opcode_t opcode)
{
    const iron_opcode_info_t *info = iron_opcode_info(opcode);
    if (info) {
        return (info->flow == IRON_FLOW_BRANCH ||
                info->flow == IRON_FLOW_COND_BRANCH) ? IRON_TRUE : IRON_FALSE;
    }
    return IRON_FALSE;
}

iron_bool iron_opcode_is_prefix(iron_opcode_t opcode)
{
    switch (opcode) {
        case IRON_CEE_UNALIGNED:
        case IRON_CEE_VOLATILE:
        case IRON_CEE_TAIL:
        case IRON_CEE_CONSTRAINED:
        case IRON_CEE_NO:
        case IRON_CEE_READONLY:
            return IRON_TRUE;
        default:
            return IRON_FALSE;
    }
}
