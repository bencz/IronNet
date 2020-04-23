#include "IncFileNm.h"
#include CLASSLAYOUTTABLE_H
#include METADATATABLES_H

namespace PEAnalyzer
{
	ClassLayoutTable::ClassLayoutTable()
	{
		initializeInstanceFields();
		_title = "ClassLayout";
	}

	void ClassLayoutTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		PackingSize = readInt16();
		ClassSize = readInt32();
		Parent = readIndex(MetadataTables::Value::TYPEDEF);
	}

	void ClassLayoutTable::getInfos(StringBuilder* sb)
	{
		//TableBase::getInfos(sb);
		//
		//sb->append(GetInfo(PackingSize, "PackingSize: a 2 byte constant"));
		//sb->append(GetInfo(ClassSize, "ClassSize: a 4 byte constant"));
		//sb->append(GetInfo(Parent, TYPEDEF, "Parent: index into TypeDef table"));
	}

	void ClassLayoutTable::initializeInstanceFields()
	{
		PackingSize = 0;
		ClassSize = 0;
		Parent = 0;
	}
}
