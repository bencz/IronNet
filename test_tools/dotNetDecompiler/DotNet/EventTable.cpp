#include "IncFileNm.h"
#include EVENTTABLE_H
#include CODEDINDICES_H

namespace PEAnalyzer
{
	EventTable::EventTable()
	{
		initializeInstanceFields();
		_title = "Event";
	}

	void EventTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		EventFlags = readInt16();
		Name = readStringsIndex();
		EventType = readIndex(CodedIndices::Value::TYPEDEFORREF);
	}

	void EventTable::getInfos(StringBuilder* sb)
	{
		//TableBase::getInfos(sb);
		//
		//sb->append(GetInfo(EventFlags, "EventFlags: a 2 byte bitmask of type EventAttribute, clause 22.1.4"));
		//sb->append(GetStringsIndexInfo(Name, "Name: index into String heap"));
		//sb->append(GetInfo(EventType, TYPEDEFORREF, "EventType: index into TypeDef, TypeRef or TypeSpec tables; more precisely, a TypeDefOrRef coded index"));
	}

	void EventTable::initializeInstanceFields()
	{
		EventFlags = 0;
		Name = 0;
		EventType = 0;
	}
}
