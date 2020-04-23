#include "IncFileNm.h"
#include ASSEMBLYPROCESSORTABLE_H

namespace PEAnalyzer
{
	AssemblyProcessorTable::AssemblyProcessorTable()
	{
		initializeInstanceFields();
		_title = "AssemblyProcessor";
	}

	void AssemblyProcessorTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);
		Processor = readInt32();
	}

	void AssemblyProcessorTable::getInfos(StringBuilder* sb)
	{
		//TableBase::getInfos(sb);
		//
		//sb->append(getInfo(Processor, "Processor: a 4 byte constant"));
	}

	void AssemblyProcessorTable::initializeInstanceFields()
	{
		Processor = 0;
	}
}
