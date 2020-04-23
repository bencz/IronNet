#ifndef __DOTNET_FIELDRVATABLE_H__
#define __DOTNET_FIELDRVATABLE_H__

#include "IncFileNm.h"
#include TABLEBASE_H

namespace PEAnalyzer
{
	/// <summary>
	/// A row in the FieldRVA table is created for each static
	/// parent field that has specified the optional data label (see
	/// Chapter 15). The RVA column is the relative virtual address
	/// of the data in the PE file (see Section 15.3).
	/// </summary>
	class FieldRVATable : public TableBase
	{
	public:
		/// <summary>
		/// a 4 byte constant
		/// </summary>
		int RVA;

		/// <summary>
		/// index into Field table
		/// </summary>
		int Field;

		FieldRVATable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};
}

#endif //__DOTNET_FIELDRVATABLE_H__