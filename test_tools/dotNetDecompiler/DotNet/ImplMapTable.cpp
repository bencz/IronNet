#include "IncFileNm.h"
#include IMPLMAPTABLE_H
#include CODEDINDICES_H
#include METADATATABLES_H

namespace PEAnalyzer
{
	ImplMapTable::ImplMapTable()
	{
		initializeInstanceFields();
		_title = "ImplMap";
	}

	void ImplMapTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		MappingFlags = readInt16();
		MemberForwarded = readIndex(CodedIndices::Value::MEMBERFORWARDED);
		ImportName = readStringsIndex();
		ImportScope = readIndex(MetadataTables::Value::MODULEREF);
	}

	void ImplMapTable::getInfos(StringBuilder* sb)
	{
		//TableBase::getInfos(sb);
		//
		//sb->append(GetInfo(MappingFlags, "MappingFlags: a 2 byte bitmask of type PInvokeAttributes, clause 22.1.7"));
		//sb->append(GetInfo(MemberForwarded, MEMBERFORWARDED, "MemberForwarded: index into the Field or MethodDef table; more precisely, a MemberForwarded coded index"));
		//sb->append(GetStringsIndexInfo(ImportName, "ImportName: index into the String heap"));
		//sb->append(GetInfo(ImportScope, MODULEREF, "ImportScope: index into the ModuleRef table"));
	}

	void ImplMapTable::initializeInstanceFields()
	{
		MappingFlags = 0;
		MemberForwarded = 0;
		ImportName = 0;
		ImportScope = 0;
	}
}
