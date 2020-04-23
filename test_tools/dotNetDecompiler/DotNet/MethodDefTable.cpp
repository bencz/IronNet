#include "IncFileNm.h"
#include METHODDEFTABLE_H
#include METADATATABLES_H

namespace PEAnalyzer
{
	MethodDefTable::MethodDefTable()
	{
		initializeInstanceFields();
		_title = "MethodDef";
	}

	void MethodDefTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		RVA = readInt32();
		ImplFlags = readInt16();
		Flags = readInt16();
		Name = readStringsIndex();
		Signature = readBlobIndex();
		ParamList = readIndex(MetadataTables::Value::PARAM);
	}

	void MethodDefTable::getInfos(StringBuilder* sb)
	{
		//TableBase::getInfos(sb);
		//
		//sb->append(GetInfo(RVA, "RVA: a 4 byte constant"));
		//sb->append(GetInfo(ImplFlags, "ImplFlags: a 2 byte bitmask of type MethodImplAttributes, clause 22.1.9"));
		//sb->append(GetInfo(Flags, "Flags: a 2 byte bitmask of type MethodAttribute, clause 22.1.9"));
		//sb->append(GetStringsIndexInfo(Name, "Name: index into String heap"));
		//sb->append(GetBlobIndexInfo(Signature, "Signature: index into Blob heap"));
		//sb->append(GetInfo(ParamList, PARAM, "ParamList: index into Param table"));
	}

	void MethodDefTable::initializeInstanceFields()
	{
		RVA = 0;
		ImplFlags = 0;
		Flags = 0;
		Name = 0;
		Signature = 0;
		ParamList = 0;
	}
}
