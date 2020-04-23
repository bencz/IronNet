#include "IncFileNm.h"
#include MODULETABLE_H

namespace PEAnalyzer
{
	ModuleTable::ModuleTable()
	{
		initializeInstanceFields();
		_title = "Module";
	}

	void ModuleTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		Generation = readInt16();
		Name = readStringsIndex();
		Mvid = readGUIDIndex();
		EncId = readGUIDIndex();
		EncBaseId = readGUIDIndex();
	}

	void ModuleTable::getInfos(StringBuilder* sb)
	{
		//TableBase::getInfos(sb);
		//
		//sb->append(GetInfo(Generation, "Generation: 2 byte value, reserved, shall be zero"));
		//sb->append(GetStringsIndexInfo(Name, "Name: index into String heap"));
		//sb->append(GetGUIDIndexInfo(Mvid, "Mvid: index into Guid heap; simply a Guid used to distinguish between two versions of the same module"));
		//sb->append(GetGUIDIndexInfo(EncId, "EncId: index into Guid heap, reserved, shall be zero"));
		//sb->append(GetGUIDIndexInfo(EncBaseId, "EncBaseId: index into Guid heap, reserved, shall be zero"));
	}

	void ModuleTable::initializeInstanceFields()
	{
		Generation = 0;
		Name = 0;
		Mvid = 0;
		EncId = 0;
		EncBaseId = 0;
	}
}
