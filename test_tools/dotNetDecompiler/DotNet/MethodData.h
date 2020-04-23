#ifndef __DOTNET_METHODDATA_H__
#define __DOTNET_METHODDATA_H__

#include "Types.h"
#include <string>
#include <vector>
#include <typeinfo>
#include "IncFileNm.h"
#include STRINGBUILDER_H

namespace PEAnalyzer
{
	class PEData;
	class TableBase;
	class StandAloneSigTable;
	class ILCode;
	class ImplMapTable;
	class MethodDefTable;
	class MemberRefTable;
	class TypeData;
	class ExceptionHandlingClauses;
}

namespace PEAnalyzer
{
	class MethodData
	{
	private:
		static std::vector<OpCode> _opTable;
		PEData* _data;
		TableBase* _table;
		std::vector<ILCode*> _ilCodes;
		int _ilAddr;
	public:
		int _flags;
		int _size;
		TypeData* _retType;
		int _maxStack;
		int _codeSize;
		int _localVarSigTok;
		StandAloneSigTable* _localVarSig;
		std::vector<TypeData*> _localVars;
		int _signatureFlags;
		int _paramCount;
		ImplMapTable* _implMap;
		int _kind;
		int _dataSize;
		std::vector<ExceptionHandlingClauses*> _ehTable;

		MethodData(PEData* data, MethodDefTable* m);
		MethodData(PEData* data, MemberRefTable* m);
		virtual ~MethodData();

		std::vector<ILCode*> get_ILCodes();

	private:
		void parse(PEData* data, TableBase* m);
		void readSignature();
		void readLocalVars();
		ILCode* readIL(int ad);

	public:
		int get_RVA() const;
		bool get_HasThis() const;
		std::string get_Name() const;
		std::string get_FullName() const;
		bool get_IsField() const;

	private:
		void initializeInstanceFields();
	};

}

#endif