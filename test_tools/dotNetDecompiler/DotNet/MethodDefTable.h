#ifndef __DOTNET_METHODDEFTABLE_H__
#define __DOTNET_METHODDEFTABLE_H__

#include "IncFileNm.h"
#include TABLEBASE_H

namespace PEAnalyzer
{
	/// <summary>
	/// The rows in the MethodDef table result from .method
	/// directives (see Chapter 14). The RVA column is computed when
	/// the image for the PE file is emitted and points to the
	/// COR_ILMETHOD structure for the body of the method (see
	/// Chapter 24.4)
	/// </summary>
	class MethodDefTable : public TableBase
	{
	public:
		/// <summary>
		/// a 4 byte constant
		/// </summary>
		int RVA;

		/// <summary>
		/// a 2 byte bitmask of type MethodImplAttributes, clause 22.1.9
		/// </summary>
		short ImplFlags;

		/// <summary>
		/// a 2 byte bitmask of type MethodAttribute, clause 22.1.9
		/// </summary>
		short Flags;

		/// <summary>
		/// index into String heap
		/// </summary>
		int Name;

		/// <summary>
		/// index into Blob heap
		/// </summary>
		int Signature;

		/// <summary>
		/// index into Param table
		/// </summary>
		int ParamList;

		MethodDefTable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};
}

#endif //__DOTNET_METHODDEFTABLE_H__