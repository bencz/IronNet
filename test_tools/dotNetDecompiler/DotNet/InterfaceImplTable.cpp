#include "IncFileNm.h"
#include INTERFACEIMPLTABLE_H
#include CODEDINDICES_H
#include METADATATABLES_H

namespace PEAnalyzer
{
	InterfaceImplTable::InterfaceImplTable()
	{
		initializeInstanceFields();
		_title = "InterfaceImpl";
	}

	void InterfaceImplTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		Class = readIndex(MetadataTables::Value::TYPEDEF);
		Interface = readIndex(CodedIndices::Value::TYPEDEFORREF);
	}

	void InterfaceImplTable::getInfos(StringBuilder* sb)
	{
		//TableBase::getInfos(sb);
		//
		//sb->append(GetInfo(Class, TYPEDEF, "Class: index into the TypeDef table"));
		//sb->append(GetInfo(Interface, TYPEDEFORREF, "Interface: index into the TypeDef, TypeRef or TypeSpec table; more precisely, a TypeDefOrRef coded index"));
	}

	void InterfaceImplTable::initializeInstanceFields()
	{
		Class = 0;
		Interface = 0;
	}
}
