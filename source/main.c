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

        if (currentILOpcode == 0x28) // Call
        {
            MetadataToken* token = CLIFile_ExpandMetadataToken(cliFile, ReadUInt32(currentDataPointer));
            MethodDefinition* methodDefinition = (MethodDefinition*)token->Data;

            printf("Opcode: 0x%02x ( CALL  ) =====> %s.%s.%s\n", currentILOpcode, methodDefinition->TypeDefinition->Namespace, methodDefinition->TypeDefinition->Name, methodDefinition->Name);

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

            uint32_t realStringSize = (uint32_t*)(strLength >> 1);
            printf("Opcode: 0x%02x ( LDSTR ) =====> ", currentILOpcode);
            for (int i = 0; i < strLength; i++)
            {
                if (*(str) == 0)
                {
                    *(str)++;
                    continue;
                }

                printf("%c", *(str)++);
            }
            printf("\n");
        }
        else if (currentILOpcode == 0x2A) // Ret
        {
            if(callingMethod != NULL)
                printf("Opcode: 0x%02x ( RET   ) =====> FROM ( %s.%s.%s ) TO ( %s.%s.%s )\n", currentILOpcode, 
                    method->TypeDefinition->Namespace, method->TypeDefinition->Name, method->Name,
                    callingMethod->TypeDefinition->Namespace, callingMethod->TypeDefinition->Name, callingMethod->Name);
            else
                printf("Opcode: 0x%02x\n", currentILOpcode);
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
