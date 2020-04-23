#ifndef __DOTNET_FILETABLE_H__
#define __DOTNET_FILETABLE_H__

#include "IncFileNm.h"
#include TABLEBASE_H

namespace PEAnalyzer
{
	/// <summary>
	/// The rows of the File table result from .file directives in
	/// an Assembly (see clause 6.2.3)
	/// </summary>
	class FileTable : public TableBase
	{
	public:
		/// <summary>
		/// a 4 byte bitmask of type FileAttributes, clause 22.1.6
		/// </summary>
		int Flags;

		/// <summary>
		/// index into String heap
		/// </summary>
		int Name;

		/// <summary>
		/// index into Blob heap
		/// </summary>
		int HashValue;

		FileTable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};
}

#endif //__DOTNET_FILETABLE_H__