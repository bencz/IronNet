#pragma once

#include <vector>
#include "TableBase.h"
#include "MetadataTables.h"

namespace PEAnalyzer
{		
	/// <summary>
	/// The Assembly table is defined using the .assembly directive
	/// (see Section 6.2); its columns are obtained from the
	/// respective .hash algorithm, .ver, .publickey, and .culture
	/// (see clause 6.2.1 For an example see Section 6.2.
	/// </summary>
	class AssemblyTable : public TableBase
	{
		/// <summary>
		/// a 4 byte constant of type AssemblyHashAlgorithm, clause
		/// 22.1.1
		/// </summary>
	public:
		int HashAlgId;

		/// <summary>
		/// 2 byte constants
		/// </summary>
		short MajorVersion;

		/// <summary>
		/// 2 byte constants
		/// </summary>
		short MinorVersion;

		/// <summary>
		/// 2 byte constants
		/// </summary>
		short BuildNumber;

		/// <summary>
		/// 2 byte constants
		/// </summary>
		short RevisionNumber;

		/// <summary>
		/// a 4 byte bitmask of type AssemblyFlags, clause 22.1.2
		/// </summary>
		int Flags;

		/// <summary>
		/// index into Blob heap
		/// </summary>
		int PublicKey;

		/// <summary>
		/// index into String heap
		/// </summary>
		int Name;

		/// <summary>
		/// index into String heap
		/// </summary>
		int Culture;

		AssemblyTable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};

	/// <summary>
	/// This record should not be emitted into any PE file. If
	/// present in a PE file, it should be treated as if all its
	/// fields were zero. It should be ignored by the CLI.
	/// </summary>
	class AssemblyOSTable : public TableBase
	{
		/// <summary>
		/// a 4 byte constant
		/// </summary>
	public:
		int OSPlatformID;

		/// <summary>
		/// a 4 byte constant
		/// </summary>
		int OSMajorVersion;

		/// <summary>
		/// a 4 byte constant
		/// </summary>
		int OSMinorVersion;

		AssemblyOSTable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};

	/// <summary>
	/// This record should not be emitted into any PE file. If
	/// present in a PE file, it should be treated as if its field
	/// were zero. It should be ignored by the CLI.
	/// </summary>
	class AssemblyProcessorTable : public TableBase
	{
		/// <summary>
		/// a 4 byte constant
		/// </summary>
	public:
		int Processor;

		AssemblyProcessorTable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};

	/// <summary>
	/// The table is defined by the .assembly extern directive (see
	/// Section 6.3). Its columns are filled using directives
	/// similar to those of the Assembly table except for the
	/// PublicKeyOrToken column which is defined using the
	/// .publickeytoken directive. For an example see Section 6.3.
	/// </summary>
	class AssemblyRefTable : public TableBase
	{
		/// <summary>
		/// 2 byte constants
		/// </summary>
	public:
		short MajorVersion;

		/// <summary>
		/// 2 byte constants
		/// </summary>
		short MinorVersion;

		/// <summary>
		/// 2 byte constants
		/// </summary>
		short BuildNumber;

		/// <summary>
		/// 2 byte constants
		/// </summary>
		short RevisionNumber;

		/// <summary>
		/// a 4 byte bitmask of type AssemblyFlags, clause 22.1.2
		/// </summary>
		int Flags;

		/// <summary>
		/// index into Blob heap -- the public key or token that
		/// identifies the author of this Assembly
		/// </summary>
		int PublicKeyOrToken;

		/// <summary>
		/// index into String heap
		/// </summary>
		int Name;

		/// <summary>
		/// index into String heap
		/// </summary>
		int Culture;

		/// <summary>
		/// index into Blob heap
		/// </summary>
		int HashValue;

		AssemblyRefTable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};

	/// <summary>
	/// These records should not be emitted into any PE file. If
	/// present in a PE file, they should be treated as-if their
	/// fields were zero. They should be ignored by the CLI.
	/// </summary>
	class AssemblyRefOSTable : public TableBase
	{
		/// <summary>
		/// 4 byte constant
		/// </summary>
	public:
		int OSPlatformId;

		/// <summary>
		/// 4 byte constant
		/// </summary>
		int OSMajorVersion;

		/// <summary>
		/// 4 byte constant
		/// </summary>
		int OSMinorVersion;

		/// <summary>
		/// index into the AssemblyRef table
		/// </summary>
		int AssemblyRef;

		AssemblyRefOSTable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};

	/// <summary>
	/// These records should not be emitted into any PE file. If
	/// present in a PE file, they should be treated as-if their
	/// fields were zero. They should be ignored by the CLI.
	/// </summary>
	class AssemblyRefProcessorTable : public TableBase
	{
		/// <summary>
		/// 4 byte constant
		/// </summary>
	public:
		int Processor;

		/// <summary>
		/// index into the AssemblyRef table
		/// </summary>
		int AssemblyRef;

		AssemblyRefProcessorTable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};

	/// <summary>
	/// The ClassLayout table is used to define how the fields of a
	/// class or value type shall be laid out by the CLI (normally,
	/// the CLI is free to reorder and/or insert gaps between the
	/// fields defined for a class or value type).
	/// The rows of the ClassLayout table are defined by placing
	/// .pack and .size directives on the body of a parent type
	/// declaration (see Section 9.2). For an example see Section
	/// 9.7.
	/// </summary>
	class ClassLayoutTable : public TableBase
	{
		/// <summary>
		/// a 2 byte constant
		/// </summary>
	public:
		short PackingSize;

		/// <summary>
		/// a 4 byte constant
		/// </summary>
		int ClassSize;

		/// <summary>
		/// index into TypeDef table
		/// </summary>
		int Parent;

		ClassLayoutTable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};

	/// <summary>
	/// The Constant table is used to store compile-time, constant
	/// values for fields, parameters and properties.
	/// Note that Constant information does not directly influence
	/// runtime behavior, although it is visible via Reflection (and
	/// hence may be used to implement functionality such as that
	/// provided by System.Enum.ToString). Compilers inspect this
	/// information, at compile time, when importing metadata; but
	/// the value of the constant itself, if used, becomes embedded
	/// into the CIL stream the compiler emits. There are no CIL
	/// instructions to access the Constant table at runtime.
	/// A row in the Constant table for a parent is created whenever
	/// a compile-time value is specified for that parent, for an
	/// example see Section 15.2.
	/// </summary>
	class ConstantTable : public TableBase
	{
		/// <summary>
		/// a 1 byte constant, followed by a 1-byte padding zero
		/// </summary>
	public:
		unsigned char Type;

		/// <summary>
		/// index into the Param or Field or Property table; more
		/// precisely, a HasConstant coded index
		/// </summary>
		int Parent;

		/// <summary>
		/// index into Blob heap
		/// </summary>
		int Value;

		ConstantTable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};

	/// <summary>
	/// The CustomAttribute table stores data that can be used to
	/// instantiate a Custom Attribute (more precisely, an object of
	/// the specified Custom Attribute class) at runtime. The column
	/// called Type is slightly misleading -- it actually indexes a
	/// constructor method -- the owner of that constructor method
	/// is the Type of the Custom Attribute.
	/// A row in the CustomAttribute table for a parent is created
	/// by the .custom attribute, which gives the value of the Type
	/// column and optionally that of the Value column (see Chapter
	/// 20)
	/// </summary>
	class CustomAttributeTable : public TableBase
	{
		/// <summary>
		/// index into any metadata table, except the CustomAttribute
		/// table itself; more precisely, a HasCustomAttribute coded
		/// index
		/// </summary>
	public:
		int Parent;

		/// <summary>
		/// index into the MethodDef or MethodRef table; more precisely,
		/// a CustomAttributeType coded index
		/// </summary>
		int Type;

		/// <summary>
		/// index into Blob heap
		/// </summary>
		int Value;

		CustomAttributeTable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};

	/// <summary>
	/// The rows of the DeclSecurity table are filled by attaching a
	/// .permission or .permissionset directive that specifies the
	/// Action and PermissionSet on a parent assembly (see Section
	/// 6.6) or parent type or method (see Section 9.2).
	/// </summary>
	class DeclSecurityTable : public TableBase
	{
		/// <summary>
		/// 2 byte value
		/// </summary>
	public:
		short Action;

		/// <summary>
		/// index into the TypeDef, MethodDef or Assembly table
		/// </summary>
		int Parent;

		/// <summary>
		/// index into Blob heap
		/// </summary>
		int PermissionSet;

		DeclSecurityTable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};

	/// <summary>
	/// Note that EventMap info does not directly influence runtime
	/// behavior; what counts is the info stored for each method
	/// that the event comprises.
	/// </summary>
	class EventMapTable : public TableBase
	{
		/// <summary>
		/// index into the TypeDef table
		/// </summary>
	public:
		int Parent;

		/// <summary>
		/// index into Event table
		/// </summary>
		int EventList;

		EventMapTable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};

	/// <summary>
	/// Events are treated within metadata much like Properties -- a
	/// way to associate a collection of methods defined on given
	/// class. There are two required methods -- add_ and remove_,
	/// plus optional raise_ and others. All of the methods gathered
	/// together as an Event shall be defined on the class.
	/// Note that Event information does not directly influence
	/// runtime behavior; what counts is the information stored for
	/// each method that the event comprises.
	/// The EventMap and Event tables result from putting the .event
	/// directive on a class (see Chapter 17).
	/// </summary>
	class EventTable : public TableBase
	{
		/// <summary>
		/// a 2 byte bitmask of type EventAttribute, clause 22.1.4
		/// </summary>
	public:
		short EventFlags;

		/// <summary>
		/// index into String heap
		/// </summary>
		int Name;

		/// <summary>
		/// index into TypeDef, TypeRef or TypeSpec tables; more
		/// precisely, a TypeDefOrRef coded index
		/// </summary>
		int EventType;

		EventTable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};

	/// <summary>
	/// The ExportedType table holds a row for each type, defined
	/// within other modules of this Assembly, that is exported out
	/// of this Assembly. In essence, it stores TypeDef row numbers
	/// of all types that are marked public in other modules that
	/// this Assembly comprises.
	/// The rows in the ExportedType table are the result of the
	/// .class extern directive (see Section 6.7).
	/// </summary>
	class ExportedTypeTable : public TableBase
	{
		/// <summary>
		/// a 4 byte bitmask of type TypeAttributes, clause 22.1.14
		/// </summary>
	public:
		int Flags;

		/// <summary>
		/// 4 byte index into a TypeDef table of another module in this
		/// Assembly
		/// </summary>
		int TypeDefId;

		/// <summary>
		/// index into the String heap
		/// </summary>
		int TypeName;

		/// <summary>
		/// index into the String heap
		/// </summary>
		int TypeNamespace;

		/// <summary>
		/// This can be an index (more precisely, an Implementation
		/// coded index) into one of 2 tables.
		/// </summary>
		int Implementation;

		ExportedTypeTable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};

	/// <summary>
	/// Each row in the Field table results from a toplevel .field
	/// directive (see Section 5.10), or a .field directive inside a
	/// Type (see Section 9.2). For an example see Section 13.5.
	/// </summary>
	class FieldTable : public TableBase
	{
		/// <summary>
		/// a 2 byte bitmask of type FieldAttributes, clause 22.1.5
		/// </summary>
	public:
		short Flags;

		/// <summary>
		/// index into String heap
		/// </summary>
		int Name;

		/// <summary>
		/// index into Blob heap
		/// </summary>
		int Signature;

		FieldTable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};

	/// <summary>
	/// A row in the FieldLayout table is created if the .field
	/// directive for the parent field has specified a field offset
	/// (see Section 9.7).
	/// </summary>
	class FieldLayoutTable : public TableBase
	{
		/// <summary>
		/// a 4 byte constant
		/// </summary>
	public:
		int Offset;

		/// <summary>
		/// index into the Field table
		/// </summary>
		int Field;

		FieldLayoutTable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};

	/// <summary>
	/// A row in the FieldMarshal table is created if the .field
	/// directive for the parent field has specified a .marshall
	/// attribute (see Section 15.1).
	/// </summary>
	class FieldMarshalTable : public TableBase
	{
		/// <summary>
		/// index into Field or Param table; more precisely, a
		/// HasFieldMarshal coded index
		/// </summary>
	public:
		int Parent;

		/// <summary>
		/// index into the Blob heap
		/// </summary>
		int NativeType;

		FieldMarshalTable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};

	/// <summary>
	/// A row in the FieldRVA table is created for each static
	/// parent field that has specified the optional data label (see
	/// Chapter 15). The RVA column is the relative virtual address
	/// of the data in the PE file (see Section 15.3).
	/// </summary>
	class FieldRVATable : public TableBase
	{
		/// <summary>
		/// a 4 byte constant
		/// </summary>
	public:
		int RVA;

		/// <summary>
		/// index into Field table
		/// </summary>
		int Field;

		FieldRVATable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};

	/// <summary>
	/// The rows of the File table result from .file directives in
	/// an Assembly (see clause 6.2.3)
	/// </summary>
	class FileTable : public TableBase
	{
		/// <summary>
		/// a 4 byte bitmask of type FileAttributes, clause 22.1.6
		/// </summary>
	public:
		int Flags;

		/// <summary>
		/// index into String heap
		/// </summary>
		int Name;

		/// <summary>
		/// index into Blob heap
		/// </summary>
		int HashValue;

		FileTable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};

	/// <summary>
	/// A row is entered in the ImplMap table for each parent Method
	/// (see Section 14.5) that is defined with a .pinvokeimpl
	/// interoperation attribute specifying the MappingFlags,
	/// ImportName and ImportScope. For an example see Section 14.5.
	/// </summary>
	class ImplMapTable : public TableBase
	{
		/// <summary>
		/// a 2 byte bitmask of type PInvokeAttributes, clause 22.1.7
		/// </summary>
	public:
		short MappingFlags;

		/// <summary>
		/// index into the Field or MethodDef table; more precisely, a
		/// MemberForwarded coded index
		/// </summary>
		int MemberForwarded;

		/// <summary>
		/// index into the String heap
		/// </summary>
		int ImportName;

		/// <summary>
		/// index into the ModuleRef table
		/// </summary>
		int ImportScope;

		ImplMapTable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};

	/// <summary>
	/// The InterfaceImpl table records which interfaces a Type
	/// implements. Conceptually, each row in the InterfaceImpl
	/// table says that Class implements Interface.
	/// </summary>
	class InterfaceImplTable : public TableBase
	{
		/// <summary>
		/// index into the TypeDef table
		/// </summary>
	public:
		int Class;

		/// <summary>
		/// index into the TypeDef, TypeRef or TypeSpec table; more
		/// precisely, a TypeDefOrRef coded index
		/// </summary>
		int Interface;

		InterfaceImplTable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};

	/// <summary>
	/// The Offset specifies the byte offset within the referenced
	/// file at which this resource record begins. The
	/// Implementation specifies which file holds this resource. The
	/// rows in the table result from .mresource directives on the
	/// Assembly (see clause 6.2.2).
	/// </summary>
	class ManifestResourceTable : public TableBase
	{
		/// <summary>
		/// a 4 byte constant
		/// </summary>
	public:
		int Offset;

		/// <summary>
		/// a 4 byte bitmask of type ManifestResourceAttributes, clause
		/// 22.1.8
		/// </summary>
		int Flags;

		/// <summary>
		/// index into the String heap
		/// </summary>
		int Name;

		/// <summary>
		/// index into File table, or AssemblyRef table, or null; more
		/// precisely, an Implementation coded index
		/// </summary>
		int Implementation;

		ManifestResourceTable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};

	/// <summary>
	/// An entry is made into the MemberRef table whenever a
	/// reference is made, in the CIL code, to a method or field
	/// which is defined in another module or assembly. (Also, an
	/// entry is made for a call to a method with a VARARG
	/// signature, even when it is defined in the same module as the
	/// callsite)
	/// </summary>
	class MemberRefTable : public TableBase
	{
		/// <summary>
		/// index into the TypeRef, ModuleRef, MethodDef, TypeSpec or
		/// TypeDef tables; more precisely, a MemberRefParent coded
		/// index
		/// </summary>
	public:
		int Class;

		/// <summary>
		/// index into String heap
		/// </summary>
		int Name;

		/// <summary>
		/// index into Blob heap
		/// </summary>
		int Signature;

		MemberRefTable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};

	/// <summary>
	/// The rows in the MethodDef table result from .method
	/// directives (see Chapter 14). The RVA column is computed when
	/// the image for the PE file is emitted and points to the
	/// COR_ILMETHOD structure for the body of the method (see
	/// Chapter 24.4)
	/// </summary>
	class MethodDefTable : public TableBase
	{
		/// <summary>
		/// a 4 byte constant
		/// </summary>
	public:
		int RVA;

		/// <summary>
		/// a 2 byte bitmask of type MethodImplAttributes, clause 22.1.9
		/// </summary>
		short ImplFlags;

		/// <summary>
		/// a 2 byte bitmask of type MethodAttribute, clause 22.1.9
		/// </summary>
		short Flags;

		/// <summary>
		/// index into String heap
		/// </summary>
		int Name;

		/// <summary>
		/// index into Blob heap
		/// </summary>
		int Signature;

		/// <summary>
		/// index into Param table
		/// </summary>
		int ParamList;

		MethodDefTable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};

	/// <summary>
	/// ilasm uses the .override directive to specify the rows of
	/// the MethodImpl table (see clause 9.3.2).
	/// </summary>
	class MethodImplTable : public TableBase
	{
		/// <summary>
		/// index into TypeDef table
		/// </summary>
	public:
		int Class;

		/// <summary>
		/// index into MethodDef or MemberRef table; more precisely, a
		/// MethodDefOrRef coded index
		/// </summary>
		int MethodBody;

		/// <summary>
		/// index into MethodDef or MemberRef table; more precisely, a
		/// MethodDefOrRef coded index
		/// </summary>
		int MethodDeclaration;

		MethodImplTable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};

	/// <summary>
	/// The rows of the MethodSemantics table are filled by
	/// .property (see Chapter 16) and .event directives (see
	/// Chapter 17). See clause 21.13 for more information.
	/// </summary>
	class MethodSemanticsTable : public TableBase
	{
		/// <summary>
		/// a 2 byte bitmask of type MethodSemanticsAttributes, clause
		/// 22.1.11
		/// </summary>
	public:
		short Semantics;

		/// <summary>
		/// index into the MethodDef table
		/// </summary>
		int Method;

		/// <summary>
		/// index into the Event or Property table; more precisely, a
		/// HasSemantics coded index
		/// </summary>
		int Association;

		MethodSemanticsTable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};

	/// <summary>
	/// The Generation, EncId and EncBaseId columns can be written
	/// as zero, and can be ignored by conforming implementations of
	/// the CLI. The rows in the Module table result from .module
	/// directives in the Assembly (see Section 6.4).
	/// </summary>
	class ModuleTable : public TableBase
	{
		/// <summary>
		/// 2 byte value, reserved, shall be zero
		/// </summary>
	public:
		short Generation;

		/// <summary>
		/// index into String heap
		/// </summary>
		int Name;

		/// <summary>
		/// index into Guid heap; simply a Guid used to distinguish
		/// between two versions of the same module
		/// </summary>
		int Mvid;

		/// <summary>
		/// index into Guid heap, reserved, shall be zero
		/// </summary>
		int EncId;

		/// <summary>
		/// index into Guid heap, reserved, shall be zero
		/// </summary>
		int EncBaseId;

		ModuleTable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};

	/// <summary>
	/// The rows in the ModuleRef table result from .module extern
	/// directives in the Assembly (see Section 6.5).
	/// </summary>
	class ModuleRefTable : public TableBase
	{
		/// <summary>
		/// index into String heap
		/// </summary>
	public:
		int Name;

		ModuleRefTable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};

	/// <summary>
	/// The NestedClass table records which Type definitions are
	/// nested within which other Type definition. In a typical
	/// high-level language, including ilasm, the nested class is
	/// defined as lexically 'inside' the text of its enclosing
	/// Type.
	/// </summary>
	class NestedClassTable : public TableBase
	{
		/// <summary>
		/// index into the TypeDef table
		/// </summary>
	public:
		int NestedClass;

		/// <summary>
		/// index into the TypeDef table
		/// </summary>
		int EnclosingClass;

		NestedClassTable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};

	/// <summary>
	/// The rows in the Param table result from the parameters in a
	/// method declaration (see Section 14.4), or from a .param
	/// attribute attached to a method (see clause 14.4.1).
	/// </summary>
	class ParamTable : public TableBase
	{
		/// <summary>
		/// a 2 byte bitmask of type ParamAttributes, clause 22.1.12
		/// </summary>
	public:
		short Flags;

		/// <summary>
		/// a 2 byte constant
		/// </summary>
		short Sequence;

		/// <summary>
		/// index into String heap
		/// </summary>
		int Name;

		ParamTable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};

	/// <summary>
	/// Properties within metadata are best viewed as a means to
	/// gather together collections of methods defined on a class,
	/// give them a name, and not much else. The methods are
	/// typically get_ and set_ methods, already defined on the
	/// class, and inserted like any other methods into the
	/// MethodDef table.
	/// </summary>
	class PropertyTable : public TableBase
	{
		/// <summary>
		/// a 2 byte bitmask of type PropertyAttributes, clause 22.1.13
		/// </summary>
	public:
		short Flags;

		/// <summary>
		/// index into String heap
		/// </summary>
		int Name;

		/// <summary>
		/// index into Blob heap
		/// </summary>
		int Type;

		PropertyTable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};

	/// <summary>
	/// The PropertyMap and Property tables result from putting the
	/// .property directive on a class (see Chapter 16).
	/// </summary>
	class PropertyMapTable : public TableBase
	{
		/// <summary>
		/// index into the TypeDef table
		/// </summary>
	public:
		int Parent;

		/// <summary>
		/// index into Property table
		/// </summary>
		int PropertyList;

		PropertyMapTable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};

	/// <summary>
	/// Signatures are stored in the metadata Blob heap. In most
	/// cases, they are indexed by a column in some table --
	/// Field.Signature, Method.Signature, MemberRef.Signature, etc.
	/// However, there are two cases that require a metadata token
	/// for a signature that is not indexed by any metadata table.
	/// The StandAloneSig table fulfils this need. It has just one
	/// column, that points to a Signature in the Blob heap.
	/// </summary>
	class StandAloneSigTable : public TableBase
	{
		/// <summary>
		/// index into the Blob heap
		/// </summary>
	public:
		int Signature;

		StandAloneSigTable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};

	class TypeDefTable : public TableBase
	{
		/// <summary>
		/// a 4 byte bitmask of type TypeAttributes, clause 22.1.14
		/// </summary>
	public:
		int Flags;

		/// <summary>
		/// index into String heap
		/// </summary>
		int Name;

		/// <summary>
		/// index into String heap
		/// </summary>
		int Namespace;

		/// <summary>
		/// index into TypeDef, TypeRef or TypeSpec table; more
		/// precisely, a TypeDefOrRef coded index
		/// </summary>
		int Extends;

		/// <summary>
		/// index into Field table; it marks the first of a continguous
		/// run of Fields owned by this Type
		/// </summary>
		int FieldList;

		/// <summary>
		/// index into MethodDef table
		/// </summary>
		int MethodList;

		TypeDefTable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};

	class TypeRefTable : public TableBase
	{
		/// <summary>
		/// index into Module, ModuleRef, AssemblyRef or TypeRef tables,
		/// or null; more precisely, a ResolutionScope coded index
		/// </summary>
	public:
		int ResolutionScope;

		/// <summary>
		/// index into String heap
		/// </summary>
		int Name;

		/// <summary>
		/// index into String heap
		/// </summary>
		int Namespace;

		TypeRefTable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};

	/// <summary>
	/// The TypeSpec table has just one column, which indexes the
	/// specification of a Type, stored in the Blob heap. This
	/// provides a metadata token for that Type (rather than simply
	/// an index into the Blob heap) -- this is required, typically,
	/// for array operations creating, or calling methods on the
	/// array class.
	/// </summary>
	class TypeSpecTable : public TableBase
	{
		/// <summary>
		/// index into the Blob heap, where the blob is formatted as
		/// specified in clause 22.2.14
		/// </summary>
	public:
		int Signature;

		TypeSpecTable();

		virtual void readData(std::vector<unsigned char>& data, int offset);

		virtual void getInfos(StringBuilder* sb);

	private:
		void initializeInstanceFields();
	};
}