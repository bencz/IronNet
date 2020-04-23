#ifndef __DOTNET_PEDATA_H__
#define __DOTNET_PEDATA_H__

#include <string>
#include <vector>

namespace PEAnalyzer
{
	class RVAManager;
	class IndexManager;
	class PEFileHeader;
	class PEHeaderStandardFields;
	class PEHeaderWindowsNTSpecificFields;
	class PEHeaderDataDirectories;
	class ImportTable;
	class SectionHeaders;
	class CLIHeader;
	class MetadataRoot;
	class StreamHeader;
	class SharpTildeStream;
}

namespace PEAnalyzer
{
	class PEData
	{
	public:
		std::vector<unsigned char> _data; 
		RVAManager* _rva;
		IndexManager* _idxm;
		PEFileHeader* _pe1;
		PEHeaderStandardFields* _pe2;
		PEHeaderWindowsNTSpecificFields* _pe3;
		PEHeaderDataDirectories* _pe4;
		std::vector<ImportTable*> _imptbl;
		std::vector<SectionHeaders*> _sects;
		CLIHeader* _cli;
		MetadataRoot* _mdroot;
		StreamHeader* _tilde;
		SharpTildeStream* _sharpTilde;
		StreamHeader* _strings;
		StreamHeader* _usrstr;
		StreamHeader* _guid;
		StreamHeader* _blob;

	public:
		PEData();
		~PEData();
		PEData(std::vector<unsigned char>& data);
		void setData(std::vector<unsigned char>& data);

	private:
		void init();
		void readPEHeaders();
		void readSections(int offset);
		void readCLIHeader();
		void setStreamHeader(StreamHeader* sh);
		void readTilde();
	};
}

#endif // __DOTNET_PEDATA_H__