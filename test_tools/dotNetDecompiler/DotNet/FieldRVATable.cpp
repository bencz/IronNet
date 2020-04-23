#include "IncFileNm.h"
#include FIELDRVATABLE_H
#include METADATATABLES_H

namespace PEAnalyzer
{
	FieldRVATable::FieldRVATable()
	{
		initializeInstanceFields();
		_title = "FieldRVA";
	}

	void FieldRVATable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		RVA = readInt32();
		Field = readIndex(MetadataTables::Value::FIELD);
	}

	void FieldRVATable::getInfos(StringBuilder* sb)
	{
		//TableBase::getInfos(sb);
		//
		//sb->append(GetInfo(RVA, "RVA: a 4 byte constant"));
		//sb->append(GetInfo(Field, FIELD, "Field: index into Field table"));
	}

	void FieldRVATable::initializeInstanceFields()
	{
		RVA = 0;
		Field = 0;
	}
}
