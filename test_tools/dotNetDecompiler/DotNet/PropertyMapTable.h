#ifndef __DOTNET_PROPERTYMAPTABLE_H__
#define __DOTNET_PROPERTYMAPTABLE_H__

#include "IncFileNm.h"
#include TABLEBASE_H

namespace PEAnalyzer
{
	/// <summary>
	/// The PropertyMap and Property tables result from putting the
	/// .property directive on a class (see Chapter 16).
	/// </summary>
	class PropertyMapTable : public TableBase
	{
	public:
		/// <summary>
		/// index into the TypeDef table
		/// </summary>
		int Parent;

		/// <summary>
		/// index into Property table
		/// </summary>
		int PropertyList;

		PropertyMapTable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};
}

#endif //__DOTNET_PROPERTYMAPTABLE_H__