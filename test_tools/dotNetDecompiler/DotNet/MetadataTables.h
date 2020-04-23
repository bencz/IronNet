#ifndef __DOTNET_METADATATABLES_H__
#define __DOTNET_METADATATABLES_H__

namespace PEAnalyzer
{
	struct MetadataTables
	{
		enum Value
		{
			MODULE = 0x00,
			TYPEREF = 0x01,
			TYPEDEF = 0x02,
			FIELD = 0x04,
			METHODDEF = 0x06,
			PARAM = 0x08,
			INTERFACEIMPL = 0x09,
			MEMBERREF = 0x0A,
			CONSTANT = 0x0B,
			CUSTOMATTRIBUTE = 0x0C,
			FIELDMARSHAL = 0x0D,
			DECLSECURITY = 0x0E,
			CLASSLAYOUT = 0x0F,
			FIELDLAYOUT = 0x10,
			STANDALONESIG = 0x11,
			EVENTMAP = 0x12,
			EVENT = 0x14,
			PROPERTYMAP = 0x15,
			PROPERTY = 0x17,
			METHODSEMANTICS = 0x18,
			METHODIMPL = 0x19,
			MODULEREF = 0x1A,
			TYPESPEC = 0x1B,
			IMPLMAP = 0x1C,
			FIELDRVA = 0x1D,
			ASSEMBLY = 0x20,
			ASSEMBLYPROCESSOR = 0x21,
			ASSEMBLYOS = 0x22,
			ASSEMBLYREF = 0x23,
			ASSEMBLYREFPROCESSOR = 0x24,
			ASSEMBLYREFOS = 0x25,
			FILE = 0x26,
			EXPORTEDTYPE = 0x27,
			MANIFESTRESOURCE = 0x28,
			NESTEDCLASS = 0x29,
			NOTUSED = 0x3f
		};
	};

	static const MetadataTables::Value AllKeysMetadataTable[] =
	{ 
		MetadataTables::MODULE,
		MetadataTables::TYPEREF,
		MetadataTables::TYPEDEF,
		MetadataTables::FIELD,
		MetadataTables::METHODDEF,
		MetadataTables::PARAM,
		MetadataTables::INTERFACEIMPL,
		MetadataTables::MEMBERREF,
		MetadataTables::CONSTANT,
		MetadataTables::CUSTOMATTRIBUTE,
		MetadataTables::FIELDMARSHAL,
		MetadataTables::DECLSECURITY,
		MetadataTables::CLASSLAYOUT,
		MetadataTables::FIELDLAYOUT,
		MetadataTables::STANDALONESIG,
		MetadataTables::EVENTMAP,
		MetadataTables::EVENT,
		MetadataTables::PROPERTYMAP,
		MetadataTables::PROPERTY,
		MetadataTables::METHODSEMANTICS,
		MetadataTables::METHODIMPL,
		MetadataTables::MODULEREF,
		MetadataTables::TYPESPEC,
		MetadataTables::IMPLMAP,
		MetadataTables::FIELDRVA,
		MetadataTables::ASSEMBLY,
		MetadataTables::ASSEMBLYPROCESSOR,
		MetadataTables::ASSEMBLYOS,
		MetadataTables::ASSEMBLYREF,
		MetadataTables::ASSEMBLYREFPROCESSOR,
		MetadataTables::ASSEMBLYREFOS,
		MetadataTables::FILE,
		MetadataTables::EXPORTEDTYPE,
		MetadataTables::MANIFESTRESOURCE,
		MetadataTables::NESTEDCLASS,
		MetadataTables::NOTUSED
	};

	class MetadataTable
	{
	public:
		static bool Contains(MetadataTables::Value value)
		{
			for (int i = 0; i < 36; i++)
				if (AllKeysMetadataTable[i] == value)
					return true;
			return false;
		}
	};


}




#endif // __DOTNET_METADATATABLES_H__