#ifndef __DOTNET_ASSEMBLYREFPROCESSORTABLE_H__
#define __DOTNET_ASSEMBLYREFPROCESSORTABLE_H__

#include "IncFileNm.h"
#include TABLEBASE_H

namespace PEAnalyzer
{
	/// <summary>
	/// These records should not be emitted into any PE file. If
	/// present in a PE file, they should be treated as-if their
	/// fields were zero. They should be ignored by the CLI.
	/// </summary>
	class AssemblyRefProcessorTable : public TableBase
	{
	public:
		/// <summary>
		/// 4 byte constant
		/// </summary>
		int Processor;

		/// <summary>
		/// index into the AssemblyRef table
		/// </summary>
		int AssemblyRef;

		AssemblyRefProcessorTable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};
}

#endif //__DOTNET_ASSEMBLYREFPROCESSORTABLE_H__