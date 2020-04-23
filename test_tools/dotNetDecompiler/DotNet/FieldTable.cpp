#include "IncFileNm.h"
#include FIELDTABLE_H

namespace PEAnalyzer
{
	FieldTable::FieldTable()
	{
		initializeInstanceFields();
		_title = "Field";
	}

	void FieldTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		Flags = readInt16();
		Name = readStringsIndex();
		Signature = readBlobIndex();
	}

	void FieldTable::getInfos(StringBuilder* sb)
	{
		//TableBase::getInfos(sb);
		//
		//sb->append(GetInfo(Flags, "Flags: a 2 byte bitmask of type FieldAttributes, clause 22.1.5"));
		//sb->append(GetStringsIndexInfo(Name, "Name: index into String heap"));
		//sb->append(GetBlobIndexInfo(Signature, "Signature: index into Blob heap"));
	}

	void FieldTable::initializeInstanceFields()
	{
		Flags = 0;
		Name = 0;
		Signature = 0;
	}
}
