#pragma once
//#define BOOST_THREAD_USE_DLL

#include <list>
#include <vector>
#include <boost/detail/lightweight_mutex.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/thread.hpp>
#include "connection.hpp"
#include "ObjectHandle.h"
#include "resource_manager.h"
//#include "log.h"
#include "Singleton2.h"


//class CMarket;

class IConnectionLogic
{
public:
	virtual void on_data_notify_logic(long connID, http::server::buffer_pool_ptr buffers) = 0;
	virtual void on_connect_logic(long connID) = 0;
	virtual void on_close_logic(long connID) = 0;
	virtual void close_socket(long connID) = 0;
	virtual	void on_reconnect_logic(long connID) = 0;
	virtual bool SendPacket(long connID, const char * pPacket, const int intLen) = 0;
	virtual bool SendPacket(long connID, http::server::buffer_ptr buf) = 0;
	virtual bool SendPackets(long connID,http::server::buffer_pool_ptr buffers) = 0;

};

template<typename Derived>
class ConnectionHandler
	: public boost::enable_shared_from_this<Derived>,
	public ObjectHandle<Derived>
{
public:
	// 资源管理 
	class ConnectionHandlerPool
		: public resource_manager<Derived>,
		private boost::noncopyable
	{
	public:
		void start(boost::shared_ptr< Derived> c)
		{
			add(c);
			c->start();
		};
	};


	// 通讯句柄
	http::server::connection_weak_ptr	m_conn;
	long		m_intConID;
	//long		m_currentCancelWriteSeq;
	volatile    bool	m_blnIsConnect;
	static int _const_max_send_buf_size;

protected:

	// 读完成事件on_read的重入计数器 
	AtomicGuard<>::AtomicTypePtr		renter_read_count_;
	// 写完成事件on_write的重入计数器 
	AtomicGuard<>::AtomicTypePtr		renter_write_count_;

	// 以下通通为通讯机制逻辑
	//boost::recursive_mutex				m_MutexSend;
	boost::detail::lightweight_mutex	m_MutexSend;
	std::list<http::server::buffer_ptr>	m_sendbuf;
	//boost::recursive_mutex				m_MutexCancel;
	//std::map<long, http::server::buffer_ptr> m_canceled_sendbuf;

	boost::detail::lightweight_mutex	m_MutexRecv;
	http::server::buffer_ptr			m_recvbuf;
	CTime								m_refreshTime;
public:
	ConnectionHandler();
	virtual ~ConnectionHandler(void);

	void attach_connection(http::server::connection_ptr conn);

	void detach_connection(long connID);

	http::server::connection_ptr get_connection();


	// 接口回调函数
	virtual void on_read(http::server::buffer_ptr buffer,long read_seq);
	// 接口回调函数
	virtual void on_write(http::server::buffer_ptr buffer = http::server::buffer_ptr(),long write_seq = 0);
	// 接口回调函数
	virtual void on_connect();

	virtual void on_accept();


	virtual void on_cancel(http::server::buffer_ptr cancel_buffer,long write_seq);


	virtual bool SendPacket(const char * pPacket, const int intLen);

	// 为了节省空间发送共享包 
	bool SendSharedPacket(http::server::buffer_ptr buffer);

	bool SendSharedPackets(http::server::buffer_pool_ptr buffers);


protected:


	std::pair<int,int> get_send_buf_size();

	//int get_cancel_buf_size();


	long do_send( long maxSend = 80 * 1024);

	void send(http::server::buffer_ptr buffer);

	void send(http::server::buffer_pool_ptr buffers);

	//long send_cancel_buffer();

	virtual void start()	= 0;

	virtual void stop();

};


template<typename Derived>
void ConnectionHandler<Derived>::on_accept()
{
	m_blnIsConnect = true;
}


template<typename Derived>
void ConnectionHandler<Derived>::on_connect()
{
	m_blnIsConnect = true;
}

template<typename Derived>
http::server::connection_ptr ConnectionHandler<Derived>::get_connection()
{
	http::server::connection_ptr conn = m_conn.lock();
	return conn;
}

template<typename Derived>
void ConnectionHandler<Derived>::attach_connection(http::server::connection_ptr conn)
{
	if(conn)
	{
		if(!m_recvbuf)
		{
			m_recvbuf = boost::make_shared<http::server::buffer>();
			m_recvbuf->reserve(http::server::connection::s_min_read_buf_size);
		}
		//m_blnIsConnect = true;
		m_conn = conn;


		m_intConID = conn->getObjectID();
	}
}

template<typename Derived>
void ConnectionHandler<Derived>::detach_connection(long connID)
{
	m_blnIsConnect = false;
	{
		boost::detail::lightweight_mutex::scoped_lock lock(m_MutexRecv);
		m_recvbuf->clear();
	}

	{
		boost::detail::lightweight_mutex::scoped_lock lock(m_MutexSend);
		m_sendbuf.clear();
	}
}

template<typename Derived>
void ConnectionHandler<Derived>::stop()
{
	m_blnIsConnect = false;
}


template<typename Derived>
ConnectionHandler<Derived>::ConnectionHandler()
	:m_intConID(0),
	 //m_currentCancelWriteSeq(0),
	 m_blnIsConnect(false),
	 renter_read_count_(boost::make_shared<AtomicGuard<>::AtomicType>(0)),
	 renter_write_count_(boost::make_shared<AtomicGuard<>::AtomicType>(0)),
	 m_recvbuf(boost::make_shared<http::server::buffer>())
{
	m_refreshTime = CTime::GetCurrentTime();
	m_recvbuf->reserve(http::server::connection::s_min_read_buf_size);
}

template<typename Derived>
ConnectionHandler<Derived>::~ConnectionHandler()
{
}

template<typename Derived>
void ConnectionHandler<Derived>::send(http::server::buffer_ptr buffer)
{
	http::server::connection_ptr conn = get_connection();
	if(conn)
		conn->write(buffer);
}

template<typename Derived>
void ConnectionHandler<Derived>::send(http::server::buffer_pool_ptr buffers)
{
	http::server::connection_ptr conn = get_connection();
	if(conn)
		conn->multi_write(buffers);
}



template<typename Derived>
void ConnectionHandler<Derived>::on_cancel(http::server::buffer_ptr cancel_buffer,long write_seq)
{
}



template<typename Derived>
void ConnectionHandler<Derived>::on_read(http::server::buffer_ptr buffer,long read_seq)
{
	if(!m_blnIsConnect)
		return;

	if (buffer && buffer->size() > 0)
	{
		
		// 读到数据放入缓存
		boost::detail::lightweight_mutex::scoped_lock lock(m_MutexRecv);
		AtomicGuard<> read_reenter_guard(renter_read_count_);
		if( read_reenter_guard > 1)
		{
			LOG(informational,"当前线程重入对象 [%d] 的读完成事件 Total [%d] left", getObjectID(),getObjectCount());
		}
		int intOldSize = m_recvbuf->size();
		const int _const_len = 1024*1024;
		m_recvbuf->resize(intOldSize+_const_len);
		char * pPos = &*(m_recvbuf->begin() + intOldSize);
		int intRecvLen = buffer->size();//recv(m_socket, pPos, _const_len, 0);
		m_recvbuf->insert(m_recvbuf->begin()+intOldSize, &*(buffer->begin()),&*(buffer->begin())+intRecvLen);
		if(intRecvLen > 0)
		{
			m_recvbuf->resize(intOldSize+intRecvLen);				
		}


	}

	{
		http::server::connection_ptr conn = get_connection();
		boost::detail::lightweight_mutex::scoped_lock lock(m_MutexSend);
		if(conn && conn->get_unfinished_write_count() == 0)
			do_send();
	}



}

template<typename Derived>
void ConnectionHandler<Derived>::on_write(http::server::buffer_ptr buffer,long write_seq)
{

	http::server::connection_ptr conn = get_connection();
	if(!conn) 
		return;


	try
	{

		if(buffer && buffer->size())
		{
			// 数据包写不完，丢包  
			LOG3(20*1000*1000,warning,"当前[%d] 数据包有[%d]个字节写不完，丢包，怎么办? 总共有连接[%d]个 这是[%d]次写完成", 
							  getObjectID(),buffer->size(), getObjectCount(),write_seq);

			long unfinished_bytes = buffer->size();

			boost::detail::lightweight_mutex::scoped_lock lock(m_MutexSend);
			if(write_seq == conn->write_seq_ 
				&&	conn->get_unfinished_write_count() < 2)
			{
				LOG3(15*1000*1000,warning,"数据包写不完,重发");

				m_sendbuf.push_front(buffer);


#if 1
				srand(time(NULL));
				long delayTime = (10 + (unfinished_bytes / 20) * rand()) % ((unfinished_bytes+20)/2);
				LOG2(100,warning,"延迟[%d]毫秒写数据 套接字：[%d] 套接字总数 [%d]",
					delayTime,
					conn->getObjectID(),
					conn->getObjectCount());

				conn->facade_.alloc_timer()
					->start_from_now(boost::bind(&ConnectionHandler::do_send,
					shared_from_this(),unfinished_bytes < 4096 ? 4096 : unfinished_bytes
					),
					boost::posix_time::milliseconds(delayTime));
#else
				do_send();
#endif

			}
			else
			{
				LOG3(15*1000*1000,warning,"数据包写不完,放弃并关闭");
				stop();
			}

		}
		else
		{
			if(m_sendbuf.size() > 0)
			{
				{
					// 清理空的buffer 
					boost::detail::lightweight_mutex::scoped_lock lock(m_MutexSend);

					while(m_sendbuf.size())
					{
						if(m_sendbuf.front()->size() == 0)
						{
							m_sendbuf.erase(m_sendbuf.begin());
							LOG(error, "删除空包");
						}
						else
						{
							break;
						}
					}
				}

				if(conn->get_unfinished_write_count() < 2)
				{
					do_send();
				}
			}

		}
	}
	catch(...)
	{
		// 异常捕捉 
	}


}

template<typename Derived>
bool ConnectionHandler<Derived>::SendSharedPackets(http::server::buffer_pool_ptr buffers)
{
	if(buffers && buffers->size())
	{
		{
			boost::detail::lightweight_mutex::scoped_lock lock(m_MutexSend);
			m_sendbuf.insert(m_sendbuf.end(),buffers->begin(),buffers->end());

		}

		{
			http::server::connection_ptr conn = get_connection();
			boost::detail::lightweight_mutex::scoped_lock lock(m_MutexSend);
			if(conn && conn->get_unfinished_write_count() == 0)
				do_send();
		
		}

	}
	else
		return false;
}

template<typename Derived>
long ConnectionHandler<Derived>::do_send( long maxSend)
{
	if(!m_blnIsConnect)
		return 0;

	long ncurrent_send_total = 0;

	typename Derived::ConnectionHandlerPtr lockThis = shared_from_this();

	http::server::connection_ptr conn = get_connection();
	if(conn)
	{

		{
			boost::detail::lightweight_mutex::scoped_lock lock(m_MutexSend);

			if( m_sendbuf.size() > 1 
#if 0
				// 为了进行流量控制 
				&& conn->write_seq_ > 4 
#endif
				)
			{
				http::server::buffer_pool_ptr buffers = 
					boost::make_shared<http::server::buffer_pool>();

				
				//BOOST_FOREACH(http::server::buffer_ptr value ,m_sendbuf)
				for(std::list<http::server::buffer_ptr>::iterator it = m_sendbuf.begin();
					it != m_sendbuf.end();
					it = m_sendbuf.begin())
				{
					int currentBuffSize = (*it)->size();

					ncurrent_send_total += currentBuffSize;

					if(ncurrent_send_total > maxSend || buffers->size() > 50)
					{
						ncurrent_send_total -= currentBuffSize;//(*it)->size();
						break;
					}
					else
					{
						buffers->push_back(*it);
						m_sendbuf.erase(it);
					}
				}

				send(buffers);
			}
			else
			{
				if(m_sendbuf.size())
				{
					do 
					{
						http::server::buffer_ptr tmp = m_sendbuf.front();
						m_sendbuf.erase(m_sendbuf.begin());
						ncurrent_send_total = tmp->size();
						if(ncurrent_send_total > 0)
						{
							send(tmp);
						}

					} while (ncurrent_send_total == 0);

				}
			}


		}
		

	}

	{
		LOG3(120*1000*1000,informational,"当前socket连接 [%d] 总共发起 [%d] 次写事件 全部连接数 [%d] ",
			conn->getObjectID(),
			conn->get_unfinished_write_count(),
			conn->getObjectCount());

		std::pair<int,int> result = get_send_buf_size();


		if(conn && result.first > _const_max_send_buf_size * 2)
		{
			LOG2(20,informational,"写缓存总共[%d]个,有数据积压,关闭当前socket连接 [%d] 全部连接数 [%d] ",
				result.second,
				conn->getObjectID(),
				conn->getObjectCount());

			stop();

		}

	}

	return ncurrent_send_total;
}

template<typename Derived>
bool ConnectionHandler<Derived>::SendSharedPacket(http::server::buffer_ptr buffer)
{

	if(buffer && buffer->size())
	{
		{
			boost::detail::lightweight_mutex::scoped_lock lock(m_MutexSend);
			m_sendbuf.push_back(buffer);
		}


		{
			http::server::connection_ptr conn = get_connection();
			boost::detail::lightweight_mutex::scoped_lock lock(m_MutexSend);
			if(conn && conn->get_unfinished_write_count() == 0)
				do_send();
		
		}


	}
	else
		return false;
}



template<typename Derived>
bool ConnectionHandler<Derived>::SendPacket(const char * pPacket, const int intLen)
{
	if(intLen == 0 || pPacket == NULL)
	{
		LOG(error, "发送空包");
		return false;
	}

	{
		boost::detail::lightweight_mutex::scoped_lock lock(m_MutexSend);
		m_sendbuf.push_back(boost::make_shared<http::server::buffer>(pPacket, pPacket + intLen));
	}
	{
		http::server::connection_ptr conn = get_connection();
		boost::detail::lightweight_mutex::scoped_lock lock(m_MutexSend);
		if(conn && conn->get_unfinished_write_count() == 0)
			do_send();
	}

	return true;
}

template<typename Derived>
std::pair<int,int> ConnectionHandler<Derived>::get_send_buf_size()
{
	int intSize = 0;
	boost::detail::lightweight_mutex::scoped_lock lock(m_MutexSend);

	for(list<http::server::buffer_ptr>::iterator it = m_sendbuf.begin(); it != m_sendbuf.end(); it++)
	{
		intSize += (*it)->size();
	}

	return std::make_pair(intSize,m_sendbuf.size());
}




template<typename Derived >
int ConnectionHandler<Derived>::_const_max_send_buf_size = 1024*1024;














