#ifndef __DOTNET_NESTEDCLASSTABLE_H__
#define __DOTNET_NESTEDCLASSTABLE_H__

#include "IncFileNm.h"
#include TABLEBASE_H

namespace PEAnalyzer
{
	/// <summary>
	/// The NestedClass table records which Type definitions are
	/// nested within which other Type definition. In a typical
	/// high-level language, including ilasm, the nested class is
	/// defined as lexically 'inside' the text of its enclosing
	/// Type.
	/// </summary>
	class NestedClassTable : public TableBase
	{
	public:
		/// <summary>
		/// index into the TypeDef table
		/// </summary>
		int NestedClass;

		/// <summary>
		/// index into the TypeDef table
		/// </summary>
		int EnclosingClass;

		NestedClassTable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};
}

#endif //__DOTNET_NESTEDCLASSTABLE_H__