#include "IncFileNm.h"
#include ASSEMBLYREFPROCESSORTABLE_H
#include METADATATABLES_H

namespace PEAnalyzer
{
	AssemblyRefProcessorTable::AssemblyRefProcessorTable()
	{
		initializeInstanceFields();
		_title = "AssemblyRefProcessor";
	}

	void AssemblyRefProcessorTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		Processor = readInt32();
		AssemblyRef = readIndex(MetadataTables::Value::ASSEMBLYREF);
	}

	void AssemblyRefProcessorTable::getInfos(StringBuilder* sb)
	{
		//TableBase::getInfos(sb);
		//
		//sb->append(GetInfo(Processor, "Processor: 4 byte constant"));
		//sb->append(GetInfo(AssemblyRef, ASSEMBLYREF, "AssemblyRef: index into the AssemblyRef table"));
	}

	void AssemblyRefProcessorTable::initializeInstanceFields()
	{
		Processor = 0;
		AssemblyRef = 0;
	}
}
