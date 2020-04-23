#include "IncFileNm.h"
#include UTIL_H
#include HEADERBASE_H
#include STRINGBUILDER_H

namespace PEAnalyzer
{
	HeaderBase::HeaderBase()
	{
		_title = "";
		_offset = 0;
	}

	void HeaderBase::readData(std::vector<unsigned char>& data, int offset)
	{
		this->_data = data;
		this->_offset = offset;
	}

	void HeaderBase::readDataDummy(std::vector<unsigned char>& data, int offset)
	{
		this->_data = data;
		this->_offset = offset;
	}

	void HeaderBase::getInfos(StringBuilder* sb)
	{
		sb->append("  *** " + this->_title + " ***\r\n\r\n");
	}

	std::string HeaderBase::getTitle()
	{
		return this->_title;
	}

	void HeaderBase::appendTitle(const std::string& text)
	{
		this->_title += text;
	}

	int HeaderBase::getOffset()
	{
		return this->_offset;
	}

	unsigned char HeaderBase::readByte(int offset)
	{
		return this->_data[this->_offset + offset];
	}

	short HeaderBase::readInt16(int offset)
	{
		return Util::getInt16(this->_data, this->_offset + offset);
	}

	int HeaderBase::readInt32(int offset)
	{
		return Util::getInt32(this->_data, this->_offset + offset);
	}

	long long HeaderBase::readInt64(int offset)
	{
		return Util::getInt64(this->_data, this->_offset + offset);
	}

	std::vector<unsigned char> HeaderBase::readBytes(int offset, int size)
	{
		return Util::getBytes(this->_data, this->_offset + offset, size);
	}

	std::string HeaderBase::getInfo(int offset, unsigned char v, const std::string& desc)
	{
		char s[256];
		sprintf(s, "%08X:%02X%14s %s\n", this->_offset + offset, v, "", desc.c_str());
		return std::string(s);
	}
	
	std::string HeaderBase::getInfo(int offset, short v, const std::string& desc)
	{
		char s[256];
		sprintf(s, "%08X:%04X%12s %s\n", this->_offset + offset, v, "", desc.c_str());
		return std::string(s);
	}
	
	std::string HeaderBase::getInfo(int offset, int v, const std::string& desc)
	{
		char s[256];
		sprintf(s, "%08X:%08X%8s %s\n", this->_offset + offset, v, "", desc.c_str());
		return std::string(s);
	}
	
	std::string HeaderBase::getInfo(int offset, long long v, const std::string& desc)
	{
		char s[256];
		sprintf(s, "%08X:%016llX %s\n", this->_offset + offset, v, desc.c_str());
		return std::string(s);
	}
	
	//std::string HeaderBase::getInfo(int offset, std::vector<unsigned char>& v, const std::string& desc)
	//{
	//	StringBuilder* sb = new StringBuilder();
	//	for (std::vector<unsigned char>::const_iterator b = v.begin(); b != v.end(); ++b)
	//	{
	//		if (sb->length() > 0)
	//			sb->append(' ');
	//
	//		char s[3];
	//		sprintf(s, "%X", b);
	//		sb->append(std::string(s));
	//	}
	//	while (sb->length() < 16)
	//		sb->append(' ');
	//
	//	return std::string::Format("{0:X8}:{1} {2}\r\n", this->_offset + offset, sb, desc);
	//}
}