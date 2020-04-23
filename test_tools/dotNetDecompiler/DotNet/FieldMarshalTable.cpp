#include "IncFileNm.h"
#include FIELDMARSHALTABLE_H
#include CODEDINDICES_H

namespace PEAnalyzer
{
	FieldMarshalTable::FieldMarshalTable()
	{
		initializeInstanceFields();
		_title = "FieldMarshal";
	}

	void FieldMarshalTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		Parent = readIndex(CodedIndices::Value::HASFIELDMARSHAL);
		NativeType = readBlobIndex();
	}

	void FieldMarshalTable::getInfos(StringBuilder* sb)
	{
		//TableBase::getInfos(sb);
		//
		//sb->append(GetInfo(Parent, HASFIELDMARSHAL, "Parent: index into Field or Param table; more precisely, a HasFieldMarshal coded index"));
		//sb->append(GetBlobIndexInfo(NativeType, "NativeType: index into the Blob heap"));
	}

	void FieldMarshalTable::initializeInstanceFields()
	{
		Parent = 0;
		NativeType = 0;
	}
}
