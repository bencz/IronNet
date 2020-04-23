#include "IncFileNm.h"
#include NESTEDCLASSTABLE_H
#include METADATATABLES_H

namespace PEAnalyzer
{
	NestedClassTable::NestedClassTable()
	{
		initializeInstanceFields();
		_title = "NestedClass";
	}

	void NestedClassTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		NestedClass = readIndex(MetadataTables::Value::TYPEDEF);
		EnclosingClass = readIndex(MetadataTables::Value::TYPEDEF);
	}

	void NestedClassTable::getInfos(StringBuilder* sb)
	{
		//TableBase::getInfos(sb);
		//
		//sb->append(GetInfo(NestedClass, TYPEDEF, "NestedClass: index into the TypeDef table"));
		//sb->append(GetInfo(EnclosingClass, TYPEDEF, "EnclosingClass: index into the TypeDef table"));
	}

	void NestedClassTable::initializeInstanceFields()
	{
		NestedClass = 0;
		EnclosingClass = 0;
	}
}
