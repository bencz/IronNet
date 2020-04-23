#include "IncFileNm.h"
#include RVAMANAGER_H

namespace PEAnalyzer
{

	RVAManager::RVAManager()
	{
		this->_addrs_p = new std::vector<int>();
		this->_addrs_v = new std::vector<int>();
	}

	RVAManager::~RVAManager()
	{
		delete _addrs_p;
		delete _addrs_v;
	}

	void RVAManager::setAddress(int phys, int virt)
	{
		if (phys == 0 || virt == 0)
			return;
		this->_addrs_p->push_back(phys);
		this->_addrs_v->push_back(virt);
	}

	int RVAManager::convertToPhysical(int virt)
	{
		int p = -1;
		int max = 0;
		for (int i = 0; i < this->_addrs_v->size(); i++)
		{
			int v = this->_addrs_v->at(i);
			if (max < v && v <= virt)
			{
				p = i;
				max = v;
			}
		}
		return (p >= 0) ? virt - max + this->_addrs_p->at(p) : virt;
	}

	int RVAManager::convertToVirtual(int phys)
	{
		int p = -1;
		int max = 0;
		for (int i = 0; i < this->_addrs_p->size(); i++)
		{
			int v = this->_addrs_p->at(i);
			if (max < v && v <= phys)
			{
				p = i;
				max = v;
			}
		}
		return (p >= 0) ? phys - max + this->_addrs_v->at(p) : phys;
	}

	int RVAManager::getAddress(long long rva)
	{
		std::cout << "GetAddress(" << std::hex << std::setw(0) << std::uppercase << rva << std::dec << std::nouppercase << ") ==> "
			<< std::hex << std::setw(1) << std::uppercase << static_cast<int>(rva & 0xffffffff) << std::dec << std::nouppercase << std::endl;
		return static_cast<int>(rva & 0xffffffff);
	}

	int RVAManager::getSize(long long rva)
	{
		std::cout << "GetSize(" << std::hex << std::setw(0) << std::uppercase << rva << std::dec << std::nouppercase << ") ==> "
			<< std::hex << std::setw(1) << std::uppercase << static_cast<int>(rva >> 32) << std::dec << std::nouppercase << std::endl;
		return static_cast<int>(rva >> 32);
	}
}
