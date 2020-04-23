#ifndef __DOTNET_ASSEMBLYTABLE_H__
#define __DOTNET_ASSEMBLYTABLE_H__

#include "IncFileNm.h"
#include TABLEBASE_H

namespace PEAnalyzer
{
	/// <summary>
	/// The Assembly table is defined using the .assembly directive
	/// (see Section 6.2); its columns are obtained from the
	/// respective .hash algorithm, .ver, .publickey, and .culture
	/// (see clause 6.2.1 For an example see Section 6.2.
	/// </summary>
	class AssemblyTable : public TableBase
	{
	public:
		/// <summary>
		/// a 4 byte constant of type AssemblyHashAlgorithm, clause
		/// 22.1.1
		/// </summary>
		int HashAlgId;

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
		/// index into Blob heap
		/// </summary>
		int PublicKey;

		/// <summary>
		/// index into String heap
		/// </summary>
		int Name;

		/// <summary>
		/// index into String heap
		/// </summary>
		int Culture;

		AssemblyTable();
		virtual void readData(std::vector<unsigned char>& data, int offset);
		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};
}


#endif // __DOTNET_ASSEMBLYTABLE_H__