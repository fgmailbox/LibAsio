#include "log.h"
///兼容控制台和文件的读写必须包含这两个文件
#include <pantheios/backends/be.N.h>                // pan_be_N_t声明  
#include <pantheios/frontends/fe.N.h>

#include <pantheios/util/test/compiler_warnings_suppression.first_include.h>

//#define PANTHEIOS_NO_USE_WIDE_STRINGS
//#define PANTHEIOS_NO_INCLUDE_OS_AND_3PTYLIB_STRING_ACCESS // Faster compilation

/* Pantheios Header Files */
//#include <pantheios/pantheios.hpp>                  // Pantheios C++ main header
//#include <platformstl/platformstl.h>
//#include <pantheios/backend.h>
#if defined(PLATFORMSTL_OS_IS_UNIX)
#include <pantheios/backends/bec.fprintf.h>        // Include the API for bec.fprintf
#include <pantheios/implicit_link/be.fprintf.h> // Implicitly link the stock back-end be.fprintf
#elif defined(PLATFORMSTL_OS_IS_WINDOWS)
#include <pantheios/frontends/fe.simple.h>
#include <pantheios/backends/bec.WindowsConsole.h>
#include <pantheios/backends/bec.file.h>      // be.file header

#include <pantheios/implicit_link/core.h> 
#include <pantheios/implicit_link/fe.simple.h> 
#include <pantheios/implicit_link/be.N.h>			///兼容控制台和文件的读写必须包含这个文件
#include <pantheios/implicit_link/be.WindowsConsole.WithCallback.h> 
#include <pantheios/implicit_link/be.file.h> 
//#include <pantheios/implicit_link/be.file.WithCallback.h> 

#else /* ? OS */
# error Platform not discriminated
#endif /* OS */


//#include <pantheios/frontend.h>
#include <pantheios/util/string/strdup.h>
#include <pantheios/init_codes.h>

#include <pantheios/inserters/threadid.hpp>     // for pantheios::threadId

#//include <pantheios/implicit_link/core.h>       // Implicitly link the core


/* Standard C/C++ Header Files */
#include <exception>                            // for std::exception
#include <new>                                  // for std::bad_alloc
#include <string>                               // for std::string
#include <stdlib.h>                             // for exit codes, atoi()
#include <stdio.h>                             // for exit codes, atoi()

/* STLSoft Header Files */
#include <pantheios/util/memory/auto_buffer_selector.hpp>   // for stlsoft::auto_buffer
#include <platformstl/system/system_traits.hpp>


#if defined(PLATFORMSTL_OS_IS_WINDOWS)
/* Windows Header Files */
# include <windows.h>                               // for console colour constants
#endif /* PLATFORMSTL_OS_IS_WINDOWS */

#include <pantheios/util/test/compiler_warnings_suppression.last_include.h>

/* ////////////////////////////////////////////////////////////////////// */






/* Define the stock front-end process identity, so that it links when using
 * fe.N, fe.simple, etc. */
//PANTHEIOS_EXTERN_C PAN_CHAR_T PANTHEIOS_FE_PROCESS_IDENTITY[] = PANTHEIOS_LITERAL_STRING("boost.asio.test");
#define PSTR(x)         PANTHEIOS_LITERAL_STRING(x)


enum beid_target  
{  
	beid_Console =   1,  
	beid_File =   2  
};  

pan_be_N_t  PAN_BE_N_BACKEND_LIST[] =  
{  
	PANTHEIOS_BE_N_STDFORM_ENTRY(beid_Console, pantheios_be_WindowsConsole, 0),  
	PANTHEIOS_BE_N_STDFORM_ENTRY(beid_File, pantheios_be_file, 1),  
	PANTHEIOS_BE_N_TERMINATOR_ENTRY  
};
pan_fe_N_t PAN_FE_N_SEVERITY_CEILINGS[]  =
{
	{ 1,  PANTHEIOS_SEV_NOTICE    }, /* Filters out everything below 'notice' */
	{ 2,  PANTHEIOS_SEV_DEBUG     }, /* Allows all severities */
	//{ 3,  PANTHEIOS_SEV_ERROR     }, /* Allows only 'error', 'critical', 'alert', 'emergency' */
	{ 0,  PANTHEIOS_SEV_NOTICE    } /* Terminates the array; sets the default ceiling to 'notice' */
};


/* ////////////////////////////////////////////////////////////////////// */




#if defined(PLATFORMSTL_OS_IS_UNIX)


#elif defined(PLATFORMSTL_OS_IS_WINDOWS)

#if 1
PANTHEIOS_CALL(void) pantheios_be_WindowsConsole_getAppInit(	int  backEndId, pan_be_WindowsConsole_init_t* init ) /* throw() */
{
	//init->flags |= PANTHEIOS_BE_INIT_F_NO_DATETIME; // Don't show date/time
	//init->flags |=   PANTHEIOS_BE_INIT_F_USE_UNIX_FORMAT; 
	init->flags |=   PANTHEIOS_BE_INIT_F_USE_SYSTEM_TIME;
	init->flags |=   PANTHEIOS_BE_INIT_F_NO_PROCESS_ID;
	init->flags |=   PANTHEIOS_BE_INIT_F_HIGH_RESOLUTION;

	//init->colours[pantheios::debug]   = FOREGROUND_BLUE | FOREGROUND_INTENSITY;              // Lose the white background
	//init->colours[pantheios::notice]  = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED; // Lose the intensity
}
#endif




#if 0

/* ////////////////////////////////////////////////////////////////////// */

#define PSTR(x)         PANTHEIOS_LITERAL_STRING(x)

#ifdef PANTHEIOS_USE_WIDE_STRINGS
# define pan_strcpy_    wcscpy
#else
# define pan_strcpy_    strcpy
#endif

/* /////////////////////////////////////////////////////////////////////////
 * Application-defined functions
 */
#endif

PANTHEIOS_CALL(void) pantheios_be_file_getAppInit(int  backEndId , pan_be_file_init_t* init)
{
	init->flags |= PANTHEIOS_BE_FILE_F_TRUNCATE;        // Truncate the contents
	init->flags |= PANTHEIOS_BE_FILE_F_DELETE_IF_EMPTY; // Delete the file if nothing written
	init->flags |=   PANTHEIOS_BE_INIT_F_USE_SYSTEM_TIME;
	init->flags |=   PANTHEIOS_BE_INIT_F_NO_PROCESS_ID;
	init->flags |=   PANTHEIOS_BE_INIT_F_HIGH_RESOLUTION;

	//init->fileName = pan_strcpy_(init->buff, PSTR(".\\Push-log-%D-%T.log"));
}





#else /* ? OS */
# error Platform not discriminated
#endif /* OS */




logger::logger()
:m_bICanWork(false),
 m_strFileName(".\\Service-Monitor-%D-%T.log"),
 m_log_format(0),
 m_severityCeiling(pantheios::informational)
{
	//init();
	// Also let's add some commonly used attributes, like timestamp and record counter.

}

logger::~logger(void)
{
	m_bICanWork = false;
}

void logger::init()
{
	init_console();
	init_file();
}



bool logger::Configure(std::map<std::string,std::map<std::string,std::string> >& cmdParam,unsigned	log_format,int	severityCeiling )
{

	m_log_format		= log_format;
	m_severityCeiling	= severityCeiling;

	if(!cmdParam["logfile"]["filename"].empty())
	{
		m_strFileName = cmdParam["logfile"]["filename"];
		if(!m_bICanWork)
		{
			init();
			m_bICanWork = true;
		}
	}
	else
	{
		m_bICanWork = true;

	}

	return true;
}







void logger::init_file(long )
{
	//如果遇到同名文件，原文件的内容会被重写  
	pantheios_be_file_setFilePath(
		m_strFileName.c_str(),
		//PSTR(".\\Push-Data-%D-%T.log"),   
		PANTHEIOS_BE_FILE_F_TRUNCATE,
		PANTHEIOS_BE_FILE_F_TRUNCATE, 
		/*beid_File*/
		/*PANTHEIOS_BEID_LOCAL*/ 
		PANTHEIOS_BEID_ALL
		);  

}
void logger::init_console()
{
	pantheios_fe_simple_setSeverityCeiling(m_severityCeiling);
}

template<int MinBufLen>
std::string logger::format_msg( const char* fmt, va_list ap )
{
	std::string msg;
	{


		unsigned BOOST_LOCAL_FUNCTION(unsigned buffLen)
		{
			buffLen++;
			unsigned mask  = 0x01;
			do
			{
				mask <<= 1;
			}
			while((buffLen >>= 1) > 0) ;
			return mask;
		} BOOST_LOCAL_FUNCTION_NAME(LocalGetBufferSize)

			int len = vsnprintf(NULL,0, fmt, ap);

		len = LocalGetBufferSize(len);

		boost::scoped_array<char> spBufContent(new char[len < MinBufLen ? MinBufLen : len]);
		//char *content = (char *)alloca(len < MinBufLen ? MinBufLen : len);

		vsnprintf(spBufContent.get(), len, fmt, ap);
		msg = spBufContent.get();

	}	
	return msg;
}



void logger::log(severity_level level,const char* fmt, ...)
{
	string msg;
	{
		va_list ap;
		va_start(ap, fmt);
		msg = format_msg<512>(fmt, ap);
		va_end(ap);
	}

	if(msg.empty())
	{
		std::cerr<<"No Data to Log"<<std::endl;		
	}
	else
		log_msg(level, msg);
	
}

void logger::log(const char *file, const char *func,long line, severity_level level,const char* fmt, ...)
{
	string msg;
	{
		va_list ap;
		va_start(ap, fmt);
		msg = format_msg<512>(fmt, ap);
		va_end(ap);
	}
	
	if(!msg.empty())
	{

		if(file)
		{
			msg = format_msg("[%s][%s][%d]%s",file,func,line,msg.c_str());
		}
		else
		{
			msg = format_msg("[%s][%d]%s",func,line,msg.c_str());
		}
	}
	if(msg.empty())
	{
		std::cerr<<"No Data to Log"<<std::endl;		
	}
	else
		log_msg(level, msg);

}

void logger::log_msg( severity_level level, string &msg )
{
	if(m_bICanWork)
	{
		boost::detail::lightweight_mutex::scoped_lock guard(lock);

		if(m_strFileName.empty())
			std::clog<<msg<<std::endl;
		else
		{
			try
			{
				switch(level)
				{

				case debug:
					pantheios::log_DEBUG(msg.c_str());
					break;

				case informational:
					pantheios::log_INFORMATIONAL(msg.c_str());
					break;

				case notice:
					pantheios::log_NOTICE(msg.c_str());
					break;


				case warning:
					pantheios::log_WARNING(msg.c_str());
					break;

				case error:
					pantheios::log_ERROR(msg.c_str());
					break;

				case critical:
					pantheios::log_CRITICAL(msg.c_str());
					break;

				case alert:
					pantheios::log_ALERT(msg.c_str());
					break;

				case emergency:
					pantheios::log_EMERGENCY(msg.c_str());
					break;


				}
			}
			catch(...)
			{
			}

		}

	}
}

std::string logger::format_msg( const char* fmt, ... )
{
	std::string msg;
	{
		va_list ap;
		va_start(ap, fmt);
		msg = format_msg<512>(fmt,ap);
		va_end(ap);
	}	
	return msg;
}

