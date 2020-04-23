#include "IncFileNm.h"
#include ASSEMBLYREFOSTABLE_H

namespace PEAnalyzer
{
	AssemblyRefOSTable::AssemblyRefOSTable()
	{
		initializeInstanceFields();
		_title = "AssemblyRefOS";
	}

	void AssemblyRefOSTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		OSPlatformId = readInt32();
		OSMajorVersion = readInt32();
		OSMinorVersion = readInt32();
		AssemblyRef = readIndex(MetadataTables::Value::ASSEMBLYREF);
	}

	void AssemblyRefOSTable::getInfos(StringBuilder* sb)
	{
		//TableBase::getInfos(sb);
		//
		//sb->append(getInfo(OSPlatformId, "OSPlatformId: 4 byte constant"));
		//sb->append(getInfo(OSMajorVersion, "OSMajorVersion: 4 byte constant"));
		//sb->append(getInfo(OSMinorVersion, "OSMinorVersion: 4 byte constant"));
		//sb->append(getInfo(AssemblyRef, MetadataTables::Value::ASSEMBLYREF, "AssemblyRef: index into the AssemblyRef table"));
	}

	void AssemblyRefOSTable::initializeInstanceFields()
	{
		OSPlatformId = 0;
		OSMajorVersion = 0;
		OSMinorVersion = 0;
		AssemblyRef = 0;
	}
}
