#include "IncFileNm.h"
#include PARAMTABLE_H

namespace PEAnalyzer
{
	ParamTable::ParamTable()
	{
		initializeInstanceFields();
		_title = "Param";
	}

	void ParamTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		Flags = readInt16();
		Sequence = readInt16();
		Name = readStringsIndex();
	}

	void ParamTable::getInfos(StringBuilder* sb)
	{
		//TableBase::getInfos(sb);
		//
		//sb->append(GetInfo(Flags, "Flags: a 2 byte bitmask of type ParamAttributes, clause 22.1.12"));
		//sb->append(GetInfo(Sequence, "Sequence: a 2 byte constant"));
		//sb->append(GetStringsIndexInfo(Name, "Name: index into String heap"));
	}

	void ParamTable::initializeInstanceFields()
	{
		Flags = 0;
		Sequence = 0;
		Name = 0;
	}
}
