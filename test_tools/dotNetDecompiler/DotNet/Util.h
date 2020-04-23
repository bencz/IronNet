#ifndef __DOTNET_UTIL_H__
#define __DOTNET_UTIL_H__

#include <string>
#include <vector>
#include "Types.h"
#include STRINGBUILDER_H

namespace PEAnalyzer
{
	class DoubleInt;
	class ILCode;
}

namespace PEAnalyzer
{
	class Util
	{
	public:
		static unsigned char getByte(std::vector<unsigned char>& data, int offset);
		static short getInt16(std::vector<unsigned char>& data, int offset);
		static int getInt24(std::vector<unsigned char>& data, int offset);
		static int getInt32(std::vector<unsigned char>& data, int offset);
		static long long getInt64(std::vector<unsigned char>& data, int offset);
		static std::vector<unsigned char> getBytes(std::vector<unsigned char>& data, int offset);
		static std::vector<unsigned char> getBytes(std::vector<unsigned char>& data, int offset, int size);
		static bool isZero(std::vector<unsigned char>& data, int offset, int size);
		static std::string getASCIIString(long long v);
		static std::string getASCIIString(std::vector<unsigned char>& data, int offset);
		static std::string getUTF8String(std::vector<unsigned char>& data, int offset, int size);
		static int pad(int n);
		static std::string escapeText(const std::string& text);
		static DoubleInt* getDataSize(std::vector<unsigned char>& data, int offset);
		static int getBrTarget(ILCode* il);
	};
}


#endif // __DOTNET_UTIL_H__