#include "IncFileNm.h"
#include STANDALONESIGTABLE_H

namespace PEAnalyzer
{
	StandAloneSigTable::StandAloneSigTable()
	{
		initializeInstanceFields();
		_title = "StandAloneSig";
	}

	void StandAloneSigTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		Signature = readBlobIndex();
	}

	void StandAloneSigTable::getInfos(StringBuilder* sb)
	{
		//TableBase::getInfos(sb);
		//
		//sb->append(GetBlobIndexInfo(Signature, "Signature: index into the Blob heap"));
	}

	void StandAloneSigTable::initializeInstanceFields()
	{
		Signature = 0;
	}
}
