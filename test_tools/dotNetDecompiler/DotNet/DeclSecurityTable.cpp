#include "IncFileNm.h"
#include DECLSECURITYTABLE_H
#include METADATATABLES_H

namespace PEAnalyzer
{
	DeclSecurityTable::DeclSecurityTable()
	{
		initializeInstanceFields();
		_title = "DeclSecurity";
	}

	void DeclSecurityTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		Action = readInt16();
		Parent = readIndex(MetadataTables::Value::METHODDEF);
		PermissionSet = readBlobIndex();
	}

	void DeclSecurityTable::getInfos(StringBuilder* sb)
	{
		//TableBase::getInfos(sb);
		//
		//sb->append(GetInfo(Action, "Action: 2 byte value"));
		//sb->append(GetInfo(Parent, METHODDEF, "Parent: index into the TypeDef, MethodDef or Assembly table"));
		//sb->append(GetBlobIndexInfo(PermissionSet, "PermissionSet: index into Blob heap"));
	}

	void DeclSecurityTable::initializeInstanceFields()
	{
		Action = 0;
		Parent = 0;
		PermissionSet = 0;
	}
}
