#ifndef __DOTNET_TYPEREFTABLE_H__
#define __DOTNET_TYPEREFTABLE_H__

#include "IncFileNm.h"
#include TABLEBASE_H

namespace PEAnalyzer
{
	class TypeRefTable : public TableBase
	{
	public:
		/// <summary>
		/// index into Module, ModuleRef, AssemblyRef or TypeRef tables,
		/// or null; more precisely, a ResolutionScope coded index
		/// </summary>
		int ResolutionScope;

		/// <summary>
		/// index into String heap
		/// </summary>
		int Name;

		/// <summary>
		/// index into String heap
		/// </summary>
		int Namespace;

		TypeRefTable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};
}

#endif //__DOTNET_TYPEREFTABLE_H__