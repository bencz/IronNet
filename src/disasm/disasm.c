/*
 * IronNet CLR Interpreter
 * disasm.c - Disassembly utilities implementation
 */

#include "iron/disasm.h"
#include "iron/metadata.h"
#include "iron/opcodes.h"
#include "iron/pe.h"
#include <stdio.h>
#include <string.h>

void iron_disasm_memberref(iron_assembly_t *assembly, iron_u32 row_index)
{
    iron_member_ref_row_t member_ref;
    iron_token_t token = IRON_MAKE_TOKEN(IRON_TABLE_MEMBER_REF, row_index);
    iron_result_t res;
    const char *name;
    iron_u32 class_token;
    iron_u32 class_table;
    const char *type_name = "?";
    const char *type_ns = "";
    const iron_u8 *sig_data = NULL;
    iron_u32 sig_size = 0;
    iron_u32 j;
    
    res = iron_metadata_read_row(&assembly->metadata, token, &member_ref);
    if (!IRON_RESULT_OK(res)) {
        printf("    [%u] <error reading row>\n", row_index);
        return;
    }
    
    name = iron_metadata_get_string(&assembly->metadata, member_ref.name);
    class_token = iron_metadata_decode_coded(&assembly->metadata, 
                                               IRON_CODED_MEMBER_REF_PARENT,
                                               member_ref.class_);
    class_table = (class_token >> 24) & 0xFF;
    
    if (class_table == IRON_TABLE_TYPE_REF) {
        iron_type_ref_row_t type_ref;
        if (IRON_RESULT_OK(iron_metadata_read_row(&assembly->metadata, class_token, &type_ref))) {
            type_name = iron_metadata_get_string(&assembly->metadata, type_ref.name);
            type_ns = iron_metadata_get_string(&assembly->metadata, type_ref.namespace_);
        }
    } else if (class_table == IRON_TABLE_TYPE_DEF) {
        iron_type_def_row_t type_def;
        if (IRON_RESULT_OK(iron_metadata_read_row(&assembly->metadata, class_token, &type_def))) {
            type_name = iron_metadata_get_string(&assembly->metadata, type_def.name);
            type_ns = iron_metadata_get_string(&assembly->metadata, type_def.namespace_);
        }
    }
    
    iron_metadata_get_blob(&assembly->metadata, member_ref.signature, &sig_data, &sig_size);
    
    printf("    [%u] %s%s%s::%s  sig=[", row_index,
           type_ns ? type_ns : "", (type_ns && type_ns[0]) ? "." : "",
           type_name ? type_name : "?", name ? name : "?");
    
    if (sig_data && sig_size > 0) {
        for (j = 0; j < sig_size && j < 16; j++) {
            printf("%02X ", sig_data[j]);
        }
        if (sig_size > 16) printf("...");
    }
    printf("]\n");
}

void iron_disasm_il_code(iron_assembly_t *assembly, const iron_u8 *code, iron_u32 code_size)
{
    iron_u32 ip = 0;
    
    while (ip < code_size) {
        iron_u32 opcode_size = 0;
        iron_opcode_t opcode = iron_opcode_decode(code + ip, &opcode_size);
        const iron_opcode_info_t *info = iron_opcode_info(opcode);
        const char *name = info ? info->name : iron_opcode_name(opcode);
        
        printf("      IL_%04X: ", ip);
        ip += opcode_size;
        
        if (!info) {
            printf("%s\n", name);
            continue;
        }
        
        /* Use operand type from opcode info */
        switch (info->operand) {
            case IRON_OP_NONE:
                printf("%s\n", name);
                break;
            
            case IRON_OP_SHORT_BRANCH:
                if (ip < code_size) {
                    iron_i8 offset = (iron_i8)code[ip];
                    printf("%s IL_%04X\n", name, ip + 1 + offset);
                    ip++;
                } else {
                    printf("%s <missing>\n", name);
                }
                break;
            
            case IRON_OP_BRANCH:
                if (ip + 4 <= code_size) {
                    iron_i32 offset = (iron_i32)(code[ip] | (code[ip+1] << 8) | (code[ip+2] << 16) | (code[ip+3] << 24));
                    printf("%s IL_%04X\n", name, ip + 4 + offset);
                    ip += 4;
                } else {
                    printf("%s <missing>\n", name);
                }
                break;
            
            case IRON_OP_I8:
            case IRON_OP_SHORT_VAR:
            case IRON_OP_SHORT_ARG:
            case IRON_OP_U8:
                if (ip < code_size) {
                    printf("%s %d\n", name, (int)(signed char)code[ip]);
                    ip++;
                } else {
                    printf("%s <missing>\n", name);
                }
                break;
            
            case IRON_OP_I32:
                if (ip + 4 <= code_size) {
                    iron_i32 val = (iron_i32)(code[ip] | (code[ip+1] << 8) | (code[ip+2] << 16) | (code[ip+3] << 24));
                    printf("%s %d\n", name, val);
                    ip += 4;
                } else {
                    printf("%s <missing>\n", name);
                }
                break;
            
            case IRON_OP_I64:
                if (ip + 8 <= code_size) {
                    printf("%s 0x%02X%02X%02X%02X%02X%02X%02X%02X\n", name,
                           code[ip+7], code[ip+6], code[ip+5], code[ip+4],
                           code[ip+3], code[ip+2], code[ip+1], code[ip]);
                    ip += 8;
                } else {
                    printf("%s <missing>\n", name);
                }
                break;
            
            case IRON_OP_VAR:
            case IRON_OP_ARG:
                if (ip + 2 <= code_size) {
                    iron_u16 idx = (iron_u16)(code[ip] | (code[ip+1] << 8));
                    printf("%s %u\n", name, idx);
                    ip += 2;
                } else {
                    printf("%s <missing>\n", name);
                }
                break;
            
            case IRON_OP_TOKEN:
            case IRON_OP_STRING:
                if (ip + 4 <= code_size) {
                    iron_u32 token = code[ip] | (code[ip+1] << 8) | (code[ip+2] << 16) | (code[ip+3] << 24);
                    iron_u32 table = (token >> 24) & 0xFF;
                    iron_u32 row = token & 0x00FFFFFF;
                    printf("%s 0x%08X", name, token);
                    if (table == IRON_TABLE_MEMBER_REF) {
                        iron_member_ref_row_t mr;
                        if (IRON_RESULT_OK(iron_metadata_read_row(&assembly->metadata, token, &mr))) {
                            const char *mname = iron_metadata_get_string(&assembly->metadata, mr.name);
                            printf(" [MemberRef:%u -> %s]", row, mname ? mname : "?");
                        }
                    } else if (table == IRON_TABLE_METHOD_DEF) {
                        iron_method_def_row_t md;
                        if (IRON_RESULT_OK(iron_metadata_read_row(&assembly->metadata, token, &md))) {
                            const char *mname = iron_metadata_get_string(&assembly->metadata, md.name);
                            printf(" [MethodDef:%u -> %s]", row, mname ? mname : "?");
                        }
                    } else if (table == 0x70) {
                        printf(" [UserString:%u]", row);
                    }
                    printf("\n");
                    ip += 4;
                } else {
                    printf("%s <missing>\n", name);
                }
                break;
            
            case IRON_OP_SWITCH:
                if (ip + 4 <= code_size) {
                    iron_u32 n = code[ip] | (code[ip+1] << 8) | (code[ip+2] << 16) | (code[ip+3] << 24);
                    printf("%s (%u targets)\n", name, n);
                    ip += 4 + n * 4;
                } else {
                    printf("%s <missing>\n", name);
                }
                break;
            
            default:
                printf("%s\n", name);
                break;
        }
    }
}

void iron_disasm_assembly(iron_assembly_t *assembly)
{
    iron_u32 i, count;
    
    printf("\n=== Assembly Disassembly ===\n");
    printf("Name: %s\n", assembly->name ? assembly->name : "?");
    
    /* TypeRef table */
    count = iron_metadata_table_rows(&assembly->metadata, IRON_TABLE_TYPE_REF);
    printf("\n--- TypeRef Table (%u rows) ---\n", count);
    for (i = 1; i <= count; i++) {
        iron_type_ref_row_t row;
        iron_token_t token = IRON_MAKE_TOKEN(IRON_TABLE_TYPE_REF, i);
        if (IRON_RESULT_OK(iron_metadata_read_row(&assembly->metadata, token, &row))) {
            const char *name = iron_metadata_get_string(&assembly->metadata, row.name);
            const char *ns = iron_metadata_get_string(&assembly->metadata, row.namespace_);
            printf("    [%u] %s%s%s\n", i, ns ? ns : "", (ns && ns[0]) ? "." : "", name ? name : "?");
        }
    }
    
    /* TypeDef table */
    count = iron_metadata_table_rows(&assembly->metadata, IRON_TABLE_TYPE_DEF);
    printf("\n--- TypeDef Table (%u rows) ---\n", count);
    for (i = 1; i <= count; i++) {
        iron_type_def_row_t row;
        iron_token_t token = IRON_MAKE_TOKEN(IRON_TABLE_TYPE_DEF, i);
        if (IRON_RESULT_OK(iron_metadata_read_row(&assembly->metadata, token, &row))) {
            const char *name = iron_metadata_get_string(&assembly->metadata, row.name);
            const char *ns = iron_metadata_get_string(&assembly->metadata, row.namespace_);
            printf("    [%u] %s%s%s  methods=%u\n", i, ns ? ns : "", (ns && ns[0]) ? "." : "", 
                   name ? name : "?", row.method_list);
        }
    }
    
    /* MemberRef table */
    count = iron_metadata_table_rows(&assembly->metadata, IRON_TABLE_MEMBER_REF);
    printf("\n--- MemberRef Table (%u rows) ---\n", count);
    for (i = 1; i <= count; i++) {
        iron_disasm_memberref(assembly, i);
    }
    
    /* MethodDef table with IL code */
    count = iron_metadata_table_rows(&assembly->metadata, IRON_TABLE_METHOD_DEF);
    printf("\n--- MethodDef Table (%u rows) ---\n", count);
    for (i = 1; i <= count; i++) {
        iron_method_def_row_t row;
        iron_token_t token = IRON_MAKE_TOKEN(IRON_TABLE_METHOD_DEF, i);
        if (IRON_RESULT_OK(iron_metadata_read_row(&assembly->metadata, token, &row))) {
            const char *name = iron_metadata_get_string(&assembly->metadata, row.name);
            const iron_u8 *sig_data = NULL;
            iron_u32 sig_size = 0;
            iron_u32 j;
            iron_metadata_get_blob(&assembly->metadata, row.signature, &sig_data, &sig_size);
            
            printf("    [%u] %s  rva=0x%X  flags=0x%X  sig=[", i, name ? name : "?", row.rva, row.flags);
            if (sig_data && sig_size > 0) {
                for (j = 0; j < sig_size && j < 16; j++) {
                    printf("%02X ", sig_data[j]);
                }
            }
            printf("]\n");
            
            /* Disassemble IL code if method has body */
            if (row.rva != 0 && (row.impl_flags & 0x1000) == 0) {
                iron_method_body_t body;
                iron_result_t res = iron_parse_method_body(&assembly->image, row.rva, &body, assembly->domain->allocator);
                if (IRON_RESULT_OK(res) && body.code && body.code_size > 0) {
                    printf("      --- IL Code (%u bytes) ---\n", body.code_size);
                    iron_disasm_il_code(assembly, body.code, body.code_size);
                }
            }
        }
    }
    
    printf("\n=== End Disassembly ===\n\n");
}
