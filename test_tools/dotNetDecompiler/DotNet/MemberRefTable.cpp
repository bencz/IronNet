#include "IncFileNm.h"
#include MEMBERREFTABLE_H
#include CODEDINDICES_H

namespace PEAnalyzer
{
	MemberRefTable::MemberRefTable()
	{
		initializeInstanceFields();
		_title = "MemberRef";
	}

	void MemberRefTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		Class = readIndex(CodedIndices::Value::MEMBERREFPARENT);
		Name = readStringsIndex();
		Signature = readBlobIndex();
	}

	void MemberRefTable::getInfos(StringBuilder* sb)
	{
		//TableBase::getInfos(sb);
		//
		//sb->append(GetInfo(Class, MEMBERREFPARENT, "Class: index into the TypeRef, ModuleRef, MethodDef, TypeSpec or TypeDef tables; more precisely, a MemberRefParent coded index"));
		//sb->append(GetStringsIndexInfo(Name, "Name: index into String heap"));
		//sb->append(GetBlobIndexInfo(Signature, "Signature: index into Blob heap"));
	}

	void MemberRefTable::initializeInstanceFields()
	{
		Class = 0;
		Name = 0;
		Signature = 0;
	}
}
