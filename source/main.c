#include <stdio.h>
#include <stdlib.h>

#include "cliFile.h"
#include "stringHelper.h"

#define TEST 1

#if TEST

#include "endian.h"

uint8_t ReadUInt8(uint8_t** pData)
{
    uint8_t value = **pData;
    (*pData)++;
    return value;
}

uint32_t ReadUInt32(uint8_t** pData)
{
    //uint32_t value =  *((uint32_t*)*pData);
    uint32_t value = READ32(*pData);
    *pData += 4;
    return value;
}

uint64_t ReadUInt64(uint8_t** pData)
{
    //uint64_t value = *((uint64_t*)*pData);
    uint64_t value = READ64(*pData);
    *pData += 8;
    return value;
}

void test_DecompileProgram(CLIFile* cliFile, MetadataToken* methodToken, MethodDefinition* callingMethod)
{
    MetadataToken* token; 
    MethodDefinition* method; 

    if (methodToken == NULL)
    {
        token = CLIFile_ExpandMetadataToken(cliFile, cliFile->CLIHeader->EntryPointToken);
        method = (MethodDefinition*)token->Data;
        printf("Entry point: %s.%s.%s\n\n", method->TypeDefinition->Namespace, method->TypeDefinition->Name, method->Name);
    }
    else
        method = (MethodDefinition*)methodToken->Data;


    uint8_t* opcodes = method->Body.Code;
    uint32_t localizedDataLength = method->Body.CodeSize;

    uint8_t** currentDataPointer = &opcodes;
    size_t originalDataPointer = (size_t)(*currentDataPointer);
    size_t currentILInstructionBase;
    uint8_t currentILOpcode;

    while ((size_t)(*currentDataPointer) - originalDataPointer < localizedDataLength)
    {
        currentILInstructionBase = (size_t)*currentDataPointer;
        //printf("currentILInstructionBase: 0x%x\n", (int)currentILInstructionBase);

        currentILOpcode = ReadUInt8(currentDataPointer);

        if (currentILOpcode == 0x00) // NOP
        {
            printf("Opcode: 0x%02x ( NOP )\n", currentILOpcode);
        }
        else if (currentILOpcode == 0x28) // Call
        {
            MetadataToken* token = CLIFile_ExpandMetadataToken(cliFile, ReadUInt32(currentDataPointer));
            MethodDefinition* methodDefinition = (MethodDefinition*)token->Data;

            printf("Opcode: 0x%02x ( CALL  ) =====> %s.%s::%s\n", currentILOpcode, methodDefinition->TypeDefinition->Namespace, methodDefinition->TypeDefinition->Name, methodDefinition->Name);

            test_DecompileProgram(cliFile, token, method);
        }
        else if (currentILOpcode == 0x72) // Ldstr
        {

            MetadataToken* token = CLIFile_ExpandMetadataToken(cliFile, ReadUInt32(currentDataPointer));
            if (!token->IsUserString)
                Panic("Invalid token after LdStr!");

            uint32_t strLength = 0;
            uint8_t* str = CLIFile_GetCompressedUnsigned((uint8_t*)token->Data, &strLength);
            strLength -= 1; // Remove the null terminator
            CLIFile_DestroyMetadataToken(token);

            uint32_t realStringSize = (uint32_t)(strLength >> 1);
            printf("Opcode: 0x%02x ( LDSTR ) =====> \"", currentILOpcode);
            for (int i = 0; i < strLength; i++)
            {
                if (*(str) == 0)
                {
                    *(str)++;
                    continue;
                }

                printf("%c", *(str)++);
            }
            printf("\"\n");
        }
        else if (currentILOpcode == 0x2A) // Ret
        {
            if(callingMethod != NULL)
                printf("Opcode: 0x%02x ( RET   ) =====> FROM ( %s.%s::%s ) TO ( %s.%s::%s )\n", currentILOpcode,
                    method->TypeDefinition->Namespace, method->TypeDefinition->Name, method->Name,
                    callingMethod->TypeDefinition->Namespace, callingMethod->TypeDefinition->Name, callingMethod->Name);
            else
                printf("Opcode: 0x%02x ( RET   ) =====> EXIT\n", currentILOpcode);
        }
        else if (currentILOpcode == 0x1f) // ldc.i4.s
        {
            printf("Opcode: 0x%02x ( LDC.I4.S ) =====> ", currentILOpcode);
            uint32_t value = (uint32_t)(int32_t)(int8_t)ReadUInt8(currentDataPointer);
            printf("0x%x\n", value);
        }
        else if (currentILOpcode == 0x0a) // stloc.0
        {
            printf("Opcode: 0x%02x ( STLOC.0 )\n", currentILOpcode);
        }
        else if (currentILOpcode == 0x0b) // stloc.1
        {
            printf("Opcode: 0x%02x ( STLOC.1 )\n", currentILOpcode);
        }
        else if (currentILOpcode == 0x0c) // stloc.2
        {
            printf("Opcode: 0x%02x ( STLOC.2 )\n", currentILOpcode);
        }
        else if (currentILOpcode == 0x0d) // stloc.3
        {
            printf("Opcode: 0x%02x ( STLOC.3 )\n", currentILOpcode);
        }
        else if (currentILOpcode == 0x11) // ldloc.s
        {
            printf("Opcode: 0x%02x ( LDLOC.S ) =====> ", currentILOpcode);
            uint32_t localIndex = ReadUInt8(currentDataPointer);
            printf("0x%x\n", localIndex);
        }
        else if (currentILOpcode == 0x13) // StLoc.S
        {
            printf("Opcode: 0x%02x ( STLOC.S ) =====> ", currentILOpcode);
            uint32_t value = ReadUInt8(currentDataPointer);
            printf("0x%x\n", value);
        }
        else if (currentILOpcode == 0x15) // Ldc.i4.m1
        {
            printf("Opcode: 0x%02x ( LDC.I4.M1 )\n", currentILOpcode);
        }
        else if (currentILOpcode == 0x16) // Ldc.i4.0
        {
            printf("Opcode: 0x%02x ( LDC.I4.0 )\n", currentILOpcode);
        }
        else if (currentILOpcode == 0x20) // Ldc.I4
        {
            printf("Opcode: 0x%02x ( LDC.I4 ) =====> ", currentILOpcode);
            uint32_t value = ReadUInt32(currentDataPointer);
            printf("0x%x\n", value);
        }
        else if (currentILOpcode == 0x21) // Ldc.I8
        {
            printf("Opcode: 0x%02x ( LDC.I8 ) =====> ", currentILOpcode);

            uint64_t* value = (uint64_t*)malloc(sizeof(uint64_t));
            *value = ReadUInt64(currentDataPointer);

            printf("0x%llx\n", *value);
        }
        else if (currentILOpcode == 0x2B) // br.s
        {
            printf("Opcode: 0x%02x ( BR.S ) =====> ", currentILOpcode);
            uint32_t branchTarget = (uint32_t)((int32_t)((int8_t)ReadUInt8(currentDataPointer)));
            printf("TARGET: %s0x%03x\n", (branchTarget != 0 ? (branchTarget > 0 ? "+" : "-") : ""), branchTarget);
            
        }
        else if (currentILOpcode == 0x6A) // conv.i8
        {
            printf("Opcode: 0x%02x ( CONV.I8 )\n", currentILOpcode);
        }
        else
            printf("Opcode: 0x%02x\n", currentILOpcode);
    }
}
#endif

void test_GetEntryPoint(CLIFile* cliFile)
{
    MetadataToken* token = CLIFile_ExpandMetadataToken(cliFile, cliFile->CLIHeader->EntryPointToken);
    if (token->Table != MetadataTable_MethodDefinition)
        Panic("Unknown entry point table!");
    MethodDefinition* methodDefinition = (MethodDefinition*)token->Data;
    CLIFile_DestroyMetadataToken(token);

    printf("Entry point: %s.%s.%s\n", methodDefinition->TypeDefinition->Namespace, methodDefinition->TypeDefinition->Name, methodDefinition->Name);
}

void tests(CLIFile* cliFile)
{
    //test_GetEntryPoint(cliFile);
    test_DecompileProgram(cliFile, NULL, NULL);
}


int main()
{
	uint32_t pFileLen = 0;

	FILE* f = fopen("testTypes.exe", "rb");
	fseek(f, 0, SEEK_END);
	pFileLen = ftell(f);
	fseek(f, 0, SEEK_SET);
	uint8_t* pData = (uint8_t*)malloc(pFileLen);
	fread(pData, 1, pFileLen, f);
	fclose(f);

	CLIFile* cliFile = CLIFile_Create(pData, pFileLen, "test.exe");

    tests(cliFile);

	CLIFile_Destroy(cliFile);

	free(pData);

	return 0;
}
