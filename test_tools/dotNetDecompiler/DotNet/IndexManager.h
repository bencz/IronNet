#ifndef __DOTNET_INDEXMANAGER_H__
#define __DOTNET_INDEXMANAGER_H__


#include <string>
#include <vector>

#include "IncFileNm.h"
#include TYPES_H
#include METADATATABLES_H
#include CODEDINDICES_H
#include STRINGBUILDER_H

namespace PEAnalyzer
{
	class RVAManager;
	class TableBase;
	class PEData;
	class TypeDefTable;
	class MethodDefTable;
	class TypeData;
	class DoubleInt;
	class TypeRefTable;
	class MemberRefTable;
}

namespace PEAnalyzer
{
	class IndexManager
	{
	protected:
		static const int max16 = 0xffff;

	public:
		RVAManager* _rva;
		std::vector<unsigned char> _data;
		bool _strings;
		int _stringsOffset;
		int _usOffset;
		bool _guid;
		int _guidOffset;
		bool _blob;
		int _blobOffset;
		std::vector<std::vector<void*>> _tables;
		std::vector<int> _tableRows;

		IndexManager(std::vector<unsigned char>& data, RVAManager* rva); 
		~IndexManager();

		void set_HeapSizes(const unsigned char& value);
		TableBase* createTable(MetadataTables::Value table);
		std::string getStringsString(int offset);
		std::string getUSString(int offset);
		std::vector<unsigned char> getBlobBytes(int offset);
		bool isInt32(MetadataTables::Value table);
		int getTagSize(CodedIndices::Value index);
		bool isInt32(CodedIndices::Value index);
		int getIndex(CodedIndices::Value index, int v);
		MetadataTables::Value getIndexType(CodedIndices::Value index, int v);
		int getToken(CodedIndices::Value index, int v);
		TableBase* getTable(MetadataTables::Value t, int v);
		TableBase* getTable(int token);
		void makeTree(PEData* data);

	private:
		void makeTree(TypeDefTable* t1, TypeDefTable* t2);
		void makeTree(MethodDefTable* m);

	public:
		TypeData* readType(int offset);
		std::string getName(ElementType::Value e);
		std::string getName(TypeData* t);
		std::string getName(TypeDefTable* t);
		std::string getName(TypeRefTable* t);
		std::string getName(MethodDefTable* m);
		std::string getName(MemberRefTable* m);
	};
}

#endif