#include "IncFileNm.h"
#include TABLEBASE_H
#include UTIL_H
#include INDEXMANAGER_H
#include CONSTANTTABLE_H

namespace PEAnalyzer
{
	TableBase::TableBase()
	{
		_ptr = 0;
		_name = "";
		_index = 0;
	}

	TableBase::~TableBase()
	{
		delete _indexManager;
		delete _parentTable;
		delete _tag;
	}

	void TableBase::readData(std::vector<unsigned char>& data, int offset)
	{
		HeaderBase::readData(data, offset);
		this->_ptr = this->_offset;
	}

	void TableBase::getInfos(StringBuilder* sb)
	{
		HeaderBase::getInfos(sb);
		this->_ptr = this->_offset;
	}

	int TableBase::getSize()
	{
		return this->_ptr - this->_offset;
	}

	unsigned char TableBase::readByte()
	{
		unsigned char ret = this->_data[this->_ptr];
		if (dynamic_cast<ConstantTable*>(this) != 0 && this->_offset == this->_ptr)
			this->_ptr++;
		this->_ptr++;
		return ret;

		//throw "TableBase::readByte()";
	}

	short TableBase::readInt16()
	{
		short ret = Util::getInt16(this->_data, this->_ptr);
		this->_ptr += 2;
		return ret;
	}

	int TableBase::readInt32()
	{
		int ret = Util::getInt32(this->_data, this->_ptr);
		this->_ptr += 4;
		return ret;
	}

	int TableBase::readStringsIndex()
	{
		return this->_indexManager->_strings ? this->readInt32() : this->readInt16();
	}

	int TableBase::readGUIDIndex()
	{
		return this->_indexManager->_guid ? this->readInt32() : this->readInt16();
	}

	int TableBase::readBlobIndex()
	{
		return this->_indexManager->_blob ? this->readInt32() : this->readInt16();
	}

	int TableBase::readIndex(MetadataTables::Value table)
	{
		return this->_indexManager->isInt32(table) ? this->readInt32() : this->readInt16();
	}
	
	int TableBase::readIndex(CodedIndices::Value index)
	{
		return this->_indexManager->isInt32(index) ? this->readInt32() : this->readInt16();
	}
}
