#pragma once

#include <vector>
#include <list>
#include <map>

#include <boost/smart_ptr.hpp>
#include <boost/system/error_code.hpp>
#include <boost/function.hpp>
#include <boost/date_time.hpp>

using boost::system::error_code;

//#define LIBASIO_API 

#ifdef LIBASIO_EXPORTS
#define LIBASIO_API __declspec(dllexport)
#else
#define LIBASIO_API __declspec(dllimport)
#endif


typedef std::vector<char> buffer;

typedef  boost::shared_ptr<buffer> buffer_ptr;

typedef  boost::weak_ptr<buffer> buffer_weak_ptr;

typedef  std::list<buffer_weak_ptr>  weak_buffer_pool;

typedef  boost::shared_ptr<weak_buffer_pool> weak_buffer_pool_ptr;


typedef  std::list<buffer_ptr>  buffer_pool;

typedef  boost::shared_ptr<buffer_pool> buffer_pool_ptr;


#pragma pack(push,1) //请务必一字节对齐

typedef  boost::function<void (long timerID )> timer_callback_func;


struct SocketState
{
	long sockID;

	long currentReadTimeWithms;			//!<当前socket完成一次读操作所需要的毫秒数
	long currentWriteTimeWithms;		//!<当前socket完成一次写操作所需要的毫秒数
	long currentConnectTimeWithms;		//!<当前socket完成一次连接操作所需要的毫秒数	
	long currentDNSResolveTimeWithms;	//!<当前socket完成一次域名解析操作所需要的毫秒数ms;


	unsigned long long finishedReadBytes;	//!<当前socket完成多少字节读操作
	unsigned long long finishedWriteBytes;	//!<当前socket完成多少字节写操作

	long pendingSyncReadNum;	//!<当前socket有多少次同步读未完成
	long pendingASyncReadNum;	//!<当前socket有多少次异步读未完成
	unsigned long long pendingReadBytes;	//!<当前socket有多少字节读未完成
	long pendingWriteNum;					//!<当前socket有多少次异步读未完成
	unsigned long long pendingWriteBytes;	//!<当前socket有多少字节写未完成

};

struct ModState		//!<模块状态
{
	long totalSockNum;				//!<socket总数
	long totalTimerNum;				//!<定时器总数
	unsigned long long totalFinishedReadBytes;	//!<当前模块完成多少字节读操作
	unsigned long long totalFinishedWriteBytes;	//!<当前模块完成多少字节写操作

};


class LIBASIO_API IAdapterHandler
{
public:
	virtual ~IAdapterHandler();
};

typedef boost::shared_ptr<IAdapterHandler>	IAdapterHandlerPtr;
typedef boost::weak_ptr<IAdapterHandler>	IAdapterHandlerWeakPtr;



class LIBASIO_API ISockHandler
{
public:

	friend class SockHandlerAdapter;
	friend class AsioFacadeImpl;

	ISockHandler()
	{

	}

	virtual ~ISockHandler(){};

	virtual void on_accept(long connID, const char* errMsg){};

	virtual void on_connect(long connID,const char* errMsg){};

	virtual void on_read(long connID,const char* errMsg,std::size_t bytes_transferred, buffer_ptr buffer,long read_seq){};

	virtual void on_write(long connID,const char* errMsg,std::size_t bytes_transferred,	buffer_ptr bufferLeft,long write_seq){};


	virtual void on_error(long connID, const char* errMsg){};

	virtual void on_warning(long connID,const char* warningMsg){};

	virtual void on_close(long connID, const char* errMsg,const char* msgReason){};

	virtual void on_cancel(long connID, buffer_ptr cancel_buffer,long write_seq){};

	void schedule_at_once(long connID,timer_callback_func callback);
	void Schedule_stop(long timerID);

	// 同步读 
	void SetSyncRead(long connID,bool bSync = true);
	int Recv(long connID,int bufSize,char *read_buf,long& read_seq,boost::system::error_code& ec);

	// 异步读 
	bool Recv(long connID,int bufSize = 4096) ;

	bool Send(long connID, const char * pPacket, const int intLen);
	bool Send(long connID, buffer_ptr buf) ;
	bool Send(long connID, buffer_pool_ptr buffers) ;
	bool Send(long connID, const std::string & strBuffer) ;
	void Close(long connID);

	 bool State(long connID,SocketState& state);
	 bool State(ModState& state);

	 void Cancel(long connID);

	 IAdapterHandlerPtr GetAdapterHandler()
	 {
		 return m_pAdapterHandler.lock();
	 }


private:
	 IAdapterHandlerWeakPtr m_pAdapterHandler;

};

typedef boost::shared_ptr<ISockHandler> ISockHandlerPtr;
typedef boost::weak_ptr<ISockHandler>	ISockHandlerWeakPtr;
#pragma pack(pop)


class LIBASIO_API IAsioFacade
{
public:
	IAsioFacade(){};
	virtual ~IAsioFacade(){};

	virtual bool Configure( std::map<std::string,std::map<std::string,std::string> >& cmdParam) = 0;
	virtual long Connect(const std::string &ip,const std::string &port) = 0;
	virtual long Close(long connID) = 0;
	virtual void SetSyncRead(long connID,bool bSync = true) = 0;
	virtual int Recv(long connID,int bufSize,char *read_buf,long& read_seq,boost::system::error_code& ec) = 0;
	virtual bool SockState(long connID,SocketState& state) = 0;
	virtual bool ModuleState(ModState& state) = 0;

#if 0
	virtual bool Recv(long connID,int bufSize = 4096) = 0;
	virtual bool Send(long connID, const char * pPacket, const int intLen) = 0;
	virtual bool Send(long connID, buffer_ptr buf) = 0;
	virtual bool Send(long connID, buffer_pool_ptr buffers) = 0;
	virtual bool Send(long connID, const std::string & strBuffer) = 0;
#endif

	virtual void Run() = 0;
	virtual void AttachHandler(ISockHandlerPtr pHandler) = 0;
	virtual void DetachHandler(ISockHandlerPtr pHandler) = 0;

	virtual long Schedule_from_now(timer_callback_func callback, boost::posix_time::time_duration timeDuration,int executeCount = 1) = 0;
	virtual long Schedule_at(timer_callback_func callback, boost::posix_time::ptime timeStamp) = 0;
	virtual long Schedule_at_once(timer_callback_func callback) = 0;
	virtual void Schedule_stop(long timerID) = 0;

	


};




enum FacadeType
{
	ServerFacade,
	ClientFacade,
};

extern "C"
{

LIBASIO_API IAsioFacade& AsioFacadeGetInstance(FacadeType facadeType, long ID = 1000);

LIBASIO_API void AsioFacadeCleanUp();

};










