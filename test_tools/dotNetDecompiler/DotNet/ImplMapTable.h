#ifndef __DOTNET_IMPLMAPTABLE_H__
#define __DOTNET_IMPLMAPTABLE_H__

#include "IncFileNm.h"
#include TABLEBASE_H

namespace PEAnalyzer
{
	/// <summary>
	/// A row is entered in the ImplMap table for each parent Method
	/// (see Section 14.5) that is defined with a .pinvokeimpl
	/// interoperation attribute specifying the MappingFlags,
	/// ImportName and ImportScope. For an example see Section 14.5.
	/// </summary>
	class ImplMapTable : public TableBase
	{
	public:
		/// <summary>
		/// a 2 byte bitmask of type PInvokeAttributes, clause 22.1.7
		/// </summary>
		short MappingFlags;

		/// <summary>
		/// index into the Field or MethodDef table; more precisely, a
		/// MemberForwarded coded index
		/// </summary>
		int MemberForwarded;

		/// <summary>
		/// index into the String heap
		/// </summary>
		int ImportName;

		/// <summary>
		/// index into the ModuleRef table
		/// </summary>
		int ImportScope;

		ImplMapTable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};
}

#endif //__DOTNET_IMPLMAPTABLE_H__