#ifndef __DOTNET_TYPEDEFTABLE_H__
#define __DOTNET_TYPEDEFTABLE_H__

#include "IncFileNm.h"
#include TABLEBASE_H

namespace PEAnalyzer
{
	class TypeDefTable : public TableBase
	{
	public:
		/// <summary>
		/// a 4 byte bitmask of type TypeAttributes, clause 22.1.14
		/// </summary>
		int Flags;

		/// <summary>
		/// index into String heap
		/// </summary>
		int Name;

		/// <summary>
		/// index into String heap
		/// </summary>
		int Namespace;

		/// <summary>
		/// index into TypeDef, TypeRef or TypeSpec table; more
		/// precisely, a TypeDefOrRef coded index
		/// </summary>
		int Extends;

		/// <summary>
		/// index into Field table; it marks the first of a continguous
		/// run of Fields owned by this Type
		/// </summary>
		int FieldList;

		/// <summary>
		/// index into MethodDef table
		/// </summary>
		int MethodList;

		TypeDefTable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};
}

#endif //__DOTNET_TYPEDEFTABLE_H__