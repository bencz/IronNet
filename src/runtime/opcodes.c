/*
 * IronNet CLR Interpreter
 * opcodes.c - CIL opcode definitions and utilities
 */

#include "iron/opcodes.h"
#include <string.h>

/* Lookup tables */
static iron_bool g_tables_initialized = IRON_FALSE;

/* Opcode names */
static const char *g_opcode_names[512];

static void init_opcode_names(void)
{
    if (g_tables_initialized) return;
    
    memset(g_opcode_names, 0, sizeof(g_opcode_names));
    
    g_opcode_names[0x00] = "nop";
    g_opcode_names[0x01] = "break";
    g_opcode_names[0x02] = "ldarg.0";
    g_opcode_names[0x03] = "ldarg.1";
    g_opcode_names[0x04] = "ldarg.2";
    g_opcode_names[0x05] = "ldarg.3";
    g_opcode_names[0x06] = "ldloc.0";
    g_opcode_names[0x07] = "ldloc.1";
    g_opcode_names[0x08] = "ldloc.2";
    g_opcode_names[0x09] = "ldloc.3";
    g_opcode_names[0x0A] = "stloc.0";
    g_opcode_names[0x0B] = "stloc.1";
    g_opcode_names[0x0C] = "stloc.2";
    g_opcode_names[0x0D] = "stloc.3";
    g_opcode_names[0x0E] = "ldarg.s";
    g_opcode_names[0x0F] = "ldarga.s";
    g_opcode_names[0x10] = "starg.s";
    g_opcode_names[0x11] = "ldloc.s";
    g_opcode_names[0x12] = "ldloca.s";
    g_opcode_names[0x13] = "stloc.s";
    g_opcode_names[0x14] = "ldnull";
    g_opcode_names[0x15] = "ldc.i4.m1";
    g_opcode_names[0x16] = "ldc.i4.0";
    g_opcode_names[0x17] = "ldc.i4.1";
    g_opcode_names[0x18] = "ldc.i4.2";
    g_opcode_names[0x19] = "ldc.i4.3";
    g_opcode_names[0x1A] = "ldc.i4.4";
    g_opcode_names[0x1B] = "ldc.i4.5";
    g_opcode_names[0x1C] = "ldc.i4.6";
    g_opcode_names[0x1D] = "ldc.i4.7";
    g_opcode_names[0x1E] = "ldc.i4.8";
    g_opcode_names[0x1F] = "ldc.i4.s";
    g_opcode_names[0x20] = "ldc.i4";
    g_opcode_names[0x21] = "ldc.i8";
    g_opcode_names[0x22] = "ldc.r4";
    g_opcode_names[0x23] = "ldc.r8";
    g_opcode_names[0x25] = "dup";
    g_opcode_names[0x26] = "pop";
    g_opcode_names[0x27] = "jmp";
    g_opcode_names[0x28] = "call";
    g_opcode_names[0x29] = "calli";
    g_opcode_names[0x2A] = "ret";
    g_opcode_names[0x2B] = "br.s";
    g_opcode_names[0x2C] = "brfalse.s";
    g_opcode_names[0x2D] = "brtrue.s";
    g_opcode_names[0x2E] = "beq.s";
    g_opcode_names[0x2F] = "bge.s";
    g_opcode_names[0x30] = "bgt.s";
    g_opcode_names[0x31] = "ble.s";
    g_opcode_names[0x32] = "blt.s";
    g_opcode_names[0x33] = "bne.un.s";
    g_opcode_names[0x34] = "bge.un.s";
    g_opcode_names[0x35] = "bgt.un.s";
    g_opcode_names[0x36] = "ble.un.s";
    g_opcode_names[0x37] = "blt.un.s";
    g_opcode_names[0x38] = "br";
    g_opcode_names[0x39] = "brfalse";
    g_opcode_names[0x3A] = "brtrue";
    g_opcode_names[0x3B] = "beq";
    g_opcode_names[0x3C] = "bge";
    g_opcode_names[0x3D] = "bgt";
    g_opcode_names[0x3E] = "ble";
    g_opcode_names[0x3F] = "blt";
    g_opcode_names[0x40] = "bne.un";
    g_opcode_names[0x41] = "bge.un";
    g_opcode_names[0x42] = "bgt.un";
    g_opcode_names[0x43] = "ble.un";
    g_opcode_names[0x44] = "blt.un";
    g_opcode_names[0x45] = "switch";
    g_opcode_names[0x46] = "ldind.i1";
    g_opcode_names[0x47] = "ldind.u1";
    g_opcode_names[0x48] = "ldind.i2";
    g_opcode_names[0x49] = "ldind.u2";
    g_opcode_names[0x4A] = "ldind.i4";
    g_opcode_names[0x4B] = "ldind.u4";
    g_opcode_names[0x4C] = "ldind.i8";
    g_opcode_names[0x4D] = "ldind.i";
    g_opcode_names[0x4E] = "ldind.r4";
    g_opcode_names[0x4F] = "ldind.r8";
    g_opcode_names[0x50] = "ldind.ref";
    g_opcode_names[0x51] = "stind.ref";
    g_opcode_names[0x52] = "stind.i1";
    g_opcode_names[0x53] = "stind.i2";
    g_opcode_names[0x54] = "stind.i4";
    g_opcode_names[0x55] = "stind.i8";
    g_opcode_names[0x56] = "stind.r4";
    g_opcode_names[0x57] = "stind.r8";
    g_opcode_names[0x58] = "add";
    g_opcode_names[0x59] = "sub";
    g_opcode_names[0x5A] = "mul";
    g_opcode_names[0x5B] = "div";
    g_opcode_names[0x5C] = "div.un";
    g_opcode_names[0x5D] = "rem";
    g_opcode_names[0x5E] = "rem.un";
    g_opcode_names[0x5F] = "and";
    g_opcode_names[0x60] = "or";
    g_opcode_names[0x61] = "xor";
    g_opcode_names[0x62] = "shl";
    g_opcode_names[0x63] = "shr";
    g_opcode_names[0x64] = "shr.un";
    g_opcode_names[0x65] = "neg";
    g_opcode_names[0x66] = "not";
    g_opcode_names[0x67] = "conv.i1";
    g_opcode_names[0x68] = "conv.i2";
    g_opcode_names[0x69] = "conv.i4";
    g_opcode_names[0x6A] = "conv.i8";
    g_opcode_names[0x6B] = "conv.r4";
    g_opcode_names[0x6C] = "conv.r8";
    g_opcode_names[0x6D] = "conv.u4";
    g_opcode_names[0x6E] = "conv.u8";
    g_opcode_names[0x6F] = "callvirt";
    g_opcode_names[0x70] = "cpobj";
    g_opcode_names[0x71] = "ldobj";
    g_opcode_names[0x72] = "ldstr";
    g_opcode_names[0x73] = "newobj";
    g_opcode_names[0x74] = "castclass";
    g_opcode_names[0x75] = "isinst";
    g_opcode_names[0x76] = "conv.r.un";
    g_opcode_names[0x79] = "unbox";
    g_opcode_names[0x7A] = "throw";
    g_opcode_names[0x7B] = "ldfld";
    g_opcode_names[0x7C] = "ldflda";
    g_opcode_names[0x7D] = "stfld";
    g_opcode_names[0x7E] = "ldsfld";
    g_opcode_names[0x7F] = "ldsflda";
    g_opcode_names[0x80] = "stsfld";
    g_opcode_names[0x81] = "stobj";
    g_opcode_names[0x8C] = "box";
    g_opcode_names[0x8D] = "newarr";
    g_opcode_names[0x8E] = "ldlen";
    g_opcode_names[0x8F] = "ldelema";
    g_opcode_names[0xA3] = "ldelem";
    g_opcode_names[0xA4] = "stelem";
    g_opcode_names[0xA5] = "unbox.any";
    g_opcode_names[0xD0] = "ldtoken";
    g_opcode_names[0xD3] = "conv.i";
    g_opcode_names[0xDC] = "endfinally";
    g_opcode_names[0xDD] = "leave";
    g_opcode_names[0xDE] = "leave.s";
    g_opcode_names[0xDF] = "stind.i";
    g_opcode_names[0xE0] = "conv.u";
    g_opcode_names[0x2A] = "ret";
    
    /* Two-byte opcodes (0x100+) */
    g_opcode_names[0x100] = "arglist";
    g_opcode_names[0x101] = "ceq";
    g_opcode_names[0x102] = "cgt";
    g_opcode_names[0x103] = "cgt.un";
    g_opcode_names[0x104] = "clt";
    g_opcode_names[0x105] = "clt.un";
    g_opcode_names[0x106] = "ldftn";
    g_opcode_names[0x107] = "ldvirtftn";
    g_opcode_names[0x109] = "ldarg";
    g_opcode_names[0x10A] = "ldarga";
    g_opcode_names[0x10B] = "starg";
    g_opcode_names[0x10C] = "ldloc";
    g_opcode_names[0x10D] = "ldloca";
    g_opcode_names[0x10E] = "stloc";
    g_opcode_names[0x10F] = "localloc";
    g_opcode_names[0x111] = "endfilter";
    g_opcode_names[0x112] = "unaligned.";
    g_opcode_names[0x113] = "volatile.";
    g_opcode_names[0x114] = "tail.";
    g_opcode_names[0x115] = "initobj";
    g_opcode_names[0x116] = "constrained.";
    g_opcode_names[0x117] = "cpblk";
    g_opcode_names[0x118] = "initblk";
    g_opcode_names[0x11A] = "rethrow";
    g_opcode_names[0x11C] = "sizeof";
    g_opcode_names[0x11D] = "refanytype";
    g_opcode_names[0x11E] = "readonly.";
    
    g_tables_initialized = IRON_TRUE;
}

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
    init_opcode_names();
    if (opcode < 512 && g_opcode_names[opcode]) {
        return g_opcode_names[opcode];
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
    switch (opcode) {
        case IRON_CEE_BR:
        case IRON_CEE_BR_S:
        case IRON_CEE_BRFALSE:
        case IRON_CEE_BRFALSE_S:
        case IRON_CEE_BRTRUE:
        case IRON_CEE_BRTRUE_S:
        case IRON_CEE_BEQ:
        case IRON_CEE_BEQ_S:
        case IRON_CEE_BGE:
        case IRON_CEE_BGE_S:
        case IRON_CEE_BGT:
        case IRON_CEE_BGT_S:
        case IRON_CEE_BLE:
        case IRON_CEE_BLE_S:
        case IRON_CEE_BLT:
        case IRON_CEE_BLT_S:
        case IRON_CEE_BNE_UN:
        case IRON_CEE_BNE_UN_S:
        case IRON_CEE_BGE_UN:
        case IRON_CEE_BGE_UN_S:
        case IRON_CEE_BGT_UN:
        case IRON_CEE_BGT_UN_S:
        case IRON_CEE_BLE_UN:
        case IRON_CEE_BLE_UN_S:
        case IRON_CEE_BLT_UN:
        case IRON_CEE_BLT_UN_S:
        case IRON_CEE_SWITCH:
        case IRON_CEE_LEAVE:
        case IRON_CEE_LEAVE_S:
            return IRON_TRUE;
        default:
            return IRON_FALSE;
    }
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

const iron_opcode_info_t *iron_opcode_info(iron_opcode_t opcode)
{
    /* Simplified - return NULL for now, full table would be too large */
    (void)opcode;
    return NULL;
}
