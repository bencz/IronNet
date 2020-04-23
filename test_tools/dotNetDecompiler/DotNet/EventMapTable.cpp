#include "IncFileNm.h"
#include EVENTMAPTABLE_H
#include METADATATABLES_H

namespace PEAnalyzer
{
	EventMapTable::EventMapTable()
	{
		initializeInstanceFields();
		_title = "EventMap";
	}

	void EventMapTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		Parent = readIndex(MetadataTables::Value::TYPEDEF);
		EventList = readIndex(MetadataTables::Value::EVENT);
	}

	void EventMapTable::getInfos(StringBuilder* sb)
	{
		//TableBase::getInfos(sb);
		//
		//sb->append(GetInfo(Parent, TYPEDEF, "Parent: index into the TypeDef table"));
		//sb->append(GetInfo(EventList, EVENT, "EventList: index into Event table"));
	}

	void EventMapTable::initializeInstanceFields()
	{
		Parent = 0;
		EventList = 0;
	}
}
