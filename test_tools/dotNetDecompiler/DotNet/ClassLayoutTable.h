#ifndef __DOTNET_CLASSLAYOUTTABLE_H__
#define __DOTNET_CLASSLAYOUTTABLE_H__

#include "IncFileNm.h"
#include TABLEBASE_H

namespace PEAnalyzer
{
	/// <summary>
	/// The ClassLayout table is used to define how the fields of a
	/// class or value type shall be laid out by the CLI (normally,
	/// the CLI is free to reorder and/or insert gaps between the
	/// fields defined for a class or value type).
	/// The rows of the ClassLayout table are defined by placing
	/// .pack and .size directives on the body of a parent type
	/// declaration (see Section 9.2). For an example see Section
	/// 9.7.
	/// </summary>
	class ClassLayoutTable : public TableBase
	{
	public:
		/// <summary>
		/// a 2 byte constant
		/// </summary>
		short PackingSize;

		/// <summary>
		/// a 4 byte constant
		/// </summary>
		int ClassSize;

		/// <summary>
		/// index into TypeDef table
		/// </summary>
		int Parent;

		ClassLayoutTable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};
}

#endif //__DOTNET_CLASSLAYOUTTABLE_H__