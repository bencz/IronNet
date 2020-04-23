#ifndef __DOTNET_RVAMANAGER_H__
#define __DOTNET_RVAMANAGER_H__

#include <iostream>
#include <iomanip>
#include <vector>


namespace PEAnalyzer
{
	class RVAManager
	{
	private:
		std::vector<int>* _addrs_p;
		std::vector<int>* _addrs_v;

	public:
		RVAManager();
		~RVAManager();

		void setAddress(int phys, int virt);
		int convertToPhysical(int virt);
		int convertToVirtual(int phys);
		static int getAddress(long long rva);
		static int getSize(long long rva);
	};
}

#endif