#ifndef __DOTNET_EVENTTABLE_H__
#define __DOTNET_EVENTTABLE_H__

#include "IncFileNm.h"
#include TABLEBASE_H

namespace PEAnalyzer
{
	/// <summary>
	/// Events are treated within metadata much like Properties -- a
	/// way to associate a collection of methods defined on given
	/// class. There are two required methods -- add_ and remove_,
	/// plus optional raise_ and others. All of the methods gathered
	/// together as an Event shall be defined on the class.
	/// Note that Event information does not directly influence
	/// runtime behavior; what counts is the information stored for
	/// each method that the event comprises.
	/// The EventMap and Event tables result from putting the .event
	/// directive on a class (see Chapter 17).
	/// </summary>
	class EventTable : public TableBase
	{
		/// <summary>
		/// a 2 byte bitmask of type EventAttribute, clause 22.1.4
		/// </summary>
	public:
		short EventFlags;

		/// <summary>
		/// index into String heap
		/// </summary>
		int Name;

		/// <summary>
		/// index into TypeDef, TypeRef or TypeSpec tables; more
		/// precisely, a TypeDefOrRef coded index
		/// </summary>
		int EventType;

		EventTable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};
}

#endif //__DOTNET_EVENTTABLE_H__