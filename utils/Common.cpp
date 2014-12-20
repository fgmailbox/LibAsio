#include "server.hpp"
#include "Common.h"
#include "log.h"



common_lib_facade::common_lib_facade(const char *strConfFile)
{

	InitCommon(strConfFile);
}

common_lib_facade::~common_lib_facade()
{
	DeInitCommon();
}


bool common_lib_facade::InitCommon(const char *strConfFile)
{

	try
	{

		Singleton2<clientFacade,2000>::configure(strConfFile);
		//Singleton2<serverFacade,2000>::configure(strConfFile);

		Singleton2<clientFacade,2000>::instance().run();


		std::map<std::string,std::map<std::string,std::string> > cmdParam;
		cmdParam["listen"]["io_service_pool_size"] = "2";
		cmdParam["listen"]["speed_threads_for_a_io_service"] = "4";
		cmdParam["network"]["io_service_pool_size"] = "8";
		cmdParam["network"]["speed_threads_for_a_io_service"] = "8";
		cmdParam["timer"]["io_service_pool_size"] = "8";
		cmdParam["timer"]["speed_threads_for_a_io_service"] = "8";
		cmdParam["host"]["address"] = "0.0.0.0";
		cmdParam["host"]["port"] = "10005";
		cmdParam["host"]["min_read_buffer_size"] = "4096";
		cmdParam["logfile"]["filename"] = ".\\Service-Monitor-%D-%T.log";


		Singleton2<logger,3000>::instance().Configure(cmdParam,logger::E_LOG_FUNC);



		//Singleton2<logger,3000>::instance().s_log_source |= logger::E_LOG_FUNC;

		Singleton2<clientFacade,2000>::instance().alloc_timer(http::server::facade::E_ALLOC_IOS_NET_POOL)->start_from_now(
			boost::bind(&logger::init_file,boost::ref(Singleton2<logger,3000>::instance()),_1),
#ifdef _DEBUG
			boost::posix_time::hours(2),
#else
			boost::posix_time::hours(8),
#endif
			-1);



		ULONG  HeapFragValue;
		SIZE_T size = sizeof(HeapFragValue);

		HeapQueryInformation(GetProcessHeap(),
			HeapCompatibilityInformation,
			&HeapFragValue,
			size,
			&size);

		if (HeapFragValue == 2)
		{
			LOG(notice,"当前是低碎片内存模式");
		}
		else
		{
			LOG(warning,"当前不是低碎片内存模式");
		}

		HeapFragValue = 2;

		if(HeapSetInformation(GetProcessHeap(),
			HeapCompatibilityInformation,
			&HeapFragValue,
			sizeof(HeapFragValue))
			)
		{
			LOG(notice,"设置低碎片内存模式成功");
		}
		else
		{
			LOG(warning,"设置低碎片内存模式失败");
		}









	}
	catch (std::exception& e)
	{
		std::cerr << "exception: " << e.what() << "\n";
	}




	return 0;



}



bool common_lib_facade::DeInitCommon()
{

	SingletonBase::SetStopAll();
	return true;
}

