#ifndef __DOTNET_FIELDTABLE_H__
#define __DOTNET_FIELDTABLE_H__

#include "IncFileNm.h"
#include TABLEBASE_H

namespace PEAnalyzer
{	/// <summary>
	/// Each row in the Field table results from a toplevel .field
	/// directive (see Section 5.10), or a .field directive inside a
	/// Type (see Section 9.2). For an example see Section 13.5.
	/// </summary>
	class FieldTable : public TableBase
	{
	public:
		/// <summary>
		/// a 2 byte bitmask of type FieldAttributes, clause 22.1.5
		/// </summary>
		short Flags;

		/// <summary>
		/// index into String heap
		/// </summary>
		int Name;

		/// <summary>
		/// index into Blob heap
		/// </summary>
		int Signature;

		FieldTable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};
}

#endif //__DOTNET_FIELDTABLE_H__