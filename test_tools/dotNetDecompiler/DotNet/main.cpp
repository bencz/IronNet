// DotNet.cpp : Defines the entry point for the console application.
//

#include <vector>
#include <fstream>
#include <iostream>
#include <map>
#include <string>

#include "Util.h"
#include "PEData.h"
#include "PEHeaders.h"
#include "IndexManager.h"
#include "MethodData.h"
#include "MetaDataIncludes.h"
#include "StreamHeader.h"
#include "OpCode.h"
#include "Types.h"
#include TABLEBASE_H

using namespace PEAnalyzer;

std::vector<unsigned char> readAllBytes(char const* filename)
{
	std::vector<unsigned char> result;


	std::ifstream in(filename, std::ios_base::binary);
	result.assign(std::istreambuf_iterator<char>(in >> std::noskipws), std::istreambuf_iterator<char>());

	return result;
}

/*void TestGetBytes()
{
	std::string result;
	auto bytes = readAllBytes("Test1.exe");

	auto result_r1 = PEAnalyzer::Util::getBytes(bytes, 0xc1a);
	result = std::string(result_r1.begin(), result_r1.end());
	if (result != "mscoree.dll")
		std::cout << "mscoree.dll ---- error" << std::endl;

	auto result_r2 = PEAnalyzer::Util::getBytes(bytes, 0x384);
	result = std::string(result_r2.begin(), result_r2.end());
	if (result != "#~")
		std::cout << "#~ ---- error" << std::endl;

	auto result_r3 = PEAnalyzer::Util::getBytes(bytes, 0x390);
	result = std::string(result_r3.begin(), result_r3.end());
	if (result != "#Strings")
		std::cout << "#Strings ---- error" << std::endl;

	auto result_r4 = PEAnalyzer::Util::getBytes(bytes, 0x3a4);
	result = std::string(result_r4.begin(), result_r4.end());
	if (result != "#US")
		std::cout << "#US ---- error" << std::endl;

	auto result_r5 = PEAnalyzer::Util::getBytes(bytes, 0x3b0);
	result = std::string(result_r5.begin(), result_r5.end());
	if (result != "#GUID")
		std::cout << "#GUID ---- error" << std::endl;

	auto result_r6 = PEAnalyzer::Util::getBytes(bytes, 0x3c0);
	result = std::string(result_r6.begin(), result_r6.end());
	if (result != "#Blob")
		std::cout << "#Blob ---- error" << std::endl;
}

void TestGetAscii()
{
	auto bytes = readAllBytes("Test1.exe");

	auto result_r1 = PEAnalyzer::Util::getASCIIString(bytes, 0xc1a);
	if (result_r1 != "mscoree.dll")
		std::cout << "mscoree.dll ---- error" << std::endl;

	auto result_r2 = PEAnalyzer::Util::getASCIIString(bytes, 0x384);
	if (result_r2 != "#~")
		std::cout << "#~ ---- error" << std::endl;

	auto result_r3 = PEAnalyzer::Util::getASCIIString(bytes, 0x390);
	if (result_r3 != "#Strings")
		std::cout << "#Strings ---- error" << std::endl;

	auto result_r4 = PEAnalyzer::Util::getASCIIString(bytes, 0x3a4);
	if (result_r4 != "#US")
		std::cout << "#US ---- error" << std::endl;

	auto result_r5 = PEAnalyzer::Util::getASCIIString(bytes, 0x3b0);
	if (result_r5 != "#GUID")
		std::cout << "#GUID ---- error" << std::endl;

	auto result_r6 = PEAnalyzer::Util::getASCIIString(bytes, 0x3c0);
	if (result_r6 != "#Blob")
		std::cout << "#Blob ---- error" << std::endl;
}

void DoTests()
{
	TestGetBytes();
	TestGetAscii();
}*/

#include "utf.h"

std::map<int, std::string> loadUsString(PEData* peData)
{
	std::map<int, std::string> ret;

	if (peData->_usrstr != NULL)
	{
		StreamHeader* stream = peData->_usrstr;
		int address = stream->getDataOffset();
		int pointer = 1;
		while (peData->_data[address + pointer] != 0 && pointer < stream->_size)
		{
			DoubleInt* dataSize = stream->getDataSize(address + pointer);
			std::vector<unsigned char> bytes = Util::getBytes(peData->_data, address + pointer + dataSize->A, dataSize->B);
			ret[pointer] = utf::convert_encoding(std::string(bytes.begin(), bytes.end() - 1), utf::encoding_type::ENCODING_UTF16LE, utf::encoding_type::ENCODING_ASCII, false);
			pointer += dataSize->A + dataSize->B;
			delete dataSize;
		}
	}

	return ret;
}

std::string getMethodName(PEData* peData, ILCode* ilCode)
{
	auto x = peData->_idxm->getTable((int)ilCode->_operand);
	auto tg = (MethodData*)x->_tag;
	return tg->get_FullName();
}

MethodData* getMethod(PEData* peData, ILCode* ilCode)
{
	auto x = peData->_idxm->getTable((int)ilCode->_operand);
	return (MethodData*)x->_tag;
}

void showOpCodes(PEAnalyzer::PEData* peData, MethodData* method, MethodData* fromMethod)
{
	CLIHeader* cli = peData->_cli;
	IndexManager* idxManager = peData->_idxm;
	MethodDefTable* m;
	MethodData* entryPoint;

	if (method == nullptr)
	{
		m = (MethodDefTable*)idxManager->getTable(cli->EntryPointToken);
		entryPoint = (MethodData*)m->_tag;
	}
	else
		entryPoint = method;

	//std::cout << "Entry point: " << entryPoint->get_FullName() << std::endl;
	std::map<int, std::string> usMap = loadUsString(peData);
	//for (std::map<int, std::string>::iterator it = usMap.begin(); it != usMap.end(); ++it)
	//	std::cout << it->first << " => " << it->second << '\n';

	std::vector<ILCode*> ilCodes = entryPoint->get_ILCodes();
	for (auto il : ilCodes)
	{

		if (il->_opCode->_name == "call")
		{
			std::string methodName = getMethodName(peData, il);

			printf("0x%02x ===> %s  :::  %s\n", il->_opCode->_value, il->_opCode->_name.c_str(), methodName.c_str());
			showOpCodes(peData, getMethod(peData, il), entryPoint);
		}
		else if (il->_opCode->_name == "ldstr")
		{
			int us = (il->_operand) && 0xffffff;
			std::string tmp = usMap[us];

			printf("0x%02x ===> %s  :::  \"%s\"\n", il->_opCode->_value, il->_opCode->_name.c_str(), tmp.c_str());
		}
		else if (il->_opCode->_name == "ret")
		{
			std::string methodName = "";
			if(fromMethod != nullptr)
				methodName = fromMethod->get_Name();

			printf("0x%02x ===> %s  :::  %s\n", il->_opCode->_value, il->_opCode->_name.c_str(), methodName.c_str());
		}
		else
			printf("0x%02x ===> %s\n", il->_opCode->_value, il->_opCode->_name.c_str());
	}
}

int main()
{
	//DoTests();
	
	auto exeBytes = readAllBytes("testTypes.exe");
	PEAnalyzer::PEData* peData = new PEAnalyzer::PEData(exeBytes);
	if (peData->_cli == NULL)
		return -1;

	showOpCodes(peData, nullptr, nullptr);
	
	return 0;
}
