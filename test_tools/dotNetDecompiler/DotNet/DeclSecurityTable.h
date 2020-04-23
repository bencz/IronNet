#ifndef __DOTNET_DECLSECURITYTABLE_H__
#define __DOTNET_DECLSECURITYTABLE_H__

#include "IncFileNm.h"
#include TABLEBASE_H

namespace PEAnalyzer
{
	/// <summary>
	/// The rows of the DeclSecurity table are filled by attaching a
	/// .permission or .permissionset directive that specifies the
	/// Action and PermissionSet on a parent assembly (see Section
	/// 6.6) or parent type or method (see Section 9.2).
	/// </summary>
	class DeclSecurityTable : public TableBase
	{
	public:
		/// <summary>
		/// 2 byte value
		/// </summary>
		short Action;

		/// <summary>
		/// index into the TypeDef, MethodDef or Assembly table
		/// </summary>
		int Parent;

		/// <summary>
		/// index into Blob heap
		/// </summary>
		int PermissionSet;

		DeclSecurityTable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};
}

#endif //__DOTNET_DECLSECURITYTABLE_H__