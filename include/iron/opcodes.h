/*
 * IronNet CLR Interpreter
 * opcodes.h - Complete CIL opcode definitions (ECMA-335 Partition III)
 * 
 * Pure C89 compatible
 */

#ifndef IRON_OPCODES_H
#define IRON_OPCODES_H

#include "platform.h"
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Opcode Operand Types
 * ============================================================================ */

typedef enum iron_operand_type {
    IRON_OP_NONE,           /* No operand */
    IRON_OP_I8,             /* 8-bit signed integer */
    IRON_OP_U8,             /* 8-bit unsigned integer */
    IRON_OP_I16,            /* 16-bit signed integer */
    IRON_OP_I32,            /* 32-bit signed integer */
    IRON_OP_I64,            /* 64-bit signed integer */
    IRON_OP_F32,            /* 32-bit float */
    IRON_OP_F64,            /* 64-bit float */
    IRON_OP_TOKEN,          /* Metadata token (32-bit) */
    IRON_OP_STRING,         /* String token */
    IRON_OP_BRANCH,         /* Branch offset (32-bit) */
    IRON_OP_SHORT_BRANCH,   /* Short branch offset (8-bit) */
    IRON_OP_SWITCH,         /* Switch table */
    IRON_OP_VAR,            /* Variable index (16-bit) */
    IRON_OP_SHORT_VAR,      /* Short variable index (8-bit) */
    IRON_OP_ARG,            /* Argument index (16-bit) */
    IRON_OP_SHORT_ARG       /* Short argument index (8-bit) */
} iron_operand_type_t;

/* ============================================================================
 * Stack Behavior
 * ============================================================================ */

typedef enum iron_stack_pop {
    IRON_POP_0,
    IRON_POP_1,
    IRON_POP_2,
    IRON_POP_3,
    IRON_POP_1_1,
    IRON_POP_REF,
    IRON_POP_REF_1,
    IRON_POP_REF_I,
    IRON_POP_REF_I_I,
    IRON_POP_REF_I_I8,
    IRON_POP_REF_I_R4,
    IRON_POP_REF_I_R8,
    IRON_POP_REF_I_REF,
    IRON_POP_VAR
} iron_stack_pop_t;

typedef enum iron_stack_push {
    IRON_PUSH_0,
    IRON_PUSH_1,
    IRON_PUSH_2,
    IRON_PUSH_I,
    IRON_PUSH_I8,
    IRON_PUSH_R4,
    IRON_PUSH_R8,
    IRON_PUSH_REF,
    IRON_PUSH_VAR
} iron_stack_push_t;

/* ============================================================================
 * Flow Control
 * ============================================================================ */

typedef enum iron_flow_control {
    IRON_FLOW_NEXT,         /* Fall through */
    IRON_FLOW_BRANCH,       /* Unconditional branch */
    IRON_FLOW_COND_BRANCH,  /* Conditional branch */
    IRON_FLOW_CALL,         /* Method call */
    IRON_FLOW_RETURN,       /* Return */
    IRON_FLOW_THROW,        /* Throw exception */
    IRON_FLOW_META,         /* Prefix instruction */
    IRON_FLOW_BREAK         /* Breakpoint */
} iron_flow_control_t;

/* ============================================================================
 * Opcode Enumeration
 * ============================================================================ */

typedef enum iron_opcode {
    /* Single-byte opcodes (0x00 - 0xFE) */
    IRON_CEE_NOP             = 0x00,
    IRON_CEE_BREAK           = 0x01,
    IRON_CEE_LDARG_0         = 0x02,
    IRON_CEE_LDARG_1         = 0x03,
    IRON_CEE_LDARG_2         = 0x04,
    IRON_CEE_LDARG_3         = 0x05,
    IRON_CEE_LDLOC_0         = 0x06,
    IRON_CEE_LDLOC_1         = 0x07,
    IRON_CEE_LDLOC_2         = 0x08,
    IRON_CEE_LDLOC_3         = 0x09,
    IRON_CEE_STLOC_0         = 0x0A,
    IRON_CEE_STLOC_1         = 0x0B,
    IRON_CEE_STLOC_2         = 0x0C,
    IRON_CEE_STLOC_3         = 0x0D,
    IRON_CEE_LDARG_S         = 0x0E,
    IRON_CEE_LDARGA_S        = 0x0F,
    IRON_CEE_STARG_S         = 0x10,
    IRON_CEE_LDLOC_S         = 0x11,
    IRON_CEE_LDLOCA_S        = 0x12,
    IRON_CEE_STLOC_S         = 0x13,
    IRON_CEE_LDNULL          = 0x14,
    IRON_CEE_LDC_I4_M1       = 0x15,
    IRON_CEE_LDC_I4_0        = 0x16,
    IRON_CEE_LDC_I4_1        = 0x17,
    IRON_CEE_LDC_I4_2        = 0x18,
    IRON_CEE_LDC_I4_3        = 0x19,
    IRON_CEE_LDC_I4_4        = 0x1A,
    IRON_CEE_LDC_I4_5        = 0x1B,
    IRON_CEE_LDC_I4_6        = 0x1C,
    IRON_CEE_LDC_I4_7        = 0x1D,
    IRON_CEE_LDC_I4_8        = 0x1E,
    IRON_CEE_LDC_I4_S        = 0x1F,
    IRON_CEE_LDC_I4          = 0x20,
    IRON_CEE_LDC_I8          = 0x21,
    IRON_CEE_LDC_R4          = 0x22,
    IRON_CEE_LDC_R8          = 0x23,
    IRON_CEE_UNUSED49        = 0x24,
    IRON_CEE_DUP             = 0x25,
    IRON_CEE_POP             = 0x26,
    IRON_CEE_JMP             = 0x27,
    IRON_CEE_CALL            = 0x28,
    IRON_CEE_CALLI           = 0x29,
    IRON_CEE_RET             = 0x2A,
    IRON_CEE_BR_S            = 0x2B,
    IRON_CEE_BRFALSE_S       = 0x2C,
    IRON_CEE_BRTRUE_S        = 0x2D,
    IRON_CEE_BEQ_S           = 0x2E,
    IRON_CEE_BGE_S           = 0x2F,
    IRON_CEE_BGT_S           = 0x30,
    IRON_CEE_BLE_S           = 0x31,
    IRON_CEE_BLT_S           = 0x32,
    IRON_CEE_BNE_UN_S        = 0x33,
    IRON_CEE_BGE_UN_S        = 0x34,
    IRON_CEE_BGT_UN_S        = 0x35,
    IRON_CEE_BLE_UN_S        = 0x36,
    IRON_CEE_BLT_UN_S        = 0x37,
    IRON_CEE_BR              = 0x38,
    IRON_CEE_BRFALSE         = 0x39,
    IRON_CEE_BRTRUE          = 0x3A,
    IRON_CEE_BEQ             = 0x3B,
    IRON_CEE_BGE             = 0x3C,
    IRON_CEE_BGT             = 0x3D,
    IRON_CEE_BLE             = 0x3E,
    IRON_CEE_BLT             = 0x3F,
    IRON_CEE_BNE_UN          = 0x40,
    IRON_CEE_BGE_UN          = 0x41,
    IRON_CEE_BGT_UN          = 0x42,
    IRON_CEE_BLE_UN          = 0x43,
    IRON_CEE_BLT_UN          = 0x44,
    IRON_CEE_SWITCH          = 0x45,
    IRON_CEE_LDIND_I1        = 0x46,
    IRON_CEE_LDIND_U1        = 0x47,
    IRON_CEE_LDIND_I2        = 0x48,
    IRON_CEE_LDIND_U2        = 0x49,
    IRON_CEE_LDIND_I4        = 0x4A,
    IRON_CEE_LDIND_U4        = 0x4B,
    IRON_CEE_LDIND_I8        = 0x4C,
    IRON_CEE_LDIND_I         = 0x4D,
    IRON_CEE_LDIND_R4        = 0x4E,
    IRON_CEE_LDIND_R8        = 0x4F,
    IRON_CEE_LDIND_REF       = 0x50,
    IRON_CEE_STIND_REF       = 0x51,
    IRON_CEE_STIND_I1        = 0x52,
    IRON_CEE_STIND_I2        = 0x53,
    IRON_CEE_STIND_I4        = 0x54,
    IRON_CEE_STIND_I8        = 0x55,
    IRON_CEE_STIND_R4        = 0x56,
    IRON_CEE_STIND_R8        = 0x57,
    IRON_CEE_ADD             = 0x58,
    IRON_CEE_SUB             = 0x59,
    IRON_CEE_MUL             = 0x5A,
    IRON_CEE_DIV             = 0x5B,
    IRON_CEE_DIV_UN          = 0x5C,
    IRON_CEE_REM             = 0x5D,
    IRON_CEE_REM_UN          = 0x5E,
    IRON_CEE_AND             = 0x5F,
    IRON_CEE_OR              = 0x60,
    IRON_CEE_XOR             = 0x61,
    IRON_CEE_SHL             = 0x62,
    IRON_CEE_SHR             = 0x63,
    IRON_CEE_SHR_UN          = 0x64,
    IRON_CEE_NEG             = 0x65,
    IRON_CEE_NOT             = 0x66,
    IRON_CEE_CONV_I1         = 0x67,
    IRON_CEE_CONV_I2         = 0x68,
    IRON_CEE_CONV_I4         = 0x69,
    IRON_CEE_CONV_I8         = 0x6A,
    IRON_CEE_CONV_R4         = 0x6B,
    IRON_CEE_CONV_R8         = 0x6C,
    IRON_CEE_CONV_U4         = 0x6D,
    IRON_CEE_CONV_U8         = 0x6E,
    IRON_CEE_CALLVIRT        = 0x6F,
    IRON_CEE_CPOBJ           = 0x70,
    IRON_CEE_LDOBJ           = 0x71,
    IRON_CEE_LDSTR           = 0x72,
    IRON_CEE_NEWOBJ          = 0x73,
    IRON_CEE_CASTCLASS       = 0x74,
    IRON_CEE_ISINST          = 0x75,
    IRON_CEE_CONV_R_UN       = 0x76,
    IRON_CEE_UNUSED58        = 0x77,
    IRON_CEE_UNUSED1         = 0x78,
    IRON_CEE_UNBOX           = 0x79,
    IRON_CEE_THROW           = 0x7A,
    IRON_CEE_LDFLD           = 0x7B,
    IRON_CEE_LDFLDA          = 0x7C,
    IRON_CEE_STFLD           = 0x7D,
    IRON_CEE_LDSFLD          = 0x7E,
    IRON_CEE_LDSFLDA         = 0x7F,
    IRON_CEE_STSFLD          = 0x80,
    IRON_CEE_STOBJ           = 0x81,
    IRON_CEE_CONV_OVF_I1_UN  = 0x82,
    IRON_CEE_CONV_OVF_I2_UN  = 0x83,
    IRON_CEE_CONV_OVF_I4_UN  = 0x84,
    IRON_CEE_CONV_OVF_I8_UN  = 0x85,
    IRON_CEE_CONV_OVF_U1_UN  = 0x86,
    IRON_CEE_CONV_OVF_U2_UN  = 0x87,
    IRON_CEE_CONV_OVF_U4_UN  = 0x88,
    IRON_CEE_CONV_OVF_U8_UN  = 0x89,
    IRON_CEE_CONV_OVF_I_UN   = 0x8A,
    IRON_CEE_CONV_OVF_U_UN   = 0x8B,
    IRON_CEE_BOX             = 0x8C,
    IRON_CEE_NEWARR          = 0x8D,
    IRON_CEE_LDLEN           = 0x8E,
    IRON_CEE_LDELEMA         = 0x8F,
    IRON_CEE_LDELEM_I1       = 0x90,
    IRON_CEE_LDELEM_U1       = 0x91,
    IRON_CEE_LDELEM_I2       = 0x92,
    IRON_CEE_LDELEM_U2       = 0x93,
    IRON_CEE_LDELEM_I4       = 0x94,
    IRON_CEE_LDELEM_U4       = 0x95,
    IRON_CEE_LDELEM_I8       = 0x96,
    IRON_CEE_LDELEM_I        = 0x97,
    IRON_CEE_LDELEM_R4       = 0x98,
    IRON_CEE_LDELEM_R8       = 0x99,
    IRON_CEE_LDELEM_REF      = 0x9A,
    IRON_CEE_STELEM_I        = 0x9B,
    IRON_CEE_STELEM_I1       = 0x9C,
    IRON_CEE_STELEM_I2       = 0x9D,
    IRON_CEE_STELEM_I4       = 0x9E,
    IRON_CEE_STELEM_I8       = 0x9F,
    IRON_CEE_STELEM_R4       = 0xA0,
    IRON_CEE_STELEM_R8       = 0xA1,
    IRON_CEE_STELEM_REF      = 0xA2,
    IRON_CEE_LDELEM          = 0xA3,
    IRON_CEE_STELEM          = 0xA4,
    IRON_CEE_UNBOX_ANY       = 0xA5,
    IRON_CEE_UNUSED5         = 0xA6,
    IRON_CEE_UNUSED6         = 0xA7,
    IRON_CEE_UNUSED7         = 0xA8,
    IRON_CEE_UNUSED8         = 0xA9,
    IRON_CEE_UNUSED9         = 0xAA,
    IRON_CEE_UNUSED10        = 0xAB,
    IRON_CEE_UNUSED11        = 0xAC,
    IRON_CEE_UNUSED12        = 0xAD,
    IRON_CEE_UNUSED13        = 0xAE,
    IRON_CEE_UNUSED14        = 0xAF,
    IRON_CEE_UNUSED15        = 0xB0,
    IRON_CEE_UNUSED16        = 0xB1,
    IRON_CEE_UNUSED17        = 0xB2,
    IRON_CEE_CONV_OVF_I1     = 0xB3,
    IRON_CEE_CONV_OVF_U1     = 0xB4,
    IRON_CEE_CONV_OVF_I2     = 0xB5,
    IRON_CEE_CONV_OVF_U2     = 0xB6,
    IRON_CEE_CONV_OVF_I4     = 0xB7,
    IRON_CEE_CONV_OVF_U4     = 0xB8,
    IRON_CEE_CONV_OVF_I8     = 0xB9,
    IRON_CEE_CONV_OVF_U8     = 0xBA,
    IRON_CEE_UNUSED50        = 0xBB,
    IRON_CEE_UNUSED18        = 0xBC,
    IRON_CEE_UNUSED19        = 0xBD,
    IRON_CEE_UNUSED20        = 0xBE,
    IRON_CEE_UNUSED21        = 0xBF,
    IRON_CEE_UNUSED22        = 0xC0,
    IRON_CEE_UNUSED23        = 0xC1,
    IRON_CEE_REFANYVAL       = 0xC2,
    IRON_CEE_CKFINITE        = 0xC3,
    IRON_CEE_UNUSED24        = 0xC4,
    IRON_CEE_UNUSED25        = 0xC5,
    IRON_CEE_MKREFANY        = 0xC6,
    IRON_CEE_UNUSED59        = 0xC7,
    IRON_CEE_UNUSED60        = 0xC8,
    IRON_CEE_UNUSED61        = 0xC9,
    IRON_CEE_UNUSED62        = 0xCA,
    IRON_CEE_UNUSED63        = 0xCB,
    IRON_CEE_UNUSED64        = 0xCC,
    IRON_CEE_UNUSED65        = 0xCD,
    IRON_CEE_UNUSED66        = 0xCE,
    IRON_CEE_UNUSED67        = 0xCF,
    IRON_CEE_LDTOKEN         = 0xD0,
    IRON_CEE_CONV_U2         = 0xD1,
    IRON_CEE_CONV_U1         = 0xD2,
    IRON_CEE_CONV_I          = 0xD3,
    IRON_CEE_CONV_OVF_I      = 0xD4,
    IRON_CEE_CONV_OVF_U      = 0xD5,
    IRON_CEE_ADD_OVF         = 0xD6,
    IRON_CEE_ADD_OVF_UN      = 0xD7,
    IRON_CEE_MUL_OVF         = 0xD8,
    IRON_CEE_MUL_OVF_UN      = 0xD9,
    IRON_CEE_SUB_OVF         = 0xDA,
    IRON_CEE_SUB_OVF_UN      = 0xDB,
    IRON_CEE_ENDFINALLY      = 0xDC,
    IRON_CEE_LEAVE           = 0xDD,
    IRON_CEE_LEAVE_S         = 0xDE,
    IRON_CEE_STIND_I         = 0xDF,
    IRON_CEE_CONV_U          = 0xE0,
    IRON_CEE_UNUSED26        = 0xE1,
    IRON_CEE_UNUSED27        = 0xE2,
    IRON_CEE_UNUSED28        = 0xE3,
    IRON_CEE_UNUSED29        = 0xE4,
    IRON_CEE_UNUSED30        = 0xE5,
    IRON_CEE_UNUSED31        = 0xE6,
    IRON_CEE_UNUSED32        = 0xE7,
    IRON_CEE_UNUSED33        = 0xE8,
    IRON_CEE_UNUSED34        = 0xE9,
    IRON_CEE_UNUSED35        = 0xEA,
    IRON_CEE_UNUSED36        = 0xEB,
    IRON_CEE_UNUSED37        = 0xEC,
    IRON_CEE_UNUSED38        = 0xED,
    IRON_CEE_UNUSED39        = 0xEE,
    IRON_CEE_UNUSED40        = 0xEF,
    IRON_CEE_UNUSED41        = 0xF0,
    IRON_CEE_UNUSED42        = 0xF1,
    IRON_CEE_UNUSED43        = 0xF2,
    IRON_CEE_UNUSED44        = 0xF3,
    IRON_CEE_UNUSED45        = 0xF4,
    IRON_CEE_UNUSED46        = 0xF5,
    IRON_CEE_UNUSED47        = 0xF6,
    IRON_CEE_UNUSED48        = 0xF7,
    IRON_CEE_PREFIX7         = 0xF8,
    IRON_CEE_PREFIX6         = 0xF9,
    IRON_CEE_PREFIX5         = 0xFA,
    IRON_CEE_PREFIX4         = 0xFB,
    IRON_CEE_PREFIX3         = 0xFC,
    IRON_CEE_PREFIX2         = 0xFD,
    IRON_CEE_PREFIX1         = 0xFE,
    IRON_CEE_PREFIXREF       = 0xFF,
    
    /* Two-byte opcodes (0xFE 0xXX) - stored as 0x100 + XX */
    IRON_CEE_ARGLIST         = 0x100,
    IRON_CEE_CEQ             = 0x101,
    IRON_CEE_CGT             = 0x102,
    IRON_CEE_CGT_UN          = 0x103,
    IRON_CEE_CLT             = 0x104,
    IRON_CEE_CLT_UN          = 0x105,
    IRON_CEE_LDFTN           = 0x106,
    IRON_CEE_LDVIRTFTN       = 0x107,
    IRON_CEE_UNUSED56        = 0x108,
    IRON_CEE_LDARG           = 0x109,
    IRON_CEE_LDARGA          = 0x10A,
    IRON_CEE_STARG           = 0x10B,
    IRON_CEE_LDLOC           = 0x10C,
    IRON_CEE_LDLOCA          = 0x10D,
    IRON_CEE_STLOC           = 0x10E,
    IRON_CEE_LOCALLOC        = 0x10F,
    IRON_CEE_UNUSED57        = 0x110,
    IRON_CEE_ENDFILTER       = 0x111,
    IRON_CEE_UNALIGNED       = 0x112,
    IRON_CEE_VOLATILE        = 0x113,
    IRON_CEE_TAIL            = 0x114,
    IRON_CEE_INITOBJ         = 0x115,
    IRON_CEE_CONSTRAINED     = 0x116,
    IRON_CEE_CPBLK           = 0x117,
    IRON_CEE_INITBLK         = 0x118,
    IRON_CEE_NO              = 0x119,
    IRON_CEE_RETHROW         = 0x11A,
    IRON_CEE_UNUSED          = 0x11B,
    IRON_CEE_SIZEOF          = 0x11C,
    IRON_CEE_REFANYTYPE      = 0x11D,
    IRON_CEE_READONLY        = 0x11E,
    
    /* Sentinel */
    IRON_CEE_INVALID         = 0xFFFF
} iron_opcode_t;

/* ============================================================================
 * Opcode Information
 * ============================================================================ */

typedef struct iron_opcode_info {
    iron_opcode_t opcode;
    const char *name;
    iron_u16 value;             /* Actual byte value(s) */
    iron_u8 size;               /* Opcode size (1 or 2) */
    iron_operand_type_t operand;
    iron_stack_pop_t pop;
    iron_stack_push_t push;
    iron_flow_control_t flow;
} iron_opcode_info_t;

/* Get opcode info by enum value */
IRON_API const iron_opcode_info_t *iron_opcode_info(iron_opcode_t opcode);

/* Decode opcode from bytes */
IRON_API iron_opcode_t iron_opcode_decode(const iron_u8 *code, iron_u32 *size);

/* Get opcode name */
IRON_API const char *iron_opcode_name(iron_opcode_t opcode);

/* Get operand size in bytes */
IRON_API iron_u32 iron_operand_size(iron_operand_type_t type);

/* Check if opcode is branch */
IRON_API iron_bool iron_opcode_is_branch(iron_opcode_t opcode);

/* Check if opcode is prefix */
IRON_API iron_bool iron_opcode_is_prefix(iron_opcode_t opcode);

#ifdef __cplusplus
}
#endif

#endif /* IRON_OPCODES_H */
