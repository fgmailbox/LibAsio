#pragma once
#include <stdarg.h>
#include <pantheios/util/test/compiler_warnings_suppression.first_include.h>
#define PANTHEIOS_NO_USE_WIDE_STRINGS
#define PANTHEIOS_NO_INCLUDE_OS_AND_3PTYLIB_STRING_ACCESS // Faster compilation
#define PANTHEIOS_FORCE_AUTO_INIT
/* Pantheios Header Files */
#include <pantheios/pantheios.hpp>                  // Pantheios C++ main header
#include <platformstl/platformstl.h>
#include <pantheios/util/test/compiler_warnings_suppression.last_include.h>

#include <boost/date_time.hpp>
#include <boost/date_time/local_time/local_time.hpp>
#include <boost/local_function.hpp>
#include <boost/detail/lightweight_mutex.hpp>

#include "Singleton2.h"

// Here we define our application severity levels.
enum severity_level
{
	emergency,
	alert,
	critical,
	error,
	warning,
	notice,
	informational,
	debug,
};

class logger
{
public:

	enum
	{
		E_LOG_FILE = 0x01,
		E_LOG_FUNC = 0x02,
	};
	logger();
	~logger(void);
	 void init();
	 void init_file(long ID = 0);
	 void init_console();

	 void log( severity_level level,const char* fmt, ...);
	 void log( const char *file, const char *func,long line, severity_level level,const char* fmt, ...);


	 void log_msg( severity_level level, string &msg );

	 unsigned	m_log_format;
	 int		m_severityCeiling;


	bool Configure(std::map<std::string,std::map<std::string,std::string> >& cmdParam,unsigned	log_format = 0,int	severityCeiling  = pantheios::informational);	

private:

	template<int MinBufLen>
	string format_msg( const char* fmt, va_list ap);

	string format_msg( const char* fmt, ...);

	boost::detail::lightweight_mutex lock;
	bool	m_bICanWork;
	std::string		m_strFileName;

};


#define LOG(level ,format , ...)\
{\
	if(!Singleton2<logger,3000>::expired())\
	{\
		if (Singleton2<logger,3000>::instance().m_log_format == (logger::E_LOG_FILE|logger::E_LOG_FUNC))\
			Singleton2<logger,3000>::instance().log(__FILE__,__FUNCTION__,__LINE__, level,format,##__VA_ARGS__);\
		else if(Singleton2<logger,3000>::instance().m_log_format == logger::E_LOG_FUNC)\
			Singleton2<logger,3000>::instance().log(NULL,__FUNCTION__,__LINE__, level,format,##__VA_ARGS__);\
		else\
			Singleton2<logger,3000>::instance().log( level,format,##__VA_ARGS__);\
	}\
}


#define LOG2(frequence,level ,format , ...)\
{\
	static boost::atomic<long>  print_count(0);\
	long  count = ++print_count;\
	if( count % frequence == 0)\
	{\
		if(!Singleton2<logger,3000>::expired())\
		{\
			if (Singleton2<logger,3000>::instance().m_log_format == (logger::E_LOG_FILE|logger::E_LOG_FUNC))\
				Singleton2<logger,3000>::instance().log(__FILE__,__FUNCTION__,__LINE__, level,format,##__VA_ARGS__);\
			else if(Singleton2<logger,3000>::instance().m_log_format == logger::E_LOG_FUNC)\
				Singleton2<logger,3000>::instance().log(NULL,__FUNCTION__,__LINE__, level,format,##__VA_ARGS__);\
			else\
				Singleton2<logger,3000>::instance().log( level,format,##__VA_ARGS__);\
		}\
	}\
}

// 1/1000000 second units
#define LOG3(timeCount,level ,format , ...)\
{\
	static boost::posix_time::ptime markTime =  boost::posix_time::microsec_clock::universal_time();\
	if( boost::posix_time::microsec_clock::universal_time() > markTime + boost::posix_time::microseconds(timeCount))\
	{\
		markTime =  boost::posix_time::microsec_clock::universal_time();\
		if(!Singleton2<logger,3000>::expired())\
		{\
			if (Singleton2<logger,3000>::instance().m_log_format == (logger::E_LOG_FILE|logger::E_LOG_FUNC))\
				Singleton2<logger,3000>::instance().log(__FILE__,__FUNCTION__,__LINE__, level,format,##__VA_ARGS__);\
			else if(Singleton2<logger,3000>::instance().m_log_format == logger::E_LOG_FUNC)\
				Singleton2<logger,3000>::instance().log(NULL,__FUNCTION__,__LINE__, level,format,##__VA_ARGS__);\
			else\
				Singleton2<logger,3000>::instance().log( level,format,##__VA_ARGS__);\
		}\
	}\
}


