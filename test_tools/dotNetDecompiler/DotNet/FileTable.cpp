#include "IncFileNm.h"
#include FILETABLE_H

namespace PEAnalyzer
{
	FileTable::FileTable()
	{
		initializeInstanceFields();
		_title = "File";
	}

	void FileTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		Flags = readInt32();
		Name = readStringsIndex();
		HashValue = readBlobIndex();
	}

	void FileTable::getInfos(StringBuilder* sb)
	{
		//TableBase::getInfos(sb);
		//
		//sb->append(GetInfo(Flags, "Flags: a 4 byte bitmask of type FileAttributes, clause 22.1.6"));
		//sb->append(GetStringsIndexInfo(Name, "Name: index into String heap"));
		//sb->append(GetBlobIndexInfo(HashValue, "HashValue: index into Blob heap"));
	}

	void FileTable::initializeInstanceFields()
	{
		Flags = 0;
		Name = 0;
		HashValue = 0;
	}
}
