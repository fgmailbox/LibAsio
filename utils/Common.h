
//#include "log.h"

class common_lib_facade
{
public:
	common_lib_facade(const char *strConfFile);

	~common_lib_facade();



	bool InitCommon(const char *strConfFile);

	bool DeInitCommon();


};
