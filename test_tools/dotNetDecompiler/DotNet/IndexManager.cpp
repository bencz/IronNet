#include "IncFileNm.h"

#include INDEXMANAGER_H
#include METADATATABLECREATOR_H
#include UTIL_H
#include CODEDINDEX_H
#include RVAMANAGER_H
#include TABLEBASE_H
#include PEDATA_H
#include METHODDATA_H
#include STRINGHELPER_H
#include UTF_H


namespace PEAnalyzer
{
	IndexManager::IndexManager(std::vector<unsigned char>& data, RVAManager * rva)
	{
		_strings = false;
		_stringsOffset = 0;
		_usOffset = 0;
		_guid = false;
		_guidOffset = 0;
		_blob = false;
		_blobOffset = 0;

		this->_data = data;
		this->_rva = rva;
		this->_strings = this->_guid = this->_blob = false;
		this->_stringsOffset = this->_usOffset = this->_guidOffset = this->_blobOffset = 0;
		this->_tables = std::vector<std::vector<void*>>(64);
		this->_tableRows = std::vector<int>(64);
		for (int i = 0; i < 64; i++)
		{
			this->_tables[i] = std::vector<void*>();
			this->_tableRows[i] = 0;
		}
	}

	IndexManager::~IndexManager()
	{
		delete _rva;
	}

	void IndexManager::set_HeapSizes(const unsigned char& value)
	{
		this->_strings = (value & 1) != 0;
		this->_guid = (value & 2) != 0;
		this->_blob = (value & 4) != 0;
	}

	TableBase* IndexManager::createTable(MetadataTables::Value table)
	{
		TableBase* ret = MetadataTableCreator::Create(table);
		ret->_indexManager = this;
		return ret;
	}

	std::string IndexManager::getStringsString(int offset)
	{
		std::vector<unsigned char> bytes = Util::getBytes(this->_data, this->_stringsOffset + offset);
		std::string ret(bytes.begin(), bytes.end());
		return ret;

		//return Encoding::UTF8->GetString(Util::getBytes(this->Data, this->StringsOffset + offset));
	}
	
	std::string IndexManager::getUSString(int offset)
	{	
		int ad = this->_usOffset + offset;
		DoubleInt* dataSize = Util::getDataSize(this->_data, ad);
		std::vector<unsigned char> bytes = Util::getBytes(this->_data, ad + dataSize->A, dataSize->B);
		std::string tret(bytes.begin(), bytes.begin() + dataSize->B - 1);
		std::string ret = utf::convert_encoding(tret, utf::encoding_type::ENCODING_UTF16LE, utf::encoding_type::ENCODING_ASCII, false);
		delete dataSize;

		return ret;
	}
	
	std::vector<unsigned char> IndexManager::getBlobBytes(int offset)
	{
		int ad = this->_blobOffset + offset;
		DoubleInt* dataSize = Util::getDataSize(this->_data, ad);
		return Util::getBytes(this->_data, ad + dataSize->A, dataSize->B);
	}
	
	bool IndexManager::isInt32(MetadataTables::Value table)
	{
		return this->_tableRows[static_cast<int>(table)] > max16;
	}
	
	int IndexManager::getTagSize(CodedIndices::Value index)
	{
		int ret = 0;
		for (size_t i = CodedIndex::Data[static_cast<int>(index)].size() - 1; i > 0; i >>= 1)
			ret++;
		return ret;
	}
	
	bool IndexManager::isInt32(CodedIndices::Value index)
	{
		int max = 0;
		for (std::vector<MetadataTables::Value>::const_iterator tbl = CodedIndex::Data[static_cast<int>(index)].begin(); tbl != CodedIndex::Data[static_cast<int>(index)].end(); ++tbl)
		{
			int sz_tbl = this->_tableRows[static_cast<int>(*tbl)];
			if (max < sz_tbl)
				max = sz_tbl;
		}
		return (max << this->getTagSize(index)) > max16;
	}
	
	int IndexManager::getIndex(CodedIndices::Value index, int v)
	{
		return v >> this->getTagSize(index);
	}
	
	MetadataTables::Value IndexManager::getIndexType(CodedIndices::Value index, int v)
	{
		int sz = this->getTagSize(index);
		int vv = (v >> sz) << sz;
		return CodedIndex::Data[static_cast<int>(index)][v - vv];
	}
	
	int IndexManager::getToken(CodedIndices::Value index, int v)
	{
		int sz = this->getTagSize(index);
		int vv = v >> sz;
		return ((static_cast<int>(CodedIndex::Data[static_cast<int>(index)][v - (vv << sz)])) << 24) | vv;
	}
	
	TableBase* IndexManager::getTable(MetadataTables::Value t, int v)
	{
		if (v == 0)
			return 0;
		return static_cast<TableBase*>(this->_tables[static_cast<int>(t)][v - 1]);
	}
	
	TableBase* IndexManager::getTable(int token)
	{
		if (token == 0)
			return 0;
		return static_cast<TableBase*>(this->_tables[token >> 24][(token & 0xffffff) - 1]);
	}
	
	void IndexManager::makeTree(PEData* data)
	{
		std::vector<void*> listType = this->_tables[static_cast<int>(MetadataTables::Value::TYPEDEF)];
		int len = (int)listType.size();
		for (int i = 0; i < len; i++)
		{
			void* objt1 = listType.at(i);
			TypeDefTable* t1 = (TypeDefTable*)(objt1);

			TypeDefTable* t2 = nullptr;
			if (i + 1 < len)
			{
				void* objt2 = listType.at(i + 1);
				t2 = (TypeDefTable*)objt2;
			}

			this->makeTree(t1, t2);
		}

		for (int i = 0; i < this->_tables[static_cast<int>(MetadataTables::Value::ASSEMBLYREF)].size(); i++)
		{
			TableBase* tblBase = static_cast<TableBase*>(this->_tables[static_cast<int>(MetadataTables::Value::ASSEMBLYREF)][i]);
			tblBase->_children.push_back(std::vector<void*>());
		}

		for (int i = 0; i < this->_tables[static_cast<int>(MetadataTables::Value::TYPEREF)].size(); i++)
		{

			TableBase* tblBase = static_cast<TableBase*>(this->_tables[static_cast<int>(MetadataTables::Value::TYPEREF)][i]);
			for (int j = 0; j < 3; j++)
				tblBase->_children.push_back(std::vector<void*>());
		}

		for (int i = 0; i < this->_tables[static_cast<int>(MetadataTables::Value::TYPEREF)].size(); i++)
		{
			TypeRefTable* t = static_cast<TypeRefTable*>(this->_tables[static_cast<int>(MetadataTables::Value::TYPEREF)][i]);
			if (t->ResolutionScope == 0)
				continue;

			t->_parentTable = this->getTable(this->getToken(CodedIndices::Value::RESOLUTIONSCOPE, t->ResolutionScope));
			t->_parentTable->_children[static_cast<int>(Children::Value::REFNESTED)].push_back(t);
		}

		for (int i = 0; i < this->_tables[static_cast<int>(MetadataTables::Value::METHODDEF)].size(); i++)
		{
			MethodDefTable* m = static_cast<MethodDefTable*>(this->_tables[static_cast<int>(MetadataTables::Value::METHODDEF)][i]);
			m->_tag = new MethodData(data, m);
		}

		for (int i = 0; i < this->_tables[static_cast<int>(MetadataTables::Value::MEMBERREF)].size(); i++)
		{
			MemberRefTable* m = static_cast<MemberRefTable*>(this->_tables[static_cast<int>(MetadataTables::Value::MEMBERREF)][i]);
			MethodData* md = new MethodData(data, m);
			m->_tag = md;
			if (m->Class == 0)
				continue;
			m->_parentTable = this->getTable(this->getToken(CodedIndices::Value::MEMBERREFPARENT, m->Class));
			if (md->get_IsField())
				m->_parentTable->_children[static_cast<int>(Children::Value::REFFIELD)].push_back(m);
			else
				m->_parentTable->_children[static_cast<int>(Children::Value::REFMETHOD)].push_back(m);
		}

		for (int i = 0; i < this->_tables[static_cast<int>(MetadataTables::Value::IMPLMAP)].size(); i++)
		{
			ImplMapTable* im = static_cast<ImplMapTable*>(this->_tables[static_cast<int>(MetadataTables::Value::IMPLMAP)][i]);
			MethodDefTable* m = static_cast<MethodDefTable*>(this->getTable(this->getToken(CodedIndices::Value::MEMBERFORWARDED, im->MemberForwarded)));
			if (m != 0)
				(static_cast<MethodData*>(m->_tag))->_implMap = im;
		}

		for (int i = 0; i < this->_tables[static_cast<int>(MetadataTables::Value::NESTEDCLASS)].size(); i++)
		{
			NestedClassTable* nc = static_cast<NestedClassTable*>(this->_tables[static_cast<int>(MetadataTables::Value::NESTEDCLASS)][i]);
			TypeDefTable* nst = (TypeDefTable*)(listType[nc->NestedClass - 1]);
			TypeDefTable* enc = (TypeDefTable*)(listType[nc->EnclosingClass - 1]);
			nst->_parentTable = enc;
			enc->_children[static_cast<int>(Children::Value::DEFNESTED)].push_back(nst);
		}
	}

	void IndexManager::makeTree(TypeDefTable* t1, TypeDefTable* t2)
	{
		t1->_children = std::vector<std::vector<void*>>
		{
			std::vector<void*>(),
			std::vector<void*>(),
			std::vector<void*>()
		};

		std::vector<void*> listField = this->_tables[static_cast<int>(MetadataTables::Value::FIELD)];
		std::vector<void*> listMethod = this->_tables[static_cast<int>(MetadataTables::Value::METHODDEF)];
		int f1 = t1->FieldList;
		int f2 = (t2 != 0) ? t2->FieldList : (int)listField.size() + 1;

		for (int i = f1; i < f2; i++)
		{
			FieldTable* f = static_cast<FieldTable*>(listField.at(i - 1));
			t1->_children[static_cast<int>(Children::Value::DEFFIELD)].push_back(f);
			f->_parentTable = t1;
		}
		int m1 = t1->MethodList, m2 = (t2 != 0) ? t2->MethodList : (int)listMethod.size() + 1;
		for (int i = m1; i < m2; i++)
		{
			MethodDefTable* m = static_cast<MethodDefTable*>(listMethod.at(i - 1));
			t1->_children[static_cast<int>(Children::Value::DEFMETHOD)].push_back(m);
			m->_parentTable = t1;
			this->makeTree(m);
		}
	}

	void IndexManager::makeTree(MethodDefTable* m)
	{
		m->_children = std::vector<std::vector<void*>>
		{
			std::vector<void*>()
		};

		std::vector<void*> listParam = this->_tables[static_cast<int>(MetadataTables::Value::PARAM)];
		int paramCount = this->getBlobBytes(m->Signature)[1];
		for (int i = 0; i < paramCount; i++)
		{
			ParamTable* p = static_cast<ParamTable*>(listParam[m->ParamList + i - 1]);
			m->_children[0].push_back(p);
			p->_parentTable = m;
		}
	}

	TypeData* IndexManager::readType(int offset)
	{
		TypeData* ret = new TypeData();

		ret->Size = 1;
		ret->Token = 0;
		ret->Element = ret->Option = ElementType::Value::END;

		int b = this->_data[offset];
		if (!ElementTypes::Contains((ElementType::Value)b))
			return ret;

		switch (b)
		{
		case ElementType::Value::SZARRAY:
		{
			ret = this->readType(offset + 1);
			ret->Size++;
			ret->Option = (ElementType::Value)b;
			break;
		}
		case ElementType::Value::VALUETYPE:
		case ElementType::Value::CLASS:
		{
			DoubleInt* ds = Util::getDataSize(this->_data, offset + 1);
			ret->Token = this->getToken(CodedIndices::Value::TYPEDEFORREF, ds->B);
			ret->Size = 1 + ds->A;
			delete ds;

			break;
		}
		default:
			ret->Element = (ElementType::Value)b;
			break;
		}
		return ret;
	}

	std::string IndexManager::getName(ElementType::Value e)
	{
		return StringHelper::toLower(ElementTypes::toString(e));
	}

	std::string IndexManager::getName(TypeData* t)
	{
		std::string n = "???";
		if (t->Token == 0)
			n = this->getName(t->Element);
		else
		{
			TableBase* tb = this->getTable(t->Token);
			if (dynamic_cast<TypeDefTable*>(tb) != 0)
				n = this->getName(dynamic_cast<TypeDefTable*>(tb));
			else if (dynamic_cast<TypeRefTable*>(tb) != 0)
				n = this->getName(dynamic_cast<TypeRefTable*>(tb));
		}
		if (t->Option == ElementType::Value::SZARRAY)
			n += "[]";
		return n;
	}

	std::string IndexManager::getName(TypeDefTable* t)
	{
		std::string n = this->getStringsString(t->Name);
		TableBase* tb = t->_parentTable;
		if (dynamic_cast<TypeDefTable*>(tb) != 0)
			return this->getName(dynamic_cast<TypeDefTable*>(tb)) + "." + n;
		if (t->Namespace != 0)
			n = this->getStringsString(t->Namespace) + "." + n;
		return n;
	}

	std::string IndexManager::getName(TypeRefTable* t)
	{
		std::string n = this->getStringsString(t->Name);
		TableBase* tb = t->_parentTable;
		if (dynamic_cast<TypeRefTable*>(tb) != 0)
			return this->getName(dynamic_cast<TypeRefTable*>(tb)) + "." + n;
		if (t->Namespace != 0)
			n = this->getStringsString(t->Namespace) + "." + n;
		if (dynamic_cast<AssemblyRefTable*>(tb) != 0)
		{
			std::string asm_Renamed = this->getStringsString((dynamic_cast<AssemblyRefTable*>(tb))->Name);
			std::string ret = "[" + asm_Renamed + "]" + n;
			return ret;
		}
		return n;
	}

	std::string IndexManager::getName(MethodDefTable* m)
	{
		std::string n = this->getStringsString(m->Name);
		if (m->_parentTable == 0)
			return n;
		return this->getName(dynamic_cast<TypeDefTable*>(m->_parentTable)) + "::" + n;
	}

	std::string IndexManager::getName(MemberRefTable* m)
	{
		std::string n = this->getStringsString(m->Name);
		if (m->_parentTable == 0)
			return n;
		return this->getName(dynamic_cast<TypeRefTable*>(m->_parentTable)) + "::" + n;
	}
}
