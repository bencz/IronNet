#ifndef __DOTNET_METHODIMPLTABLE_H__
#define __DOTNET_METHODIMPLTABLE_H__

#include "IncFileNm.h"
#include TABLEBASE_H

namespace PEAnalyzer
{
	/// <summary>
	/// ilasm uses the .override directive to specify the rows of
	/// the MethodImpl table (see clause 9.3.2).
	/// </summary>
	class MethodImplTable : public TableBase
	{
	public:
		/// <summary>
		/// index into TypeDef table
		/// </summary>
		int Class;

		/// <summary>
		/// index into MethodDef or MemberRef table; more precisely, a
		/// MethodDefOrRef coded index
		/// </summary>
		int MethodBody;

		/// <summary>
		/// index into MethodDef or MemberRef table; more precisely, a
		/// MethodDefOrRef coded index
		/// </summary>
		int MethodDeclaration;

		MethodImplTable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};
}

#endif //__DOTNET_METHODIMPLTABLE_H__