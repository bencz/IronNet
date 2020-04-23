#include "IncFileNm.h"
#include CUSTOMATTRIBUTETABLE_H
#include CODEDINDICES_H

namespace PEAnalyzer
{
	CustomAttributeTable::CustomAttributeTable()
	{
		initializeInstanceFields();
		_title = "CustomAttribute";
	}

	void CustomAttributeTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		Parent = readIndex(CodedIndices::Value::HASCUSTOMATTRIBUTE);
		Type = readIndex(CodedIndices::Value::CUSTOMATTRIBUTETYPE);
		Value = readBlobIndex();
	}

	void CustomAttributeTable::getInfos(StringBuilder* sb)
	{
		//TableBase::getInfos(sb);
		//
		//sb->append(GetInfo(Parent, HASCUSTOMATTRIBUTE, "Parent: index into any metadata table, except the CustomAttribute table itself; more precisely, a HasCustomAttribute coded index"));
		//sb->append(GetInfo(Type, CUSTOMATTRIBUTETYPE, "Type: index into the MethodDef or MethodRef table; more precisely, a CustomAttributeType coded index"));
		//sb->append(GetBlobIndexInfo(Value, "Value: index into Blob heap"));
	}

	void CustomAttributeTable::initializeInstanceFields()
	{
		Parent = 0;
		Type = 0;
		Value = 0;
	}
}
