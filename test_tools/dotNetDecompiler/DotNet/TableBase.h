#ifndef __DOTNET_TABLEBASE_H__
#define __DOTNET_TABLEBASE_H__


#include <string>
#include <vector>
#include <typeinfo>

#include "IncFileNm.h"
#include HEADERBASE_H
#include METADATATABLES_H
#include TYPES_H
#include METADATATABLES_H
#include CODEDINDICES_H

namespace PEAnalyzer
{
	class IndexManager;
}

namespace PEAnalyzer
{
	class TableBase : public HeaderBase
	{
	public:
		IndexManager* _indexManager;

	protected:
		int _ptr;
		std::string _name;

	public:
		int _index;
		TableBase* _parentTable;
		std::vector<std::vector<void*>> _children;
		void* _tag;

		TableBase();
		~TableBase();

		virtual void readData(std::vector<unsigned char>& data, int offset);
		virtual void getInfos(StringBuilder* sb);
		int getSize();

	protected:
		unsigned char readByte();
		short readInt16();
		int readInt32();
		int readStringsIndex();
		int readGUIDIndex();
		int readBlobIndex();
		int readIndex(MetadataTables::Value table);
		int readIndex(CodedIndices::Value index);

	};
}

#endif // __DOTNET_TABLEBASE_H__