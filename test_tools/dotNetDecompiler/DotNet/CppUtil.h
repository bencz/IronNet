#ifndef __DOTNET_CPPUTIL_H__
#define __DOTNET_CPPUTIL_H__

class CppUtil
{
public:
	template<typename CheckType, typename InstanceType>
	static bool isInstanceOf(InstanceType &Instance) {
		return (dynamic_cast<CheckType *>(&Instance) != NULL);
	}
};

#endif