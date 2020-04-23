#include "IncFileNm.h"
#include PROPERTYMAPTABLE_H
#include METADATATABLES_H

namespace PEAnalyzer
{
	PropertyMapTable::PropertyMapTable()
	{
		initializeInstanceFields();
		_title = "PropertyMap";
	}

	void PropertyMapTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		Parent = readIndex(MetadataTables::Value::TYPEDEF);
		PropertyList = readIndex(MetadataTables::Value::PROPERTY);
	}

	void PropertyMapTable::getInfos(StringBuilder* sb)
	{
		//TableBase::getInfos(sb);
		//
		//sb->append(GetInfo(Parent, TYPEDEF, "Parent: index into the TypeDef table"));
		//sb->append(GetInfo(PropertyList, PROPERTY, "PropertyList: index into Property table"));
	}

	void PropertyMapTable::initializeInstanceFields()
	{
		Parent = 0;
		PropertyList = 0;
	}
}
