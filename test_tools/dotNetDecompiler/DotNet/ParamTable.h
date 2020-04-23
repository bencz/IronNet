#ifndef __DOTNET_PARAMTABLE_H__
#define __DOTNET_PARAMTABLE_H__

#include "IncFileNm.h"
#include TABLEBASE_H

namespace PEAnalyzer
{
	/// <summary>
	/// The rows in the Param table result from the parameters in a
	/// method declaration (see Section 14.4), or from a .param
	/// attribute attached to a method (see clause 14.4.1).
	/// </summary>
	class ParamTable : public TableBase
	{
	public:
		/// <summary>
		/// a 2 byte bitmask of type ParamAttributes, clause 22.1.12
		/// </summary>
		short Flags;

		/// <summary>
		/// a 2 byte constant
		/// </summary>
		short Sequence;

		/// <summary>
		/// index into String heap
		/// </summary>
		int Name;

		ParamTable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};
}

#endif //__DOTNET_PARAMTABLE_H__