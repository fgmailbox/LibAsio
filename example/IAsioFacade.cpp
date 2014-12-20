#include "IAsioFacade.h"
#include "Singleton2.h"
#include "Subscribe.h"
#include "Server.hpp"
//#include "log.h"


using http::server::facade;
using http::server::subscribe;
using http::server::connection;
using http::server::timer;
using http::server::connection_weak_ptr;
using http::server::connection_ptr;
//using http::server::connection::s_min_read_buf_size;

using http::server::timer_ptr;

class SockHandlerAdapter;

/********************************************************************************/
/*					头文件定义													*/
/********************************************************************************/


class AsioFacadeImpl
	:public IAsioFacade
{
public:
	AsioFacadeImpl();
	~AsioFacadeImpl();
	bool Configure( std::map<std::string,std::map<std::string,std::string> >& cmdParam) ;
	long Connect(const std::string &ip,const std::string &port);
	long Close(long connID);
	bool Recv(long connID,int bufSize = 4096);
	int Recv(long connID,int bufSize,char *read_buf,long& read_seq, boost::system::error_code& ec);
	void SetSyncRead(long connID,bool bSync);
	bool SockState(long connID,SocketState& state);
	bool ModuleState(ModState& state);

	bool Send(long connID, const char * pPacket, const int intLen);
	bool Send(long connID, buffer_ptr buf);
	bool Send(long connID, buffer_pool_ptr buffers);
	bool Send(long connID, const std::string & strBuffer);
	void Cancel(long connID);


	void AttachHandler(ISockHandlerPtr pHandler);
	void DetachHandler(ISockHandlerPtr pHandler);
	void Run();

	long Schedule_from_now(timer_callback_func callback, boost::posix_time::time_duration timeDuration,int executeCount );

	long Schedule_at(timer_callback_func callback, boost::posix_time::ptime timeStamp);

	long Schedule_at_once(timer_callback_func callback);

	void Schedule_stop(long timerID);



	void	setFacadeType(FacadeType facadeType);
	facade	&GetFacade();
private:
	FacadeType m_facadeType;
	std::map<ISockHandler *,boost::shared_ptr<SockHandlerAdapter> > m_handlers;
	std::map<long, AtomicGuard<>::AtomicGuardPtr>					m_syncReadConfigGuard;
	boost::detail::lightweight_mutex								lock;

};
/********************************************************************************/
/*					回调接口适配器												*/
/********************************************************************************/

IAdapterHandler::~IAdapterHandler()
{

}



class SockHandlerAdapter
	:public IAdapterHandler,
	 public subscribe,
	 public boost::enable_shared_from_this<SockHandlerAdapter>
{

public:
	friend class ISockHandler;
	friend class AsioFacadeImpl;

	SockHandlerAdapter(facade&context, AsioFacadeImpl &facadeImpl,ISockHandlerPtr pHandler)
		:m_pHandler(pHandler),
		 m_context(context),
		 m_facadeImpl(facadeImpl)
	{
	}

	~SockHandlerAdapter()
	{
		//ISockHandlerPtr pHandler = GetHandler();
		//if(pHandler)
		//	pHandler->m_pAdapterHandler = NULL;
	}

	std::string get_error_msg(const boost::system::error_code& e)
	{
		std::string msg;
		if(e)
			msg = e.message();
		return msg;
	}

	virtual void on_accept(connection_weak_ptr conn,
		const boost::system::error_code& e)
	{
		boost::shared_ptr<SockHandlerAdapter> lockThis = shared_from_this();

		GetHandler()->m_pAdapterHandler = lockThis;

		connection_ptr conn_lock = conn.lock();

		if(GetHandler() && conn_lock)
		{

			GetHandler()->on_accept(conn_lock->getObjectID(),get_error_msg(e).c_str());
			GetHandler()->Recv(conn_lock->getObjectID(),http::server::connection::s_min_read_buf_size);
		}
		
	}


	virtual void on_connect(connection_weak_ptr conn,
		const boost::system::error_code& e)
	{
		boost::shared_ptr<SockHandlerAdapter> lockThis = shared_from_this();

		GetHandler()->m_pAdapterHandler = lockThis;

		connection_ptr conn_lock = conn.lock();

		if(GetHandler() && conn_lock)
		{

			GetHandler()->on_connect(conn_lock->getObjectID(),get_error_msg(e).c_str());
			// 将发起接收的控制权交由应用层完成 
			#if 0
			GetHandler()->Recv(conn_lock->getObjectID(),http::server::connection::s_min_read_buf_size);
			#endif
		}


	}



	virtual void on_read(connection_weak_ptr conn,
		const boost::system::error_code& e,
		std::size_t bytes_transferred,
		buffer_ptr buffer,
		long read_seq)
	{
		boost::shared_ptr<SockHandlerAdapter> lockThis = shared_from_this();

		connection_ptr conn_lock = conn.lock();

		if(GetHandler() && conn_lock)
		{

			GetHandler()->on_read(conn_lock->getObjectID(),get_error_msg(e).c_str(),bytes_transferred,buffer,read_seq);
		}

	}

	virtual void on_write(connection_weak_ptr conn,
		const boost::system::error_code& e,
		std::size_t bytes_transferred,
		buffer_ptr bufferLeft,
		long write_seq) 
	{
		boost::shared_ptr<SockHandlerAdapter> lockThis = shared_from_this();

		connection_ptr conn_lock = conn.lock();

		if(GetHandler() && conn_lock)
		{

			GetHandler()->on_write(conn_lock->getObjectID(),get_error_msg(e).c_str(),bytes_transferred,bufferLeft,write_seq);
			GetHandler()->Recv(conn_lock->getObjectID(),http::server::connection::s_min_read_buf_size);

		}

	}
	virtual void on_cancel(connection_weak_ptr conn, buffer_ptr cancel_buffer,long write_seq) 
	{
		boost::shared_ptr<SockHandlerAdapter> lockThis = shared_from_this();
		
		connection_ptr conn_lock = conn.lock();

		if(GetHandler() && conn_lock)
		{
			GetHandler()->on_cancel(conn_lock->getObjectID(),cancel_buffer,write_seq);
		}

	}


	virtual void on_warning(connection_weak_ptr conn,const std::string & warningMsg) 
	{
		boost::shared_ptr<SockHandlerAdapter> lockThis = shared_from_this();

		connection_ptr conn_lock = conn.lock();

		if(GetHandler() && conn_lock)
		{

			GetHandler()->on_warning(conn_lock->getObjectID(),warningMsg.c_str());
		}

	}

	virtual void on_error(connection_weak_ptr conn, const boost::system::error_code& e) 
	{
		boost::shared_ptr<SockHandlerAdapter> lockThis = shared_from_this();
		
		connection_ptr conn_lock = conn.lock();

		if(GetHandler() && conn_lock)
		{

			GetHandler()->on_error(conn_lock->getObjectID(),get_error_msg(e).c_str());
		}
	}

	virtual void on_close(long connID, const boost::system::error_code& e,std::string reason)
	{
		boost::shared_ptr<SockHandlerAdapter> lockThis = shared_from_this();

		if(GetHandler())
		{
			GetHandler()->SetSyncRead(connID,false);
			GetHandler()->on_close(connID,get_error_msg(e).c_str(),reason.c_str());
		}

	}

	ISockHandlerPtr GetHandler()
	{
		return m_pHandler.lock();
	}

private:
	ISockHandlerWeakPtr m_pHandler;
	facade		 &m_context;
	AsioFacadeImpl &m_facadeImpl;

};




/********************************************************************************/
/*					接口类														*/
/********************************************************************************/

IAsioFacade& AsioFacadeGetInstance(FacadeType facadeType,long ID )
{
	switch(facadeType)
	{
	case ClientFacade:
		Singleton2<AsioFacadeImpl,1000>::instance().setFacadeType(facadeType);
		return Singleton2<AsioFacadeImpl,1000>::instance();
	case ServerFacade:
		Singleton2<AsioFacadeImpl,2000>::instance().setFacadeType(facadeType);
		return Singleton2<AsioFacadeImpl,2000>::instance();

	}
}


void AsioFacadeCleanUp()
{
	SingletonBase::SetStopAll();
	SingletonBase::Release();
}


void ISockHandler::Close(long connID)
{
	if(!m_pAdapterHandler.expired())
	{
		boost::shared_ptr<SockHandlerAdapter> pAdapter = boost::dynamic_pointer_cast<SockHandlerAdapter>(GetAdapterHandler());
		if(pAdapter)
			pAdapter->m_facadeImpl.Close(connID);
	}

}
void ISockHandler::Schedule_stop(long timerID)
{
	if(!m_pAdapterHandler.expired())
	{
		boost::shared_ptr<SockHandlerAdapter> pAdapter = boost::dynamic_pointer_cast<SockHandlerAdapter>(GetAdapterHandler());
		if(pAdapter)
		{
			timer_ptr timer = pAdapter->m_context.get_timer(timerID);
			timer->stop();
		}
	}

}


void ISockHandler::schedule_at_once(long connID,timer_callback_func callback)
{
	if(!m_pAdapterHandler.expired())
	{
		boost::shared_ptr<SockHandlerAdapter> pAdapter = boost::dynamic_pointer_cast<SockHandlerAdapter>(GetAdapterHandler());
		if(pAdapter)
		{
			connection_ptr conn = pAdapter->m_context.get_connection(connID);
			connection::async_callback  func = boost::bind(callback,conn->getObjectID());
			conn->schedule_defer_func(func);
		}
	}
}


bool ISockHandler::Recv(long connID,int bufSize)
{
	if(!m_pAdapterHandler.expired())
	{
		boost::shared_ptr<SockHandlerAdapter> pAdapter = boost::dynamic_pointer_cast<SockHandlerAdapter>(GetAdapterHandler());
		if(pAdapter)
		{
			pAdapter->m_facadeImpl.Recv(connID,bufSize);
			return true;
		}
		else
			return false;
	}
	else
		return false;

}

int ISockHandler::Recv(long connID,int bufSize,char *read_buf, long& read_seq,boost::system::error_code& ec)
{
	if(!m_pAdapterHandler.expired())
	{
		boost::shared_ptr<SockHandlerAdapter> pAdapter = boost::dynamic_pointer_cast<SockHandlerAdapter>(GetAdapterHandler());
		if(pAdapter)
		{
			return pAdapter->m_facadeImpl.Recv(connID,bufSize,read_buf,read_seq,ec);
		}
		else
			return -1;
	}
	else
		return -1;

}

void ISockHandler::SetSyncRead(long connID,bool bSync)
{
	if(!m_pAdapterHandler.expired())
	{
		boost::shared_ptr<SockHandlerAdapter> pAdapter = boost::dynamic_pointer_cast<SockHandlerAdapter>(GetAdapterHandler());
		if(pAdapter)
		{
			pAdapter->m_facadeImpl.SetSyncRead(connID,bSync);
		}
	}

}



bool ISockHandler::Send(long connID, const char * pPacket, const int intLen)
{
	if(!m_pAdapterHandler.expired())
	{
		boost::shared_ptr<SockHandlerAdapter> pAdapter = boost::dynamic_pointer_cast<SockHandlerAdapter>(GetAdapterHandler());
		if(pAdapter)
		{
			pAdapter->m_facadeImpl.Send(connID,pPacket,intLen);
			return true;
		}
		else
			return false;
	}
	else
		return false;

}
bool ISockHandler::Send(long connID, buffer_ptr buf)
{
	if(!m_pAdapterHandler.expired())
	{
		boost::shared_ptr<SockHandlerAdapter> pAdapter = boost::dynamic_pointer_cast<SockHandlerAdapter>(GetAdapterHandler());
		if(pAdapter)
		{
			pAdapter->m_facadeImpl.Send(connID,buf);
			return true;
		}
		else
			return false;
	}
	else
		return false;

}
bool ISockHandler::Send(long connID, buffer_pool_ptr buffers)
{
	if(!m_pAdapterHandler.expired())
	{
		boost::shared_ptr<SockHandlerAdapter> pAdapter = boost::dynamic_pointer_cast<SockHandlerAdapter>(GetAdapterHandler());
		if(pAdapter)
		{
			pAdapter->m_facadeImpl.Send(connID,buffers);
			return true;
		}
		else
			return false;
	}
	else
		return false;

}
bool ISockHandler::Send(long connID, const std::string & strBuffer)
{
	if(!m_pAdapterHandler.expired())
	{
		boost::shared_ptr<SockHandlerAdapter> pAdapter = boost::dynamic_pointer_cast<SockHandlerAdapter>(GetAdapterHandler());
		if(pAdapter)
		{
			pAdapter->m_facadeImpl.Send(connID,strBuffer);
			return true;
		}
		else
			return false;
	}
	else
		return false;

}

void ISockHandler::Cancel(long connID)
{
	if(!m_pAdapterHandler.expired())
	{
		boost::shared_ptr<SockHandlerAdapter> pAdapter = boost::dynamic_pointer_cast<SockHandlerAdapter>(GetAdapterHandler());
		if(pAdapter)
		{
			pAdapter->m_facadeImpl.Cancel(connID);
		}
	}

}


bool ISockHandler::State(long connID,SocketState& state)
{
	if(!m_pAdapterHandler.expired())
	{
		boost::shared_ptr<SockHandlerAdapter> pAdapter = boost::dynamic_pointer_cast<SockHandlerAdapter>(GetAdapterHandler());
		if(pAdapter)
		{
			return pAdapter->m_facadeImpl.SockState(connID,state);
		}
		else
			return false;
	}
	else
		return false;

}

bool ISockHandler::State(ModState& state)
{
	if(!m_pAdapterHandler.expired())
	{
		boost::shared_ptr<SockHandlerAdapter> pAdapter = boost::dynamic_pointer_cast<SockHandlerAdapter>(GetAdapterHandler());
		if(pAdapter)
		{
			return pAdapter->m_facadeImpl.ModuleState(state);
		}
		else
			return false;
	}
	else
		return false;

}

/********************************************************************************/
/*					实现类														*/
/********************************************************************************/


AsioFacadeImpl::AsioFacadeImpl()
:m_facadeType(ClientFacade)
{


}
AsioFacadeImpl::~AsioFacadeImpl()
{

}

void AsioFacadeImpl::setFacadeType(FacadeType facadeType)
{
	m_facadeType = facadeType;
}


facade &AsioFacadeImpl::GetFacade()
{
	if(m_facadeType == ClientFacade)
		return Singleton2<clientFacade,1000>::instance();
	else
		return Singleton2<serverFacade,2000>::instance();
}


bool AsioFacadeImpl::Configure(std::map<std::string,std::map<std::string,std::string> >& cmdParam)
{

	//Singleton2<logger,3000>::instance().s_log_source |= logger::E_LOG_FUNC;

	//Singleton2<logger,3000>::instance().Configure(cmdParam,logger::E_LOG_FUNC);

	return GetFacade().Configure(cmdParam);

}
long AsioFacadeImpl::Connect(const std::string &ip,const std::string &port)
{
	connection_ptr conn = GetFacade().alloc_connection();
	conn->connect(ip,port);
	return conn ? conn->getObjectID():0;
}

long AsioFacadeImpl::Close(long connID)
{
	connection_ptr conn = GetFacade().get_connection(connID);
	if(conn)
	{
		conn->stop();
		return conn->getObjectID();
	}
	else
		return 0;
}

void AsioFacadeImpl::AttachHandler(ISockHandlerPtr pHandler)
{
	//BOOST_ASSERT(_HEAPOK == _heapchk());
	{
		boost::detail::lightweight_mutex::scoped_lock	guard(lock);
		m_handlers[pHandler.get()] = boost::make_shared<SockHandlerAdapter>(boost::ref(GetFacade()),boost::ref(*this),pHandler);
		//BOOST_ASSERT(_HEAPOK == _heapchk());

		GetFacade().attach_subscriber(m_handlers[pHandler.get()]);
	}
	//BOOST_ASSERT(_HEAPOK == _heapchk());

}

void AsioFacadeImpl::DetachHandler(ISockHandlerPtr pHandler)
{

	{
		boost::detail::lightweight_mutex::scoped_lock	guard(lock);

		if(m_handlers.count(pHandler.get()))
		{
			GetFacade().detach_subscriber(m_handlers[pHandler.get()]);

			//m_handlers[pHandler.get()]->GetHandler()->m_pAdapterHandler = NULL;

			m_handlers.erase(pHandler.get());
		}
		else 
		{
			//BOOST_ASSERT(false);	
		}

	}
}

void AsioFacadeImpl::Run()
{
	GetFacade().run();
}

long AsioFacadeImpl::Schedule_from_now(timer_callback_func callback, boost::posix_time::time_duration timeDuration,int executeCount )
{
	timer_ptr timer = GetFacade().alloc_timer();
	timer->start_from_now(callback,timeDuration,executeCount);
	return timer->getObjectID();
}

long AsioFacadeImpl::Schedule_at(timer_callback_func callback, boost::posix_time::ptime timeStamp)
{
	timer_ptr timer = GetFacade().alloc_timer();
	timer->start_at(callback,timeStamp);
	return timer->getObjectID();

}
long AsioFacadeImpl::Schedule_at_once(timer_callback_func callback)
{

#ifdef _DEBUG
	int len = sizeof(http::server::timer);
	int len1 = sizeof(boost::enable_shared_from_this<http::server::timer>);
	int len2 = sizeof(ObjectHandle<http::server::timer>);
	int len3 = sizeof(boost::noncopyable);
	int len4 = offsetof(http::server::timer,bStoped_);
	int len5 = offsetof(http::server::timer,strand_);
	int len5_1 = offsetof(http::server::timer,condition_);
	int len5_2 = offsetof(http::server::timer,timer_);
	int len5_3 = sizeof(boost::asio::deadline_timer);
	int len6 = offsetof(http::server::timer,fire_timeout_);

#endif

	timer_ptr timer = GetFacade().alloc_timer();
	timer->start_at_once(callback);
	//BOOST_ASSERT(_HEAPOK == _heapchk());
	return timer->getObjectID();
}

void AsioFacadeImpl::Schedule_stop(long timerID)
{
	timer_ptr timer = GetFacade().get_timer(timerID);
	if(timer)
		timer->stop();
}


bool AsioFacadeImpl::Send(long connID, const char * pPacket, const int intLen)
{
	connection_ptr conn = GetFacade().get_connection(connID);
	if(conn)
	{

		conn->write(boost::make_shared<http::server::buffer>(pPacket, pPacket + intLen));
		return true;
	}
	else
	{
		//LOG(debug,"关闭不存在的套接字");
		return false;
	}

}

bool AsioFacadeImpl::Send(long connID, buffer_ptr buf)
{
	connection_ptr conn = GetFacade().get_connection(connID);
	if(conn)
	{

		conn->write(buf);
		return true;
	}
	else
	{
		//LOG(debug,"关闭不存在的套接字");
		return false;
	}

}


bool AsioFacadeImpl::Send(long connID, buffer_pool_ptr buffers)
{
	connection_ptr conn = GetFacade().get_connection(connID);
	if(conn)
	{

		conn->multi_write(buffers);
			return true;
	}
	else
	{
		//LOG(debug,"关闭不存在的套接字");
		return false;
	}

}

bool AsioFacadeImpl::Send(long connID, const std::string & strBuffer)
{
	connection_ptr conn = GetFacade().get_connection(connID);
	if(conn)
	{

		conn->write(boost::make_shared<http::server::buffer>(strBuffer.begin(), strBuffer.end()));
		return true;
	}
	else
	{
		//LOG(debug,"关闭不存在的套接字");
		return false;
	}

}

 bool AsioFacadeImpl::Recv(long connID,int bufSize)
 {
	 connection_ptr conn = GetFacade().get_connection(connID);
	 if(conn)
	 {

		 conn->read(bufSize);
		 return true;
	 }
	 else
	 {
		// LOG(debug,"关闭不存在的套接字");
		 return false;
	 }

 }

 int AsioFacadeImpl::Recv(long connID,int bufSize,char *read_buf,long& read_seq,boost::system::error_code& ec)
 {
	 connection_ptr conn = GetFacade().get_connection(connID);
	 if(conn)
	 {
		return conn->sync_read(bufSize,read_buf,read_seq,ec);
	 }
	 else
	 {
		 //LOG(debug,"关闭不存在的套接字");
		 return -1;
	 }

 }

 void AsioFacadeImpl::SetSyncRead(long connID,bool bSync)
 {
	 connection_ptr conn = GetFacade().get_connection(connID);
	 if(conn)
	 {
		 boost::detail::lightweight_mutex::scoped_lock	guard(lock);
		 if(bSync)
		 {
			// 同步读 
			m_syncReadConfigGuard[conn->getObjectID()] = conn->enable_sync_read();
		 }
		 else
		 {
			// 异步读 
			m_syncReadConfigGuard.erase(conn->getObjectID());
			conn->read();
		 }
	 }

 }

 /***************************************************************/
/*				Socket性能监控									*/
 /***************************************************************/
bool AsioFacadeImpl::SockState(long connID,SocketState& state)
{
	connection_ptr conn = GetFacade().get_connection(connID);
	if(conn)
	{

			state.sockID				= conn->getObjectID();
			state.currentReadTimeWithms	= conn->currentReadTimeWithms;
			state.currentWriteTimeWithms= conn->currentWriteTimeWithms;

			state.currentConnectTimeWithms= conn->currentConnectTimeWithms;	
			state.currentDNSResolveTimeWithms= conn->currentDNSResolveTimeWithms;



			state.finishedReadBytes		= /*(unsigned long long)*/conn->current_total_read_bytes_;
			state.finishedWriteBytes	= /*(unsigned long long)*/conn->current_total_write_bytes_;
			state.pendingSyncReadNum	= *(conn->unfinished_sync_read_count_);
			state.pendingASyncReadNum	= *(conn->unfinished_read_count_);
			state.pendingReadBytes		=/* (unsigned long long)*/*(conn->unfinished_read_bytes_);
			state.pendingWriteNum		= *(conn->unfinished_write_count_);
			state.pendingWriteBytes		= /*(unsigned long long)*/*(conn->unfinished_write_bytes_);

			return true;

	}
	else
		return false;


}

bool AsioFacadeImpl::ModuleState(ModState& state)
{
	state.totalSockNum				=	connection::getObjectCount();				//!<socket总数
	state.totalTimerNum				=	timer::getObjectCount();				//!<定时器总数
	state.totalFinishedReadBytes	=	(long long)connection::s_system_total_read_bytes_;	//!<当前模块完成多少字节读操作
	state.totalFinishedWriteBytes	=	(long long)connection::s_system_total_write_bytes_;	//!<当前模块完成多少字节写操作
	return true;
}

void AsioFacadeImpl::Cancel(long connID)
{
	connection_ptr conn = GetFacade().get_connection(connID);
	if(conn)
	{
		conn->cancel();
	}
}