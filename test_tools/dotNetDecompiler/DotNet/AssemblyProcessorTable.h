#ifndef __DOTNET_ASSEMBLYPROCESSORTABLE_H__
#define __DOTNET_ASSEMBLYPROCESSORTABLE_H__

#include "IncFileNm.h"
#include TABLEBASE_H

namespace PEAnalyzer
{
	/// <summary>
	/// This record should not be emitted into any PE file. If
	/// present in a PE file, it should be treated as if its field
	/// were zero. It should be ignored by the CLI.
	/// </summary>
	class AssemblyProcessorTable : public TableBase
	{
	public:
		/// <summary>
		/// a 4 byte constant
		/// </summary>
		int Processor;

		AssemblyProcessorTable();
		virtual void readData(std::vector<unsigned char>& data, int offset);
		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};
}

#endif //__DOTNET_ASSEMBLYPROCESSORTABLE_H__