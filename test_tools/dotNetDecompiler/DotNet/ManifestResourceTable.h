#ifndef __DOTNET_MANIFESTRESOURCETABLE_H__
#define __DOTNET_MANIFESTRESOURCETABLE_H__

#include "IncFileNm.h"
#include TABLEBASE_H

namespace PEAnalyzer
{
	/// <summary>
	/// The Offset specifies the byte offset within the referenced
	/// file at which this resource record begins. The
	/// Implementation specifies which file holds this resource. The
	/// rows in the table result from .mresource directives on the
	/// Assembly (see clause 6.2.2).
	/// </summary>
	class ManifestResourceTable : public TableBase
	{
	public:
		/// <summary>
		/// a 4 byte constant
		/// </summary>
		int Offset;

		/// <summary>
		/// a 4 byte bitmask of type ManifestResourceAttributes, clause
		/// 22.1.8
		/// </summary>
		int Flags;

		/// <summary>
		/// index into the String heap
		/// </summary>
		int Name;

		/// <summary>
		/// index into File table, or AssemblyRef table, or null; more
		/// precisely, an Implementation coded index
		/// </summary>
		int Implementation;

		ManifestResourceTable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};
}

#endif //__DOTNET_MANIFESTRESOURCETABLE_H__