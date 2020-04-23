#ifndef __DOTNET_MODULEREFTABLE_H__
#define __DOTNET_MODULEREFTABLE_H__

#include "IncFileNm.h"
#include TABLEBASE_H

namespace PEAnalyzer
{
	/// <summary>
	/// The rows in the ModuleRef table result from .module extern
	/// directives in the Assembly (see Section 6.5).
	/// </summary>
	class ModuleRefTable : public TableBase
	{
	public:
		/// <summary>
		/// index into String heap
		/// </summary>
		int Name;

		ModuleRefTable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};
}

#endif //__DOTNET_MODULEREFTABLE_H__