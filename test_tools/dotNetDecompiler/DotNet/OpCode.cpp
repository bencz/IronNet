#include "IncFileNm.h"
#include OPCODE_H

namespace PEAnalyzer
{
	OpCode::OpCode(std::string name)
		: _name(name)
	{
	}

	std::map<short, OpCode*> OpCodes::Data;
	void OpCodes::initOpCodes()
	{
		// Opcode: nop
		OpCodes::Data[0x00] = new OpCode("nop");
		OpCodes::Data[0x00]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x00]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x00]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x00]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP0;
		OpCodes::Data[0x00]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x00]->_size = 1;
		OpCodes::Data[0x00]->_value = 0x00;

		// Opcode: break
		OpCodes::Data[0x01] = new OpCode("break");
		OpCodes::Data[0x01]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x01]->_flowControl = IlOpcodeFlowControl::Value::BREAK;
		OpCodes::Data[0x01]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x01]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP0;
		OpCodes::Data[0x01]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x01]->_size = 1;
		OpCodes::Data[0x01]->_value = 0x01;

		// Opcode: ldarg.0
		OpCodes::Data[0x02] = new OpCode("ldarg.0");
		OpCodes::Data[0x02]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x02]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x02]->_opCodeType = IlOpcodeType::Value::MACRO;
		OpCodes::Data[0x02]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP0;
		OpCodes::Data[0x02]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH1;
		OpCodes::Data[0x02]->_size = 1;
		OpCodes::Data[0x02]->_value = 0x02;

		// Opcode: ldarg.1
		OpCodes::Data[0x03] = new OpCode("ldarg.1");
		OpCodes::Data[0x03]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x03]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x03]->_opCodeType = IlOpcodeType::Value::MACRO;
		OpCodes::Data[0x03]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP0;
		OpCodes::Data[0x03]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH1;
		OpCodes::Data[0x03]->_size = 1;
		OpCodes::Data[0x03]->_value = 0x03;

		// Opcode: ldarg.2
		OpCodes::Data[0x04] = new OpCode("ldarg.2");
		OpCodes::Data[0x04]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x04]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x04]->_opCodeType = IlOpcodeType::Value::MACRO;
		OpCodes::Data[0x04]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP0;
		OpCodes::Data[0x04]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH1;
		OpCodes::Data[0x04]->_size = 1;
		OpCodes::Data[0x04]->_value = 0x04;

		// Opcode: ldarg.3
		OpCodes::Data[0x05] = new OpCode("ldarg.3");
		OpCodes::Data[0x05]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x05]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x05]->_opCodeType = IlOpcodeType::Value::MACRO;
		OpCodes::Data[0x05]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP0;
		OpCodes::Data[0x05]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH1;
		OpCodes::Data[0x05]->_size = 1;
		OpCodes::Data[0x05]->_value = 0x05;

		// Opcode: ldloc.0
		OpCodes::Data[0x06] = new OpCode("ldloc.0");
		OpCodes::Data[0x06]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x06]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x06]->_opCodeType = IlOpcodeType::Value::MACRO;
		OpCodes::Data[0x06]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP0;
		OpCodes::Data[0x06]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH1;
		OpCodes::Data[0x06]->_size = 1;
		OpCodes::Data[0x06]->_value = 0x06;

		// Opcode: ldloc.1
		OpCodes::Data[0x07] = new OpCode("ldloc.1");
		OpCodes::Data[0x07]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x07]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x07]->_opCodeType = IlOpcodeType::Value::MACRO;
		OpCodes::Data[0x07]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP0;
		OpCodes::Data[0x07]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH1;
		OpCodes::Data[0x07]->_size = 1;
		OpCodes::Data[0x07]->_value = 0x07;

		// Opcode: ldloc.2
		OpCodes::Data[0x08] = new OpCode("ldloc.2");
		OpCodes::Data[0x08]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x08]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x08]->_opCodeType = IlOpcodeType::Value::MACRO;
		OpCodes::Data[0x08]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP0;
		OpCodes::Data[0x08]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH1;
		OpCodes::Data[0x08]->_size = 1;
		OpCodes::Data[0x08]->_value = 0x08;

		// Opcode: ldloc.3
		OpCodes::Data[0x09] = new OpCode("ldloc.3");
		OpCodes::Data[0x09]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x09]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x09]->_opCodeType = IlOpcodeType::Value::MACRO;
		OpCodes::Data[0x09]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP0;
		OpCodes::Data[0x09]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH1;
		OpCodes::Data[0x09]->_size = 1;
		OpCodes::Data[0x09]->_value = 0x09;

		// Opcode: stloc.0
		OpCodes::Data[0x0a] = new OpCode("stloc.0");
		OpCodes::Data[0x0a]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x0a]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x0a]->_opCodeType = IlOpcodeType::Value::MACRO;
		OpCodes::Data[0x0a]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1;
		OpCodes::Data[0x0a]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x0a]->_size = 1;
		OpCodes::Data[0x0a]->_value = 0x0a;

		// Opcode: stloc.1
		OpCodes::Data[0x0b] = new OpCode("stloc.1");
		OpCodes::Data[0x0b]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x0b]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x0b]->_opCodeType = IlOpcodeType::Value::MACRO;
		OpCodes::Data[0x0b]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1;
		OpCodes::Data[0x0b]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x0b]->_size = 1;
		OpCodes::Data[0x0b]->_value = 0x0b;

		// Opcode: stloc.2
		OpCodes::Data[0x0c] = new OpCode("stloc.2");
		OpCodes::Data[0x0c]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x0c]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x0c]->_opCodeType = IlOpcodeType::Value::MACRO;
		OpCodes::Data[0x0c]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1;
		OpCodes::Data[0x0c]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x0c]->_size = 1;
		OpCodes::Data[0x0c]->_value = 0x0c;

		// Opcode: stloc.3
		OpCodes::Data[0x0d] = new OpCode("stloc.3");
		OpCodes::Data[0x0d]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x0d]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x0d]->_opCodeType = IlOpcodeType::Value::MACRO;
		OpCodes::Data[0x0d]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1;
		OpCodes::Data[0x0d]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x0d]->_size = 1;
		OpCodes::Data[0x0d]->_value = 0x0d;

		// Opcode: ldarg.s
		OpCodes::Data[0x0e] = new OpCode("ldarg.s");
		OpCodes::Data[0x0e]->_operandType = IlOpcodeOperangType::Value::SHORTINLINEVAR;
		OpCodes::Data[0x0e]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x0e]->_opCodeType = IlOpcodeType::Value::MACRO;
		OpCodes::Data[0x0e]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP0;
		OpCodes::Data[0x0e]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH1;
		OpCodes::Data[0x0e]->_size = 1;
		OpCodes::Data[0x0e]->_value = 0x0e;

		// Opcode: ldarga.s
		OpCodes::Data[0x0f] = new OpCode("ldarga.s");
		OpCodes::Data[0x0f]->_operandType = IlOpcodeOperangType::Value::SHORTINLINEVAR;
		OpCodes::Data[0x0f]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x0f]->_opCodeType = IlOpcodeType::Value::MACRO;
		OpCodes::Data[0x0f]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP0;
		OpCodes::Data[0x0f]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0x0f]->_size = 1;
		OpCodes::Data[0x0f]->_value = 0x0f;

		// Opcode: starg.s
		OpCodes::Data[0x10] = new OpCode("starg.s");
		OpCodes::Data[0x10]->_operandType = IlOpcodeOperangType::Value::SHORTINLINEVAR;
		OpCodes::Data[0x10]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x10]->_opCodeType = IlOpcodeType::Value::MACRO;
		OpCodes::Data[0x10]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1;
		OpCodes::Data[0x10]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x10]->_size = 1;
		OpCodes::Data[0x10]->_value = 0x10;

		// Opcode: ldloc.s
		OpCodes::Data[0x11] = new OpCode("ldloc.s");
		OpCodes::Data[0x11]->_operandType = IlOpcodeOperangType::Value::SHORTINLINEVAR;
		OpCodes::Data[0x11]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x11]->_opCodeType = IlOpcodeType::Value::MACRO;
		OpCodes::Data[0x11]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP0;
		OpCodes::Data[0x11]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH1;
		OpCodes::Data[0x11]->_size = 1;
		OpCodes::Data[0x11]->_value = 0x11;

		// Opcode: ldloca.s
		OpCodes::Data[0x12] = new OpCode("ldloca.s");
		OpCodes::Data[0x12]->_operandType = IlOpcodeOperangType::Value::SHORTINLINEVAR;
		OpCodes::Data[0x12]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x12]->_opCodeType = IlOpcodeType::Value::MACRO;
		OpCodes::Data[0x12]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP0;
		OpCodes::Data[0x12]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0x12]->_size = 1;
		OpCodes::Data[0x12]->_value = 0x12;

		// Opcode: stloc.s
		OpCodes::Data[0x13] = new OpCode("stloc.s");
		OpCodes::Data[0x13]->_operandType = IlOpcodeOperangType::Value::SHORTINLINEVAR;
		OpCodes::Data[0x13]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x13]->_opCodeType = IlOpcodeType::Value::MACRO;
		OpCodes::Data[0x13]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1;
		OpCodes::Data[0x13]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x13]->_size = 1;
		OpCodes::Data[0x13]->_value = 0x13;

		// Opcode: ldnull
		OpCodes::Data[0x14] = new OpCode("ldnull");
		OpCodes::Data[0x14]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x14]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x14]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x14]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP0;
		OpCodes::Data[0x14]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHREF;
		OpCodes::Data[0x14]->_size = 1;
		OpCodes::Data[0x14]->_value = 0x14;

		// Opcode: ldc.i4.m1
		OpCodes::Data[0x15] = new OpCode("ldc.i4.m1");
		OpCodes::Data[0x15]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x15]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x15]->_opCodeType = IlOpcodeType::Value::MACRO;
		OpCodes::Data[0x15]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP0;
		OpCodes::Data[0x15]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0x15]->_size = 1;
		OpCodes::Data[0x15]->_value = 0x15;

		// Opcode: ldc.i4.0
		OpCodes::Data[0x16] = new OpCode("ldc.i4.0");
		OpCodes::Data[0x16]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x16]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x16]->_opCodeType = IlOpcodeType::Value::MACRO;
		OpCodes::Data[0x16]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP0;
		OpCodes::Data[0x16]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0x16]->_size = 1;
		OpCodes::Data[0x16]->_value = 0x16;

		// Opcode: ldc.i4.1
		OpCodes::Data[0x17] = new OpCode("ldc.i4.1");
		OpCodes::Data[0x17]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x17]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x17]->_opCodeType = IlOpcodeType::Value::MACRO;
		OpCodes::Data[0x17]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP0;
		OpCodes::Data[0x17]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0x17]->_size = 1;
		OpCodes::Data[0x17]->_value = 0x17;

		// Opcode: ldc.i4.2
		OpCodes::Data[0x18] = new OpCode("ldc.i4.2");
		OpCodes::Data[0x18]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x18]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x18]->_opCodeType = IlOpcodeType::Value::MACRO;
		OpCodes::Data[0x18]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP0;
		OpCodes::Data[0x18]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0x18]->_size = 1;
		OpCodes::Data[0x18]->_value = 0x18;

		// Opcode: ldc.i4.3
		OpCodes::Data[0x19] = new OpCode("ldc.i4.3");
		OpCodes::Data[0x19]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x19]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x19]->_opCodeType = IlOpcodeType::Value::MACRO;
		OpCodes::Data[0x19]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP0;
		OpCodes::Data[0x19]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0x19]->_size = 1;
		OpCodes::Data[0x19]->_value = 0x19;

		// Opcode: ldc.i4.4
		OpCodes::Data[0x1a] = new OpCode("ldc.i4.4");
		OpCodes::Data[0x1a]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x1a]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x1a]->_opCodeType = IlOpcodeType::Value::MACRO;
		OpCodes::Data[0x1a]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP0;
		OpCodes::Data[0x1a]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0x1a]->_size = 1;
		OpCodes::Data[0x1a]->_value = 0x1a;

		// Opcode: ldc.i4.5
		OpCodes::Data[0x1b] = new OpCode("ldc.i4.5");
		OpCodes::Data[0x1b]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x1b]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x1b]->_opCodeType = IlOpcodeType::Value::MACRO;
		OpCodes::Data[0x1b]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP0;
		OpCodes::Data[0x1b]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0x1b]->_size = 1;
		OpCodes::Data[0x1b]->_value = 0x1b;

		// Opcode: ldc.i4.6
		OpCodes::Data[0x1c] = new OpCode("ldc.i4.6");
		OpCodes::Data[0x1c]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x1c]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x1c]->_opCodeType = IlOpcodeType::Value::MACRO;
		OpCodes::Data[0x1c]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP0;
		OpCodes::Data[0x1c]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0x1c]->_size = 1;
		OpCodes::Data[0x1c]->_value = 0x1c;

		// Opcode: ldc.i4.7
		OpCodes::Data[0x1d] = new OpCode("ldc.i4.7");
		OpCodes::Data[0x1d]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x1d]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x1d]->_opCodeType = IlOpcodeType::Value::MACRO;
		OpCodes::Data[0x1d]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP0;
		OpCodes::Data[0x1d]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0x1d]->_size = 1;
		OpCodes::Data[0x1d]->_value = 0x1d;

		// Opcode: ldc.i4.8
		OpCodes::Data[0x1e] = new OpCode("ldc.i4.8");
		OpCodes::Data[0x1e]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x1e]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x1e]->_opCodeType = IlOpcodeType::Value::MACRO;
		OpCodes::Data[0x1e]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP0;
		OpCodes::Data[0x1e]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0x1e]->_size = 1;
		OpCodes::Data[0x1e]->_value = 0x1e;

		// Opcode: ldc.i4.s
		OpCodes::Data[0x1f] = new OpCode("ldc.i4.s");
		OpCodes::Data[0x1f]->_operandType = IlOpcodeOperangType::Value::SHORTINLINEI;
		OpCodes::Data[0x1f]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x1f]->_opCodeType = IlOpcodeType::Value::MACRO;
		OpCodes::Data[0x1f]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP0;
		OpCodes::Data[0x1f]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0x1f]->_size = 1;
		OpCodes::Data[0x1f]->_value = 0x1f;

		// Opcode: ldc.i4
		OpCodes::Data[0x20] = new OpCode("ldc.i4");
		OpCodes::Data[0x20]->_operandType = IlOpcodeOperangType::Value::INLINEI;
		OpCodes::Data[0x20]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x20]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x20]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP0;
		OpCodes::Data[0x20]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0x20]->_size = 1;
		OpCodes::Data[0x20]->_value = 0x20;

		// Opcode: ldc.i8
		OpCodes::Data[0x21] = new OpCode("ldc.i8");
		OpCodes::Data[0x21]->_operandType = IlOpcodeOperangType::Value::INLINEI8;
		OpCodes::Data[0x21]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x21]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x21]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP0;
		OpCodes::Data[0x21]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI8;
		OpCodes::Data[0x21]->_size = 1;
		OpCodes::Data[0x21]->_value = 0x21;

		// Opcode: ldc.r4
		OpCodes::Data[0x22] = new OpCode("ldc.r4");
		OpCodes::Data[0x22]->_operandType = IlOpcodeOperangType::Value::SHORTINLINER;
		OpCodes::Data[0x22]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x22]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x22]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP0;
		OpCodes::Data[0x22]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHR4;
		OpCodes::Data[0x22]->_size = 1;
		OpCodes::Data[0x22]->_value = 0x22;

		// Opcode: ldc.r8
		OpCodes::Data[0x23] = new OpCode("ldc.r8");
		OpCodes::Data[0x23]->_operandType = IlOpcodeOperangType::Value::INLINER;
		OpCodes::Data[0x23]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x23]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x23]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP0;
		OpCodes::Data[0x23]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHR8;
		OpCodes::Data[0x23]->_size = 1;
		OpCodes::Data[0x23]->_value = 0x23;

		// Opcode: dup
		OpCodes::Data[0x25] = new OpCode("dup");
		OpCodes::Data[0x25]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x25]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x25]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x25]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1;
		OpCodes::Data[0x25]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH1_PUSH1;
		OpCodes::Data[0x25]->_size = 1;
		OpCodes::Data[0x25]->_value = 0x25;

		// Opcode: pop
		OpCodes::Data[0x26] = new OpCode("pop");
		OpCodes::Data[0x26]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x26]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x26]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x26]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1;
		OpCodes::Data[0x26]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x26]->_size = 1;
		OpCodes::Data[0x26]->_value = 0x26;

		// Opcode: jmp
		OpCodes::Data[0x27] = new OpCode("jmp");
		OpCodes::Data[0x27]->_operandType = IlOpcodeOperangType::Value::INLINEMETHOD;
		OpCodes::Data[0x27]->_flowControl = IlOpcodeFlowControl::Value::CALL;
		OpCodes::Data[0x27]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x27]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP0;
		OpCodes::Data[0x27]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x27]->_size = 1;
		OpCodes::Data[0x27]->_value = 0x27;

		// Opcode: call
		OpCodes::Data[0x28] = new OpCode("call");
		OpCodes::Data[0x28]->_operandType = IlOpcodeOperangType::Value::INLINEMETHOD;
		OpCodes::Data[0x28]->_flowControl = IlOpcodeFlowControl::Value::CALL;
		OpCodes::Data[0x28]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x28]->_stackBehaviourPop = IlOpcodeBehaviour::Value::VARPOP;
		OpCodes::Data[0x28]->_stackBehaviourPush = IlOpcodeBehaviour::Value::VARPUSH;
		OpCodes::Data[0x28]->_size = 1;
		OpCodes::Data[0x28]->_value = 0x28;

		// Opcode: calli
		OpCodes::Data[0x29] = new OpCode("calli");
		OpCodes::Data[0x29]->_operandType = IlOpcodeOperangType::Value::INLINESIG;
		OpCodes::Data[0x29]->_flowControl = IlOpcodeFlowControl::Value::CALL;
		OpCodes::Data[0x29]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x29]->_stackBehaviourPop = IlOpcodeBehaviour::Value::VARPOP;
		OpCodes::Data[0x29]->_stackBehaviourPush = IlOpcodeBehaviour::Value::VARPUSH;
		OpCodes::Data[0x29]->_size = 1;
		OpCodes::Data[0x29]->_value = 0x29;

		// Opcode: ret
		OpCodes::Data[0x2a] = new OpCode("ret");
		OpCodes::Data[0x2a]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x2a]->_flowControl = IlOpcodeFlowControl::Value::RETURN;
		OpCodes::Data[0x2a]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x2a]->_stackBehaviourPop = IlOpcodeBehaviour::Value::VARPOP;
		OpCodes::Data[0x2a]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x2a]->_size = 1;
		OpCodes::Data[0x2a]->_value = 0x2a;

		// Opcode: br.s
		OpCodes::Data[0x2b] = new OpCode("br.s");
		OpCodes::Data[0x2b]->_operandType = IlOpcodeOperangType::Value::SHORTINLINEBRTARGET;
		OpCodes::Data[0x2b]->_flowControl = IlOpcodeFlowControl::Value::BRANCH;
		OpCodes::Data[0x2b]->_opCodeType = IlOpcodeType::Value::MACRO;
		OpCodes::Data[0x2b]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP0;
		OpCodes::Data[0x2b]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x2b]->_size = 1;
		OpCodes::Data[0x2b]->_value = 0x2b;

		// Opcode: brfalse.s
		OpCodes::Data[0x2c] = new OpCode("brfalse.s");
		OpCodes::Data[0x2c]->_operandType = IlOpcodeOperangType::Value::SHORTINLINEBRTARGET;
		OpCodes::Data[0x2c]->_flowControl = IlOpcodeFlowControl::Value::COND_BRANCH;
		OpCodes::Data[0x2c]->_opCodeType = IlOpcodeType::Value::MACRO;
		OpCodes::Data[0x2c]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPI;
		OpCodes::Data[0x2c]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x2c]->_size = 1;
		OpCodes::Data[0x2c]->_value = 0x2c;

		// Opcode: brtrue.s
		OpCodes::Data[0x2d] = new OpCode("brtrue.s");
		OpCodes::Data[0x2d]->_operandType = IlOpcodeOperangType::Value::SHORTINLINEBRTARGET;
		OpCodes::Data[0x2d]->_flowControl = IlOpcodeFlowControl::Value::COND_BRANCH;
		OpCodes::Data[0x2d]->_opCodeType = IlOpcodeType::Value::MACRO;
		OpCodes::Data[0x2d]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPI;
		OpCodes::Data[0x2d]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x2d]->_size = 1;
		OpCodes::Data[0x2d]->_value = 0x2d;

		// Opcode: beq.s
		OpCodes::Data[0x2e] = new OpCode("beq.s");
		OpCodes::Data[0x2e]->_operandType = IlOpcodeOperangType::Value::SHORTINLINEBRTARGET;
		OpCodes::Data[0x2e]->_flowControl = IlOpcodeFlowControl::Value::COND_BRANCH;
		OpCodes::Data[0x2e]->_opCodeType = IlOpcodeType::Value::MACRO;
		OpCodes::Data[0x2e]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1_POP1;
		OpCodes::Data[0x2e]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x2e]->_size = 1;
		OpCodes::Data[0x2e]->_value = 0x2e;

		// Opcode: bge.s
		OpCodes::Data[0x2f] = new OpCode("bge.s");
		OpCodes::Data[0x2f]->_operandType = IlOpcodeOperangType::Value::SHORTINLINEBRTARGET;
		OpCodes::Data[0x2f]->_flowControl = IlOpcodeFlowControl::Value::COND_BRANCH;
		OpCodes::Data[0x2f]->_opCodeType = IlOpcodeType::Value::MACRO;
		OpCodes::Data[0x2f]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1_POP1;
		OpCodes::Data[0x2f]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x2f]->_size = 1;
		OpCodes::Data[0x2f]->_value = 0x2f;

		// Opcode: bgt.s
		OpCodes::Data[0x30] = new OpCode("bgt.s");
		OpCodes::Data[0x30]->_operandType = IlOpcodeOperangType::Value::SHORTINLINEBRTARGET;
		OpCodes::Data[0x30]->_flowControl = IlOpcodeFlowControl::Value::COND_BRANCH;
		OpCodes::Data[0x30]->_opCodeType = IlOpcodeType::Value::MACRO;
		OpCodes::Data[0x30]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1_POP1;
		OpCodes::Data[0x30]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x30]->_size = 1;
		OpCodes::Data[0x30]->_value = 0x30;

		// Opcode: ble.s
		OpCodes::Data[0x31] = new OpCode("ble.s");
		OpCodes::Data[0x31]->_operandType = IlOpcodeOperangType::Value::SHORTINLINEBRTARGET;
		OpCodes::Data[0x31]->_flowControl = IlOpcodeFlowControl::Value::COND_BRANCH;
		OpCodes::Data[0x31]->_opCodeType = IlOpcodeType::Value::MACRO;
		OpCodes::Data[0x31]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1_POP1;
		OpCodes::Data[0x31]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x31]->_size = 1;
		OpCodes::Data[0x31]->_value = 0x31;

		// Opcode: blt.s
		OpCodes::Data[0x32] = new OpCode("blt.s");
		OpCodes::Data[0x32]->_operandType = IlOpcodeOperangType::Value::SHORTINLINEBRTARGET;
		OpCodes::Data[0x32]->_flowControl = IlOpcodeFlowControl::Value::COND_BRANCH;
		OpCodes::Data[0x32]->_opCodeType = IlOpcodeType::Value::MACRO;
		OpCodes::Data[0x32]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1_POP1;
		OpCodes::Data[0x32]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x32]->_size = 1;
		OpCodes::Data[0x32]->_value = 0x32;

		// Opcode: bne.un.s
		OpCodes::Data[0x33] = new OpCode("bne.un.s");
		OpCodes::Data[0x33]->_operandType = IlOpcodeOperangType::Value::SHORTINLINEBRTARGET;
		OpCodes::Data[0x33]->_flowControl = IlOpcodeFlowControl::Value::COND_BRANCH;
		OpCodes::Data[0x33]->_opCodeType = IlOpcodeType::Value::MACRO;
		OpCodes::Data[0x33]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1_POP1;
		OpCodes::Data[0x33]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x33]->_size = 1;
		OpCodes::Data[0x33]->_value = 0x33;

		// Opcode: bge.un.s
		OpCodes::Data[0x34] = new OpCode("bge.un.s");
		OpCodes::Data[0x34]->_operandType = IlOpcodeOperangType::Value::SHORTINLINEBRTARGET;
		OpCodes::Data[0x34]->_flowControl = IlOpcodeFlowControl::Value::COND_BRANCH;
		OpCodes::Data[0x34]->_opCodeType = IlOpcodeType::Value::MACRO;
		OpCodes::Data[0x34]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1_POP1;
		OpCodes::Data[0x34]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x34]->_size = 1;
		OpCodes::Data[0x34]->_value = 0x34;

		// Opcode: bgt.un.s
		OpCodes::Data[0x35] = new OpCode("bgt.un.s");
		OpCodes::Data[0x35]->_operandType = IlOpcodeOperangType::Value::SHORTINLINEBRTARGET;
		OpCodes::Data[0x35]->_flowControl = IlOpcodeFlowControl::Value::COND_BRANCH;
		OpCodes::Data[0x35]->_opCodeType = IlOpcodeType::Value::MACRO;
		OpCodes::Data[0x35]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1_POP1;
		OpCodes::Data[0x35]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x35]->_size = 1;
		OpCodes::Data[0x35]->_value = 0x35;

		// Opcode: ble.un.s
		OpCodes::Data[0x36] = new OpCode("ble.un.s");
		OpCodes::Data[0x36]->_operandType = IlOpcodeOperangType::Value::SHORTINLINEBRTARGET;
		OpCodes::Data[0x36]->_flowControl = IlOpcodeFlowControl::Value::COND_BRANCH;
		OpCodes::Data[0x36]->_opCodeType = IlOpcodeType::Value::MACRO;
		OpCodes::Data[0x36]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1_POP1;
		OpCodes::Data[0x36]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x36]->_size = 1;
		OpCodes::Data[0x36]->_value = 0x36;

		// Opcode: blt.un.s
		OpCodes::Data[0x37] = new OpCode("blt.un.s");
		OpCodes::Data[0x37]->_operandType = IlOpcodeOperangType::Value::SHORTINLINEBRTARGET;
		OpCodes::Data[0x37]->_flowControl = IlOpcodeFlowControl::Value::COND_BRANCH;
		OpCodes::Data[0x37]->_opCodeType = IlOpcodeType::Value::MACRO;
		OpCodes::Data[0x37]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1_POP1;
		OpCodes::Data[0x37]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x37]->_size = 1;
		OpCodes::Data[0x37]->_value = 0x37;

		// Opcode: br
		OpCodes::Data[0x38] = new OpCode("br");
		OpCodes::Data[0x38]->_operandType = IlOpcodeOperangType::Value::INLINEBRTARGET;
		OpCodes::Data[0x38]->_flowControl = IlOpcodeFlowControl::Value::BRANCH;
		OpCodes::Data[0x38]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x38]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP0;
		OpCodes::Data[0x38]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x38]->_size = 1;
		OpCodes::Data[0x38]->_value = 0x38;

		// Opcode: brfalse
		OpCodes::Data[0x39] = new OpCode("brfalse");
		OpCodes::Data[0x39]->_operandType = IlOpcodeOperangType::Value::INLINEBRTARGET;
		OpCodes::Data[0x39]->_flowControl = IlOpcodeFlowControl::Value::COND_BRANCH;
		OpCodes::Data[0x39]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x39]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPI;
		OpCodes::Data[0x39]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x39]->_size = 1;
		OpCodes::Data[0x39]->_value = 0x39;

		// Opcode: brtrue
		OpCodes::Data[0x3a] = new OpCode("brtrue");
		OpCodes::Data[0x3a]->_operandType = IlOpcodeOperangType::Value::INLINEBRTARGET;
		OpCodes::Data[0x3a]->_flowControl = IlOpcodeFlowControl::Value::COND_BRANCH;
		OpCodes::Data[0x3a]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x3a]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPI;
		OpCodes::Data[0x3a]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x3a]->_size = 1;
		OpCodes::Data[0x3a]->_value = 0x3a;

		// Opcode: beq
		OpCodes::Data[0x3b] = new OpCode("beq");
		OpCodes::Data[0x3b]->_operandType = IlOpcodeOperangType::Value::INLINEBRTARGET;
		OpCodes::Data[0x3b]->_flowControl = IlOpcodeFlowControl::Value::COND_BRANCH;
		OpCodes::Data[0x3b]->_opCodeType = IlOpcodeType::Value::MACRO;
		OpCodes::Data[0x3b]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1_POP1;
		OpCodes::Data[0x3b]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x3b]->_size = 1;
		OpCodes::Data[0x3b]->_value = 0x3b;

		// Opcode: bge
		OpCodes::Data[0x3c] = new OpCode("bge");
		OpCodes::Data[0x3c]->_operandType = IlOpcodeOperangType::Value::INLINEBRTARGET;
		OpCodes::Data[0x3c]->_flowControl = IlOpcodeFlowControl::Value::COND_BRANCH;
		OpCodes::Data[0x3c]->_opCodeType = IlOpcodeType::Value::MACRO;
		OpCodes::Data[0x3c]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1_POP1;
		OpCodes::Data[0x3c]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x3c]->_size = 1;
		OpCodes::Data[0x3c]->_value = 0x3c;

		// Opcode: bgt
		OpCodes::Data[0x3d] = new OpCode("bgt");
		OpCodes::Data[0x3d]->_operandType = IlOpcodeOperangType::Value::INLINEBRTARGET;
		OpCodes::Data[0x3d]->_flowControl = IlOpcodeFlowControl::Value::COND_BRANCH;
		OpCodes::Data[0x3d]->_opCodeType = IlOpcodeType::Value::MACRO;
		OpCodes::Data[0x3d]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1_POP1;
		OpCodes::Data[0x3d]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x3d]->_size = 1;
		OpCodes::Data[0x3d]->_value = 0x3d;

		// Opcode: ble
		OpCodes::Data[0x3e] = new OpCode("ble");
		OpCodes::Data[0x3e]->_operandType = IlOpcodeOperangType::Value::INLINEBRTARGET;
		OpCodes::Data[0x3e]->_flowControl = IlOpcodeFlowControl::Value::COND_BRANCH;
		OpCodes::Data[0x3e]->_opCodeType = IlOpcodeType::Value::MACRO;
		OpCodes::Data[0x3e]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1_POP1;
		OpCodes::Data[0x3e]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x3e]->_size = 1;
		OpCodes::Data[0x3e]->_value = 0x3e;

		// Opcode: blt
		OpCodes::Data[0x3f] = new OpCode("blt");
		OpCodes::Data[0x3f]->_operandType = IlOpcodeOperangType::Value::INLINEBRTARGET;
		OpCodes::Data[0x3f]->_flowControl = IlOpcodeFlowControl::Value::COND_BRANCH;
		OpCodes::Data[0x3f]->_opCodeType = IlOpcodeType::Value::MACRO;
		OpCodes::Data[0x3f]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1_POP1;
		OpCodes::Data[0x3f]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x3f]->_size = 1;
		OpCodes::Data[0x3f]->_value = 0x3f;

		// Opcode: bne.un
		OpCodes::Data[0x40] = new OpCode("bne.un");
		OpCodes::Data[0x40]->_operandType = IlOpcodeOperangType::Value::INLINEBRTARGET;
		OpCodes::Data[0x40]->_flowControl = IlOpcodeFlowControl::Value::COND_BRANCH;
		OpCodes::Data[0x40]->_opCodeType = IlOpcodeType::Value::MACRO;
		OpCodes::Data[0x40]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1_POP1;
		OpCodes::Data[0x40]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x40]->_size = 1;
		OpCodes::Data[0x40]->_value = 0x40;

		// Opcode: bge.un
		OpCodes::Data[0x41] = new OpCode("bge.un");
		OpCodes::Data[0x41]->_operandType = IlOpcodeOperangType::Value::INLINEBRTARGET;
		OpCodes::Data[0x41]->_flowControl = IlOpcodeFlowControl::Value::COND_BRANCH;
		OpCodes::Data[0x41]->_opCodeType = IlOpcodeType::Value::MACRO;
		OpCodes::Data[0x41]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1_POP1;
		OpCodes::Data[0x41]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x41]->_size = 1;
		OpCodes::Data[0x41]->_value = 0x41;

		// Opcode: bgt.un
		OpCodes::Data[0x42] = new OpCode("bgt.un");
		OpCodes::Data[0x42]->_operandType = IlOpcodeOperangType::Value::INLINEBRTARGET;
		OpCodes::Data[0x42]->_flowControl = IlOpcodeFlowControl::Value::COND_BRANCH;
		OpCodes::Data[0x42]->_opCodeType = IlOpcodeType::Value::MACRO;
		OpCodes::Data[0x42]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1_POP1;
		OpCodes::Data[0x42]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x42]->_size = 1;
		OpCodes::Data[0x42]->_value = 0x42;

		// Opcode: ble.un
		OpCodes::Data[0x43] = new OpCode("ble.un");
		OpCodes::Data[0x43]->_operandType = IlOpcodeOperangType::Value::INLINEBRTARGET;
		OpCodes::Data[0x43]->_flowControl = IlOpcodeFlowControl::Value::COND_BRANCH;
		OpCodes::Data[0x43]->_opCodeType = IlOpcodeType::Value::MACRO;
		OpCodes::Data[0x43]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1_POP1;
		OpCodes::Data[0x43]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x43]->_size = 1;
		OpCodes::Data[0x43]->_value = 0x43;

		// Opcode: blt.un
		OpCodes::Data[0x44] = new OpCode("blt.un");
		OpCodes::Data[0x44]->_operandType = IlOpcodeOperangType::Value::INLINEBRTARGET;
		OpCodes::Data[0x44]->_flowControl = IlOpcodeFlowControl::Value::COND_BRANCH;
		OpCodes::Data[0x44]->_opCodeType = IlOpcodeType::Value::MACRO;
		OpCodes::Data[0x44]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1_POP1;
		OpCodes::Data[0x44]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x44]->_size = 1;
		OpCodes::Data[0x44]->_value = 0x44;

		// Opcode: switch
		OpCodes::Data[0x45] = new OpCode("switch");
		OpCodes::Data[0x45]->_operandType = IlOpcodeOperangType::Value::INLINESWITCH;
		OpCodes::Data[0x45]->_flowControl = IlOpcodeFlowControl::Value::COND_BRANCH;
		OpCodes::Data[0x45]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x45]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPI;
		OpCodes::Data[0x45]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x45]->_size = 1;
		OpCodes::Data[0x45]->_value = 0x45;

		// Opcode: ldind.i1
		OpCodes::Data[0x46] = new OpCode("ldind.i1");
		OpCodes::Data[0x46]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x46]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x46]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x46]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPI;
		OpCodes::Data[0x46]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0x46]->_size = 1;
		OpCodes::Data[0x46]->_value = 0x46;

		// Opcode: ldind.u1
		OpCodes::Data[0x47] = new OpCode("ldind.u1");
		OpCodes::Data[0x47]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x47]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x47]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x47]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPI;
		OpCodes::Data[0x47]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0x47]->_size = 1;
		OpCodes::Data[0x47]->_value = 0x47;

		// Opcode: ldind.i2
		OpCodes::Data[0x48] = new OpCode("ldind.i2");
		OpCodes::Data[0x48]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x48]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x48]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x48]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPI;
		OpCodes::Data[0x48]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0x48]->_size = 1;
		OpCodes::Data[0x48]->_value = 0x48;

		// Opcode: ldind.u2
		OpCodes::Data[0x49] = new OpCode("ldind.u2");
		OpCodes::Data[0x49]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x49]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x49]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x49]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPI;
		OpCodes::Data[0x49]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0x49]->_size = 1;
		OpCodes::Data[0x49]->_value = 0x49;

		// Opcode: ldind.i4
		OpCodes::Data[0x4a] = new OpCode("ldind.i4");
		OpCodes::Data[0x4a]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x4a]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x4a]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x4a]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPI;
		OpCodes::Data[0x4a]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0x4a]->_size = 1;
		OpCodes::Data[0x4a]->_value = 0x4a;

		// Opcode: ldind.u4
		OpCodes::Data[0x4b] = new OpCode("ldind.u4");
		OpCodes::Data[0x4b]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x4b]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x4b]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x4b]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPI;
		OpCodes::Data[0x4b]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0x4b]->_size = 1;
		OpCodes::Data[0x4b]->_value = 0x4b;

		// Opcode: ldind.i8
		OpCodes::Data[0x4c] = new OpCode("ldind.i8");
		OpCodes::Data[0x4c]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x4c]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x4c]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x4c]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPI;
		OpCodes::Data[0x4c]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI8;
		OpCodes::Data[0x4c]->_size = 1;
		OpCodes::Data[0x4c]->_value = 0x4c;

		// Opcode: ldind.i
		OpCodes::Data[0x4d] = new OpCode("ldind.i");
		OpCodes::Data[0x4d]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x4d]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x4d]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x4d]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPI;
		OpCodes::Data[0x4d]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0x4d]->_size = 1;
		OpCodes::Data[0x4d]->_value = 0x4d;

		// Opcode: ldind.r4
		OpCodes::Data[0x4e] = new OpCode("ldind.r4");
		OpCodes::Data[0x4e]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x4e]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x4e]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x4e]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPI;
		OpCodes::Data[0x4e]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHR4;
		OpCodes::Data[0x4e]->_size = 1;
		OpCodes::Data[0x4e]->_value = 0x4e;

		// Opcode: ldind.r8
		OpCodes::Data[0x4f] = new OpCode("ldind.r8");
		OpCodes::Data[0x4f]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x4f]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x4f]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x4f]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPI;
		OpCodes::Data[0x4f]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHR8;
		OpCodes::Data[0x4f]->_size = 1;
		OpCodes::Data[0x4f]->_value = 0x4f;

		// Opcode: ldind.ref
		OpCodes::Data[0x50] = new OpCode("ldind.ref");
		OpCodes::Data[0x50]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x50]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x50]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x50]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPI;
		OpCodes::Data[0x50]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHREF;
		OpCodes::Data[0x50]->_size = 1;
		OpCodes::Data[0x50]->_value = 0x50;

		// Opcode: stind.ref
		OpCodes::Data[0x51] = new OpCode("stind.ref");
		OpCodes::Data[0x51]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x51]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x51]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x51]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPI_POPI;
		OpCodes::Data[0x51]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x51]->_size = 1;
		OpCodes::Data[0x51]->_value = 0x51;

		// Opcode: stind.i1
		OpCodes::Data[0x52] = new OpCode("stind.i1");
		OpCodes::Data[0x52]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x52]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x52]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x52]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPI_POPI;
		OpCodes::Data[0x52]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x52]->_size = 1;
		OpCodes::Data[0x52]->_value = 0x52;

		// Opcode: stind.i2
		OpCodes::Data[0x53] = new OpCode("stind.i2");
		OpCodes::Data[0x53]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x53]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x53]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x53]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPI_POPI;
		OpCodes::Data[0x53]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x53]->_size = 1;
		OpCodes::Data[0x53]->_value = 0x53;

		// Opcode: stind.i4
		OpCodes::Data[0x54] = new OpCode("stind.i4");
		OpCodes::Data[0x54]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x54]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x54]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x54]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPI_POPI;
		OpCodes::Data[0x54]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x54]->_size = 1;
		OpCodes::Data[0x54]->_value = 0x54;

		// Opcode: stind.i8
		OpCodes::Data[0x55] = new OpCode("stind.i8");
		OpCodes::Data[0x55]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x55]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x55]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x55]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPI_POPI8;
		OpCodes::Data[0x55]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x55]->_size = 1;
		OpCodes::Data[0x55]->_value = 0x55;

		// Opcode: stind.r4
		OpCodes::Data[0x56] = new OpCode("stind.r4");
		OpCodes::Data[0x56]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x56]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x56]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x56]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPI_POPR4;
		OpCodes::Data[0x56]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x56]->_size = 1;
		OpCodes::Data[0x56]->_value = 0x56;

		// Opcode: stind.r8
		OpCodes::Data[0x57] = new OpCode("stind.r8");
		OpCodes::Data[0x57]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x57]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x57]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x57]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPI_POPR8;
		OpCodes::Data[0x57]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x57]->_size = 1;
		OpCodes::Data[0x57]->_value = 0x57;

		// Opcode: add
		OpCodes::Data[0x58] = new OpCode("add");
		OpCodes::Data[0x58]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x58]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x58]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x58]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1_POP1;
		OpCodes::Data[0x58]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH1;
		OpCodes::Data[0x58]->_size = 1;
		OpCodes::Data[0x58]->_value = 0x58;

		// Opcode: sub
		OpCodes::Data[0x59] = new OpCode("sub");
		OpCodes::Data[0x59]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x59]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x59]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x59]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1_POP1;
		OpCodes::Data[0x59]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH1;
		OpCodes::Data[0x59]->_size = 1;
		OpCodes::Data[0x59]->_value = 0x59;

		// Opcode: mul
		OpCodes::Data[0x5a] = new OpCode("mul");
		OpCodes::Data[0x5a]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x5a]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x5a]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x5a]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1_POP1;
		OpCodes::Data[0x5a]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH1;
		OpCodes::Data[0x5a]->_size = 1;
		OpCodes::Data[0x5a]->_value = 0x5a;

		// Opcode: div
		OpCodes::Data[0x5b] = new OpCode("div");
		OpCodes::Data[0x5b]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x5b]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x5b]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x5b]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1_POP1;
		OpCodes::Data[0x5b]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH1;
		OpCodes::Data[0x5b]->_size = 1;
		OpCodes::Data[0x5b]->_value = 0x5b;

		// Opcode: div.un
		OpCodes::Data[0x5c] = new OpCode("div.un");
		OpCodes::Data[0x5c]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x5c]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x5c]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x5c]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1_POP1;
		OpCodes::Data[0x5c]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH1;
		OpCodes::Data[0x5c]->_size = 1;
		OpCodes::Data[0x5c]->_value = 0x5c;

		// Opcode: rem
		OpCodes::Data[0x5d] = new OpCode("rem");
		OpCodes::Data[0x5d]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x5d]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x5d]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x5d]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1_POP1;
		OpCodes::Data[0x5d]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH1;
		OpCodes::Data[0x5d]->_size = 1;
		OpCodes::Data[0x5d]->_value = 0x5d;

		// Opcode: rem.un
		OpCodes::Data[0x5e] = new OpCode("rem.un");
		OpCodes::Data[0x5e]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x5e]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x5e]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x5e]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1_POP1;
		OpCodes::Data[0x5e]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH1;
		OpCodes::Data[0x5e]->_size = 1;
		OpCodes::Data[0x5e]->_value = 0x5e;

		// Opcode: and
		OpCodes::Data[0x5f] = new OpCode("and");
		OpCodes::Data[0x5f]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x5f]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x5f]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x5f]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1_POP1;
		OpCodes::Data[0x5f]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH1;
		OpCodes::Data[0x5f]->_size = 1;
		OpCodes::Data[0x5f]->_value = 0x5f;

		// Opcode: or
		OpCodes::Data[0x60] = new OpCode("or");
		OpCodes::Data[0x60]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x60]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x60]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x60]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1_POP1;
		OpCodes::Data[0x60]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH1;
		OpCodes::Data[0x60]->_size = 1;
		OpCodes::Data[0x60]->_value = 0x60;

		// Opcode: xor
		OpCodes::Data[0x61] = new OpCode("xor");
		OpCodes::Data[0x61]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x61]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x61]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x61]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1_POP1;
		OpCodes::Data[0x61]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH1;
		OpCodes::Data[0x61]->_size = 1;
		OpCodes::Data[0x61]->_value = 0x61;

		// Opcode: shl
		OpCodes::Data[0x62] = new OpCode("shl");
		OpCodes::Data[0x62]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x62]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x62]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x62]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1_POP1;
		OpCodes::Data[0x62]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH1;
		OpCodes::Data[0x62]->_size = 1;
		OpCodes::Data[0x62]->_value = 0x62;

		// Opcode: shr
		OpCodes::Data[0x63] = new OpCode("shr");
		OpCodes::Data[0x63]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x63]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x63]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x63]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1_POP1;
		OpCodes::Data[0x63]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH1;
		OpCodes::Data[0x63]->_size = 1;
		OpCodes::Data[0x63]->_value = 0x63;

		// Opcode: shr.un
		OpCodes::Data[0x64] = new OpCode("shr.un");
		OpCodes::Data[0x64]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x64]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x64]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x64]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1_POP1;
		OpCodes::Data[0x64]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH1;
		OpCodes::Data[0x64]->_size = 1;
		OpCodes::Data[0x64]->_value = 0x64;

		// Opcode: neg
		OpCodes::Data[0x65] = new OpCode("neg");
		OpCodes::Data[0x65]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x65]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x65]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x65]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1;
		OpCodes::Data[0x65]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH1;
		OpCodes::Data[0x65]->_size = 1;
		OpCodes::Data[0x65]->_value = 0x65;

		// Opcode: not
		OpCodes::Data[0x66] = new OpCode("not");
		OpCodes::Data[0x66]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x66]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x66]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x66]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1;
		OpCodes::Data[0x66]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH1;
		OpCodes::Data[0x66]->_size = 1;
		OpCodes::Data[0x66]->_value = 0x66;

		// Opcode: conv.i1
		OpCodes::Data[0x67] = new OpCode("conv.i1");
		OpCodes::Data[0x67]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x67]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x67]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x67]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1;
		OpCodes::Data[0x67]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0x67]->_size = 1;
		OpCodes::Data[0x67]->_value = 0x67;

		// Opcode: conv.i2
		OpCodes::Data[0x68] = new OpCode("conv.i2");
		OpCodes::Data[0x68]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x68]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x68]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x68]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1;
		OpCodes::Data[0x68]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0x68]->_size = 1;
		OpCodes::Data[0x68]->_value = 0x68;

		// Opcode: conv.i4
		OpCodes::Data[0x69] = new OpCode("conv.i4");
		OpCodes::Data[0x69]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x69]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x69]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x69]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1;
		OpCodes::Data[0x69]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0x69]->_size = 1;
		OpCodes::Data[0x69]->_value = 0x69;

		// Opcode: conv.i8
		OpCodes::Data[0x6a] = new OpCode("conv.i8");
		OpCodes::Data[0x6a]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x6a]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x6a]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x6a]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1;
		OpCodes::Data[0x6a]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI8;
		OpCodes::Data[0x6a]->_size = 1;
		OpCodes::Data[0x6a]->_value = 0x6a;

		// Opcode: conv.r4
		OpCodes::Data[0x6b] = new OpCode("conv.r4");
		OpCodes::Data[0x6b]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x6b]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x6b]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x6b]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1;
		OpCodes::Data[0x6b]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHR4;
		OpCodes::Data[0x6b]->_size = 1;
		OpCodes::Data[0x6b]->_value = 0x6b;

		// Opcode: conv.r8
		OpCodes::Data[0x6c] = new OpCode("conv.r8");
		OpCodes::Data[0x6c]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x6c]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x6c]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x6c]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1;
		OpCodes::Data[0x6c]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHR8;
		OpCodes::Data[0x6c]->_size = 1;
		OpCodes::Data[0x6c]->_value = 0x6c;

		// Opcode: conv.u4
		OpCodes::Data[0x6d] = new OpCode("conv.u4");
		OpCodes::Data[0x6d]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x6d]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x6d]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x6d]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1;
		OpCodes::Data[0x6d]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0x6d]->_size = 1;
		OpCodes::Data[0x6d]->_value = 0x6d;

		// Opcode: conv.u8
		OpCodes::Data[0x6e] = new OpCode("conv.u8");
		OpCodes::Data[0x6e]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x6e]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x6e]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x6e]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1;
		OpCodes::Data[0x6e]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI8;
		OpCodes::Data[0x6e]->_size = 1;
		OpCodes::Data[0x6e]->_value = 0x6e;

		// Opcode: callvirt
		OpCodes::Data[0x6f] = new OpCode("callvirt");
		OpCodes::Data[0x6f]->_operandType = IlOpcodeOperangType::Value::INLINEMETHOD;
		OpCodes::Data[0x6f]->_flowControl = IlOpcodeFlowControl::Value::CALL;
		OpCodes::Data[0x6f]->_opCodeType = IlOpcodeType::Value::OBJMODEL;
		OpCodes::Data[0x6f]->_stackBehaviourPop = IlOpcodeBehaviour::Value::VARPOP;
		OpCodes::Data[0x6f]->_stackBehaviourPush = IlOpcodeBehaviour::Value::VARPUSH;
		OpCodes::Data[0x6f]->_size = 1;
		OpCodes::Data[0x6f]->_value = 0x6f;

		// Opcode: cpobj
		OpCodes::Data[0x70] = new OpCode("cpobj");
		OpCodes::Data[0x70]->_operandType = IlOpcodeOperangType::Value::INLINETYPE;
		OpCodes::Data[0x70]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x70]->_opCodeType = IlOpcodeType::Value::OBJMODEL;
		OpCodes::Data[0x70]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPI_POPI;
		OpCodes::Data[0x70]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x70]->_size = 1;
		OpCodes::Data[0x70]->_value = 0x70;

		// Opcode: ldobj
		OpCodes::Data[0x71] = new OpCode("ldobj");
		OpCodes::Data[0x71]->_operandType = IlOpcodeOperangType::Value::INLINETYPE;
		OpCodes::Data[0x71]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x71]->_opCodeType = IlOpcodeType::Value::OBJMODEL;
		OpCodes::Data[0x71]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPI;
		OpCodes::Data[0x71]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH1;
		OpCodes::Data[0x71]->_size = 1;
		OpCodes::Data[0x71]->_value = 0x71;

		// Opcode: ldstr
		OpCodes::Data[0x72] = new OpCode("ldstr");
		OpCodes::Data[0x72]->_operandType = IlOpcodeOperangType::Value::INLINESTRING;
		OpCodes::Data[0x72]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x72]->_opCodeType = IlOpcodeType::Value::OBJMODEL;
		OpCodes::Data[0x72]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP0;
		OpCodes::Data[0x72]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHREF;
		OpCodes::Data[0x72]->_size = 1;
		OpCodes::Data[0x72]->_value = 0x72;

		// Opcode: newobj
		OpCodes::Data[0x73] = new OpCode("newobj");
		OpCodes::Data[0x73]->_operandType = IlOpcodeOperangType::Value::INLINEMETHOD;
		OpCodes::Data[0x73]->_flowControl = IlOpcodeFlowControl::Value::CALL;
		OpCodes::Data[0x73]->_opCodeType = IlOpcodeType::Value::OBJMODEL;
		OpCodes::Data[0x73]->_stackBehaviourPop = IlOpcodeBehaviour::Value::VARPOP;
		OpCodes::Data[0x73]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHREF;
		OpCodes::Data[0x73]->_size = 1;
		OpCodes::Data[0x73]->_value = 0x73;

		// Opcode: castclass
		OpCodes::Data[0x74] = new OpCode("castclass");
		OpCodes::Data[0x74]->_operandType = IlOpcodeOperangType::Value::INLINETYPE;
		OpCodes::Data[0x74]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x74]->_opCodeType = IlOpcodeType::Value::OBJMODEL;
		OpCodes::Data[0x74]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPREF;
		OpCodes::Data[0x74]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHREF;
		OpCodes::Data[0x74]->_size = 1;
		OpCodes::Data[0x74]->_value = 0x74;

		// Opcode: isinst
		OpCodes::Data[0x75] = new OpCode("isinst");
		OpCodes::Data[0x75]->_operandType = IlOpcodeOperangType::Value::INLINETYPE;
		OpCodes::Data[0x75]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x75]->_opCodeType = IlOpcodeType::Value::OBJMODEL;
		OpCodes::Data[0x75]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPREF;
		OpCodes::Data[0x75]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0x75]->_size = 1;
		OpCodes::Data[0x75]->_value = 0x75;

		// Opcode: conv.r.un
		OpCodes::Data[0x76] = new OpCode("conv.r.un");
		OpCodes::Data[0x76]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x76]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x76]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x76]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1;
		OpCodes::Data[0x76]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHR8;
		OpCodes::Data[0x76]->_size = 1;
		OpCodes::Data[0x76]->_value = 0x76;

		// Opcode: unbox
		OpCodes::Data[0x79] = new OpCode("unbox");
		OpCodes::Data[0x79]->_operandType = IlOpcodeOperangType::Value::INLINETYPE;
		OpCodes::Data[0x79]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x79]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x79]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPREF;
		OpCodes::Data[0x79]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0x79]->_size = 1;
		OpCodes::Data[0x79]->_value = 0x79;

		// Opcode: throw
		OpCodes::Data[0x7a] = new OpCode("throw");
		OpCodes::Data[0x7a]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x7a]->_flowControl = IlOpcodeFlowControl::Value::THROW;
		OpCodes::Data[0x7a]->_opCodeType = IlOpcodeType::Value::OBJMODEL;
		OpCodes::Data[0x7a]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPREF;
		OpCodes::Data[0x7a]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x7a]->_size = 1;
		OpCodes::Data[0x7a]->_value = 0x7a;

		// Opcode: ldfld
		OpCodes::Data[0x7b] = new OpCode("ldfld");
		OpCodes::Data[0x7b]->_operandType = IlOpcodeOperangType::Value::INLINEFIELD;
		OpCodes::Data[0x7b]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x7b]->_opCodeType = IlOpcodeType::Value::OBJMODEL;
		OpCodes::Data[0x7b]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPREF;
		OpCodes::Data[0x7b]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH1;
		OpCodes::Data[0x7b]->_size = 1;
		OpCodes::Data[0x7b]->_value = 0x7b;

		// Opcode: ldflda
		OpCodes::Data[0x7c] = new OpCode("ldflda");
		OpCodes::Data[0x7c]->_operandType = IlOpcodeOperangType::Value::INLINEFIELD;
		OpCodes::Data[0x7c]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x7c]->_opCodeType = IlOpcodeType::Value::OBJMODEL;
		OpCodes::Data[0x7c]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPREF;
		OpCodes::Data[0x7c]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0x7c]->_size = 1;
		OpCodes::Data[0x7c]->_value = 0x7c;

		// Opcode: stfld
		OpCodes::Data[0x7d] = new OpCode("stfld");
		OpCodes::Data[0x7d]->_operandType = IlOpcodeOperangType::Value::INLINEFIELD;
		OpCodes::Data[0x7d]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x7d]->_opCodeType = IlOpcodeType::Value::OBJMODEL;
		OpCodes::Data[0x7d]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPREF_POP1;
		OpCodes::Data[0x7d]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x7d]->_size = 1;
		OpCodes::Data[0x7d]->_value = 0x7d;

		// Opcode: ldsfld
		OpCodes::Data[0x7e] = new OpCode("ldsfld");
		OpCodes::Data[0x7e]->_operandType = IlOpcodeOperangType::Value::INLINEFIELD;
		OpCodes::Data[0x7e]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x7e]->_opCodeType = IlOpcodeType::Value::OBJMODEL;
		OpCodes::Data[0x7e]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP0;
		OpCodes::Data[0x7e]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH1;
		OpCodes::Data[0x7e]->_size = 1;
		OpCodes::Data[0x7e]->_value = 0x7e;

		// Opcode: ldsflda
		OpCodes::Data[0x7f] = new OpCode("ldsflda");
		OpCodes::Data[0x7f]->_operandType = IlOpcodeOperangType::Value::INLINEFIELD;
		OpCodes::Data[0x7f]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x7f]->_opCodeType = IlOpcodeType::Value::OBJMODEL;
		OpCodes::Data[0x7f]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP0;
		OpCodes::Data[0x7f]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0x7f]->_size = 1;
		OpCodes::Data[0x7f]->_value = 0x7f;

		// Opcode: stsfld
		OpCodes::Data[0x80] = new OpCode("stsfld");
		OpCodes::Data[0x80]->_operandType = IlOpcodeOperangType::Value::INLINEFIELD;
		OpCodes::Data[0x80]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x80]->_opCodeType = IlOpcodeType::Value::OBJMODEL;
		OpCodes::Data[0x80]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1;
		OpCodes::Data[0x80]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x80]->_size = 1;
		OpCodes::Data[0x80]->_value = 0x80;

		// Opcode: stobj
		OpCodes::Data[0x81] = new OpCode("stobj");
		OpCodes::Data[0x81]->_operandType = IlOpcodeOperangType::Value::INLINETYPE;
		OpCodes::Data[0x81]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x81]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x81]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPI_POP1;
		OpCodes::Data[0x81]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x81]->_size = 1;
		OpCodes::Data[0x81]->_value = 0x81;

		// Opcode: conv.ovf.i1.un
		OpCodes::Data[0x82] = new OpCode("conv.ovf.i1.un");
		OpCodes::Data[0x82]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x82]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x82]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x82]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1;
		OpCodes::Data[0x82]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0x82]->_size = 1;
		OpCodes::Data[0x82]->_value = 0x82;

		// Opcode: conv.ovf.i2.un
		OpCodes::Data[0x83] = new OpCode("conv.ovf.i2.un");
		OpCodes::Data[0x83]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x83]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x83]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x83]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1;
		OpCodes::Data[0x83]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0x83]->_size = 1;
		OpCodes::Data[0x83]->_value = 0x83;

		// Opcode: conv.ovf.i4.un
		OpCodes::Data[0x84] = new OpCode("conv.ovf.i4.un");
		OpCodes::Data[0x84]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x84]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x84]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x84]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1;
		OpCodes::Data[0x84]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0x84]->_size = 1;
		OpCodes::Data[0x84]->_value = 0x84;

		// Opcode: conv.ovf.i8.un
		OpCodes::Data[0x85] = new OpCode("conv.ovf.i8.un");
		OpCodes::Data[0x85]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x85]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x85]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x85]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1;
		OpCodes::Data[0x85]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI8;
		OpCodes::Data[0x85]->_size = 1;
		OpCodes::Data[0x85]->_value = 0x85;

		// Opcode: conv.ovf.u1.un
		OpCodes::Data[0x86] = new OpCode("conv.ovf.u1.un");
		OpCodes::Data[0x86]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x86]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x86]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x86]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1;
		OpCodes::Data[0x86]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0x86]->_size = 1;
		OpCodes::Data[0x86]->_value = 0x86;

		// Opcode: conv.ovf.u2.un
		OpCodes::Data[0x87] = new OpCode("conv.ovf.u2.un");
		OpCodes::Data[0x87]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x87]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x87]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x87]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1;
		OpCodes::Data[0x87]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0x87]->_size = 1;
		OpCodes::Data[0x87]->_value = 0x87;

		// Opcode: conv.ovf.u4.un
		OpCodes::Data[0x88] = new OpCode("conv.ovf.u4.un");
		OpCodes::Data[0x88]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x88]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x88]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x88]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1;
		OpCodes::Data[0x88]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0x88]->_size = 1;
		OpCodes::Data[0x88]->_value = 0x88;

		// Opcode: conv.ovf.u8.un
		OpCodes::Data[0x89] = new OpCode("conv.ovf.u8.un");
		OpCodes::Data[0x89]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x89]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x89]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x89]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1;
		OpCodes::Data[0x89]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI8;
		OpCodes::Data[0x89]->_size = 1;
		OpCodes::Data[0x89]->_value = 0x89;

		// Opcode: conv.ovf.i.un
		OpCodes::Data[0x8a] = new OpCode("conv.ovf.i.un");
		OpCodes::Data[0x8a]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x8a]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x8a]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x8a]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1;
		OpCodes::Data[0x8a]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0x8a]->_size = 1;
		OpCodes::Data[0x8a]->_value = 0x8a;

		// Opcode: conv.ovf.u.un
		OpCodes::Data[0x8b] = new OpCode("conv.ovf.u.un");
		OpCodes::Data[0x8b]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x8b]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x8b]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x8b]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1;
		OpCodes::Data[0x8b]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0x8b]->_size = 1;
		OpCodes::Data[0x8b]->_value = 0x8b;

		// Opcode: box
		OpCodes::Data[0x8c] = new OpCode("box");
		OpCodes::Data[0x8c]->_operandType = IlOpcodeOperangType::Value::INLINETYPE;
		OpCodes::Data[0x8c]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x8c]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x8c]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1;
		OpCodes::Data[0x8c]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHREF;
		OpCodes::Data[0x8c]->_size = 1;
		OpCodes::Data[0x8c]->_value = 0x8c;

		// Opcode: newarr
		OpCodes::Data[0x8d] = new OpCode("newarr");
		OpCodes::Data[0x8d]->_operandType = IlOpcodeOperangType::Value::INLINETYPE;
		OpCodes::Data[0x8d]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x8d]->_opCodeType = IlOpcodeType::Value::OBJMODEL;
		OpCodes::Data[0x8d]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPI;
		OpCodes::Data[0x8d]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHREF;
		OpCodes::Data[0x8d]->_size = 1;
		OpCodes::Data[0x8d]->_value = 0x8d;

		// Opcode: ldlen
		OpCodes::Data[0x8e] = new OpCode("ldlen");
		OpCodes::Data[0x8e]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x8e]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x8e]->_opCodeType = IlOpcodeType::Value::OBJMODEL;
		OpCodes::Data[0x8e]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPREF;
		OpCodes::Data[0x8e]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0x8e]->_size = 1;
		OpCodes::Data[0x8e]->_value = 0x8e;

		// Opcode: ldelema
		OpCodes::Data[0x8f] = new OpCode("ldelema");
		OpCodes::Data[0x8f]->_operandType = IlOpcodeOperangType::Value::INLINETYPE;
		OpCodes::Data[0x8f]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x8f]->_opCodeType = IlOpcodeType::Value::OBJMODEL;
		OpCodes::Data[0x8f]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPREF_POPI;
		OpCodes::Data[0x8f]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0x8f]->_size = 1;
		OpCodes::Data[0x8f]->_value = 0x8f;

		// Opcode: ldelem.i1
		OpCodes::Data[0x90] = new OpCode("ldelem.i1");
		OpCodes::Data[0x90]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x90]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x90]->_opCodeType = IlOpcodeType::Value::OBJMODEL;
		OpCodes::Data[0x90]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPREF_POPI;
		OpCodes::Data[0x90]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0x90]->_size = 1;
		OpCodes::Data[0x90]->_value = 0x90;

		// Opcode: ldelem.u1
		OpCodes::Data[0x91] = new OpCode("ldelem.u1");
		OpCodes::Data[0x91]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x91]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x91]->_opCodeType = IlOpcodeType::Value::OBJMODEL;
		OpCodes::Data[0x91]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPREF_POPI;
		OpCodes::Data[0x91]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0x91]->_size = 1;
		OpCodes::Data[0x91]->_value = 0x91;

		// Opcode: ldelem.i2
		OpCodes::Data[0x92] = new OpCode("ldelem.i2");
		OpCodes::Data[0x92]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x92]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x92]->_opCodeType = IlOpcodeType::Value::OBJMODEL;
		OpCodes::Data[0x92]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPREF_POPI;
		OpCodes::Data[0x92]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0x92]->_size = 1;
		OpCodes::Data[0x92]->_value = 0x92;

		// Opcode: ldelem.u2
		OpCodes::Data[0x93] = new OpCode("ldelem.u2");
		OpCodes::Data[0x93]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x93]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x93]->_opCodeType = IlOpcodeType::Value::OBJMODEL;
		OpCodes::Data[0x93]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPREF_POPI;
		OpCodes::Data[0x93]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0x93]->_size = 1;
		OpCodes::Data[0x93]->_value = 0x93;

		// Opcode: ldelem.i4
		OpCodes::Data[0x94] = new OpCode("ldelem.i4");
		OpCodes::Data[0x94]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x94]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x94]->_opCodeType = IlOpcodeType::Value::OBJMODEL;
		OpCodes::Data[0x94]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPREF_POPI;
		OpCodes::Data[0x94]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0x94]->_size = 1;
		OpCodes::Data[0x94]->_value = 0x94;

		// Opcode: ldelem.u4
		OpCodes::Data[0x95] = new OpCode("ldelem.u4");
		OpCodes::Data[0x95]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x95]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x95]->_opCodeType = IlOpcodeType::Value::OBJMODEL;
		OpCodes::Data[0x95]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPREF_POPI;
		OpCodes::Data[0x95]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0x95]->_size = 1;
		OpCodes::Data[0x95]->_value = 0x95;

		// Opcode: ldelem.i8
		OpCodes::Data[0x96] = new OpCode("ldelem.i8");
		OpCodes::Data[0x96]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x96]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x96]->_opCodeType = IlOpcodeType::Value::OBJMODEL;
		OpCodes::Data[0x96]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPREF_POPI;
		OpCodes::Data[0x96]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI8;
		OpCodes::Data[0x96]->_size = 1;
		OpCodes::Data[0x96]->_value = 0x96;

		// Opcode: ldelem.i
		OpCodes::Data[0x97] = new OpCode("ldelem.i");
		OpCodes::Data[0x97]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x97]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x97]->_opCodeType = IlOpcodeType::Value::OBJMODEL;
		OpCodes::Data[0x97]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPREF_POPI;
		OpCodes::Data[0x97]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0x97]->_size = 1;
		OpCodes::Data[0x97]->_value = 0x97;

		// Opcode: ldelem.r4
		OpCodes::Data[0x98] = new OpCode("ldelem.r4");
		OpCodes::Data[0x98]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x98]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x98]->_opCodeType = IlOpcodeType::Value::OBJMODEL;
		OpCodes::Data[0x98]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPREF_POPI;
		OpCodes::Data[0x98]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHR4;
		OpCodes::Data[0x98]->_size = 1;
		OpCodes::Data[0x98]->_value = 0x98;

		// Opcode: ldelem.r8
		OpCodes::Data[0x99] = new OpCode("ldelem.r8");
		OpCodes::Data[0x99]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x99]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x99]->_opCodeType = IlOpcodeType::Value::OBJMODEL;
		OpCodes::Data[0x99]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPREF_POPI;
		OpCodes::Data[0x99]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHR8;
		OpCodes::Data[0x99]->_size = 1;
		OpCodes::Data[0x99]->_value = 0x99;

		// Opcode: ldelem.ref
		OpCodes::Data[0x9a] = new OpCode("ldelem.ref");
		OpCodes::Data[0x9a]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x9a]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x9a]->_opCodeType = IlOpcodeType::Value::OBJMODEL;
		OpCodes::Data[0x9a]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPREF_POPI;
		OpCodes::Data[0x9a]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHREF;
		OpCodes::Data[0x9a]->_size = 1;
		OpCodes::Data[0x9a]->_value = 0x9a;

		// Opcode: stelem.i
		OpCodes::Data[0x9b] = new OpCode("stelem.i");
		OpCodes::Data[0x9b]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x9b]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x9b]->_opCodeType = IlOpcodeType::Value::OBJMODEL;
		OpCodes::Data[0x9b]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPREF_POPI_POPI;
		OpCodes::Data[0x9b]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x9b]->_size = 1;
		OpCodes::Data[0x9b]->_value = 0x9b;

		// Opcode: stelem.i1
		OpCodes::Data[0x9c] = new OpCode("stelem.i1");
		OpCodes::Data[0x9c]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x9c]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x9c]->_opCodeType = IlOpcodeType::Value::OBJMODEL;
		OpCodes::Data[0x9c]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPREF_POPI_POPI;
		OpCodes::Data[0x9c]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x9c]->_size = 1;
		OpCodes::Data[0x9c]->_value = 0x9c;

		// Opcode: stelem.i2
		OpCodes::Data[0x9d] = new OpCode("stelem.i2");
		OpCodes::Data[0x9d]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x9d]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x9d]->_opCodeType = IlOpcodeType::Value::OBJMODEL;
		OpCodes::Data[0x9d]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPREF_POPI_POPI;
		OpCodes::Data[0x9d]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x9d]->_size = 1;
		OpCodes::Data[0x9d]->_value = 0x9d;

		// Opcode: stelem.i4
		OpCodes::Data[0x9e] = new OpCode("stelem.i4");
		OpCodes::Data[0x9e]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x9e]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x9e]->_opCodeType = IlOpcodeType::Value::OBJMODEL;
		OpCodes::Data[0x9e]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPREF_POPI_POPI;
		OpCodes::Data[0x9e]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x9e]->_size = 1;
		OpCodes::Data[0x9e]->_value = 0x9e;

		// Opcode: stelem.i8
		OpCodes::Data[0x9f] = new OpCode("stelem.i8");
		OpCodes::Data[0x9f]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x9f]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x9f]->_opCodeType = IlOpcodeType::Value::OBJMODEL;
		OpCodes::Data[0x9f]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPREF_POPI_POPI8;
		OpCodes::Data[0x9f]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x9f]->_size = 1;
		OpCodes::Data[0x9f]->_value = 0x9f;

		// Opcode: stelem.r4
		OpCodes::Data[0xa0] = new OpCode("stelem.r4");
		OpCodes::Data[0xa0]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0xa0]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0xa0]->_opCodeType = IlOpcodeType::Value::OBJMODEL;
		OpCodes::Data[0xa0]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPREF_POPI_POPR4;
		OpCodes::Data[0xa0]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0xa0]->_size = 1;
		OpCodes::Data[0xa0]->_value = 0xa0;

		// Opcode: stelem.r8
		OpCodes::Data[0xa1] = new OpCode("stelem.r8");
		OpCodes::Data[0xa1]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0xa1]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0xa1]->_opCodeType = IlOpcodeType::Value::OBJMODEL;
		OpCodes::Data[0xa1]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPREF_POPI_POPR8;
		OpCodes::Data[0xa1]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0xa1]->_size = 1;
		OpCodes::Data[0xa1]->_value = 0xa1;

		// Opcode: stelem.ref
		OpCodes::Data[0xa2] = new OpCode("stelem.ref");
		OpCodes::Data[0xa2]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0xa2]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0xa2]->_opCodeType = IlOpcodeType::Value::OBJMODEL;
		OpCodes::Data[0xa2]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPREF_POPI_POPREF;
		OpCodes::Data[0xa2]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0xa2]->_size = 1;
		OpCodes::Data[0xa2]->_value = 0xa2;

		// Opcode: ldelem
		OpCodes::Data[0xa3] = new OpCode("ldelem");
		OpCodes::Data[0xa3]->_operandType = IlOpcodeOperangType::Value::INLINETYPE;
		OpCodes::Data[0xa3]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0xa3]->_opCodeType = IlOpcodeType::Value::OBJMODEL;
		OpCodes::Data[0xa3]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPREF_POPI;
		OpCodes::Data[0xa3]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH1;
		OpCodes::Data[0xa3]->_size = 1;
		OpCodes::Data[0xa3]->_value = 0xa3;

		// Opcode: stelem
		OpCodes::Data[0xa4] = new OpCode("stelem");
		OpCodes::Data[0xa4]->_operandType = IlOpcodeOperangType::Value::INLINETYPE;
		OpCodes::Data[0xa4]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0xa4]->_opCodeType = IlOpcodeType::Value::OBJMODEL;
		OpCodes::Data[0xa4]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPREF_POPI_POP1;
		OpCodes::Data[0xa4]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0xa4]->_size = 1;
		OpCodes::Data[0xa4]->_value = 0xa4;

		// Opcode: unbox.any
		OpCodes::Data[0xa5] = new OpCode("unbox.any");
		OpCodes::Data[0xa5]->_operandType = IlOpcodeOperangType::Value::INLINETYPE;
		OpCodes::Data[0xa5]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0xa5]->_opCodeType = IlOpcodeType::Value::OBJMODEL;
		OpCodes::Data[0xa5]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPREF;
		OpCodes::Data[0xa5]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH1;
		OpCodes::Data[0xa5]->_size = 1;
		OpCodes::Data[0xa5]->_value = 0xa5;

		// Opcode: conv.ovf.i1
		OpCodes::Data[0xb3] = new OpCode("conv.ovf.i1");
		OpCodes::Data[0xb3]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0xb3]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0xb3]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0xb3]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1;
		OpCodes::Data[0xb3]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0xb3]->_size = 1;
		OpCodes::Data[0xb3]->_value = 0xb3;

		// Opcode: conv.ovf.u1
		OpCodes::Data[0xb4] = new OpCode("conv.ovf.u1");
		OpCodes::Data[0xb4]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0xb4]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0xb4]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0xb4]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1;
		OpCodes::Data[0xb4]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0xb4]->_size = 1;
		OpCodes::Data[0xb4]->_value = 0xb4;

		// Opcode: conv.ovf.i2
		OpCodes::Data[0xb5] = new OpCode("conv.ovf.i2");
		OpCodes::Data[0xb5]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0xb5]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0xb5]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0xb5]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1;
		OpCodes::Data[0xb5]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0xb5]->_size = 1;
		OpCodes::Data[0xb5]->_value = 0xb5;

		// Opcode: conv.ovf.u2
		OpCodes::Data[0xb6] = new OpCode("conv.ovf.u2");
		OpCodes::Data[0xb6]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0xb6]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0xb6]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0xb6]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1;
		OpCodes::Data[0xb6]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0xb6]->_size = 1;
		OpCodes::Data[0xb6]->_value = 0xb6;

		// Opcode: conv.ovf.i4
		OpCodes::Data[0xb7] = new OpCode("conv.ovf.i4");
		OpCodes::Data[0xb7]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0xb7]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0xb7]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0xb7]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1;
		OpCodes::Data[0xb7]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0xb7]->_size = 1;
		OpCodes::Data[0xb7]->_value = 0xb7;

		// Opcode: conv.ovf.u4
		OpCodes::Data[0xb8] = new OpCode("conv.ovf.u4");
		OpCodes::Data[0xb8]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0xb8]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0xb8]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0xb8]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1;
		OpCodes::Data[0xb8]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0xb8]->_size = 1;
		OpCodes::Data[0xb8]->_value = 0xb8;

		// Opcode: conv.ovf.i8
		OpCodes::Data[0xb9] = new OpCode("conv.ovf.i8");
		OpCodes::Data[0xb9]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0xb9]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0xb9]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0xb9]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1;
		OpCodes::Data[0xb9]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI8;
		OpCodes::Data[0xb9]->_size = 1;
		OpCodes::Data[0xb9]->_value = 0xb9;

		// Opcode: conv.ovf.u8
		OpCodes::Data[0xba] = new OpCode("conv.ovf.u8");
		OpCodes::Data[0xba]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0xba]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0xba]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0xba]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1;
		OpCodes::Data[0xba]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI8;
		OpCodes::Data[0xba]->_size = 1;
		OpCodes::Data[0xba]->_value = 0xba;

		// Opcode: refanyval
		OpCodes::Data[0xc2] = new OpCode("refanyval");
		OpCodes::Data[0xc2]->_operandType = IlOpcodeOperangType::Value::INLINETYPE;
		OpCodes::Data[0xc2]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0xc2]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0xc2]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1;
		OpCodes::Data[0xc2]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0xc2]->_size = 1;
		OpCodes::Data[0xc2]->_value = 0xc2;

		// Opcode: ckfinite
		OpCodes::Data[0xc3] = new OpCode("ckfinite");
		OpCodes::Data[0xc3]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0xc3]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0xc3]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0xc3]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1;
		OpCodes::Data[0xc3]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHR8;
		OpCodes::Data[0xc3]->_size = 1;
		OpCodes::Data[0xc3]->_value = 0xc3;

		// Opcode: mkrefany
		OpCodes::Data[0xc6] = new OpCode("mkrefany");
		OpCodes::Data[0xc6]->_operandType = IlOpcodeOperangType::Value::INLINETYPE;
		OpCodes::Data[0xc6]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0xc6]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0xc6]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPI;
		OpCodes::Data[0xc6]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH1;
		OpCodes::Data[0xc6]->_size = 1;
		OpCodes::Data[0xc6]->_value = 0xc6;

		// Opcode: ldtoken
		OpCodes::Data[0xd0] = new OpCode("ldtoken");
		OpCodes::Data[0xd0]->_operandType = IlOpcodeOperangType::Value::INLINETOK;
		OpCodes::Data[0xd0]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0xd0]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0xd0]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP0;
		OpCodes::Data[0xd0]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0xd0]->_size = 1;
		OpCodes::Data[0xd0]->_value = 0xd0;

		// Opcode: conv.u2
		OpCodes::Data[0xd1] = new OpCode("conv.u2");
		OpCodes::Data[0xd1]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0xd1]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0xd1]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0xd1]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1;
		OpCodes::Data[0xd1]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0xd1]->_size = 1;
		OpCodes::Data[0xd1]->_value = 0xd1;

		// Opcode: conv.u1
		OpCodes::Data[0xd2] = new OpCode("conv.u1");
		OpCodes::Data[0xd2]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0xd2]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0xd2]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0xd2]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1;
		OpCodes::Data[0xd2]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0xd2]->_size = 1;
		OpCodes::Data[0xd2]->_value = 0xd2;

		// Opcode: conv.i
		OpCodes::Data[0xd3] = new OpCode("conv.i");
		OpCodes::Data[0xd3]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0xd3]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0xd3]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0xd3]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1;
		OpCodes::Data[0xd3]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0xd3]->_size = 1;
		OpCodes::Data[0xd3]->_value = 0xd3;

		// Opcode: conv.ovf.i
		OpCodes::Data[0xd4] = new OpCode("conv.ovf.i");
		OpCodes::Data[0xd4]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0xd4]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0xd4]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0xd4]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1;
		OpCodes::Data[0xd4]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0xd4]->_size = 1;
		OpCodes::Data[0xd4]->_value = 0xd4;

		// Opcode: conv.ovf.u
		OpCodes::Data[0xd5] = new OpCode("conv.ovf.u");
		OpCodes::Data[0xd5]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0xd5]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0xd5]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0xd5]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1;
		OpCodes::Data[0xd5]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0xd5]->_size = 1;
		OpCodes::Data[0xd5]->_value = 0xd5;

		// Opcode: add.ovf
		OpCodes::Data[0xd6] = new OpCode("add.ovf");
		OpCodes::Data[0xd6]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0xd6]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0xd6]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0xd6]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1_POP1;
		OpCodes::Data[0xd6]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH1;
		OpCodes::Data[0xd6]->_size = 1;
		OpCodes::Data[0xd6]->_value = 0xd6;

		// Opcode: add.ovf.un
		OpCodes::Data[0xd7] = new OpCode("add.ovf.un");
		OpCodes::Data[0xd7]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0xd7]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0xd7]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0xd7]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1_POP1;
		OpCodes::Data[0xd7]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH1;
		OpCodes::Data[0xd7]->_size = 1;
		OpCodes::Data[0xd7]->_value = 0xd7;

		// Opcode: mul.ovf
		OpCodes::Data[0xd8] = new OpCode("mul.ovf");
		OpCodes::Data[0xd8]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0xd8]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0xd8]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0xd8]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1_POP1;
		OpCodes::Data[0xd8]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH1;
		OpCodes::Data[0xd8]->_size = 1;
		OpCodes::Data[0xd8]->_value = 0xd8;

		// Opcode: mul.ovf.un
		OpCodes::Data[0xd9] = new OpCode("mul.ovf.un");
		OpCodes::Data[0xd9]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0xd9]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0xd9]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0xd9]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1_POP1;
		OpCodes::Data[0xd9]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH1;
		OpCodes::Data[0xd9]->_size = 1;
		OpCodes::Data[0xd9]->_value = 0xd9;

		// Opcode: sub.ovf
		OpCodes::Data[0xda] = new OpCode("sub.ovf");
		OpCodes::Data[0xda]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0xda]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0xda]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0xda]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1_POP1;
		OpCodes::Data[0xda]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH1;
		OpCodes::Data[0xda]->_size = 1;
		OpCodes::Data[0xda]->_value = 0xda;

		// Opcode: sub.ovf.un
		OpCodes::Data[0xdb] = new OpCode("sub.ovf.un");
		OpCodes::Data[0xdb]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0xdb]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0xdb]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0xdb]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1_POP1;
		OpCodes::Data[0xdb]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH1;
		OpCodes::Data[0xdb]->_size = 1;
		OpCodes::Data[0xdb]->_value = 0xdb;

		// Opcode: endfinally
		OpCodes::Data[0xdc] = new OpCode("endfinally");
		OpCodes::Data[0xdc]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0xdc]->_flowControl = IlOpcodeFlowControl::Value::RETURN;
		OpCodes::Data[0xdc]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0xdc]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP0;
		OpCodes::Data[0xdc]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0xdc]->_size = 1;
		OpCodes::Data[0xdc]->_value = 0xdc;

		// Opcode: leave
		OpCodes::Data[0xdd] = new OpCode("leave");
		OpCodes::Data[0xdd]->_operandType = IlOpcodeOperangType::Value::INLINEBRTARGET;
		OpCodes::Data[0xdd]->_flowControl = IlOpcodeFlowControl::Value::BRANCH;
		OpCodes::Data[0xdd]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0xdd]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP0;
		OpCodes::Data[0xdd]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0xdd]->_size = 1;
		OpCodes::Data[0xdd]->_value = 0xdd;

		// Opcode: leave.s
		OpCodes::Data[0xde] = new OpCode("leave.s");
		OpCodes::Data[0xde]->_operandType = IlOpcodeOperangType::Value::SHORTINLINEBRTARGET;
		OpCodes::Data[0xde]->_flowControl = IlOpcodeFlowControl::Value::BRANCH;
		OpCodes::Data[0xde]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0xde]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP0;
		OpCodes::Data[0xde]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0xde]->_size = 1;
		OpCodes::Data[0xde]->_value = 0xde;

		// Opcode: stind.i
		OpCodes::Data[0xdf] = new OpCode("stind.i");
		OpCodes::Data[0xdf]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0xdf]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0xdf]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0xdf]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPI_POPI;
		OpCodes::Data[0xdf]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0xdf]->_size = 1;
		OpCodes::Data[0xdf]->_value = 0xdf;

		// Opcode: conv.u
		OpCodes::Data[0xe0] = new OpCode("conv.u");
		OpCodes::Data[0xe0]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0xe0]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0xe0]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0xe0]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1;
		OpCodes::Data[0xe0]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0xe0]->_size = 1;
		OpCodes::Data[0xe0]->_value = 0xe0;

		// Opcode: prefix7
		OpCodes::Data[0xf8] = new OpCode("prefix7");
		OpCodes::Data[0xf8]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0xf8]->_flowControl = IlOpcodeFlowControl::Value::META;
		OpCodes::Data[0xf8]->_opCodeType = IlOpcodeType::Value::NTERNAL;
		OpCodes::Data[0xf8]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP0;
		OpCodes::Data[0xf8]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0xf8]->_size = 1;
		OpCodes::Data[0xf8]->_value = 0xf8;

		// Opcode: prefix6
		OpCodes::Data[0xf9] = new OpCode("prefix6");
		OpCodes::Data[0xf9]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0xf9]->_flowControl = IlOpcodeFlowControl::Value::META;
		OpCodes::Data[0xf9]->_opCodeType = IlOpcodeType::Value::NTERNAL;
		OpCodes::Data[0xf9]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP0;
		OpCodes::Data[0xf9]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0xf9]->_size = 1;
		OpCodes::Data[0xf9]->_value = 0xf9;

		// Opcode: prefix5
		OpCodes::Data[0xfa] = new OpCode("prefix5");
		OpCodes::Data[0xfa]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0xfa]->_flowControl = IlOpcodeFlowControl::Value::META;
		OpCodes::Data[0xfa]->_opCodeType = IlOpcodeType::Value::NTERNAL;
		OpCodes::Data[0xfa]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP0;
		OpCodes::Data[0xfa]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0xfa]->_size = 1;
		OpCodes::Data[0xfa]->_value = 0xfa;

		// Opcode: prefix4
		OpCodes::Data[0xfb] = new OpCode("prefix4");
		OpCodes::Data[0xfb]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0xfb]->_flowControl = IlOpcodeFlowControl::Value::META;
		OpCodes::Data[0xfb]->_opCodeType = IlOpcodeType::Value::NTERNAL;
		OpCodes::Data[0xfb]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP0;
		OpCodes::Data[0xfb]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0xfb]->_size = 1;
		OpCodes::Data[0xfb]->_value = 0xfb;

		// Opcode: prefix3
		OpCodes::Data[0xfc] = new OpCode("prefix3");
		OpCodes::Data[0xfc]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0xfc]->_flowControl = IlOpcodeFlowControl::Value::META;
		OpCodes::Data[0xfc]->_opCodeType = IlOpcodeType::Value::NTERNAL;
		OpCodes::Data[0xfc]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP0;
		OpCodes::Data[0xfc]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0xfc]->_size = 1;
		OpCodes::Data[0xfc]->_value = 0xfc;

		// Opcode: prefix2
		OpCodes::Data[0xfd] = new OpCode("prefix2");
		OpCodes::Data[0xfd]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0xfd]->_flowControl = IlOpcodeFlowControl::Value::META;
		OpCodes::Data[0xfd]->_opCodeType = IlOpcodeType::Value::NTERNAL;
		OpCodes::Data[0xfd]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP0;
		OpCodes::Data[0xfd]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0xfd]->_size = 1;
		OpCodes::Data[0xfd]->_value = 0xfd;

		// Opcode: prefix1
		OpCodes::Data[0xfe] = new OpCode("prefix1");
		OpCodes::Data[0xfe]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0xfe]->_flowControl = IlOpcodeFlowControl::Value::META;
		OpCodes::Data[0xfe]->_opCodeType = IlOpcodeType::Value::NTERNAL;
		OpCodes::Data[0xfe]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP0;
		OpCodes::Data[0xfe]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0xfe]->_size = 1;
		OpCodes::Data[0xfe]->_value = 0xfe;

		// Opcode: prefixref
		OpCodes::Data[0xff] = new OpCode("prefixref");
		OpCodes::Data[0xff]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0xff]->_flowControl = IlOpcodeFlowControl::Value::META;
		OpCodes::Data[0xff]->_opCodeType = IlOpcodeType::Value::NTERNAL;
		OpCodes::Data[0xff]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP0;
		OpCodes::Data[0xff]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0xff]->_size = 1;
		OpCodes::Data[0xff]->_value = 0xff;

		// Opcode: arglist
		OpCodes::Data[0x100] = new OpCode("arglist");
		OpCodes::Data[0x100]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x100]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x100]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x100]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP0;
		OpCodes::Data[0x100]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0x100]->_size = 2;
		OpCodes::Data[0x100]->_value = 0x100;

		// Opcode: ceq
		OpCodes::Data[0x101] = new OpCode("ceq");
		OpCodes::Data[0x101]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x101]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x101]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x101]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1_POP1;
		OpCodes::Data[0x101]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0x101]->_size = 2;
		OpCodes::Data[0x101]->_value = 0x101;

		// Opcode: cgt
		OpCodes::Data[0x102] = new OpCode("cgt");
		OpCodes::Data[0x102]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x102]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x102]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x102]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1_POP1;
		OpCodes::Data[0x102]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0x102]->_size = 2;
		OpCodes::Data[0x102]->_value = 0x102;

		// Opcode: cgt.un
		OpCodes::Data[0x103] = new OpCode("cgt.un");
		OpCodes::Data[0x103]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x103]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x103]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x103]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1_POP1;
		OpCodes::Data[0x103]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0x103]->_size = 2;
		OpCodes::Data[0x103]->_value = 0x103;

		// Opcode: clt
		OpCodes::Data[0x104] = new OpCode("clt");
		OpCodes::Data[0x104]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x104]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x104]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x104]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1_POP1;
		OpCodes::Data[0x104]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0x104]->_size = 2;
		OpCodes::Data[0x104]->_value = 0x104;

		// Opcode: clt.un
		OpCodes::Data[0x105] = new OpCode("clt.un");
		OpCodes::Data[0x105]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x105]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x105]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x105]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1_POP1;
		OpCodes::Data[0x105]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0x105]->_size = 2;
		OpCodes::Data[0x105]->_value = 0x105;

		// Opcode: ldftn
		OpCodes::Data[0x106] = new OpCode("ldftn");
		OpCodes::Data[0x106]->_operandType = IlOpcodeOperangType::Value::INLINEMETHOD;
		OpCodes::Data[0x106]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x106]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x106]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP0;
		OpCodes::Data[0x106]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0x106]->_size = 2;
		OpCodes::Data[0x106]->_value = 0x106;

		// Opcode: ldvirtftn
		OpCodes::Data[0x107] = new OpCode("ldvirtftn");
		OpCodes::Data[0x107]->_operandType = IlOpcodeOperangType::Value::INLINEMETHOD;
		OpCodes::Data[0x107]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x107]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x107]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPREF;
		OpCodes::Data[0x107]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0x107]->_size = 2;
		OpCodes::Data[0x107]->_value = 0x107;

		// Opcode: ldarg
		OpCodes::Data[0x109] = new OpCode("ldarg");
		OpCodes::Data[0x109]->_operandType = IlOpcodeOperangType::Value::INLINEVAR;
		OpCodes::Data[0x109]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x109]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x109]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP0;
		OpCodes::Data[0x109]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH1;
		OpCodes::Data[0x109]->_size = 2;
		OpCodes::Data[0x109]->_value = 0x109;

		// Opcode: ldarga
		OpCodes::Data[0x10a] = new OpCode("ldarga");
		OpCodes::Data[0x10a]->_operandType = IlOpcodeOperangType::Value::INLINEVAR;
		OpCodes::Data[0x10a]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x10a]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x10a]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP0;
		OpCodes::Data[0x10a]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0x10a]->_size = 2;
		OpCodes::Data[0x10a]->_value = 0x10a;

		// Opcode: starg
		OpCodes::Data[0x10b] = new OpCode("starg");
		OpCodes::Data[0x10b]->_operandType = IlOpcodeOperangType::Value::INLINEVAR;
		OpCodes::Data[0x10b]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x10b]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x10b]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1;
		OpCodes::Data[0x10b]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x10b]->_size = 2;
		OpCodes::Data[0x10b]->_value = 0x10b;

		// Opcode: ldloc
		OpCodes::Data[0x10c] = new OpCode("ldloc");
		OpCodes::Data[0x10c]->_operandType = IlOpcodeOperangType::Value::INLINEVAR;
		OpCodes::Data[0x10c]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x10c]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x10c]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP0;
		OpCodes::Data[0x10c]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH1;
		OpCodes::Data[0x10c]->_size = 2;
		OpCodes::Data[0x10c]->_value = 0x10c;

		// Opcode: ldloca
		OpCodes::Data[0x10d] = new OpCode("ldloca");
		OpCodes::Data[0x10d]->_operandType = IlOpcodeOperangType::Value::INLINEVAR;
		OpCodes::Data[0x10d]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x10d]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x10d]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP0;
		OpCodes::Data[0x10d]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0x10d]->_size = 2;
		OpCodes::Data[0x10d]->_value = 0x10d;

		// Opcode: stloc
		OpCodes::Data[0x10e] = new OpCode("stloc");
		OpCodes::Data[0x10e]->_operandType = IlOpcodeOperangType::Value::INLINEVAR;
		OpCodes::Data[0x10e]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x10e]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x10e]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1;
		OpCodes::Data[0x10e]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x10e]->_size = 2;
		OpCodes::Data[0x10e]->_value = 0x10e;

		// Opcode: localloc
		OpCodes::Data[0x10f] = new OpCode("localloc");
		OpCodes::Data[0x10f]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x10f]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x10f]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x10f]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPI;
		OpCodes::Data[0x10f]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0x10f]->_size = 2;
		OpCodes::Data[0x10f]->_value = 0x10f;

		// Opcode: endfilter
		OpCodes::Data[0x111] = new OpCode("endfilter");
		OpCodes::Data[0x111]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x111]->_flowControl = IlOpcodeFlowControl::Value::RETURN;
		OpCodes::Data[0x111]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x111]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPI;
		OpCodes::Data[0x111]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x111]->_size = 2;
		OpCodes::Data[0x111]->_value = 0x111;

		// Opcode: unaligned.
		OpCodes::Data[0x112] = new OpCode("unaligned.");
		OpCodes::Data[0x112]->_operandType = IlOpcodeOperangType::Value::SHORTINLINEI;
		OpCodes::Data[0x112]->_flowControl = IlOpcodeFlowControl::Value::META;
		OpCodes::Data[0x112]->_opCodeType = IlOpcodeType::Value::PREFIX;
		OpCodes::Data[0x112]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP0;
		OpCodes::Data[0x112]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x112]->_size = 2;
		OpCodes::Data[0x112]->_value = 0x112;

		// Opcode: volatile.
		OpCodes::Data[0x113] = new OpCode("volatile.");
		OpCodes::Data[0x113]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x113]->_flowControl = IlOpcodeFlowControl::Value::META;
		OpCodes::Data[0x113]->_opCodeType = IlOpcodeType::Value::PREFIX;
		OpCodes::Data[0x113]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP0;
		OpCodes::Data[0x113]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x113]->_size = 2;
		OpCodes::Data[0x113]->_value = 0x113;

		// Opcode: tail.
		OpCodes::Data[0x114] = new OpCode("tail.");
		OpCodes::Data[0x114]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x114]->_flowControl = IlOpcodeFlowControl::Value::META;
		OpCodes::Data[0x114]->_opCodeType = IlOpcodeType::Value::PREFIX;
		OpCodes::Data[0x114]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP0;
		OpCodes::Data[0x114]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x114]->_size = 2;
		OpCodes::Data[0x114]->_value = 0x114;

		// Opcode: initobj
		OpCodes::Data[0x115] = new OpCode("initobj");
		OpCodes::Data[0x115]->_operandType = IlOpcodeOperangType::Value::INLINETYPE;
		OpCodes::Data[0x115]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x115]->_opCodeType = IlOpcodeType::Value::OBJMODEL;
		OpCodes::Data[0x115]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPI;
		OpCodes::Data[0x115]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x115]->_size = 2;
		OpCodes::Data[0x115]->_value = 0x115;

		// Opcode: constrained.
		OpCodes::Data[0x116] = new OpCode("constrained.");
		OpCodes::Data[0x116]->_operandType = IlOpcodeOperangType::Value::INLINETYPE;
		OpCodes::Data[0x116]->_flowControl = IlOpcodeFlowControl::Value::META;
		OpCodes::Data[0x116]->_opCodeType = IlOpcodeType::Value::PREFIX;
		OpCodes::Data[0x116]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP0;
		OpCodes::Data[0x116]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x116]->_size = 2;
		OpCodes::Data[0x116]->_value = 0x116;

		// Opcode: cpblk
		OpCodes::Data[0x117] = new OpCode("cpblk");
		OpCodes::Data[0x117]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x117]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x117]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x117]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPI_POPI_POPI;
		OpCodes::Data[0x117]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x117]->_size = 2;
		OpCodes::Data[0x117]->_value = 0x117;

		// Opcode: initblk
		OpCodes::Data[0x118] = new OpCode("initblk");
		OpCodes::Data[0x118]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x118]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x118]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x118]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POPI_POPI_POPI;
		OpCodes::Data[0x118]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x118]->_size = 2;
		OpCodes::Data[0x118]->_value = 0x118;

		// Opcode: rethrow
		OpCodes::Data[0x11a] = new OpCode("rethrow");
		OpCodes::Data[0x11a]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x11a]->_flowControl = IlOpcodeFlowControl::Value::THROW;
		OpCodes::Data[0x11a]->_opCodeType = IlOpcodeType::Value::OBJMODEL;
		OpCodes::Data[0x11a]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP0;
		OpCodes::Data[0x11a]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x11a]->_size = 2;
		OpCodes::Data[0x11a]->_value = 0x11a;

		// Opcode: sizeof
		OpCodes::Data[0x11c] = new OpCode("sizeof");
		OpCodes::Data[0x11c]->_operandType = IlOpcodeOperangType::Value::INLINETYPE;
		OpCodes::Data[0x11c]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x11c]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x11c]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP0;
		OpCodes::Data[0x11c]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0x11c]->_size = 2;
		OpCodes::Data[0x11c]->_value = 0x11c;

		// Opcode: refanytype
		OpCodes::Data[0x11d] = new OpCode("refanytype");
		OpCodes::Data[0x11d]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x11d]->_flowControl = IlOpcodeFlowControl::Value::NEXT;
		OpCodes::Data[0x11d]->_opCodeType = IlOpcodeType::Value::PRIMITIVE;
		OpCodes::Data[0x11d]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP1;
		OpCodes::Data[0x11d]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSHI;
		OpCodes::Data[0x11d]->_size = 2;
		OpCodes::Data[0x11d]->_value = 0x11d;

		// Opcode: readonly.
		OpCodes::Data[0x11e] = new OpCode("readonly.");
		OpCodes::Data[0x11e]->_operandType = IlOpcodeOperangType::Value::INLINENONE;
		OpCodes::Data[0x11e]->_flowControl = IlOpcodeFlowControl::Value::META;
		OpCodes::Data[0x11e]->_opCodeType = IlOpcodeType::Value::PREFIX;
		OpCodes::Data[0x11e]->_stackBehaviourPop = IlOpcodeBehaviour::Value::POP0;
		OpCodes::Data[0x11e]->_stackBehaviourPush = IlOpcodeBehaviour::Value::PUSH0;
		OpCodes::Data[0x11e]->_size = 2;
		OpCodes::Data[0x11e]->_value = 0x11e;
	}
}
