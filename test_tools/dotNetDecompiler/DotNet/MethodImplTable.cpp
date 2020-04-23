#include "IncFileNm.h"
#include METHODIMPLTABLE_H
#include CODEDINDICES_H
#include METADATATABLES_H

namespace PEAnalyzer
{
	MethodImplTable::MethodImplTable()
	{
		initializeInstanceFields();
		_title = "MethodImpl";
	}

	void MethodImplTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		Class = readIndex(MetadataTables::Value::TYPEDEF);
		MethodBody = readIndex(CodedIndices::Value::METHODDEFORREF);
		MethodDeclaration = readIndex(MetadataTables::Value::METHODDEF);
	}

	void MethodImplTable::getInfos(StringBuilder* sb)
	{
		//TableBase::getInfos(sb);
		//
		//sb->append(GetInfo(Class, TYPEDEF, "Class: index into TypeDef table"));
		//sb->append(GetInfo(MethodBody, METHODDEFORREF, "MethodBody: index into MethodDef or MemberRef table; more precisely, a MethodDefOrRef coded index"));
		//sb->append(GetInfo(MethodDeclaration, METHODDEF, "MethodDeclaration: index into MethodDef or MemberRef table; more precisely, a\nMethodDefOrRef coded index"));
	}

	void MethodImplTable::initializeInstanceFields()
	{
		Class = 0;
		MethodBody = 0;
		MethodDeclaration = 0;
	}
}
