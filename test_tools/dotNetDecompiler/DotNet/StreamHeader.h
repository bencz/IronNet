#ifndef __DOTNET_STREAMHEADER_H__
#define __DOTNET_STREAMHEADER_H__

#include <string>
#include <vector>
#include "HeaderBase.h"
#include STRINGBUILDER_H


namespace PEAnalyzer
{
	class MetadataRoot;
	class DoubleInt;
}

namespace PEAnalyzer
{
	class StreamHeader : public HeaderBase
	{
	public:
		int _offset;
		int _size;
		std::string _name;
		MetadataRoot* _mdroot;

		StreamHeader();
		~StreamHeader();

		virtual void readData(std::vector<unsigned char>& data, int offset);
		int getStreamSize();
		int getDataOffset();
		DoubleInt* getDataSize(int offset);

		//virtual void GetInfos(StringBuilder* sb);
	};
}

#endif // __DOTNET_STREAMHEADER_H__