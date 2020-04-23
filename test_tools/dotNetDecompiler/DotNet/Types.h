#ifndef __DOTNET_TYPES_H__
#define __DOTNET_TYPES_H__

#include "IncFileNm.h"
#include OPCODE_H
#include ELEMENTTYPE_H

namespace PEAnalyzer
{
	struct CorILMethod
	{
		enum Value
		{
			/// <summary>Method header is fat.</summary>
			FATFORMAT = 0x3,
			/// <summary>Method header is tiny.</summary>
			TINYFORMAT = 0x2,
			/// <summary>More sections follow after this header (see Section 24.4.5).</summary>
			MORESECTS = 0x8,
			/// <summary>Call default constructor on all local variables.</summary>
			INITLOCALS = 0x10
		};
	};

	struct MethodFlags
	{
		enum Value
		{
			GENERIC = 0x10,
			/// <summary>used to encode the keyword instance in the calling convention, see Section 14.3</summary>
			HASTHIS = 0x20,
			/// <summary>used to encode the keyword explicit in the calling convention, see Section 14.3</summary>
			EXPLICITTHIS = 0x40,
			/// <summary>used to encode the keyword default in the calling convention, see Section 14.3</summary>
			DEFAULT = 0x0,
			/// <summary>used to encode the keyword vararg in the calling convention, see Section 14.3</summary>
			VARARG = 0x5,
			C = 0x1,
			STDCALL = 0x2,
			THISCALL = 0x3,
			FASTCALL = 0x4,
		};
	};

	struct MethodAttributes
	{
		enum Value
		{
			MEMBERACCESSMASK = 0x0007,
			///<summary>Member not referenceable</summary>
			COMPILERCONTROLLED = 0x0000,
			///<summary>Accessible only by the parent type</summary>
			PRIVATE = 0x0001,
			///<summary>Accessible by sub-types only in this Assembly</summary>
			FAMANDASSEM = 0x0002,
			///<summary>Accessibly by anyone in the Assembly</summary>
			ASSEM = 0x0003,
			///<summary>Accessible only by type and sub-types</summary>
			FAMILY = 0x0004,
			///<summary>Accessibly by sub-types anywhere, plus anyone in assembly</summary>
			FAMORASSEM = 0x0005,
			///<summary>Accessibly by anyone who has visibility to this scope</summary>
			PUBLIC = 0x0006,
			///<summary>Defined on type, else per instance</summary>
			STATIC = 0x0010,
			///<summary>Method may not be overridden</summary>
			FINAL = 0x0020,
			///<summary>Method is virtual</summary>
			VIRTUAL = 0x0040,
			///<summary>Method hides by name+sig, else just by name</summary>
			HIDEBYSIG = 0x0080,
			///<summary>Use this mask to retrieve vtable attributes</summary>
			VTABLELAYOUTMASK = 0x0100,
			///<summary>Method reuses existing slot in vtable</summary>
			REUSESLOT = 0x0000,
			///<summary>Method always gets a new slot in the vtable</summary>
			NEWSLOT = 0x0100,
			///<summary>Method does not provide an implementation</summary>
			ABSTRACT = 0x0400,
			///<summary>Method is special</summary>
			SPECIALNAME = 0x0800,

			// Interop attributes
			///<summary>Implementation is forwarded through PInvoke</summary>
			PINVOKEIMPL = 0x2000,
			///<summary>Reserved: shall be zero for conforming implementations</summary>
			UNMANAGEDEXPORT = 0x0008,

			// Additional flags
			///<summary>CLI provides 'special' behavior, depending upon the name of the method</summary>
			RTSPECIALNAME = 0x1000,
			///<summary>Method has security associate with it</summary>
			HASSECURITY = 0x4000,
			///<summary>Method calls another method containing security code.</summary>
			REQUIRESECOBJECT = 0x8000
		};
	};

	struct CodeTypeMask
	{
		enum Value
		{
			///<summary>Method impl is CIL</summary>
			IL = 0x0000,
			///<summary>Method impl is native</summary>
			NATIVE = 0x0001,
			///<summary>Reserved: shall be zero in conforming implementations</summary>
			OPTIL = 0x0002,
			///<summary>Method impl is provided by the runtime</summary>
			RUNTIME = 0x0003
		};
	};

	///<summary>
	///Flags specifying whether the code is managed or unmanaged.
	///</summary>
	struct ManagedMask
	{
		enum Value
		{
			///<summary>Method impl is unmanaged, otherwise managed</summary>
			UNMANAGED = 0x0004,
			///<summary>Method impl is managed</summary>
			MANAGED = 0x0000
		};
	};

	/// <summary>
	/// Implementation info and interop
	/// </summary>
	struct MethodImplAttributes
	{
		enum Value
		{
			///<summary>Indicates method is defined; used primarily in merge scenarios</summary>
			FORWARDREF = 0x0010,
			///<summary>Reserved: conforming implementations may ignore</summary>
			PRESERVESIG = 0x0080,
			///<summary>Reserved: shall be zero in conforming implementations</summary>
			INTERNALCALL = 0x1000,
			///<summary>Method is single threaded through the body</summary>
			SYNCHRONIZED = 0x0020,
			///<summary>Method may not be inlined</summary>
			NOINLINING = 0x0008,
			///<summary>Range check value</summary>
			MAXMETHODIMPLVAL = 0xffff
		};
	};

	/// <summary>
	/// Use this mask to retrieve visibility information
	/// </summary>
	struct VisibilityMask
	{
		enum Value
		{
			///<summary>Class has no public scope</summary>
			NOTPUBLIC = 0x00000000,
			///<summary>Class has public scope</summary>
			PUBLIC = 0x00000001,
			///<summary>Class is nested with public visibility</summary>
			NESTEDPUBLIC = 0x00000002,
			///<summary>Class is nested with private visibility</summary>
			NESTEDPRIVATE = 0x00000003,
			///<summary>Class is nested with family visibility</summary>
			NESTEDFAMILY = 0x00000004,
			///<summary>Class is nested with assembly visibility</summary>
			NESTEDASSEMBLY = 0x00000005,
			///<summary>Class is nested with family and assembly visibility</summary>
			NESTEDFAMANDASSEM = 0x00000006,
			///<summary>Class is nested with family or assembly visibility</summary>
			NESTEDFAMORASSEM = 0x00000007
		};
	};

	/// <summary>
	/// Use this mask to retrieve class layout information
	/// </summary>
	struct LayoutMask
	{
		enum Value
		{
			///<summary>Class fields are auto-laid out</summary>
			AUTOLAYOUT = 0x00000000,
			///<summary>Class fields are laid out sequentially</summary>
			SEQUENTIALLAYOUT = 0x00000008,
			///<summary>Layout is supplied explicitly</summary>
			EXPLICITLAYOUT = 0x00000010
		};
	};

	/// <summary>
	/// Use this mask to retrive class semantics information
	/// </summary>
	struct ClassSemanticsMask
	{
		enum Value
		{
			///<summary>Type is a class</summary>
			CLASS = 0x00000000,
			///<summary>Type is an interface</summary>
			INTERFACE = 0x00000020
		};
	};

	struct TypeAttributes
	{
		enum Value
		{
			// Special semantics in addition to class semantics
			///<summary>Class is abstract</summary>
			ABSTRACT = 0x00000080,
			///<summary>Class cannot be extended</summary>
			SEALED = 0x00000100,
			///<summary>Class name is special</summary>
			SPECIALNAME = 0x00000400,

			// Implementation Attributes
			///<summary>Class/Interface is imported</summary>
			IMPORT = 0x00001000,
			///<summary>Class is serializable</summary>
			SERIALIZABLE = 0x00002000,

			// String formatting Attributes
			///<summary>Use this mask to retrieve string information for native interop</summary>
			STRINGFORMATMASK = 0x00030000,
			///<summary>LPSTR is interpreted as ANSI</summary>
			ANSICLASS = 0x00000000,
			///<summary>LPSTR is interpreted as Unicode</summary>
			UNICODECLASS = 0x00010000,
			///<summary>LPSTR is interpreted automatically</summary>
			AUTOCLASS = 0x00020000,

			// Class Initialization Attributes
			///<summary>Initialize the class before first static field access</summary>
			BEFOREFIELDINIT = 0x00100000,

			// Additional Flags
			///<summary>CLI provides 'special' behavior, depending upon the name of the Type</summary>
			RTSPECIALNAME = 0x00000800,
			///<summary>Type has security associate with it</summary>
			HASSECURITY = 0x00040000
		};
	};

	struct ParamAttributes
	{
		enum Value
		{
			///<summary>Param is [In]</summary>
			IN = 0x0001,
			///<summary>Param is [out]</summary>
			OUT = 0x0002,
			///<summary>Param is optional</summary>
			OPTIONAL = 0x0010,
			///<summary>Param has default value</summary>
			HASDEFAULT = 0x1000,
			///<summary>Param has FieldMarshal</summary>
			HASFIELDMARSHAL = 0x2000,
			///<summary>Reserved: shall be zero in a conforming implementation</summary>
			UNUSED = 0xcfe0
		};
	};

	struct FieldAccessMask
	{
		enum Value
		{
			///<summary>Member not referenceable</summary>
			COMPILERCONTROLLED = 0x0000,
			///<summary>Accessible only by the parent type</summary>
			PRIVATE = 0x0001,
			///<summary>Accessible by sub-types only in this Assembly</summary>
			FAMANDASSEM = 0x0002,
			///<summary>Accessibly by anyone in the Assembly</summary>
			ASSEMBLY = 0x0003,
			///<summary>Accessible only by type and sub-types</summary>
			FAMILY = 0x0004,
			///<summary>Accessibly by sub-types anywhere, plus anyone in assembly</summary>
			FAMORASSEM = 0x0005,
			///<summary>Accessibly by anyone who has visibility to this scope field contract attributes</summary>
			PUBLIC = 0x0006
		};
	};

	struct FieldAttributes
	{
		enum Value
		{
			///<summary>Defined on type, else per instance</summary>
			STATIC = 0x0010,
			///<summary>Field may only be initialized, not written to after init</summary>
			INITONLY = 0x0020,
			///<summary>Value is compile time constant</summary>
			LITERAL = 0x0040,
			///<summary>Field does not have to be serialized when type is remoted</summary>
			NOTSERIALIZED = 0x0080,
			///<summary>Field is special</summary>
			SPECIALNAME = 0x0200,

			// Interop Attributes
			///<summary>Implementation is forwarded through PInvoke.</summary>
			PINVOKEIMPL = 0x2000,

			// Additional flags
			///<summary>CLI provides 'special' behavior, depending upon the name of the field</summary>
			RTSPECIALNAME = 0x0400,
			///<summary>Field has marshalling information</summary>
			HASFIELDMARSHAL = 0x1000,
			///<summary>Field has default</summary>
			HASDEFAULT = 0x8000,
			///<summary>Field has RVA</summary>
			HASFIELDRVA = 0x0100
		};
	};

	struct CorILMethod_Sect
	{
		enum Value
		{
			///<summary>Exception handling data.</summary>
			EHTABLE = 0x1,
			///<summary>Reserved, shall be 0.</summary>
			OPTILTABLE = 0x2,
			///<summary>Data format is of the fat variety, meaning there is a 3 byte length.  If not set, the header is small with a  1 byte length</summary>
			FATFORMAT = 0x40,
			///<summary>Another data section occurs after this current section</summary>
			MORESECTS = 0x80
		};
	};

	struct COR_ILEXCEPTION_CLAUSE
	{
		enum Value
		{
			///<summary>A typed exception clause</summary>
			EXCEPTION = 0x0000,
			///<summary>An exception filter and handler clause</summary>
			FILTER = 0x0001,
			///<summary>A finally clause</summary>
			FINALLY = 0x0002,
			///<summary>Fault clause (finally that is called on exception only)</summary>
			FAULT = 0x0004
		};
	};

	class DoubleInt
	{
	public:
		int A, B;
	};

	class TypeData
	{
	public:
		int Size, Token;
		ElementType::Value Element, Option;
	};

	class ILCode
	{
	public:
		int _address;
		OpCode* _opCode;
		int _opCodeLength;
		void* _operand;
		int _operandLength;
		bool _isBrTarget;
		
		ILCode() {}
		~ILCode()
		{
			delete _operand;
		}


	private:
		void InitializeInstanceFields()
		{
			_address = 0;
			_opCodeLength = 0;
			_operandLength = 0;
			_isBrTarget = false;
		}

	public:
		ILCode(OpCode* opcode)
			: _opCode(opcode)
		{
			InitializeInstanceFields();
		}
	};

	struct Children
	{
		enum Value
		{
			DEFNESTED = 0,
			DEFMETHOD,
			DEFFIELD,
			REFNESTED = 0,
			REFMETHOD,
			REFFIELD
		};
	};
}


#endif // __DOTNET_TYPES_H__