#include "IncFileNm.h"
#include MANIFESTRESOURCETABLE_H
#include CODEDINDICES_H

namespace PEAnalyzer
{
	ManifestResourceTable::ManifestResourceTable()
	{
		initializeInstanceFields();
		_title = "ManifestResource";
	}

	void ManifestResourceTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		Offset = readInt32();
		Flags = readInt32();
		Name = readStringsIndex();
		Implementation = readIndex(CodedIndices::Value::IMPLEMENTATION);
	}

	void ManifestResourceTable::getInfos(StringBuilder* sb)
	{
		//TableBase::getInfos(sb);
		//
		//sb->append(GetInfo(Offset, "Offset: a 4 byte constant"));
		//sb->append(GetInfo(Flags, "Flags: a 4 byte bitmask of type ManifestResourceAttributes, clause 22.1.8"));
		//sb->append(GetStringsIndexInfo(Name, "Name: index into the String heap"));
		//sb->append(GetInfo(Implementation, IMPLEMENTATION, "Implementation: index into File table, or AssemblyRef table, or null; more precisely, an Implementation coded index"));
	}

	void ManifestResourceTable::initializeInstanceFields()
	{
		Offset = 0;
		Flags = 0;
		Name = 0;
		Implementation = 0;
	}
}
