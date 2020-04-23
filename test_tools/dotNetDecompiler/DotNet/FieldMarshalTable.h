#ifndef __DOTNET_FIELDMARSHALTABLE_H__
#define __DOTNET_FIELDMARSHALTABLE_H__

#include "IncFileNm.h"
#include TABLEBASE_H

namespace PEAnalyzer
{
	/// <summary>
	/// A row in the FieldMarshal table is created if the .field
	/// directive for the parent field has specified a .marshall
	/// attribute (see Section 15.1).
	/// </summary>
	class FieldMarshalTable : public TableBase
	{
	public:
		/// <summary>
		/// index into Field or Param table; more precisely, a
		/// HasFieldMarshal coded index
		/// </summary>
		int Parent;

		/// <summary>
		/// index into the Blob heap
		/// </summary>
		int NativeType;

		FieldMarshalTable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};
}

#endif //__DOTNET_FIELDMARSHALTABLE_H__