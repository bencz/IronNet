#ifndef __DOTNET_MODULETABLE_H__
#define __DOTNET_MODULETABLE_H__

#include "IncFileNm.h"
#include TABLEBASE_H

namespace PEAnalyzer
{
	/// <summary>
	/// The Generation, EncId and EncBaseId columns can be written
	/// as zero, and can be ignored by conforming implementations of
	/// the CLI. The rows in the Module table result from .module
	/// directives in the Assembly (see Section 6.4).
	/// </summary>
	class ModuleTable : public TableBase
	{
	public:
		/// <summary>
		/// 2 byte value, reserved, shall be zero
		/// </summary>	
		short Generation;

		/// <summary>
		/// index into String heap
		/// </summary>
		int Name;

		/// <summary>
		/// index into Guid heap; simply a Guid used to distinguish
		/// between two versions of the same module
		/// </summary>
		int Mvid;

		/// <summary>
		/// index into Guid heap, reserved, shall be zero
		/// </summary>
		int EncId;

		/// <summary>
		/// index into Guid heap, reserved, shall be zero
		/// </summary>
		int EncBaseId;

		ModuleTable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};
}

#endif //__DOTNET_MODULETABLE_H__