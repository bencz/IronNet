#ifndef __DOTNET_CONSTANTTABLE_H__
#define __DOTNET_CONSTANTTABLE_H__

#include "IncFileNm.h"
#include TABLEBASE_H

namespace PEAnalyzer
{
	/// <summary>
	/// The Constant table is used to store compile-time, constant
	/// values for fields, parameters and properties.
	/// Note that Constant information does not directly influence
	/// runtime behavior, although it is visible via Reflection (and
	/// hence may be used to implement functionality such as that
	/// provided by System.Enum.ToString). Compilers inspect this
	/// information, at compile time, when importing metadata; but
	/// the value of the constant itself, if used, becomes embedded
	/// into the CIL stream the compiler emits. There are no CIL
	/// instructions to access the Constant table at runtime.
	/// A row in the Constant table for a parent is created whenever
	/// a compile-time value is specified for that parent, for an
	/// example see Section 15.2.
	/// </summary>
	class ConstantTable : public TableBase
	{
	public:
		/// <summary>
		/// a 1 byte constant, followed by a 1-byte padding zero
		/// </summary>
		unsigned char Type;

		/// <summary>
		/// index into the Param or Field or Property table; more
		/// precisely, a HasConstant coded index
		/// </summary>
		int Parent;

		/// <summary>
		/// index into Blob heap
		/// </summary>
		int Value;

		ConstantTable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};
}

#endif //__DOTNET_CONSTANTTABLE_H__