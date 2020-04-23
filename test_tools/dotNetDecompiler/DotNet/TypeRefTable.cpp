#include "IncFileNm.h"
#include TYPEREFTABLE_H
#include CODEDINDICES_H	

namespace PEAnalyzer
{
	TypeRefTable::TypeRefTable()
	{
		initializeInstanceFields();
		_title = "TypeRef";
	}

	void TypeRefTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		ResolutionScope = readIndex(CodedIndices::Value::RESOLUTIONSCOPE);
		Name = readStringsIndex();
		Namespace = readStringsIndex();
	}

	void TypeRefTable::getInfos(StringBuilder* sb)
	{
		//TableBase::getInfos(sb);
		//
		//sb->append(GetInfo(ResolutionScope, RESOLUTIONSCOPE, "ResolutionScope: index into Module, ModuleRef, AssemblyRef or TypeRef tables, or null; more precisely, a ResolutionScope coded index"));
		//sb->append(GetStringsIndexInfo(Name, "Name: index into String heap"));
		//sb->append(GetStringsIndexInfo(Namespace, "Namespace: index into String heap"));
	}

	void TypeRefTable::initializeInstanceFields()
	{
		ResolutionScope = 0;
		Name = 0;
		Namespace = 0;
	}
}
