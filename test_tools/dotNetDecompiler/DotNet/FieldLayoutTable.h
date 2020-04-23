#ifndef __DOTNET_FIELDLAYOUTTABLE_H__
#define __DOTNET_FIELDLAYOUTTABLE_H__

#include "IncFileNm.h"
#include TABLEBASE_H

namespace PEAnalyzer
{
	/// <summary>
	/// A row in the FieldLayout table is created if the .field
	/// directive for the parent field has specified a field offset
	/// (see Section 9.7).
	/// </summary>
	class FieldLayoutTable : public TableBase
	{
	public:
		/// <summary>
		/// a 4 byte constant
		/// </summary>
		int Offset;

		/// <summary>
		/// index into the Field table
		/// </summary>
		int Field;

		FieldLayoutTable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};
}

#endif //__DOTNET_FIELDLAYOUTTABLE_H__