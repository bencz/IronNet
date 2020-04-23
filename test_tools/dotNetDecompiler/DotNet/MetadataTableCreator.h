#ifndef __DOTNET_METADATATABLECREATOR_H__
#define __DOTNET_METADATATABLECREATOR_H__

#include "IncFileNm.h"
#include TABLEBASE_H
#include METADATATABLES_H 
#include METADATAINCLUDES_H

namespace PEAnalyzer
{
	class MetadataTableCreator
	{
	public:
		static TableBase* Create(MetadataTables::Value tbl)
		{
			if (tbl == MetadataTables::Value::ASSEMBLY)
				return new AssemblyTable();
			else if (tbl == MetadataTables::Value::ASSEMBLYOS)
				return new AssemblyOSTable();
			else if (tbl == MetadataTables::Value::ASSEMBLYPROCESSOR)
				return new AssemblyProcessorTable();
			else if (tbl == MetadataTables::Value::ASSEMBLYREF)
				return new AssemblyRefTable();
			else if (tbl == MetadataTables::Value::ASSEMBLYREFOS)
				return new AssemblyRefOSTable();
			else if (tbl == MetadataTables::Value::ASSEMBLYREFPROCESSOR)
				return new AssemblyRefProcessorTable();
			else if (tbl == MetadataTables::Value::CLASSLAYOUT)
				return new ClassLayoutTable();
			else if (tbl == MetadataTables::Value::CONSTANT)
				return new ConstantTable();
			else if (tbl == MetadataTables::Value::CUSTOMATTRIBUTE)
				return new CustomAttributeTable();
			else if (tbl == MetadataTables::Value::DECLSECURITY)
				return new DeclSecurityTable();
			else if (tbl == MetadataTables::Value::EVENTMAP)
				return new EventMapTable();
			else if (tbl == MetadataTables::Value::EVENT)
				return new EventTable();
			else if (tbl == MetadataTables::Value::EXPORTEDTYPE)
				return new ExportedTypeTable();
			else if (tbl == MetadataTables::Value::FIELD)
				return new FieldTable();
			else if (tbl == MetadataTables::Value::FIELDLAYOUT)
				return new FieldLayoutTable();
			else if (tbl == MetadataTables::Value::FIELDMARSHAL)
				return new FieldMarshalTable();
			else if (tbl == MetadataTables::Value::FIELDRVA)
				return new FieldRVATable();
			else if (tbl == MetadataTables::Value::FILE)
				return new FileTable();
			else if (tbl == MetadataTables::Value::IMPLMAP)
				return new ImplMapTable();
			else if (tbl == MetadataTables::Value::INTERFACEIMPL)
				return new InterfaceImplTable();
			else if (tbl == MetadataTables::Value::MANIFESTRESOURCE)
				return new ManifestResourceTable();
			else if (tbl == MetadataTables::Value::MEMBERREF)
				return new MemberRefTable();
			else if (tbl == MetadataTables::Value::METHODDEF)
				return new MethodDefTable();
			else if (tbl == MetadataTables::Value::METHODIMPL)
				return new MethodImplTable();
			else if (tbl == MetadataTables::Value::METHODSEMANTICS)
				return new MethodSemanticsTable();
			else if (tbl == MetadataTables::Value::MODULE)
				return new ModuleTable();
			else if (tbl == MetadataTables::Value::MODULEREF)
				return new ModuleRefTable();
			else if (tbl == MetadataTables::Value::NESTEDCLASS)
				return new NestedClassTable();
			else if (tbl == MetadataTables::Value::PARAM)
				return new ParamTable();
			else if (tbl == MetadataTables::Value::PROPERTY)
				return new PropertyTable();
			else if (tbl == MetadataTables::Value::PROPERTYMAP)
				return new PropertyMapTable();
			else if (tbl == MetadataTables::Value::STANDALONESIG)
				return new StandAloneSigTable();
			else if (tbl == MetadataTables::Value::TYPEDEF)
				return new TypeDefTable();
			else if (tbl == MetadataTables::Value::TYPEREF)
				return new TypeRefTable();
			else if (tbl == MetadataTables::Value::TYPESPEC)
				return new TypeSpecTable();
			else
				return NULL;
		}
	};
}

#endif // __DOTNET_METADATATABLECREATOR_H__