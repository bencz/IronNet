#include "IncFileNm.h"
#include PROPERTYTABLE_H

namespace PEAnalyzer
{
	PropertyTable::PropertyTable()
	{
		initializeInstanceFields();
		_title = "Property";
	}

	void PropertyTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		Flags = readInt16();
		Name = readStringsIndex();
		Type = readBlobIndex();
	}

	void PropertyTable::getInfos(StringBuilder* sb)
	{
		//TableBase::getInfos(sb);
		//
		//sb->append(GetInfo(Flags, "Flags: a 2 byte bitmask of type PropertyAttributes, clause 22.1.13"));
		//sb->append(GetStringsIndexInfo(Name, "Name: index into String heap"));
		//sb->append(GetBlobIndexInfo(Type, "Type: index into Blob heap"));
	}

	void PropertyTable::initializeInstanceFields()
	{
		Flags = 0;
		Name = 0;
		Type = 0;
	}
}
