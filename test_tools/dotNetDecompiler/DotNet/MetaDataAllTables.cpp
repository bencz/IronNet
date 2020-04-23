#include "MetaDataAllTables.h"
#include "MetadataTables.h"
#include "CodedIndex.h"

namespace PEAnalyzer
{
	AssemblyTable::AssemblyTable()
	{
		initializeInstanceFields();
		_title = "Assembly";
	}

	void AssemblyTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		HashAlgId = readInt32();
		MajorVersion = readInt16();
		MinorVersion = readInt16();
		BuildNumber = readInt16();
		RevisionNumber = readInt16();
		Flags = readInt32();
		PublicKey = readBlobIndex();
		Name = readStringsIndex();
		Culture = readStringsIndex();
	}

	void AssemblyTable::getInfos(StringBuilder* sb)
	{
		// TableBase::GetInfos(sb);

		// sb->append(GetInfo(HashAlgId, "HashAlgId: a 4 byte constant of type AssemblyHashAlgorithm, clause 22.1.1"));
		// sb->append(GetInfo(MajorVersion, "MajorVersion: 2 byte constants"));
		// sb->append(GetInfo(MinorVersion, "MinorVersion: 2 byte constants"));
		// sb->append(GetInfo(BuildNumber, "BuildNumber: 2 byte constants"));
		// sb->append(GetInfo(RevisionNumber, "RevisionNumber: 2 byte constants"));
		// sb->append(GetInfo(Flags, "Flags: a 4 byte bitmask of type AssemblyFlags, clause 22.1.2"));
		// sb->append(GetBlobIndexInfo(PublicKey, "PublicKey: index into Blob heap"));
		// sb->append(GetStringsIndexInfo(Name, "Name: index into String heap"));
		// sb->append(GetStringsIndexInfo(Culture, "Culture: index into String heap"));
	}

	void AssemblyTable::initializeInstanceFields()
	{
		HashAlgId = 0;
		MajorVersion = 0;
		MinorVersion = 0;
		BuildNumber = 0;
		RevisionNumber = 0;
		Flags = 0;
		PublicKey = 0;
		Name = 0;
		Culture = 0;
	}

	AssemblyOSTable::AssemblyOSTable()
	{
		initializeInstanceFields();
		_title = "AssemblyOS";
	}

	void AssemblyOSTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		OSPlatformID = readInt32();
		OSMajorVersion = readInt32();
		OSMinorVersion = readInt32();
	}

	void AssemblyOSTable::getInfos(StringBuilder* sb)
	{
		// TableBase::GetInfos(sb);

		// sb->append(GetInfo(OSPlatformID, "OSPlatformID: a 4 byte constant"));
		// sb->append(GetInfo(OSMajorVersion, "OSMajorVersion: a 4 byte constant"));
		// sb->append(GetInfo(OSMinorVersion, "OSMinorVersion: a 4 byte constant"));
	}

	void AssemblyOSTable::initializeInstanceFields()
	{
		OSPlatformID = 0;
		OSMajorVersion = 0;
		OSMinorVersion = 0;
	}

	AssemblyProcessorTable::AssemblyProcessorTable()
	{
		initializeInstanceFields();
		_title = "AssemblyProcessor";
	}

	void AssemblyProcessorTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		Processor = readInt32();
	}

	void AssemblyProcessorTable::getInfos(StringBuilder* sb)
	{
		// TableBase::GetInfos(sb);

		// sb->append(GetInfo(Processor, "Processor: a 4 byte constant"));
	}

	void AssemblyProcessorTable::initializeInstanceFields()
	{
		Processor = 0;
	}

	AssemblyRefTable::AssemblyRefTable()
	{
		initializeInstanceFields();
		_title = "AssemblyRef";
	}

	void AssemblyRefTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		MajorVersion = readInt16();
		MinorVersion = readInt16();
		BuildNumber = readInt16();
		RevisionNumber = readInt16();
		Flags = readInt32();
		PublicKeyOrToken = readBlobIndex();
		Name = readStringsIndex();
		Culture = readStringsIndex();
		HashValue = readBlobIndex();
	}

	void AssemblyRefTable::getInfos(StringBuilder* sb)
	{
		// TableBase::GetInfos(sb);

		// sb->append(GetInfo(MajorVersion, "MajorVersion: 2 byte constants"));
		// sb->append(GetInfo(MinorVersion, "MinorVersion: 2 byte constants"));
		// sb->append(GetInfo(BuildNumber, "BuildNumber: 2 byte constants"));
		// sb->append(GetInfo(RevisionNumber, "RevisionNumber: 2 byte constants"));
		// sb->append(GetInfo(Flags, "Flags: a 4 byte bitmask of type AssemblyFlags, clause 22.1.2"));
		// sb->append(GetBlobIndexInfo(PublicKeyOrToken, "PublicKeyOrToken: index into Blob heap -- the public key or token that identifies the author of this Assembly"));
		// sb->append(GetStringsIndexInfo(Name, "Name: index into String heap"));
		// sb->append(GetStringsIndexInfo(Culture, "Culture: index into String heap"));
		// sb->append(GetBlobIndexInfo(HashValue, "HashValue: index into Blob heap"));
	}

	void AssemblyRefTable::initializeInstanceFields()
	{
		MajorVersion = 0;
		MinorVersion = 0;
		BuildNumber = 0;
		RevisionNumber = 0;
		Flags = 0;
		PublicKeyOrToken = 0;
		Name = 0;
		Culture = 0;
		HashValue = 0;
	}

	AssemblyRefOSTable::AssemblyRefOSTable()
	{
		initializeInstanceFields();
		_title = "AssemblyRefOS";
	}

	void AssemblyRefOSTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		OSPlatformId = readInt32();
		OSMajorVersion = readInt32();
		OSMinorVersion = readInt32();
		AssemblyRef = readIndex(MetadataTables::Value::ASSEMBLYREF);
	}

	void AssemblyRefOSTable::getInfos(StringBuilder* sb)
	{
		// TableBase::GetInfos(sb);

		// sb->append(GetInfo(OSPlatformId, "OSPlatformId: 4 byte constant"));
		// sb->append(GetInfo(OSMajorVersion, "OSMajorVersion: 4 byte constant"));
		// sb->append(GetInfo(OSMinorVersion, "OSMinorVersion: 4 byte constant"));
		// sb->append(GetInfo(AssemblyRef, ASSEMBLYREF, "AssemblyRef: index into the AssemblyRef table"));
	}

	void AssemblyRefOSTable::initializeInstanceFields()
	{
		OSPlatformId = 0;
		OSMajorVersion = 0;
		OSMinorVersion = 0;
		AssemblyRef = 0;
	}

	AssemblyRefProcessorTable::AssemblyRefProcessorTable()
	{
		initializeInstanceFields();
		_title = "AssemblyRefProcessor";
	}

	void AssemblyRefProcessorTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		Processor = readInt32();
		AssemblyRef = readIndex(MetadataTables::Value::ASSEMBLYREF);
	}

	void AssemblyRefProcessorTable::getInfos(StringBuilder* sb)
	{
		// TableBase::GetInfos(sb);

		// sb->append(GetInfo(Processor, "Processor: 4 byte constant"));
		// sb->append(GetInfo(AssemblyRef, ASSEMBLYREF, "AssemblyRef: index into the AssemblyRef table"));
	}

	void AssemblyRefProcessorTable::initializeInstanceFields()
	{
		Processor = 0;
		AssemblyRef = 0;
	}

	ClassLayoutTable::ClassLayoutTable()
	{
		initializeInstanceFields();
		_title = "ClassLayout";
	}

	void ClassLayoutTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		PackingSize = readInt16();
		ClassSize = readInt32();
		Parent = readIndex(MetadataTables::Value::TYPEDEF);
	}

	void ClassLayoutTable::getInfos(StringBuilder* sb)
	{
		// TableBase::GetInfos(sb);

		// sb->append(GetInfo(PackingSize, "PackingSize: a 2 byte constant"));
		// sb->append(GetInfo(ClassSize, "ClassSize: a 4 byte constant"));
		// sb->append(GetInfo(Parent, TYPEDEF, "Parent: index into TypeDef table"));
	}

	void ClassLayoutTable::initializeInstanceFields()
	{
		PackingSize = 0;
		ClassSize = 0;
		Parent = 0;
	}

	ConstantTable::ConstantTable()
	{
		initializeInstanceFields();
		_title = "Constant";
	}

	void ConstantTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		Type = readByte();
		Parent = readIndex(CodedIndices::Value::HASCONSTANT);
		Value = readBlobIndex();
	}

	void ConstantTable::getInfos(StringBuilder* sb)
	{
		// TableBase::GetInfos(sb);

		// sb->append(GetInfo(Type, "Type: a 1 byte constant, followed by a 1-byte padding zero"));
		// sb->append(GetInfo(Parent, CodedIndices::HasConstant, "Parent: index into the Param or Field or Property table; more precisely, a HasConstant coded index"));
		// sb->append(GetBlobIndexInfo(Value, "Value: index into Blob heap"));
	}

	void ConstantTable::initializeInstanceFields()
	{
		Type = 0;
		Parent = 0;
		Value = 0;
	}

	CustomAttributeTable::CustomAttributeTable()
	{
		initializeInstanceFields();
		_title = "CustomAttribute";
	}

	void CustomAttributeTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		Parent = readIndex(CodedIndices::Value::HASCUSTOMATTRIBUTE);
		Type = readIndex(CodedIndices::Value::CUSTOMATTRIBUTETYPE);
		Value = readBlobIndex();
	}

	void CustomAttributeTable::getInfos(StringBuilder* sb)
	{
		// TableBase::GetInfos(sb);

		// sb->append(GetInfo(Parent, CodedIndices::HasCustomAttribute, "Parent: index into any metadata table, except the CustomAttribute table itself; more precisely, a HasCustomAttribute coded index"));
		// sb->append(GetInfo(Type, CodedIndices::CustomAttributeType, "Type: index into the MethodDef or MethodRef table; more precisely, a CustomAttributeType coded index"));
		// sb->append(GetBlobIndexInfo(Value, "Value: index into Blob heap"));
	}

	void CustomAttributeTable::initializeInstanceFields()
	{
		Parent = 0;
		Type = 0;
		Value = 0;
	}

	DeclSecurityTable::DeclSecurityTable()
	{
		initializeInstanceFields();
		_title = "DeclSecurity";
	}

	void DeclSecurityTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		Action = readInt16();
		Parent = readIndex(MetadataTables::Value::METHODDEF);
		PermissionSet = readBlobIndex();
	}

	void DeclSecurityTable::getInfos(StringBuilder* sb)
	{
		// TableBase::GetInfos(sb);

		// sb->append(GetInfo(Action, "Action: 2 byte value"));
		// sb->append(GetInfo(Parent, METHODDEF, "Parent: index into the TypeDef, MethodDef or Assembly table"));
		// sb->append(GetBlobIndexInfo(PermissionSet, "PermissionSet: index into Blob heap"));
	}

	void DeclSecurityTable::initializeInstanceFields()
	{
		Action = 0;
		Parent = 0;
		PermissionSet = 0;
	}

	EventMapTable::EventMapTable()
	{
		initializeInstanceFields();
		_title = "EventMap";
	}

	void EventMapTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		Parent = readIndex(MetadataTables::Value::TYPEDEF);
		EventList = readIndex(MetadataTables::Value::EVENT);
	}

	void EventMapTable::getInfos(StringBuilder* sb)
	{
		// TableBase::GetInfos(sb);

		// sb->append(GetInfo(Parent, TYPEDEF, "Parent: index into the TypeDef table"));
		// sb->append(GetInfo(EventList, EVENT, "EventList: index into Event table"));
	}

	void EventMapTable::initializeInstanceFields()
	{
		Parent = 0;
		EventList = 0;
	}

	EventTable::EventTable()
	{
		initializeInstanceFields();
		_title = "Event";
	}

	void EventTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		EventFlags = readInt16();
		Name = readStringsIndex();
		EventType = readIndex(CodedIndices::Value::TYPEDEFORREF);
	}

	void EventTable::getInfos(StringBuilder* sb)
	{
		// TableBase::GetInfos(sb);

		// sb->append(GetInfo(EventFlags, "EventFlags: a 2 byte bitmask of type EventAttribute, clause 22.1.4"));
		// sb->append(GetStringsIndexInfo(Name, "Name: index into String heap"));
		// sb->append(GetInfo(EventType, CodedIndices::TypeDefOrRef, "EventType: index into TypeDef, TypeRef or TypeSpec tables; more precisely, a TypeDefOrRef coded index"));
	}

	void EventTable::initializeInstanceFields()
	{
		EventFlags = 0;
		Name = 0;
		EventType = 0;
	}

	ExportedTypeTable::ExportedTypeTable()
	{
		initializeInstanceFields();
		_title = "ExportedType";
	}

	void ExportedTypeTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		Flags = readInt32();
		TypeDefId = readIndex(MetadataTables::Value::TYPEDEF);
		TypeName = readStringsIndex();
		TypeNamespace = readStringsIndex();
		Implementation = readIndex(CodedIndices::Value::IMPLEMENTATION);
	}

	void ExportedTypeTable::getInfos(StringBuilder* sb)
	{
		// TableBase::GetInfos(sb);

		// sb->append(GetInfo(Flags, "Flags: a 4 byte bitmask of type TypeAttributes, clause 22.1.14"));
		// sb->append(GetInfo(TypeDefId, TYPEDEF, "TypeDefId: 4 byte index into a TypeDef table of another module in this Assembly"));
		// sb->append(GetStringsIndexInfo(TypeName, "TypeName: index into the String heap"));
		// sb->append(GetStringsIndexInfo(TypeNamespace, "TypeNamespace: index into the String heap"));
		// sb->append(GetInfo(Implementation, CodedIndices::Implementation, "Implementation: This can be an index (more precisely, an Implementation coded index) into one of 2 tables."));
	}

	void ExportedTypeTable::initializeInstanceFields()
	{
		Flags = 0;
		TypeDefId = 0;
		TypeName = 0;
		TypeNamespace = 0;
		Implementation = 0;
	}

	FieldTable::FieldTable()
	{
		initializeInstanceFields();
		_title = "Field";
	}

	void FieldTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		Flags = readInt16();
		Name = readStringsIndex();
		Signature = readBlobIndex();
	}

	void FieldTable::getInfos(StringBuilder* sb)
	{
		// TableBase::GetInfos(sb);

		// sb->append(GetInfo(Flags, "Flags: a 2 byte bitmask of type FieldAttributes, clause 22.1.5"));
		// sb->append(GetStringsIndexInfo(Name, "Name: index into String heap"));
		// sb->append(GetBlobIndexInfo(Signature, "Signature: index into Blob heap"));
	}

	void FieldTable::initializeInstanceFields()
	{
		Flags = 0;
		Name = 0;
		Signature = 0;
	}

	FieldLayoutTable::FieldLayoutTable()
	{
		initializeInstanceFields();
		_title = "FieldLayout";
	}

	void FieldLayoutTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		Offset = readInt32();
		Field = readIndex(MetadataTables::Value::FIELD);
	}

	void FieldLayoutTable::getInfos(StringBuilder* sb)
	{
		// TableBase::GetInfos(sb);

		// sb->append(GetInfo(Offset, "Offset: a 4 byte constant"));
		// sb->append(GetInfo(Field, FIELD, "Field: index into the Field table"));
	}

	void FieldLayoutTable::initializeInstanceFields()
	{
		Offset = 0;
		Field = 0;
	}

	FieldMarshalTable::FieldMarshalTable()
	{
		initializeInstanceFields();
		_title = "FieldMarshal";
	}

	void FieldMarshalTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		Parent = readIndex(CodedIndices::Value::HASFIELDMARSHAL);
		NativeType = readBlobIndex();
	}

	void FieldMarshalTable::getInfos(StringBuilder* sb)
	{
		// TableBase::GetInfos(sb);

		// sb->append(GetInfo(Parent, CodedIndices::HasFieldMarshal, "Parent: index into Field or Param table; more precisely, a HasFieldMarshal coded index"));
		// sb->append(GetBlobIndexInfo(NativeType, "NativeType: index into the Blob heap"));
	}

	void FieldMarshalTable::initializeInstanceFields()
	{
		Parent = 0;
		NativeType = 0;
	}

	FieldRVATable::FieldRVATable()
	{
		initializeInstanceFields();
		_title = "FieldRVA";
	}

	void FieldRVATable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		RVA = readInt32();
		Field = readIndex(MetadataTables::Value::FIELD);
	}

	void FieldRVATable::getInfos(StringBuilder* sb)
	{
		// TableBase::GetInfos(sb);

		// sb->append(GetInfo(RVA, "RVA: a 4 byte constant"));
		// sb->append(GetInfo(Field, FIELD, "Field: index into Field table"));
	}

	void FieldRVATable::initializeInstanceFields()
	{
		RVA = 0;
		Field = 0;
	}

	FileTable::FileTable()
	{
		initializeInstanceFields();
		_title = "File";
	}

	void FileTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		Flags = readInt32();
		Name = readStringsIndex();
		HashValue = readBlobIndex();
	}

	void FileTable::getInfos(StringBuilder* sb)
	{
		// TableBase::GetInfos(sb);

		// sb->append(GetInfo(Flags, "Flags: a 4 byte bitmask of type FileAttributes, clause 22.1.6"));
		// sb->append(GetStringsIndexInfo(Name, "Name: index into String heap"));
		// sb->append(GetBlobIndexInfo(HashValue, "HashValue: index into Blob heap"));
	}

	void FileTable::initializeInstanceFields()
	{
		Flags = 0;
		Name = 0;
		HashValue = 0;
	}

	ImplMapTable::ImplMapTable()
	{
		initializeInstanceFields();
		_title = "ImplMap";
	}

	void ImplMapTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		MappingFlags = readInt16();
		MemberForwarded = readIndex(CodedIndices::Value::MEMBERFORWARDED);
		ImportName = readStringsIndex();
		ImportScope = readIndex(MetadataTables::Value::MODULEREF);
	}

	void ImplMapTable::getInfos(StringBuilder* sb)
	{
		// TableBase::GetInfos(sb);

		// sb->append(GetInfo(MappingFlags, "MappingFlags: a 2 byte bitmask of type PInvokeAttributes, clause 22.1.7"));
		// sb->append(GetInfo(MemberForwarded, CodedIndices::MemberForwarded, "MemberForwarded: index into the Field or MethodDef table; more precisely, a MemberForwarded coded index"));
		// sb->append(GetStringsIndexInfo(ImportName, "ImportName: index into the String heap"));
		// sb->append(GetInfo(ImportScope, MODULEREF, "ImportScope: index into the ModuleRef table"));
	}

	void ImplMapTable::initializeInstanceFields()
	{
		MappingFlags = 0;
		MemberForwarded = 0;
		ImportName = 0;
		ImportScope = 0;
	}

	InterfaceImplTable::InterfaceImplTable()
	{
		initializeInstanceFields();
		_title = "InterfaceImpl";
	}

	void InterfaceImplTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		Class = readIndex(MetadataTables::Value::TYPEDEF);
		Interface = readIndex(CodedIndices::Value::TYPEDEFORREF);
	}

	void InterfaceImplTable::getInfos(StringBuilder* sb)
	{
		// TableBase::GetInfos(sb);

		// sb->append(GetInfo(Class, TYPEDEF, "Class: index into the TypeDef table"));
		// sb->append(GetInfo(Interface, CodedIndices::TypeDefOrRef, "Interface: index into the TypeDef, TypeRef or TypeSpec table; more precisely, a TypeDefOrRef coded index"));
	}

	void InterfaceImplTable::initializeInstanceFields()
	{
		Class = 0;
		Interface = 0;
	}

	ManifestResourceTable::ManifestResourceTable()
	{
		initializeInstanceFields();
		_title = "ManifestResource";
	}

	void ManifestResourceTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		Offset = readInt32();
		Flags = readInt32();
		Name = readStringsIndex();
		Implementation = readIndex(CodedIndices::Value::IMPLEMENTATION);
	}

	void ManifestResourceTable::getInfos(StringBuilder* sb)
	{
		// TableBase::GetInfos(sb);

		// sb->append(GetInfo(Offset, "Offset: a 4 byte constant"));
		// sb->append(GetInfo(Flags, "Flags: a 4 byte bitmask of type ManifestResourceAttributes, clause 22.1.8"));
		// sb->append(GetStringsIndexInfo(Name, "Name: index into the String heap"));
		// sb->append(GetInfo(Implementation, CodedIndices::Implementation, "Implementation: index into File table, or AssemblyRef table, or null; more precisely, an Implementation coded index"));
	}

	void ManifestResourceTable::initializeInstanceFields()
	{
		Offset = 0;
		Flags = 0;
		Name = 0;
		Implementation = 0;
	}

	MemberRefTable::MemberRefTable()
	{
		initializeInstanceFields();
		_title = "MemberRef";
	}

	void MemberRefTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		Class = readIndex(CodedIndices::Value::MEMBERREFPARENT);
		Name = readStringsIndex();
		Signature = readBlobIndex();
	}

	void MemberRefTable::getInfos(StringBuilder* sb)
	{
		// TableBase::GetInfos(sb);

		// sb->append(GetInfo(Class, CodedIndices::MemberRefParent, "Class: index into the TypeRef, ModuleRef, MethodDef, TypeSpec or TypeDef tables; more precisely, a MemberRefParent coded index"));
		// sb->append(GetStringsIndexInfo(Name, "Name: index into String heap"));
		// sb->append(GetBlobIndexInfo(Signature, "Signature: index into Blob heap"));
	}

	void MemberRefTable::initializeInstanceFields()
	{
		Class = 0;
		Name = 0;
		Signature = 0;
	}

	MethodDefTable::MethodDefTable()
	{
		initializeInstanceFields();
		_title = "MethodDef";
	}

	void MethodDefTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		RVA = readInt32();
		ImplFlags = readInt16();
		Flags = readInt16();
		Name = readStringsIndex();
		Signature = readBlobIndex();
		ParamList = readIndex(MetadataTables::Value::PARAM);
	}

	void MethodDefTable::getInfos(StringBuilder* sb)
	{
		// TableBase::GetInfos(sb);

		// sb->append(GetInfo(RVA, "RVA: a 4 byte constant"));
		// sb->append(GetInfo(ImplFlags, "ImplFlags: a 2 byte bitmask of type MethodImplAttributes, clause 22.1.9"));
		// sb->append(GetInfo(Flags, "Flags: a 2 byte bitmask of type MethodAttribute, clause 22.1.9"));
		// sb->append(GetStringsIndexInfo(Name, "Name: index into String heap"));
		// sb->append(GetBlobIndexInfo(Signature, "Signature: index into Blob heap"));
		// sb->append(GetInfo(ParamList, PARAM, "ParamList: index into Param table"));
	}

	void MethodDefTable::initializeInstanceFields()
	{
		RVA = 0;
		ImplFlags = 0;
		Flags = 0;
		Name = 0;
		Signature = 0;
		ParamList = 0;
	}

	MethodImplTable::MethodImplTable()
	{
		initializeInstanceFields();
		_title = "MethodImpl";
	}

	void MethodImplTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		Class = readIndex(MetadataTables::Value::TYPEDEF);
		MethodBody = readIndex(CodedIndices::Value::METHODDEFORREF);
		MethodDeclaration = readIndex(MetadataTables::Value::METHODDEF);
	}

	void MethodImplTable::getInfos(StringBuilder* sb)
	{
		// TableBase::GetInfos(sb);

		// sb->append(GetInfo(Class, TYPEDEF, "Class: index into TypeDef table"));
		// sb->append(GetInfo(MethodBody, CodedIndices::MethodDefOrRef, "MethodBody: index into MethodDef or MemberRef table; more precisely, a MethodDefOrRef coded index"));
		// sb->append(GetInfo(MethodDeclaration, METHODDEF, "MethodDeclaration: index into MethodDef or MemberRef table; more precisely, a\nMethodDefOrRef coded index"));
	}

	void MethodImplTable::initializeInstanceFields()
	{
		Class = 0;
		MethodBody = 0;
		MethodDeclaration = 0;
	}

	MethodSemanticsTable::MethodSemanticsTable()
	{
		initializeInstanceFields();
		_title = "MethodSemantics";
	}

	void MethodSemanticsTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		Semantics = readInt16();
		Method = readIndex(MetadataTables::Value::METHODDEF);
		Association = readIndex(CodedIndices::Value::HASSEMANTICS);
	}

	void MethodSemanticsTable::getInfos(StringBuilder* sb)
	{
		// TableBase::GetInfos(sb);

		// sb->append(GetInfo(Semantics, "Semantics: a 2 byte bitmask of type MethodSemanticsAttributes, clause 22.1.11"));
		// sb->append(GetInfo(Method, METHODDEF, "Method: index into the MethodDef table"));
		// sb->append(GetInfo(Association, CodedIndices::HasSemantics, "Association: index into the Event or Property table; more precisely, a HasSemantics coded index"));
	}

	void MethodSemanticsTable::initializeInstanceFields()
	{
		Semantics = 0;
		Method = 0;
		Association = 0;
	}

	ModuleTable::ModuleTable()
	{
		initializeInstanceFields();
		_title = "Module";
	}

	void ModuleTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		Generation = readInt16();
		Name = readStringsIndex();
		Mvid = readGUIDIndex();
		EncId = readGUIDIndex();
		EncBaseId = readGUIDIndex();
	}

	void ModuleTable::getInfos(StringBuilder* sb)
	{
		// TableBase::GetInfos(sb);

		// sb->append(GetInfo(Generation, "Generation: 2 byte value, reserved, shall be zero"));
		// sb->append(GetStringsIndexInfo(Name, "Name: index into String heap"));
		// sb->append(GetGUIDIndexInfo(Mvid, "Mvid: index into Guid heap; simply a Guid used to distinguish between two versions of the same module"));
		// sb->append(GetGUIDIndexInfo(EncId, "EncId: index into Guid heap, reserved, shall be zero"));
		// sb->append(GetGUIDIndexInfo(EncBaseId, "EncBaseId: index into Guid heap, reserved, shall be zero"));
	}

	void ModuleTable::initializeInstanceFields()
	{
		Generation = 0;
		Name = 0;
		Mvid = 0;
		EncId = 0;
		EncBaseId = 0;
	}

	ModuleRefTable::ModuleRefTable()
	{
		initializeInstanceFields();
		_title = "ModuleRef";
	}

	void ModuleRefTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		Name = readStringsIndex();
	}

	void ModuleRefTable::getInfos(StringBuilder* sb)
	{
		// TableBase::GetInfos(sb);

		// sb->append(GetStringsIndexInfo(Name, "Name: index into String heap"));
	}

	void ModuleRefTable::initializeInstanceFields()
	{
		Name = 0;
	}

	NestedClassTable::NestedClassTable()
	{
		initializeInstanceFields();
		_title = "NestedClass";
	}

	void NestedClassTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		NestedClass = readIndex(MetadataTables::Value::TYPEDEF);
		EnclosingClass = readIndex(MetadataTables::Value::TYPEDEF);
	}

	void NestedClassTable::getInfos(StringBuilder* sb)
	{
		// TableBase::GetInfos(sb);

		// sb->append(GetInfo(NestedClass, TYPEDEF, "NestedClass: index into the TypeDef table"));
		// sb->append(GetInfo(EnclosingClass, TYPEDEF, "EnclosingClass: index into the TypeDef table"));
	}

	void NestedClassTable::initializeInstanceFields()
	{
		NestedClass = 0;
		EnclosingClass = 0;
	}

	// ------------------------------------------------------------------------------------
	// ---
	// ------------------------------------------------------------------------------------
	ParamTable::ParamTable()
	{
		initializeInstanceFields();
		_title = "Param";
	}

	void ParamTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		Flags = readInt16();
		Sequence = readInt16();
		Name = readStringsIndex();
	}

	void ParamTable::getInfos(StringBuilder* sb)
	{
		// TableBase::GetInfos(sb);

		// sb->append(GetInfo(Flags, "Flags: a 2 byte bitmask of type ParamAttributes, clause 22.1.12"));
		// sb->append(GetInfo(Sequence, "Sequence: a 2 byte constant"));
		// sb->append(GetStringsIndexInfo(Name, "Name: index into String heap"));
	}

	void ParamTable::initializeInstanceFields()
	{
		Flags = 0;
		Sequence = 0;
		Name = 0;
	}

	// ------------------------------------------------------------------------------------
	// ---
	// ------------------------------------------------------------------------------------
	PropertyTable::PropertyTable()
	{
		initializeInstanceFields();
		_title = "Property";
	}

	void PropertyTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		Flags = readInt16();
		Name = readStringsIndex();
		Type = readBlobIndex();
	}

	void PropertyTable::getInfos(StringBuilder* sb)
	{
		// TableBase::GetInfos(sb);

		// sb->append(GetInfo(Flags, "Flags: a 2 byte bitmask of type PropertyAttributes, clause 22.1.13"));
		// sb->append(GetStringsIndexInfo(Name, "Name: index into String heap"));
		// sb->append(GetBlobIndexInfo(Type, "Type: index into Blob heap"));
	}

	void PropertyTable::initializeInstanceFields()
	{
		Flags = 0;
		Name = 0;
		Type = 0;
	}

	// ------------------------------------------------------------------------------------
	// ---
	// ------------------------------------------------------------------------------------
	PropertyMapTable::PropertyMapTable()
	{
		initializeInstanceFields();
		_title = "PropertyMap";
	}

	void PropertyMapTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		Parent = readIndex(MetadataTables::Value::TYPEDEF);
		PropertyList = readIndex(MetadataTables::Value::PROPERTY);
	}

	void PropertyMapTable::getInfos(StringBuilder* sb)
	{
		// TableBase::GetInfos(sb);

		// sb->append(GetInfo(Parent, TYPEDEF, "Parent: index into the TypeDef table"));
		// sb->append(GetInfo(PropertyList, PROPERTY, "PropertyList: index into Property table"));
	}

	void PropertyMapTable::initializeInstanceFields()
	{
		Parent = 0;
		PropertyList = 0;
	}

	// ------------------------------------------------------------------------------------
	// ---
	// ------------------------------------------------------------------------------------
	StandAloneSigTable::StandAloneSigTable()
	{
		initializeInstanceFields();
		_title = "StandAloneSig";
	}

	void StandAloneSigTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		Signature = readBlobIndex();
	}

	void StandAloneSigTable::getInfos(StringBuilder* sb)
	{
		// TableBase::GetInfos(sb);

		// sb->append(GetBlobIndexInfo(Signature, "Signature: index into the Blob heap"));
	}

	void StandAloneSigTable::initializeInstanceFields()
	{
		Signature = 0;
	}

	// ------------------------------------------------------------------------------------
	// ---
	// ------------------------------------------------------------------------------------
	TypeDefTable::TypeDefTable()
	{
		initializeInstanceFields();
		_title = "TypeDef";
	}

	void TypeDefTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		Flags = readInt32();
		Name = readStringsIndex();
		Namespace = readStringsIndex();
		Extends = readIndex(CodedIndices::Value::TYPEDEFORREF);
		FieldList = readIndex(MetadataTables::Value::FIELD);
		MethodList = readIndex(MetadataTables::Value::METHODDEF);
	}

	void TypeDefTable::getInfos(StringBuilder* sb)
	{
		// TableBase::GetInfos(sb);

		// sb->append(GetInfo(Flags, "Flags: a 4 byte bitmask of type TypeAttributes, clause 22.1.14"));
		// sb->append(GetStringsIndexInfo(Name, "Name: index into String heap"));
		// sb->append(GetStringsIndexInfo(Namespace, "Namespace: index into String heap"));
		// sb->append(GetInfo(Extends, CodedIndices::TypeDefOrRef, "Extends: index into TypeDef, TypeRef or TypeSpec table; more precisely, a TypeDefOrRef coded index"));
		// sb->append(GetInfo(FieldList, FIELD, "FieldList: index into Field table; it marks the first of a continguous run of Fields owned by this Type"));
		// sb->append(GetInfo(MethodList, METHODDEF, "MethodList: index into MethodDef table"));
	}

	void TypeDefTable::initializeInstanceFields()
	{
		Flags = 0;
		Name = 0;
		Namespace = 0;
		Extends = 0;
		FieldList = 0;
		MethodList = 0;
	}

	// ------------------------------------------------------------------------------------
	// ---
	// ------------------------------------------------------------------------------------
	TypeRefTable::TypeRefTable()
	{
		initializeInstanceFields();
		_title = "TypeRef";
	}

	void TypeRefTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		ResolutionScope = readIndex(CodedIndices::Value::RESOLUTIONSCOPE);
		Name = readStringsIndex();
		Namespace = readStringsIndex();
	}

	void TypeRefTable::getInfos(StringBuilder* sb)
	{
		// TableBase::GetInfos(sb);

		// sb->append(GetInfo(ResolutionScope, CodedIndices::ResolutionScope, "ResolutionScope: index into Module, ModuleRef, AssemblyRef or TypeRef tables, or null; more precisely, a ResolutionScope coded index"));
		// sb->append(GetStringsIndexInfo(Name, "Name: index into String heap"));
		// sb->append(GetStringsIndexInfo(Namespace, "Namespace: index into String heap"));
	}

	void TypeRefTable::initializeInstanceFields()
	{
		ResolutionScope = 0;
		Name = 0;
		Namespace = 0;
	}

	// ------------------------------------------------------------------------------------
	// ---
	// ------------------------------------------------------------------------------------
	TypeSpecTable::TypeSpecTable()
	{
		initializeInstanceFields();
		_title = "TypeSpec";
	}

	void TypeSpecTable::readData(std::vector<unsigned char>& data, int offset)
	{
		TableBase::readData(data, offset);

		Signature = readBlobIndex();
	}

	void TypeSpecTable::getInfos(StringBuilder* sb)
	{
		// TableBase::GetInfos(sb);

		// sb->append(GetBlobIndexInfo(Signature, "Signature: index into the Blob heap, where the blob is formatted as specified in clause 22.2.14"));
	}

	void TypeSpecTable::initializeInstanceFields()
	{
		Signature = 0;
	}
}
