#ifndef __DOTNET_ASSEMBLYREFTABLE_H__
#define __DOTNET_ASSEMBLYREFTABLE_H__

#include "IncFileNm.h"
#include TABLEBASE_H

namespace PEAnalyzer
{
	/// <summary>
	/// The table is defined by the .assembly extern directive (see
	/// Section 6.3). Its columns are filled using directives
	/// similar to those of the Assembly table except for the
	/// PublicKeyOrToken column which is defined using the
	/// .publickeytoken directive. For an example see Section 6.3.
	/// </summary>
	class AssemblyRefTable : public TableBase
	{
	public:
		/// <summary>
		/// 2 byte constants
		/// </summary>
		short MajorVersion;

		/// <summary>
		/// 2 byte constants
		/// </summary>
		short MinorVersion;

		/// <summary>
		/// 2 byte constants
		/// </summary>
		short BuildNumber;

		/// <summary>
		/// 2 byte constants
		/// </summary>
		short RevisionNumber;

		/// <summary>
		/// a 4 byte bitmask of type AssemblyFlags, clause 22.1.2
		/// </summary>
		int Flags;

		/// <summary>
		/// index into Blob heap -- the public key or token that
		/// identifies the author of this Assembly
		/// </summary>
		int PublicKeyOrToken;

		/// <summary>
		/// index into String heap
		/// </summary>
		int Name;

		/// <summary>
		/// index into String heap
		/// </summary>
		int Culture;

		/// <summary>
		/// index into Blob heap
		/// </summary>
		int HashValue;

		AssemblyRefTable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};
}

#endif //__DOTNET_ASSEMBLYREFTABLE_H__