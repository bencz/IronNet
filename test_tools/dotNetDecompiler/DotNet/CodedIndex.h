#ifndef __DOTNET_CODEDINDEX_H__
#define __DOTNET_CODEDINDEX_H__

#include <vector>

#include "IncFileNm.h"
#include METADATATABLES_H

namespace PEAnalyzer
{
	class CodedIndex
	{
	public:
		static void initCodedIndex();
		static std::vector< std::vector<MetadataTables::Value> > Data;
	};
}

#endif