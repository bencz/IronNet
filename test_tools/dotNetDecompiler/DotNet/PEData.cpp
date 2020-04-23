#include "IncFileNm.h"
#include PEDATA_H
#include RVAMANAGER_H
#include INDEXMANAGER_H
#include PEHEADERS_H
#include UTIL_H
#include METADATATABLES_H
#include TABLEBASE_H
#include STREAMHEADER_H
#include CODEDINDEX_H

namespace PEAnalyzer
{
	PEData::PEData()
	{
		this->init();
	}

	PEData::~PEData()
	{
		delete _rva;
		delete _idxm;
		delete _pe1;
		delete _pe2;
		delete _pe3;
		delete _pe4;
		delete _cli;
		delete _mdroot;
		delete _tilde;
		delete _sharpTilde;
		delete _strings;
		delete _usrstr;
		delete _guid;
		delete _blob;
	}

	PEData::PEData(std::vector<unsigned char>& data)
	{
		this->setData(data);
	}

	void PEData::setData(std::vector<unsigned char>& data)
	{
		this->init();
		this->_data = data; 
		this->readPEHeaders();
		this->readCLIHeader();
		if (this->_tilde == 0 || this->_sharpTilde == 0)
			return;
		this->_idxm->makeTree(this);
	}

	void PEData::init()
	{
		this->_data.clear();
		this->_rva = 0;
		this->_idxm = 0;
		this->_pe1 = 0;
		this->_pe2 = 0;
		this->_pe3 = 0;
		this->_pe4 = 0;
		this->_imptbl.clear();
		this->_sects.clear();
		this->_cli = 0;
		this->_mdroot = 0;
		CodedIndex::initCodedIndex();
		OpCodes::initOpCodes();
	}

	void PEData::readPEHeaders()
	{
		int offset = Util::getInt32(this->_data, 0x3c);
		offset += 4;
		this->_pe1 = new PEFileHeader();
		this->_pe1->readData(this->_data, offset);
		offset += 20;
		this->_pe2 = new PEHeaderStandardFields();
		this->_pe2->readData(this->_data, offset);
		this->_pe3 = new PEHeaderWindowsNTSpecificFields();
		this->_pe3->readData(this->_data, offset);
		this->_pe4 = new PEHeaderDataDirectories();
		this->_pe4->readData(this->_data, offset);
		offset += 224;
		this->readSections(offset);
		int addr_it = RVAManager::getAddress(this->_pe4->ImportTable);
		if (addr_it == 0)
			return;

		std::vector<ImportTable*> list;
		for (int ad = this->_rva->convertToPhysical(addr_it); !Util::isZero(this->_data, ad, 20); ad += 20)
		{
			ImportTable* it = new ImportTable();
			it->readData(this->_data, ad);
			list.push_back(it);


			std::cout << Util::getASCIIString(this->_data, _rva->convertToPhysical(it->Name)) << std::endl;
		}
		this->_imptbl = list;
	}

	void PEData::readSections(int offset)
	{
		this->_rva = new RVAManager();
		int len = this->_pe1->NumberOfSections;
		this->_sects = std::vector<SectionHeaders*>(len);
		for (int i = 0; i < len; i++)
		{
			_sects[i] = new SectionHeaders();
			_sects[i]->readData(this->_data, offset);
			offset += 40;
			this->_rva->setAddress(_sects[i]->PointerToRawData, _sects[i]->VirtualAddress);
		}
	}

	void PEData::readCLIHeader()
	{
		int addr_cli = RVAManager::getAddress(this->_pe4->CLIHeader);
		if (addr_cli == 0)
			return;
		_cli = new CLIHeader();
		this->_cli->readData(this->_data, this->_rva->convertToPhysical(addr_cli));
		int offset = this->_rva->convertToPhysical(RVAManager::getAddress(this->_cli->MetaData));
		this->_mdroot = new MetadataRoot();
		this->_mdroot->readData(this->_data, offset);
		offset += this->_mdroot->Length + 18;
		int nStreams = Util::getInt16(this->_data, offset);
		offset += 2;
		this->_idxm = new IndexManager(this->_data, this->_rva);
		for (int i = 0; i < nStreams; i++)
		{
			StreamHeader* sh = new StreamHeader();
			sh->_mdroot = _mdroot;
			sh->readData(this->_data, offset);
			offset += sh->getStreamSize();
			this->setStreamHeader(sh);
		}
		this->readTilde();
	}

	void PEData::setStreamHeader(StreamHeader* sh)
	{
		if (sh->_name == "#~")
		{
			this->_tilde = sh;
			this->_sharpTilde = new SharpTildeStream();
			this->_sharpTilde->readData(this->_data, this->_mdroot->getOffset() + sh->_offset);
		}
		else if (sh->_name == "#Strings")
		{
			this->_strings = sh;
			this->_idxm->_stringsOffset = sh->getDataOffset();
		}
		else if (sh->_name == "#US")
		{
			this->_usrstr = sh;
			this->_idxm->_usOffset = sh->getDataOffset();
		}
		else if (sh->_name == "#GUID")
		{
			this->_guid = sh;
			this->_idxm->_guidOffset = sh->getDataOffset();
		}
		else if (sh->_name == "#Blob")
		{
			this->_blob = sh;
			this->_idxm->_blobOffset = sh->getDataOffset();
		}
	}

	void PEData::readTilde()
	{
		if (this->_tilde == 0 || this->_sharpTilde == 0)
			return;
		this->_idxm->set_HeapSizes(this->_sharpTilde->HeapSizes);
		int num = 0, ad = this->_sharpTilde->getOffset() + 24;
		for (int i = 0; i < 63; i++)
		{
			if ((this->_sharpTilde->Valid &(1LL << i)) == 0)
				continue;
			int rows = Util::getInt32(this->_data, ad);
			bool is_def = MetadataTable::Contains((MetadataTables::Value)i);
			if (is_def)
				_idxm->_tableRows[i] = rows;
			num++;
			ad += 4;
		}
		for (int i = 0; i < 63; i++)
		{
			int rows = _idxm->_tableRows[i];
			for (int j = 0; j < rows; j++)
			{
				TableBase* tbl = this->_idxm->createTable(static_cast<MetadataTables::Value>(i));
				this->_idxm->_tables[i].push_back(tbl);
				tbl->readData(_data, ad);
				tbl->_index = j + 1;
				//tbl->AppendTitle(std::string::Format(" {0:X}", j + 1));
				//tbl->AppendName();
				ad += tbl->getSize();
			}
		}
	}
}