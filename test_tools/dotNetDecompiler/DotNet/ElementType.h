#ifndef __DOTNET_ELEMENTTYPE_H__
#define __DOTNET_ELEMENTTYPE_H__

namespace PEAnalyzer
{
	struct ElementType
	{
		enum Value
		{
			/// <summary>Marks end of a list</summary>
			END = 0x00,
			VOID = 0x01,
			BOOLEAN = 0x02,
			CHAR = 0x03,
			I1 = 0x04,
			U1 = 0x05,
			I2 = 0x06,
			U2 = 0x07,
			I4 = 0x08,
			U4 = 0x09,
			I8 = 0x0a,
			U8 = 0x0b,
			R4 = 0x0c,
			R8 = 0x0d,
			STRING = 0x0e,
			/// <summary>Followed by &lt;type&gt; token</summary>
			PTR = 0x0f,
			/// <summary>Followed by &lt;type&gt; token</summary>
			BYREF = 0x10,
			/// <summary>Followed by TypeDef or TypeRef token</summary>
			VALUETYPE = 0x11,
			/// <summary>Followed by TypeDef or TypeRef token</summary>
			CLASS = 0x12,
			/// <summary>&lt;type&gt; &lt;rank&gt; &lt;boundsCount&gt; &lt;bound1&gt; … &lt;loCount&gt; &lt;lo1&gt; …</summary>
			ARRAY = 0x14,
			TYPEDBYREF = 0x16,
			/// <summary>System.IntPtr</summary>
			I = 0x18,
			/// <summary>System.UIntPtr</summary>
			U = 0x19,
			/// <summary>Followed by full method signature</summary>
			FNPTR = 0x1b,
			/// <summary>System.Object</summary>
			OBJECT = 0x1c,
			/// <summary>Single-dim array with 0 lower bound</summary>
			SZARRAY = 0x1d,
			/// <summary>Required modifier : followed by a TypeDef or TypeRef token</summary>
			CMOD_REQD = 0x1f,
			/// <summary>Optional modifier : followed by a TypeDef or TypeRef token</summary>
			CMOD_OPT = 0x20,
			/// <summary>Implemented within the CLI</summary>
			INTERNAL = 0x21,
			/// <summary>Or'd with following element types</summary>
			MODIFIER = 0x40,
			/// <summary>Sentinel for varargs method signature</summary>
			SENTINEL = 0x41,
			/// <summary>Denotes a local variable that points at a pinned object</summary>
			PINNED = 0x45
		};
	};

	static const ElementType::Value AllKeysElementTypes[] =
	{
		ElementType::Value::END,
		ElementType::Value::VOID,
		ElementType::Value::BOOLEAN,
		ElementType::Value::CHAR,
		ElementType::Value::I1,
		ElementType::Value::U1,
		ElementType::Value::I2,
		ElementType::Value::U2,
		ElementType::Value::I4,
		ElementType::Value::U4,
		ElementType::Value::I8,
		ElementType::Value::U8,
		ElementType::Value::R4,
		ElementType::Value::R8,
		ElementType::Value::STRING,
		ElementType::Value::PTR,
		ElementType::Value::BYREF,
		ElementType::Value::VALUETYPE,
		ElementType::Value::CLASS,
		ElementType::Value::ARRAY,
		ElementType::Value::TYPEDBYREF,
		ElementType::Value::I,
		ElementType::Value::U,
		ElementType::Value::FNPTR,
		ElementType::Value::OBJECT,
		ElementType::Value::SZARRAY,
		ElementType::Value::CMOD_REQD,
		ElementType::Value::CMOD_OPT,
		ElementType::Value::INTERNAL,
		ElementType::Value::MODIFIER,
		ElementType::Value::SENTINEL,
		ElementType::Value::PINNED
	};

	class ElementTypes
	{
	public:
		static bool Contains(ElementType::Value value)
		{
			for (int i = 0; i < 32; i++)
				if (AllKeysElementTypes[i] == value)
					return true;
			return false;
		}

		static std::string toString(ElementType::Value value)
		{
			switch (value)
			{
			case ElementType::Value::END:
				return "END";
			case ElementType::Value::VOID:
				return "VOID";
			case ElementType::Value::BOOLEAN:
				return "BOOLEAN";
			case ElementType::Value::CHAR:
				return "CHAR";
			case ElementType::Value::I1:
				return "I1";
			case ElementType::Value::U1:
				return "U1";
			case ElementType::Value::I2:
				return "I2";
			case ElementType::Value::U2:
				return "U2";
			case ElementType::Value::I4:
				return "I4";
			case ElementType::Value::U4:
				return "U4";
			case ElementType::Value::I8:
				return "I8";
			case ElementType::Value::U8:
				return "U8";
			case ElementType::Value::R4:
				return "R4";
			case ElementType::Value::R8:
				return "R8";
			case ElementType::Value::STRING:
				return "STRING";
			case ElementType::Value::PTR:
				return "PTR";
			case ElementType::Value::BYREF:
				return "BYREF";
			case ElementType::Value::VALUETYPE:
				return "VALUETYPE";
			case ElementType::Value::CLASS:
				return "CLASS";
			case ElementType::Value::ARRAY:
				return "ARRAY";
			case ElementType::Value::TYPEDBYREF:
				return "TYPEDBYREF";
			case ElementType::Value::I:
				return "I";
			case ElementType::Value::U:
				return "U";
			case ElementType::Value::FNPTR:
				return "FNPTR";
			case ElementType::Value::OBJECT:
				return "OBJECT";
			case ElementType::Value::SZARRAY:
				return "SZARRAY";
			case ElementType::Value::CMOD_REQD:
				return "CMOD_REQD";
			case ElementType::Value::CMOD_OPT:
				return "CMOD_OPT";
			case ElementType::Value::INTERNAL:
				return "INTERNAL";
			case ElementType::Value::MODIFIER:
				return "MODIFIER";
			case ElementType::Value::SENTINEL:
				return "SENTINEL";
			case ElementType::Value::PINNED:
				return "PINNED";
			default:
				break;
			}
			return "";
		}
	};
}

#endif