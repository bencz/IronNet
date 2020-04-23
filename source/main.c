#include <stdio.h>
#include <stdlib.h>

#include "cliFile.h"


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

	MetadataToken* token = CLIFile_ExpandMetadataToken(cliFile, cliFile->CLIHeader->EntryPointToken);
	if (token->Table != MetadataTable_MethodDefinition)
		Panic("Unknown entry point table!");
	MethodDefinition* methodDefinition = (MethodDefinition*)token->Data;
	CLIFile_DestroyMetadataToken(token);

	printf("Entry point: %s.%s.%s\n", methodDefinition->TypeDefinition->Namespace, methodDefinition->TypeDefinition->Name, methodDefinition->Name);

	CLIFile_Destroy(cliFile);

	free(pData);

	return 0;
}
