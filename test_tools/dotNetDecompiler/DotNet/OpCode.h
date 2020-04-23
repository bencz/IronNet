#ifndef __DOTNET_OPCODE_H__
#define __DOTNET_OPCODE_H__

#include <string>
#include <map>

namespace PEAnalyzer
{
	struct IlOpcodeType
	{
		enum Value
		{
			ANNOTATION = 0,
			MACRO = 1,
			NTERNAL = 2,
			OBJMODEL = 3,
			PREFIX = 4,
			PRIMITIVE = 5
		};
	};

	struct IlOpcodeOperangType
	{
		enum Value
		{
			INLINEBRTARGET = 0,
			INLINEFIELD = 1,
			INLINEI = 2,
			INLINEI8 = 3,
			INLINEMETHOD = 4,
			INLINENONE = 5,
			INLINEPHI = 6,
			INLINER = 7,
			INLINESIG = 9,
			INLINESTRING = 10,
			INLINESWITCH = 11,
			INLINETOK = 12,
			INLINETYPE = 13,
			INLINEVAR = 14,
			SHORTINLINEBRTARGET = 15,
			SHORTINLINEI = 16,
			SHORTINLINER = 17,
			SHORTINLINEVAR = 18
		};
	};

	struct IlOpcodeFlowControl
	{
		enum Value
		{
			BRANCH = 0,
			BREAK = 1,
			CALL = 2,
			COND_BRANCH = 3,
			META = 4,
			NEXT = 5,
			PHI = 6,
			RETURN = 7,
			THROW = 8
		};
	};

	struct IlOpcodeBehaviour
	{
		enum Value
		{
			POP0 = 0,
			POP1 = 1,
			POP1_POP1 = 2,
			POPI = 3,
			POPI_POP1 = 4,
			POPI_POPI = 5,
			POPI_POPI8 = 6,
			POPI_POPI_POPI = 7,
			POPI_POPR4 = 8,
			POPI_POPR8 = 9,
			POPREF = 10,
			POPREF_POP1 = 11,
			POPREF_POPI = 12,
			POPREF_POPI_POPI = 13,
			POPREF_POPI_POPI8 = 14,
			POPREF_POPI_POPR4 = 15,
			POPREF_POPI_POPR8 = 16,
			POPREF_POPI_POPREF = 17,
			PUSH0 = 18,
			PUSH1 = 19,
			PUSH1_PUSH1 = 20,
			PUSHI = 21,
			PUSHI8 = 22,
			PUSHR4 = 23,
			PUSHR8 = 24,
			PUSHREF = 25,
			VARPOP = 26,
			VARPUSH = 27,
			POPREF_POPI_POP1 = 28
		};
	};

	class OpCode
	{
	public:
		IlOpcodeOperangType::Value _operandType;
		IlOpcodeFlowControl::Value _flowControl;
		IlOpcodeType::Value _opCodeType;
		IlOpcodeBehaviour::Value _stackBehaviourPop;
		IlOpcodeBehaviour::Value _stackBehaviourPush;
		int _size;
		short _value;
		std::string _name;

		OpCode(std::string name);
	};

	class OpCodes
	{
	public:
		static void initOpCodes();
		static std::map<short, OpCode*> Data;
	};
}

#endif