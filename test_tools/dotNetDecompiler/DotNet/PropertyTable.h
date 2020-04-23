#ifndef __DOTNET_PROPERTYTABLE_H__
#define __DOTNET_PROPERTYTABLE_H__

#include "IncFileNm.h"
#include TABLEBASE_H

namespace PEAnalyzer
{
	/// <summary>
	/// Properties within metadata are best viewed as a means to
	/// gather together collections of methods defined on a class,
	/// give them a name, and not much else. The methods are
	/// typically get_ and set_ methods, already defined on the
	/// class, and inserted like any other methods into the
	/// MethodDef table.
	/// </summary>
	class PropertyTable : public TableBase
	{
	public:
		/// <summary>
		/// a 2 byte bitmask of type PropertyAttributes, clause 22.1.13
		/// </summary>
		short Flags;

		/// <summary>
		/// index into String heap
		/// </summary>
		int Name;

		/// <summary>
		/// index into Blob heap
		/// </summary>
		int Type;

		PropertyTable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};
}

#endif //__DOTNET_PROPERTYTABLE_H__