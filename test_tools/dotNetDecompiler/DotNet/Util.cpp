#include "IncFileNm.h"
#include UTIL_H
#include TYPES_H
#include UTF_H

namespace PEAnalyzer
{
	unsigned char Util::getByte(std::vector<unsigned char>& data, int offset)
	{
		return data[offset];
	}

	short Util::getInt16(std::vector<unsigned char>& data, int offset)
	{
		short b0 = data[offset];
		short b1 = data[offset + 1];
		return static_cast<short>(b0 + (b1 << 8));
	}

	int Util::getInt24(std::vector<unsigned char>& data, int offset)
	{
		int b0 = data[offset];
		int b1 = data[offset + 1];
		int b2 = data[offset + 2];
		return b0 + (b1 << 8) + (b2 << 16);
	}

	int Util::getInt32(std::vector<unsigned char>& data, int offset)
	{
		int b0 = data[offset];
		int b1 = data[offset + 1];
		int b2 = data[offset + 2];
		int b3 = data[offset + 3];
		return b0 + (b1 << 8) + (b2 << 16) + (b3 << 24);
	}

	long long Util::getInt64(std::vector<unsigned char>& data, int offset)
	{
		long long b0 = data[offset];
		long long b1 = data[offset + 1];
		long long b2 = data[offset + 2];
		long long b3 = data[offset + 3];
		long long b4 = data[offset + 4];
		long long b5 = data[offset + 5];
		long long b6 = data[offset + 6];
		long long b7 = data[offset + 7];

		return b0 + (b1 << 8) + (b2 << 16) + (b3 << 24) + (b4 << 32) + (b5 << 40) + (b6 << 48) + (b7 << 56);
	}

	std::vector<unsigned char> Util::getBytes(std::vector<unsigned char>& data, int offset)
	{
		int d = 0;
		for (; data[offset + d] != 0; d++)
			;
		std::vector<unsigned char> ret(data.begin() + offset, data.begin() + offset + d);
		return ret;
	}

	std::vector<unsigned char> Util::getBytes(std::vector<unsigned char>& data, int offset, int size)
	{
		std::vector<unsigned char> ret(data.begin() + offset, data.begin() + offset + size);
		return ret;
	}

	bool Util::isZero(std::vector<unsigned char>& data, int offset, int size)
	{
		for (int i = 0; i < size; i++)
		{
			if (data[offset + i] != 0)
				return false;
		}
		return true;
	}

	std::string Util::getASCIIString(long long v)
	{
		StringBuilder* sb = new StringBuilder();
		for (int i = 0; i < 8; i++)
		{
			char ch = static_cast<char>(v & 0xff);
			if (ch == '\0')
				break;
			v >>= 8;
			sb->append(ch);
		}

		std::string ret(sb->toString());
		delete sb;

		return ret;
	}

	std::string Util::getASCIIString(std::vector<unsigned char>& data, int offset)
	{
		auto retBytes = getBytes(data, offset);
		return std::string(retBytes.begin(), retBytes.end());
	}

	std::string Util::getUTF8String(std::vector<unsigned char>& data, int offset, int size)
	{
		throw "";
		//std::vector<unsigned char> bytes(size);
		//for (int i = 0; i < size; i++)
		//{
		//	unsigned char b = data[offset + i];
		//	if (b == 0)
		//	{
		//		size = i;
		//		break;
		//	}
		//	bytes[i] = b;
		//}
		//return Encoding::UTF8->GetString(bytes, 0, size);
	}

	int Util::pad(int n)
	{
		int a = n & 3;
		return (a == 0) ? n : n + 4 - a;
	}

	std::string Util::escapeText(const std::string& text)
	{
		StringBuilder* sb = new StringBuilder();
		for (std::string::const_iterator ch = text.begin(); ch != text.end(); ++ch)
		{
			if (*ch == '\t')
				sb->append("\\t");
			else if (*ch == '"')
				sb->append("\\\"");
			else if (*ch == '\r')
				sb->append("\\r");
			else if (*ch == '\n')
				sb->append("\\n");
			else
				sb->append(*ch);
		}
		return sb->toString();
	}

	DoubleInt* Util::getDataSize(std::vector<unsigned char>& data, int offset)
	{
		DoubleInt* ret = new DoubleInt();

		unsigned char b0 = data[offset];
		if ((b0 & 0xe0) == 0xc0)
		{
			ret->A = 4;
			ret->B = ((b0 & 0x1f) << 24) + (data[offset + 1] << 16) + (data[offset + 2] << 8) + data[offset + 3];
		}
		else if ((b0 & 0xc0) == 0x80)
		{
			ret->A = 2;
			ret->B = ((b0 & 0x3f) << 8) + data[offset + 1];
		}
		else
		{
			ret->A = 1;
			ret->B = b0;
		}
		return ret;
	}

	int Util::getBrTarget(ILCode* il)
	{
		switch (il->_opCode->_operandType)
		{
		case IlOpcodeOperangType::Value::SHORTINLINEBRTARGET:
		{
			int addr = il->_address + il->_opCodeLength + il->_operandLength;
			int offset = (int)((unsigned char)(il->_operand));
			if (offset >= 0x80)
				offset = offset - 0x0100;
			return addr + offset;
		}
		case IlOpcodeOperangType::Value::INLINEBRTARGET:
			return il->_address + il->_opCodeLength + il->_operandLength + (int)(il->_operand);
		}
		return 0;
	}
}
