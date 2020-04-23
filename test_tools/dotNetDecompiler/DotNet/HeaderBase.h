#ifndef __DOTNET_HEADERBASE_H__
#define __DOTNET_HEADERBASE_H__

#include <string>
#include <vector>
#include "IncFileNm.h"
#include STRINGBUILDER_H

namespace PEAnalyzer
{
	class HeaderBase
	{
	protected:
		std::string _title;
		std::vector<unsigned char> _data;
		int _offset;

	public:
		HeaderBase();
		virtual void readData(std::vector<unsigned char>& data, int offset);
		void readDataDummy(std::vector<unsigned char>& data, int offset);
		virtual void getInfos(StringBuilder* sb);
		std::string getTitle();
		void appendTitle(const std::string& text);
		int getOffset();


	protected:
		unsigned char readByte(int offset);
		short readInt16(int offset);
		int readInt32(int offset);
		long long readInt64(int offset);
		std::vector<unsigned char> readBytes(int offset, int size);

		std::string getInfo(int offset, unsigned char v, const std::string& desc);
		std::string getInfo(int offset, short v, const std::string& desc);
		std::string getInfo(int offset, int v, const std::string& desc);
		std::string getInfo(int offset, long long v, const std::string& desc);
		//std::string getInfo(int offset, std::vector<unsigned char>& v,
		//	                const std::string& desc);
	};
}

#endif // __DOTNET_HEADERBASE_H__
