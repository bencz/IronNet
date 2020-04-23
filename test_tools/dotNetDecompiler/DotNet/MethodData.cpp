#include "IncFileNm.h"
#include METHODDATA_H
#include PEDATA_H
#include TABLEBASE_H
#include METADATATABLES_H
#include UTIL_H
#include PEHEADERS_H
#include METADATAINCLUDES_H
#include RVAMANAGER_H
#include INDEXMANAGER_H
#include CPPUTIL_H

namespace PEAnalyzer
{
	MethodData::MethodData(PEData* data, MethodDefTable* m)
	{
		initializeInstanceFields();
		this->parse(data, (TableBase*)m);
	}

	MethodData::MethodData(PEData* data, MemberRefTable* m)
	{
		initializeInstanceFields();
		this->parse(data, (TableBase*)m);
	}

	MethodData::~MethodData()
	{
		delete _data;
		delete _table;
		delete _localVarSig;
		delete _implMap;
	}

	std::vector<ILCode*> MethodData::get_ILCodes()
	{
		if (this->_ilCodes.size() > 0)
			return this->_ilCodes;
		if (this->_ilAddr == 0)
			return std::vector<ILCode*>();

		std::vector<ILCode*> list;
		int ad = this->_ilAddr;
		int end = ad + this->_codeSize;

		while (ad < end)
		{
			ILCode* il = this->readIL(ad);
			list.push_back(il);
			ad += il->_opCodeLength + il->_operandLength;
		}
		this->_ilCodes = list;
		for (int i = 0; i < _ilCodes.size(); i++)
		{
			ILCode* ilcode = _ilCodes[i];
			int addr = Util::getBrTarget(ilcode);
			if (addr == 0)
				continue;

			for (int j = 0; j < _ilCodes.size(); j++)
			{
				ILCode* ilcodeBrTarget = _ilCodes[i];
				if (ilcodeBrTarget->_address == addr)
				{
					ilcodeBrTarget->_isBrTarget = true;
					break;
				}
			}
		}
		return this->_ilCodes;
	}

	void MethodData::parse(PEData* data, TableBase* m)
	{
		this->_data = data;
		this->_table = m;
		this->_flags = this->_size = this->_maxStack = this->_codeSize = this->_localVarSigTok = 0;
		this->_ilCodes.clear();
		this->_ilAddr = 0;
		this->_localVarSigTok = 0;
		this->_localVarSig = 0;
		this->_localVars.clear();
		this->_implMap = 0;
		this->_kind = this->_dataSize = 0;
		this->_ehTable.clear();
		this->readSignature();
		int ad = this->get_RVA();
		if (ad == 0)
			return;
		int h = this->_data->_data[ad];
		if ((h & 3) == static_cast<int>(CorILMethod::Value::TINYFORMAT))
		{
			this->_codeSize = h >> 2;
			ad++;
		}
		else
		{
			h = Util::getInt16(this->_data->_data, ad);
			this->_flags = h & 0xfff;
			this->_size = (h >> 12) * 4;
			ad += 2;
			this->_maxStack = Util::getInt16(this->_data->_data, ad);
			ad += 2;
			this->_codeSize = Util::getInt32(this->_data->_data, ad);
			ad += 4;
			this->_localVarSigTok = Util::getInt32(this->_data->_data, ad);
			ad += 4;
			this->readLocalVars();
		}

		this->_ilAddr = ad;
		ad += this->_codeSize;
		if ((this->_flags &static_cast<int>(CorILMethod::Value::MORESECTS)) == 0)
			return;
		/// More Sections
		int pad = 4 - (ad & 3);
		if (pad < 4)
			ad += pad;
		int end = ad;
		this->_kind = this->_data->_data[ad++];
		bool isFat = (this->_kind &static_cast<int>(CorILMethod::Value::FATFORMAT)) != 0;
		if (!isFat)
			this->_dataSize = this->_data->_data[ad];
		else
			this->_dataSize = Util::getInt24(this->_data->_data, ad);
		ad += 3;
		end += this->_dataSize;
		while (ad < end)
		{
			ExceptionHandlingClauses* ex = new ExceptionHandlingClauses();
			if (!isFat)
			{
				ex->Flags = Util::getInt16(data->_data, ad);
				ad += 2;
				ex->TryOffset = Util::getInt16(data->_data, ad);
				ad += 2;
				ex->TryLength = data->_data[ad++];
				ex->HandlerOffset = Util::getInt16(data->_data, ad);
				ad += 2;
				ex->HandlerLength = data->_data[ad++];
			}
			else
			{
				ex->Flags = Util::getInt32(data->_data, ad);
				ad += 4;
				ex->TryOffset = Util::getInt32(data->_data, ad);
				ad += 4;
				ex->TryLength = Util::getInt32(data->_data, ad);
				ad += 4;
				ex->HandlerOffset = Util::getInt32(data->_data, ad);
				ad += 4;
				ex->HandlerLength = Util::getInt32(data->_data, ad);
				ad += 4;
			}
			if (ex->Flags == static_cast<int>(COR_ILEXCEPTION_CLAUSE::Value::EXCEPTION))
				ex->ClassToken = Util::getInt32(data->_data, ad);
			else
				ex->FilterOffset = Util::getInt32(data->_data, ad);
			ad += 4;
			this->_ehTable.push_back(ex);
		}
	}

	void MethodData::readSignature()
	{
		int sign;

		if (dynamic_cast<MethodDefTable*>(this->_table) != 0)
			sign = (dynamic_cast<MethodDefTable*>(this->_table))->Signature;
		else if (dynamic_cast<MemberRefTable*>(this->_table) != 0)
		{
			sign = (dynamic_cast<MemberRefTable*>(this->_table))->Signature;
			if (this->_table->_children.size() == 0)
				this->_table->_children.push_back(std::vector< void* >());
		}
		else
			return;

		int ad = this->_data->_idxm->_blobOffset + sign;
		DoubleInt* dataSize = Util::getDataSize(this->_data->_data, ad);
		ad += dataSize->A;
		this->_signatureFlags = this->_data->_data[ad++];
		if (this->get_IsField())
		{
			this->_retType = this->_data->_idxm->readType(ad);
			return;
		}
		delete dataSize;

		DoubleInt* count = Util::getDataSize(this->_data->_data, ad);
		this->_paramCount = count->B;
		ad += count->A;
		this->_retType = this->_data->_idxm->readType(ad);
		ad += this->_retType->Size;
		for (int i = 0; i < this->_paramCount; i++)
		{
			TypeData* ds = this->_data->_idxm->readType(ad);
			if (dynamic_cast<MethodDefTable*>(_table) != 0)
			{
				ParamTable* p = (ParamTable*)this->_table->_children[0][i];
				ParamTable* parmTable = dynamic_cast<ParamTable*>(p);
				parmTable->_tag = ds;
			}
			else
				this->_table->_children[0].push_back(ds);
			ad += ds->Size;
		}
		delete count;
	}

	void MethodData::readLocalVars()
	{
		this->_localVarSig = dynamic_cast<StandAloneSigTable*>(this->_data->_idxm->getTable(this->_localVarSigTok));
		if (this->_localVarSig == 0)
			return;
		int ad = this->_data->_idxm->_blobOffset + this->_localVarSig->Signature;
		DoubleInt* dataSize = Util::getDataSize(this->_data->_data, ad);
		ad += dataSize->A;
		ad++;
		DoubleInt* count = Util::getDataSize(this->_data->_data, ad);
		ad += count->A;
		for (int i = 0; i < count->B; i++)
		{
			TypeData* ds = this->_data->_idxm->readType(ad);
			this->_localVars.push_back(ds);
			ad += ds->Size;
		}

		delete dataSize;
		delete count;
	}

	ILCode* MethodData::readIL(int ad)
	{
		ILCode* il = new ILCode();
		il->_address = ad;
		int v = this->_data->_data[ad++];
		il->_opCodeLength = 1;
		if (v == 0xfe)
		{
			v = 0x0100 + this->_data->_data[ad++];
			il->_opCodeLength = 2;
		}
		il->_opCode = OpCodes::Data[v];
		switch (il->_opCode->_operandType)
		{
		case IlOpcodeOperangType::Value::INLINENONE:
			il->_operand = 0;
			il->_operandLength = 0;
			break;
		case IlOpcodeOperangType::Value::SHORTINLINEBRTARGET: 
		case IlOpcodeOperangType::Value::SHORTINLINEVAR: 
		case IlOpcodeOperangType::Value::SHORTINLINEI: 
			il->_operand = (void*)this->_data->_data[ad];
			il->_operandLength = 1;
			break;
		case IlOpcodeOperangType::Value::INLINEVAR:
			il->_operand = (void*)Util::getInt16(this->_data->_data, ad);
			il->_operandLength = 2;
			break;
		case IlOpcodeOperangType::Value::INLINEBRTARGET: 
		case IlOpcodeOperangType::Value::INLINEFIELD: 
		case IlOpcodeOperangType::Value::INLINEI:
		case IlOpcodeOperangType::Value::INLINEMETHOD:
		case IlOpcodeOperangType::Value::INLINESIG: 
		case IlOpcodeOperangType::Value::INLINESTRING: 
		case IlOpcodeOperangType::Value::INLINETOK: 
		case IlOpcodeOperangType::Value::INLINETYPE: 
		case IlOpcodeOperangType::Value::SHORTINLINER: 
			il->_operand = (void*)Util::getInt32(this->_data->_data, ad);
			il->_operandLength = 4;
			break;
		case IlOpcodeOperangType::Value::INLINEI8: 
		case IlOpcodeOperangType::Value::INLINER:
			il->_operand = (void*)Util::getInt64(this->_data->_data, ad);
			il->_operandLength = 8;
			break;
		case IlOpcodeOperangType::Value::INLINESWITCH:
			il->_operand = (void*)Util::getInt32(this->_data->_data, ad);
			il->_operandLength = 4 + ((int)(il->_operand)) * 4;
			break;
		}
		return il;
	}

	int MethodData::get_RVA() const
	{
		MethodDefTable* m = dynamic_cast<MethodDefTable*>(_table);
		if (m == 0 || m->RVA == 0)
			return 0;
		return (this->_data->_rva)->convertToPhysical(m->RVA);
	}

	bool MethodData::get_HasThis() const
	{
		return (this->_signatureFlags &static_cast<int>(MethodFlags::Value::HASTHIS)) != 0;
	}

	std::string MethodData::get_Name() const
	{
		if (dynamic_cast<MethodDefTable*>(_table) != 0)
			return this->_data->_idxm->getName(dynamic_cast<MethodDefTable*>(_table));
		else if (dynamic_cast<MemberRefTable*>(_table) != 0)
			return this->_data->_idxm->getName(dynamic_cast<MemberRefTable*>(_table));
		return "";
	}

	std::string MethodData::get_FullName() const
	{
		StringBuilder* sb = new StringBuilder();
		if (this->get_HasThis())
			sb->append("instance ");
		sb->append(this->_data->_idxm->getName(_retType));
		sb->append(' ');
		sb->append(this->get_Name());
		if (this->get_IsField())
			return sb->toString();
		sb->append('(');
		for (int i = 0; i < this->_paramCount; i++)
		{
			if (i > 0)
				sb->append(", ");
			void* obj = this->_table->_children[0][i];
			if (CppUtil::isInstanceOf<ParamTable>(*(ParamTable*)obj))
			{
				ParamTable* pTable = (ParamTable*)(obj);
				TypeData* tpData = (TypeData*)pTable->_tag;
				sb->append(this->_data->_idxm->getName(tpData));
			}
			else if (CppUtil::isInstanceOf<TypeData>(*(TypeData*)obj))
			{
				TypeData* tpData = (TypeData*)obj;
				sb->append(this->_data->_idxm->getName(tpData));
			}
			else
				sb->append("???");
		}
		sb->append(')');
		std::string ret = sb->toString();
		delete sb;
		
		return ret;
	}

	bool MethodData::get_IsField() const
	{
		return dynamic_cast<MemberRefTable*>(this->_table) != 0 && this->_signatureFlags == 6;
	}

	void MethodData::initializeInstanceFields()
	{
		_flags = 0;
		_size = 0;
		_retType = new TypeData();
		_maxStack = 0;
		_codeSize = 0;
		_localVarSigTok = 0;
		_ilAddr = 0;
		_signatureFlags = 0;
		_paramCount = 0;
		_kind = 0;
		_dataSize = 0;
	}
}