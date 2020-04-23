#include "IncFileNm.h"
#include ASSEMBLYOSTABLE_H

namespace PEAnalyzer
{
	AssemblyOSTable::AssemblyOSTable()
	{
		InitializeInstanceFields();
		_title = "AssemblyOS";
	}

	void AssemblyOSTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		OSPlatformID = readInt32();
		OSMajorVersion = readInt32();
		OSMinorVersion = readInt32();
	}

	void AssemblyOSTable::getInfos(StringBuilder* sb)
	{
		//TableBase::getInfos(sb);
		//
		//sb->append(getInfo(OSPlatformID, "OSPlatformID: a 4 byte constant"));
		//sb->append(getInfo(OSMajorVersion, "OSMajorVersion: a 4 byte constant"));
		//sb->append(getInfo(OSMinorVersion, "OSMinorVersion: a 4 byte constant"));
	}

	void AssemblyOSTable::InitializeInstanceFields()
	{
		OSPlatformID = 0;
		OSMajorVersion = 0;
		OSMinorVersion = 0;
	}
}