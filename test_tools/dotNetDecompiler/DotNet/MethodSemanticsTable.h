#ifndef __DOTNET_METHODSEMANTICSTABLE_H__
#define __DOTNET_METHODSEMANTICSTABLE_H__

#include "IncFileNm.h"
#include TABLEBASE_H

namespace PEAnalyzer
{
	/// <summary>
	/// The rows of the MethodSemantics table are filled by
	/// .property (see Chapter 16) and .event directives (see
	/// Chapter 17). See clause 21.13 for more information.
	/// </summary>
	class MethodSemanticsTable : public TableBase
	{
	public:
		/// <summary>
		/// a 2 byte bitmask of type MethodSemanticsAttributes, clause
		/// 22.1.11
		/// </summary>
		short Semantics;

		/// <summary>
		/// index into the MethodDef table
		/// </summary>
		int Method;

		/// <summary>
		/// index into the Event or Property table; more precisely, a
		/// HasSemantics coded index
		/// </summary>
		int Association;

		MethodSemanticsTable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};
}

#endif //__DOTNET_METHODSEMANTICSTABLE_H__