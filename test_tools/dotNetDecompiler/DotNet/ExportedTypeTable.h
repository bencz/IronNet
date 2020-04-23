#ifndef __DOTNET_EXPORTEDTYPETABLE_H__
#define __DOTNET_EXPORTEDTYPETABLE_H__

#include "IncFileNm.h"
#include TABLEBASE_H

namespace PEAnalyzer
{
	/// <summary>
	/// The ExportedType table holds a row for each type, defined
	/// within other modules of this Assembly, that is exported out
	/// of this Assembly. In essence, it stores TypeDef row numbers
	/// of all types that are marked public in other modules that
	/// this Assembly comprises.
	/// The rows in the ExportedType table are the result of the
	/// .class extern directive (see Section 6.7).
	/// </summary>
	class ExportedTypeTable : public TableBase
	{
	public:
		/// <summary>
		/// a 4 byte bitmask of type TypeAttributes, clause 22.1.14
		/// </summary>
		int Flags;

		/// <summary>
		/// 4 byte index into a TypeDef table of another module in this
		/// Assembly
		/// </summary>
		int TypeDefId;

		/// <summary>
		/// index into the String heap
		/// </summary>
		int TypeName;

		/// <summary>
		/// index into the String heap
		/// </summary>
		int TypeNamespace;

		/// <summary>
		/// This can be an index (more precisely, an Implementation
		/// coded index) into one of 2 tables.
		/// </summary>
		int Implementation;

		ExportedTypeTable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};
}

#endif //__DOTNET_EXPORTEDTYPETABLE_H__