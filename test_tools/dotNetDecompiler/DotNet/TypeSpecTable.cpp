#include "IncFileNm.h"
#include TYPESPECTABLE_H

namespace PEAnalyzer
{
	TypeSpecTable::TypeSpecTable()
	{
		initializeInstanceFields();
		_title = "TypeSpec";
	}

	void TypeSpecTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		Signature = readBlobIndex();
	}

	void TypeSpecTable::getInfos(StringBuilder* sb)
	{
		//TableBase::getInfos(sb);
		//
		//sb->append(GetBlobIndexInfo(Signature, "Signature: index into the Blob heap, where the blob is formatted as specified in clause 22.2.14"));
	}

	void TypeSpecTable::initializeInstanceFields()
	{
		Signature = 0;
	}
}
