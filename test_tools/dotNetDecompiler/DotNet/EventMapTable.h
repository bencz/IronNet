#ifndef __DOTNET_EVENTMAPTABLE_H__
#define __DOTNET_EVENTMAPTABLE_H__

#include "IncFileNm.h"
#include TABLEBASE_H

namespace PEAnalyzer
{
	/// <summary>
	/// Note that EventMap info does not directly influence runtime
	/// behavior; what counts is the info stored for each method
	/// that the event comprises.
	/// </summary>
	class EventMapTable : public TableBase
	{
	public:
		/// <summary>
		/// index into the TypeDef table
		/// </summary>
		int Parent;

		/// <summary>
		/// index into Event table
		/// </summary>
		int EventList;

		EventMapTable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};
}

#endif //__DOTNET_EVENTMAPTABLE_H__