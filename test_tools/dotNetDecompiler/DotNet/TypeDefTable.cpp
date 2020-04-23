#include "IncFileNm.h"
#include TYPEDEFTABLE_H
#include CODEDINDICES_H	
#include METADATATABLES_H

namespace PEAnalyzer
{
	TypeDefTable::TypeDefTable()
	{
		initializeInstanceFields();
		_title = "TypeDef";
	}

	void TypeDefTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		Flags = readInt32();
		Name = readStringsIndex();
		Namespace = readStringsIndex();
		Extends = readIndex(CodedIndices::Value::TYPEDEFORREF);
		FieldList = readIndex(MetadataTables::Value::FIELD);
		MethodList = readIndex(MetadataTables::Value::METHODDEF);
	}

	void TypeDefTable::getInfos(StringBuilder* sb)
	{
		//TableBase::getInfos(sb);
		//
		//sb->append(GetInfo(Flags, "Flags: a 4 byte bitmask of type TypeAttributes, clause 22.1.14"));
		//sb->append(GetStringsIndexInfo(Name, "Name: index into String heap"));
		//sb->append(GetStringsIndexInfo(Namespace, "Namespace: index into String heap"));
		//sb->append(GetInfo(Extends, TYPEDEFORREF, "Extends: index into TypeDef, TypeRef or TypeSpec table; more precisely, a TypeDefOrRef coded index"));
		//sb->append(GetInfo(FieldList, FIELD, "FieldList: index into Field table; it marks the first of a continguous run of Fields owned by this Type"));
		//sb->append(GetInfo(MethodList, METHODDEF, "MethodList: index into MethodDef table"));
	}

	void TypeDefTable::initializeInstanceFields()
	{
		Flags = 0;
		Name = 0;
		Namespace = 0;
		Extends = 0;
		FieldList = 0;
		MethodList = 0;
	}
}
