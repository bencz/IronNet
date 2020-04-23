#include "IncFileNm.h"
#include METHODSEMANTICSTABLE_H
#include CODEDINDICES_H
#include METADATATABLES_H

namespace PEAnalyzer
{
	MethodSemanticsTable::MethodSemanticsTable()
	{
		initializeInstanceFields();
		_title = "MethodSemantics";
	}

	void MethodSemanticsTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		Semantics = readInt16();
		Method = readIndex(MetadataTables::Value::METHODDEF);
		Association = readIndex(CodedIndices::Value::HASSEMANTICS);
	}

	void MethodSemanticsTable::getInfos(StringBuilder* sb)
	{
		//TableBase::getInfos(sb);
		//
		//sb->append(GetInfo(Semantics, "Semantics: a 2 byte bitmask of type MethodSemanticsAttributes, clause 22.1.11"));
		//sb->append(GetInfo(Method, METHODDEF, "Method: index into the MethodDef table"));
		//sb->append(GetInfo(Association, HASSEMANTICS, "Association: index into the Event or Property table; more precisely, a HasSemantics coded index"));
	}

	void MethodSemanticsTable::initializeInstanceFields()
	{
		Semantics = 0;
		Method = 0;
		Association = 0;
	}
}
