#ifndef __DOTNET_INTERFACEIMPLTABLE_H__
#define __DOTNET_INTERFACEIMPLTABLE_H__

#include "IncFileNm.h"
#include TABLEBASE_H

namespace PEAnalyzer
{
	/// <summary>
	/// The InterfaceImpl table records which interfaces a Type
	/// implements. Conceptually, each row in the InterfaceImpl
	/// table says that Class implements Interface.
	/// </summary>
	class InterfaceImplTable : public TableBase
	{
	public:
		/// <summary>
		/// index into the TypeDef table
		/// </summary>
		int Class;

		/// <summary>
		/// index into the TypeDef, TypeRef or TypeSpec table; more
		/// precisely, a TypeDefOrRef coded index
		/// </summary>
		int Interface;

		InterfaceImplTable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};
}

#endif //__DOTNET_INTERFACEIMPLTABLE_H__