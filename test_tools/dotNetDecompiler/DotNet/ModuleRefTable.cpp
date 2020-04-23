#include "IncFileNm.h"
#include MODULEREFTABLE_H

namespace PEAnalyzer
{
	ModuleRefTable::ModuleRefTable()
	{
		initializeInstanceFields();
		_title = "ModuleRef";
	}

	void ModuleRefTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		Name = readStringsIndex();
	}

	void ModuleRefTable::getInfos(StringBuilder* sb)
	{
		//TableBase::getInfos(sb);
		//
		//sb->append(GetStringsIndexInfo(Name, "Name: index into String heap"));
	}

	void ModuleRefTable::initializeInstanceFields()
	{
		Name = 0;
	}
}
