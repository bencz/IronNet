#ifndef __DOTNET_ASSEMBLYOSTABLE_H__
#define __DOTNET_ASSEMBLYOSTABLE_H__

#include "IncFileNm.h"
#include TABLEBASE_H

namespace PEAnalyzer
{
	/// <summary>
	/// This record should not be emitted into any PE file. If
	/// present in a PE file, it should be treated as if all its
	/// fields were zero. It should be ignored by the CLI.
	/// </summary>
	class AssemblyOSTable : public TableBase
	{
	public:
		/// <summary>
		/// a 4 byte constant
		/// </summary>
		int OSPlatformID;

		/// <summary>
		/// a 4 byte constant
		/// </summary>
		int OSMajorVersion;

		/// <summary>
		/// a 4 byte constant
		/// </summary>
		int OSMinorVersion;

		AssemblyOSTable();
		virtual void readData(std::vector<unsigned char>& data, int offset);
		virtual void getInfos(StringBuilder* sb);

	private:
		void InitializeInstanceFields();
	};
}


#endif //__DOTNET_ASSEMBLYOSTABLE_H__