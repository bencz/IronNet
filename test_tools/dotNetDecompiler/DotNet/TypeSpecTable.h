#ifndef __DOTNET_TYPESPECTABLE_H__
#define __DOTNET_TYPESPECTABLE_H__

#include "IncFileNm.h"
#include TABLEBASE_H

namespace PEAnalyzer
{
	/// <summary>
	/// The TypeSpec table has just one column, which indexes the
	/// specification of a Type, stored in the Blob heap. This
	/// provides a metadata token for that Type (rather than simply
	/// an index into the Blob heap) -- this is required, typically,
	/// for array operations creating, or calling methods on the
	/// array class.
	/// </summary>
	class TypeSpecTable : public TableBase
	{
	public:
		/// <summary>
		/// index into the Blob heap, where the blob is formatted as
		/// specified in clause 22.2.14
		/// </summary>
		int Signature;

		TypeSpecTable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};
}

#endif //__DOTNET_TYPESPECTABLE_H__