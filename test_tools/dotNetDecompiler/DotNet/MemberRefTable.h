#ifndef __DOTNET_MEMBERREFTABLE_H__
#define __DOTNET_MEMBERREFTABLE_H__

#include "IncFileNm.h"
#include TABLEBASE_H

namespace PEAnalyzer
{
	/// <summary>
	/// An entry is made into the MemberRef table whenever a
	/// reference is made, in the CIL code, to a method or field
	/// which is defined in another module or assembly. (Also, an
	/// entry is made for a call to a method with a VARARG
	/// signature, even when it is defined in the same module as the
	/// callsite)
	/// </summary>
	class MemberRefTable : public TableBase
	{
	public:
		/// <summary>
		/// index into the TypeRef, ModuleRef, MethodDef, TypeSpec or
		/// TypeDef tables; more precisely, a MemberRefParent coded
		/// index
		/// </summary>
		int Class;

		/// <summary>
		/// index into String heap
		/// </summary>
		int Name;

		/// <summary>
		/// index into Blob heap
		/// </summary>
		int Signature;

		MemberRefTable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};
}

#endif //__DOTNET_MEMBERREFTABLE_H__