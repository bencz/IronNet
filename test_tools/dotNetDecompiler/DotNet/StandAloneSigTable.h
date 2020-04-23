#ifndef __DOTNET_STANDALONESIGTABLE_H__
#define __DOTNET_STANDALONESIGTABLE_H__

#include "IncFileNm.h"
#include TABLEBASE_H

namespace PEAnalyzer
{
	/// <summary>
	/// Signatures are stored in the metadata Blob heap. In most
	/// cases, they are indexed by a column in some table --
	/// Field.Signature, Method.Signature, MemberRef.Signature, etc.
	/// However, there are two cases that require a metadata token
	/// for a signature that is not indexed by any metadata table.
	/// The StandAloneSig table fulfils this need. It has just one
	/// column, that points to a Signature in the Blob heap.
	/// </summary>
	class StandAloneSigTable : public TableBase
	{
	public:
		/// <summary>
		/// index into the Blob heap
		/// </summary>
		int Signature;

		StandAloneSigTable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};
}

#endif //__DOTNET_STANDALONESIGTABLE_H__