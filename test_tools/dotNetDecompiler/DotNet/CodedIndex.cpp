#include "IncFileNm.h"
#include CODEDINDEX_H

namespace PEAnalyzer
{
	std::vector< std::vector<MetadataTables::Value> > CodedIndex::Data;

	void CodedIndex::initCodedIndex()
	{
		for(int i=0;i<12;i++)
			CodedIndex::Data.push_back(std::vector<MetadataTables::Value>());

		{
			CodedIndex::Data[0].push_back(MetadataTables::Value::TYPEDEF);
			CodedIndex::Data[0].push_back(MetadataTables::Value::TYPEREF);
			CodedIndex::Data[0].push_back(MetadataTables::Value::TYPESPEC);
		}

		{
			CodedIndex::Data[1].push_back(MetadataTables::Value::FIELD);
			CodedIndex::Data[1].push_back(MetadataTables::Value::PARAM);
			CodedIndex::Data[1].push_back(MetadataTables::Value::PROPERTY);
		}

		{
			CodedIndex::Data[2].push_back(MetadataTables::Value::METHODDEF);
			CodedIndex::Data[2].push_back(MetadataTables::Value::FIELD);
			CodedIndex::Data[2].push_back(MetadataTables::Value::TYPEREF);
			CodedIndex::Data[2].push_back(MetadataTables::Value::TYPEDEF);
			CodedIndex::Data[2].push_back(MetadataTables::Value::PARAM);
			CodedIndex::Data[2].push_back(MetadataTables::Value::INTERFACEIMPL);
			CodedIndex::Data[2].push_back(MetadataTables::Value::MEMBERREF);
			CodedIndex::Data[2].push_back(MetadataTables::Value::MODULE);
			CodedIndex::Data[2].push_back(MetadataTables::Value::DECLSECURITY);
			CodedIndex::Data[2].push_back(MetadataTables::Value::PROPERTY);
			CodedIndex::Data[2].push_back(MetadataTables::Value::EVENT);
			CodedIndex::Data[2].push_back(MetadataTables::Value::STANDALONESIG);
			CodedIndex::Data[2].push_back(MetadataTables::Value::MODULEREF);
			CodedIndex::Data[2].push_back(MetadataTables::Value::TYPESPEC);
			CodedIndex::Data[2].push_back(MetadataTables::Value::ASSEMBLY);
			CodedIndex::Data[2].push_back(MetadataTables::Value::ASSEMBLYREF);
			CodedIndex::Data[2].push_back(MetadataTables::Value::FILE);
			CodedIndex::Data[2].push_back(MetadataTables::Value::EXPORTEDTYPE);
			CodedIndex::Data[2].push_back(MetadataTables::Value::MANIFESTRESOURCE);
		}

		{
			CodedIndex::Data[3].push_back(MetadataTables::Value::FIELD);
			CodedIndex::Data[3].push_back(MetadataTables::Value::PARAM);
		}

		{
			CodedIndex::Data[4].push_back(MetadataTables::Value::TYPEDEF);
			CodedIndex::Data[4].push_back(MetadataTables::Value::METHODDEF);
			CodedIndex::Data[4].push_back(MetadataTables::Value::ASSEMBLY);
		}

		{
			CodedIndex::Data[5].push_back(MetadataTables::Value::NOTUSED);
			CodedIndex::Data[5].push_back(MetadataTables::Value::TYPEREF);
			CodedIndex::Data[5].push_back(MetadataTables::Value::MODULEREF);
			CodedIndex::Data[5].push_back(MetadataTables::Value::METHODDEF);
			CodedIndex::Data[5].push_back(MetadataTables::Value::TYPESPEC);
		}

		{
			CodedIndex::Data[6].push_back(MetadataTables::Value::EVENT);
			CodedIndex::Data[6].push_back(MetadataTables::Value::PROPERTY);
		}

		{
			CodedIndex::Data[7].push_back(MetadataTables::Value::METHODDEF);
			CodedIndex::Data[7].push_back(MetadataTables::Value::MEMBERREF);
		}

		{
			CodedIndex::Data[8].push_back(MetadataTables::Value::FIELD);
			CodedIndex::Data[8].push_back(MetadataTables::Value::METHODDEF);
		}

		{
			CodedIndex::Data[9].push_back(MetadataTables::Value::FILE);
			CodedIndex::Data[9].push_back(MetadataTables::Value::ASSEMBLYREF);
			CodedIndex::Data[9].push_back(MetadataTables::Value::EXPORTEDTYPE);
		}

		{
			CodedIndex::Data[10].push_back(MetadataTables::Value::NOTUSED);
			CodedIndex::Data[10].push_back(MetadataTables::Value::NOTUSED);
			CodedIndex::Data[10].push_back(MetadataTables::Value::METHODDEF);
			CodedIndex::Data[10].push_back(MetadataTables::Value::MEMBERREF);
			CodedIndex::Data[10].push_back(MetadataTables::Value::NOTUSED);
		}

		{
			CodedIndex::Data[11].push_back(MetadataTables::Value::MODULE);
			CodedIndex::Data[11].push_back(MetadataTables::Value::MODULEREF);
			CodedIndex::Data[11].push_back(MetadataTables::Value::ASSEMBLYREF);
			CodedIndex::Data[11].push_back(MetadataTables::Value::TYPEREF);
		}
	}
}