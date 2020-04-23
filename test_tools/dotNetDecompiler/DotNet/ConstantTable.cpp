#include "IncFileNm.h"
#include CONSTANTTABLE_H
#include CODEDINDICES_H

namespace PEAnalyzer
{
	ConstantTable::ConstantTable()
	{
		initializeInstanceFields();
		_title = "Constant";
	}

	void ConstantTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		Type = readByte();
		Parent = readIndex(CodedIndices::Value::HASCONSTANT);
		Value = readBlobIndex();
	}

	void ConstantTable::getInfos(StringBuilder* sb)
	{
		//TableBase::getInfos(sb);
		//
		//sb->append(GetInfo(Type, "Type: a 1 byte constant, followed by a 1-byte padding zero"));
		//sb->append(GetInfo(Parent, HASCONSTANT, "Parent: index into the Param or Field or Property table; more precisely, a HasConstant coded index"));
		//sb->append(GetBlobIndexInfo(Value, "Value: index into Blob heap"));
	}

	void ConstantTable::initializeInstanceFields()
	{
		Type = 0;
		Parent = 0;
		Value = 0;
	}
}
