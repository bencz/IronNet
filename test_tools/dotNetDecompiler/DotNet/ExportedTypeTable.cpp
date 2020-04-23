#include "IncFileNm.h"
#include EXPORTEDTYPETABLE_H
#include CODEDINDICES_H
#include METADATATABLES_H

namespace PEAnalyzer
{
	ExportedTypeTable::ExportedTypeTable()
	{
		initializeInstanceFields();
		_title = "ExportedType";
	}

	void ExportedTypeTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		Flags = readInt32();
		TypeDefId = readIndex(MetadataTables::Value::TYPEDEF);
		TypeName = readStringsIndex();
		TypeNamespace = readStringsIndex();
		Implementation = readIndex(CodedIndices::Value::IMPLEMENTATION);
	}

	void ExportedTypeTable::getInfos(StringBuilder* sb)
	{
		//TableBase::getInfos(sb);
		//
		//sb->append(GetInfo(Flags, "Flags: a 4 byte bitmask of type TypeAttributes, clause 22.1.14"));
		//sb->append(GetInfo(TypeDefId, TYPEDEF, "TypeDefId: 4 byte index into a TypeDef table of another module in this Assembly"));
		//sb->append(GetStringsIndexInfo(TypeName, "TypeName: index into the String heap"));
		//sb->append(GetStringsIndexInfo(TypeNamespace, "TypeNamespace: index into the String heap"));
		//sb->append(GetInfo(Implementation, IMPLEMENTATION, "Implementation: This can be an index (more precisely, an Implementation coded index) into one of 2 tables."));
	}

	void ExportedTypeTable::initializeInstanceFields()
	{
		Flags = 0;
		TypeDefId = 0;
		TypeName = 0;
		TypeNamespace = 0;
		Implementation = 0;
	}
}
