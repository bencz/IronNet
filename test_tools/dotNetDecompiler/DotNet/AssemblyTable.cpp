#include "IncFileNm.h"
#include ASSEMBLYTABLE_H

namespace PEAnalyzer
{
	AssemblyTable::AssemblyTable()
	{
		initializeInstanceFields();
		_title = "Assembly";
	}

	void AssemblyTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		HashAlgId = readInt32();
		MajorVersion = readInt16();
		MinorVersion = readInt16();
		BuildNumber = readInt16();
		RevisionNumber = readInt16();
		Flags = readInt32();
		PublicKey = readBlobIndex();
		Name = readStringsIndex();
		Culture = readStringsIndex();
	}

	void AssemblyTable::getInfos(StringBuilder* sb)
	{
		//TableBase::GetInfos(sb);
		//
		//sb->append(GetInfo(HashAlgId, "HashAlgId: a 4 byte constant of type AssemblyHashAlgorithm, clause 22.1.1"));
		//sb->append(GetInfo(MajorVersion, "MajorVersion: 2 byte constants"));
		//sb->append(GetInfo(MinorVersion, "MinorVersion: 2 byte constants"));
		//sb->append(GetInfo(BuildNumber, "BuildNumber: 2 byte constants"));
		//sb->append(GetInfo(RevisionNumber, "RevisionNumber: 2 byte constants"));
		//sb->append(GetInfo(Flags, "Flags: a 4 byte bitmask of type AssemblyFlags, clause 22.1.2"));
		//sb->append(GetBlobIndexInfo(PublicKey, "PublicKey: index into Blob heap"));
		//sb->append(GetStringsIndexInfo(Name, "Name: index into String heap"));
		//sb->append(GetStringsIndexInfo(Culture, "Culture: index into String heap"));
	}

	void AssemblyTable::initializeInstanceFields()
	{
		HashAlgId = 0;
		MajorVersion = 0;
		MinorVersion = 0;
		BuildNumber = 0;
		RevisionNumber = 0;
		Flags = 0;
		PublicKey = 0;
		Name = 0;
		Culture = 0;
	}
}
