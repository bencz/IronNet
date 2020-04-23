#include "IncFileNm.h"
#include FIELDLAYOUTTABLE_H
#include METADATATABLES_H

namespace PEAnalyzer
{
	FieldLayoutTable::FieldLayoutTable()
	{
		initializeInstanceFields();
		_title = "FieldLayout";
	}

	void FieldLayoutTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		Offset = readInt32();
		Field = readIndex(MetadataTables::Value::FIELD);
	}

	void FieldLayoutTable::getInfos(StringBuilder* sb)
	{
		//TableBase::getInfos(sb);
		//
		//sb->append(GetInfo(Offset, "Offset: a 4 byte constant"));
		//sb->append(GetInfo(Field, FIELD, "Field: index into the Field table"));
	}

	void FieldLayoutTable::initializeInstanceFields()
	{
		Offset = 0;
		Field = 0;
	}
}
