#ifndef __DOTNET_CUSTOMATTRIBUTETABLE_H__
#define __DOTNET_CUSTOMATTRIBUTETABLE_H__

#include "IncFileNm.h"
#include TABLEBASE_H

namespace PEAnalyzer
{
	/// <summary>
	/// The CustomAttribute table stores data that can be used to
	/// instantiate a Custom Attribute (more precisely, an object of
	/// the specified Custom Attribute class) at runtime. The column
	/// called Type is slightly misleading -- it actually indexes a
	/// constructor method -- the owner of that constructor method
	/// is the Type of the Custom Attribute.
	/// A row in the CustomAttribute table for a parent is created
	/// by the .custom attribute, which gives the value of the Type
	/// column and optionally that of the Value column (see Chapter
	/// 20)
	/// </summary>
	class CustomAttributeTable : public TableBase
	{
	public:
		/// <summary>
		/// index into any metadata table, except the CustomAttribute
		/// table itself; more precisely, a HasCustomAttribute coded
		/// index
		/// </summary>
		int Parent;

		/// <summary>
		/// index into the MethodDef or MethodRef table; more precisely,
		/// a CustomAttributeType coded index
		/// </summary>
		int Type;

		/// <summary>
		/// index into Blob heap
		/// </summary>
		int Value;

		CustomAttributeTable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};
}

#endif //__DOTNET_CUSTOMATTRIBUTETABLE_H__