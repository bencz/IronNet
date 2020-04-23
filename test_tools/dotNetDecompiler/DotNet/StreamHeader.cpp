#include "IncFileNm.h"
#include STREAMHEADER_H
#include PEHEADERS_H
#include UTIL_H
#include TYPES_H

namespace PEAnalyzer
{
	StreamHeader::StreamHeader()
	{
		this->_offset = 0;
		this->_size = 0;
		this->_name = "";
		this->_title = "Stream Header";	
	}

	StreamHeader::~StreamHeader()
	{
		delete _mdroot;
	}

	void StreamHeader::readData(std::vector<unsigned char>& data, int offset)
	{
		HeaderBase::readData(data, offset);
		this->_offset = this->readInt32(0);
		this->_size = this->readInt32(4);
		this->_name = Util::getASCIIString(data, offset + 8);
		//this->AppendTitle(std::string::Format(" \"{0}\"", Util::EscapeText(this->Name)));
	}

	int StreamHeader::getStreamSize()
	{
		return 8 + Util::pad((int)this->_name.size() + 1);
	}

	int StreamHeader::getDataOffset()
	{
		return this->_mdroot->getOffset() + this->_offset;
	}

	DoubleInt* StreamHeader::getDataSize(int offset)
	{
		return Util::getDataSize(this->_data, offset);
	}
}