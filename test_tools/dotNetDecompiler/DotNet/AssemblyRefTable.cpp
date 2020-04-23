#include "IncFileNm.h"
#include ASSEMBLYREFTABLE_H

namespace PEAnalyzer
{
	AssemblyRefTable::AssemblyRefTable()
	{
		initializeInstanceFields();
		_title = "AssemblyRef";
	}

	void AssemblyRefTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		MajorVersion = readInt16();
		MinorVersion = readInt16();
		BuildNumber = readInt16();
		RevisionNumber = readInt16();
		Flags = readInt32();
		PublicKeyOrToken = readBlobIndex();
		Name = readStringsIndex();
		Culture = readStringsIndex();
		HashValue = readBlobIndex();
	}

	void AssemblyRefTable::getInfos(StringBuilder* sb)
	{
		//TableBase::getInfos(sb);
		//
		//sb->append(GetInfo(MajorVersion, "MajorVersion: 2 byte constants"));
		//sb->append(GetInfo(MinorVersion, "MinorVersion: 2 byte constants"));
		//sb->append(GetInfo(BuildNumber, "BuildNumber: 2 byte constants"));
		//sb->append(GetInfo(RevisionNumber, "RevisionNumber: 2 byte constants"));
		//sb->append(GetInfo(Flags, "Flags: a 4 byte bitmask of type AssemblyFlags, clause 22.1.2"));
		//sb->append(GetBlobIndexInfo(PublicKeyOrToken, "PublicKeyOrToken: index into Blob heap -- the public key or token that identifies the author of this Assembly"));
		//sb->append(GetStringsIndexInfo(Name, "Name: index into String heap"));
		//sb->append(GetStringsIndexInfo(Culture, "Culture: index into String heap"));
		//sb->append(GetBlobIndexInfo(HashValue, "HashValue: index into Blob heap"));
	}

	void AssemblyRefTable::initializeInstanceFields()
	{
		MajorVersion = 0;
		MinorVersion = 0;
		BuildNumber = 0;
		RevisionNumber = 0;
		Flags = 0;
		PublicKeyOrToken = 0;
		Name = 0;
		Culture = 0;
		HashValue = 0;
	}
}
