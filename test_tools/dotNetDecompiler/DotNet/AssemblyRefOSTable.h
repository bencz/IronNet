#ifndef __DOTNET_ASSEMBLYREFOSTABLE_H__
#define __DOTNET_ASSEMBLYREFOSTABLE_H__

#include "IncFileNm.h"
#include TABLEBASE_H

namespace PEAnalyzer
{
	/// <summary>
	/// These records should not be emitted into any PE file. If
	/// present in a PE file, they should be treated as-if their
	/// fields were zero. They should be ignored by the CLI.
	/// </summary>
	class AssemblyRefOSTable : public TableBase
	{
	public:
		/// <summary>
		/// 4 byte constant
		/// </summary>
		int OSPlatformId;

		/// <summary>
		/// 4 byte constant
		/// </summary>
		int OSMajorVersion;

		/// <summary>
		/// 4 byte constant
		/// </summary>
		int OSMinorVersion;

		/// <summary>
		/// index into the AssemblyRef table
		/// </summary>
		int AssemblyRef;

		AssemblyRefOSTable();
		virtual void readData(std::vector<unsigned char>& data, int offset);
		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};
}

#endif //__DOTNET_ASSEMBLYREFOSTABLE_H__