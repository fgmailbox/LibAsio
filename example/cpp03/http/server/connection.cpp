//
// connection.cpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <vector>
#include <boost/bind.hpp>

#include "connection.hpp"
#include "connection_manager.hpp"
#include "facade.hpp"
//#include "log.h"

namespace http {
namespace server {


volatile int connection::s_min_read_buf_size = 1024*8;

boost::atomic<unsigned long long>		connection::s_system_total_read_bytes_(0);

boost::atomic<unsigned long long>		connection::s_system_total_write_bytes_(0);

connection::connection(io_service_ptr io_service,
					   io_service_ptr io_service_work,
					   facade& facade,ConnectionType connType)
  : strand_(*io_service),
	resolver(*io_service_work),
    strand_work_(*io_service_work),
  	socket_(*io_service),
	ios(io_service),
	ios_work(io_service_work),
	bDestroying_(false),
    facade_(facade),
	type(connType),
	unfinished_sync_read_count_(boost::make_shared<AtomicGuard<>::AtomicType>(0)),
	unfinished_read_count_(boost::make_shared<AtomicGuard<>::AtomicType>(0)),
	unfinished_write_count_(boost::make_shared<AtomicGuard<>::AtomicType>(0)),
	unfinished_read_bytes_(boost::make_shared<AtomicGuard<unsigned long long>::AtomicType>(0)),
	unfinished_write_bytes_(boost::make_shared<AtomicGuard<unsigned long long>::AtomicType>(0)),
	current_total_read_bytes_(0),
	current_total_write_bytes_(0),
	currentReadTimeWithms(0),	
	currentWriteTimeWithms(0),
	currentConnectTimeWithms(0),
	currentDNSResolveTimeWithms(0),
	read_seq_(0),
	write_seq_(0),
	max_read_buf_size_(s_min_read_buf_size)
{
}

connection::~connection()
{

#if 0
	LOG2(300,informational,"Connection [%d] Destroyed Total [%d] left",getObjectID(),getObjectCount());
#endif
	if(!bDestroying_)
	{
		boost::detail::lightweight_mutex::scoped_lock guard(lock);
		if(!bDestroying_)
		{
			bDestroying_ = true;
			stop();
		}
	}

	boost::system::error_code err;
	
	if(!facade_.bDestroying_ && !facade_.fire_close_.empty())
		facade_.fire_close_(getObjectID(),err,"destroyed");

}

boost::asio::ip::tcp::socket& connection::socket()
{
  return socket_;
}


// 返回未完成的同步读事件计数 
long connection::get_unfinished_sync_read_count()
{
	connection_ptr  conn  = shared_from_this();
	if(conn)
		return   *unfinished_sync_read_count_;
	else
		return 0;
}





// 返回未完成的读事件计数 
long connection::get_unfinished_read_count()
{
	connection_ptr  conn  = shared_from_this();
	if(conn)
		return   *unfinished_read_count_;
	else
		return 0;
}

// 返回未完成的写事件计数 
long connection::get_unfinished_write_count()
{
	connection_ptr  conn  = shared_from_this();
	if(conn)
		return	*unfinished_write_count_;
	else
		return 0;
}

// 返回未完成的读字节计数 
long connection::get_unfinished_read_bytes()
{
	connection_ptr  conn  = shared_from_this();
	if(conn)
		return   *unfinished_read_bytes_;
	else
		return 0;
}
// 返回未完成的写字节计数 
long connection::get_unfinished_write_bytes()
{
	connection_ptr  conn  = shared_from_this();
	if(conn)
		return	*unfinished_write_bytes_;
	else
		return 0;
}





void connection::handle_accept(const boost::system::error_code& e)
{
	connection_ptr  conn  = shared_from_this();
	if (!e)
	{
		//boost::asio::socket_base::non_blocking_io non_blocking_io(true);
		//socket_.io_control(non_blocking_io);
		//int no_delay = 1;
		//setsockopt(socket_.native(),IPPROTO_TCP,TCP_NODELAY,(char*)&no_delay,sizeof(int));

		boost::asio::ip::tcp::no_delay no_delay_opt(true);
		boost::system::error_code err;
		socket_.set_option(no_delay_opt, err);

		unsigned int timeout_milli = 60000;
		setsockopt(socket_.native(), SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout_milli, sizeof(timeout_milli));
		//setsockopt(socket_.native(), SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout_milli, sizeof(timeout_milli));

		//boost::asio::socket_base::non_blocking_io non_blocking_io(true);
		//socket_.io_control(non_blocking_io);


		facade_.add(conn);

		if(!facade_.fire_accept_.empty())
			facade_.fire_accept_(conn,e);

		facade_.start(conn);

	}


}

void connection::cancel()
{
	socket_.cancel();
}



void connection::connect(const std::string& address, const std::string& port)
{

	connection_ptr  conn  = shared_from_this();
	if(conn)
	{
		// Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
		//boost::asio::ip::tcp::resolver resolver(socket_.get_io_service());
		boost::asio::ip::tcp::resolver::query query(address, port);
#if 0
		boost::asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

		//return boost::asio::connect(socket_, endpoint_iterator);
		boost::asio::async_connect(socket_, endpoint_iterator,
			boost::bind(&connection::handle_connect, conn,
			boost::asio::placeholders::error
			));

#else

		typedef boost::tuple<boost::posix_time::ptime> TypeContext;

		resolver.async_resolve(query,
			boost::bind(&connection::handle_resolve<TypeContext>, this,
			boost::asio::placeholders::error,
			boost::asio::placeholders::iterator,
			boost::make_tuple(boost::posix_time::microsec_clock::local_time())
			));
#endif



		facade_.add(conn);
	}

}

void connection::handle_resolve(const boost::system::error_code& err,
					 boost::asio::ip::tcp::resolver::iterator endpoint_iterator)
{
	connection_ptr  conn  = shared_from_this();

	if (!err)
	{


		typedef boost::tuple<boost::posix_time::ptime> TypeContext;

		//return boost::asio::connect(socket_, endpoint_iterator);
		boost::asio::async_connect(socket_, endpoint_iterator,
			boost::bind(&connection::handle_connect<TypeContext>, conn,
			boost::asio::placeholders::error,
			boost::make_tuple(boost::posix_time::microsec_clock::local_time())
			));



	}
	else
	{
#if 0
		LOG(error, "DNS Error: %s", err.message().c_str());
#endif
		facade_.stop(conn);
	}
}




void connection::schedule_defer_func(connection::async_callback callback)
{
	// 引入这个与timer类似的功能是为了让函数尽量在与socket相同的线程里处理，以减少同步开销 
	connection_ptr  conn  = shared_from_this();
	if(conn)
		strand_work_.get_io_service().post(strand_work_.wrap(callback));
}


void connection::handle_connect(const boost::system::error_code& err)
{
	connection_ptr conn = shared_from_this();

	if (!err)
	{

		if(conn && !facade_.bDestroying_ && !facade_.fire_connect_.empty())
		{

			//int no_delay = 1;
			//setsockopt(socket_.native(),IPPROTO_TCP,TCP_NODELAY,(char*)&no_delay,sizeof(int));

			boost::asio::ip::tcp::no_delay no_delay_opt(true);
			boost::system::error_code err;
			socket_.set_option(no_delay_opt, err);

			unsigned int timeout_milli = 60000;
			setsockopt(socket_.native(), SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout_milli, sizeof(timeout_milli));
			//setsockopt(socket_.native(), SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout_milli, sizeof(timeout_milli));

			//boost::asio::socket_base::non_blocking_io non_blocking_io(true);
			//socket_.io_control(non_blocking_io);

			facade_.fire_connect_(conn,err);
#if 0
			// 异步调用的目的是为了防止在处理连接的回调过程中，又发起异步读操作 
			// 有可能会出问题
			facade_.alloc_timer()->start_at_once(boost::bind(&connection_manager::start,
											 boost::ref(connection_manager_),
											 conn));
#else
			//connection_manager_.start(conn);
			facade_.start(conn);
#endif
		}
	}
	else 
	{

		if (err == boost::asio::error::operation_aborted)
		{
			std::string errMsg("IO被cancel掉啦,现在正在释放资源 [");
			errMsg += err.message()+']';

			if(conn && !facade_.bDestroying_ && !facade_.fire_warning_.empty())
				facade_.fire_warning_(conn,errMsg);
			//LOG2(100,informational,"IO被cancel掉啦,现在正在释放资源 [%s]",errMsg.c_str());


		}

		if(conn && conn->type != E_INVALID_HANDLE)
		{
			if(!facade_.bDestroying_ && !facade_.fire_error_.empty())
				facade_.fire_error_(conn,err);

			facade_.stop(conn);

		}

	}

}


void connection::start()
{
	read(max_read_buf_size_);
}

void connection::stop()
{

	try
	{
		//Double Check
		if(type != E_INVALID_HANDLE && socket_.is_open())
		{

			boost::detail::lightweight_mutex::scoped_lock guard(lock);
			if(type != E_INVALID_HANDLE && socket_.is_open())
			{
				type = E_INVALID_HANDLE;
	#if 1
				// windows 2003 不支持
				socket_.cancel();
#endif 
				boost::system::error_code ignored_ec;
				socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
				socket_.close();
		
			}
		}
	}
	catch (std::exception & e)
	{
#if 0
		LOG(error,"关闭socket异常,原因:[%s]",e.what());
#endif
	}


	if(!bDestroying_)
	{
		// 不是被析构函数调用 

		connection_ptr  conn  = shared_from_this();

		if(!conn) return;

#if 0
		LOG2(200,informational,"Connection [%d] Stoped Total [%d] left",getObjectID(),getObjectCount());
#endif
		//connection_manager_.remove(conn);
		facade_.remove(conn);
	}
	else
	{
#if 0
		LOG2(200,informational,"Connection [%d] Stoped and Deleted Total[%d] left",getObjectID(),getObjectCount());
#endif
	}

}


int connection::sync_read(int bufsize,char* read_buffer,long& read_seq, boost::system::error_code& ec)
{

	connection_ptr  conn  = shared_from_this();

	if(conn 
		&& conn->type != connection::E_INVALID_HANDLE 
		&& socket_.is_open())
	{


#if 1
		if(conn->get_unfinished_sync_read_count() <= 1)
#else
		if(conn->get_unfinished_sync_read_count() < 4)
#endif
		{
			boost::shared_ptr<boost::detail::lightweight_mutex::scoped_lock> guard = boost::make_shared<boost::detail::lightweight_mutex::scoped_lock>(boost::ref(lock));
			if(socket_.is_open())
			{
				boost::posix_time::ptime startTime = boost::posix_time::microsec_clock::local_time();

				AtomicGuard<>::AtomicGuardPtr guard_sync_read_count =	enable_sync_read(2);

				if(*guard_sync_read_count <= 0 || *guard_sync_read_count >= 3)
				{
					std::string strMsg;
					{
						char bufferMsg[256];
						sprintf(bufferMsg,"当前连接[%d]只能有一个同步读,目前已有一个同步读或者没有发起同步读",conn->getObjectID());
						strMsg = bufferMsg;
					}

					if(!facade_.bDestroying_ && !facade_.fire_warning_.empty())
						facade_.fire_warning_(conn,strMsg);

					return -1;
				}
				// 统计当前正在读的字节数 
				AtomicGuard<unsigned long long>::AtomicGuardPtr guard_read_bytes		=	boost::make_shared<AtomicGuard<unsigned long long> >(unfinished_read_bytes_, bufsize);

				io_buffer_pool_ptr io_buffers = boost::make_shared<io_buffer_pool>( );

				io_buffers->push_back(boost::asio::buffer(read_buffer,bufsize));


				//boost::system::error_code ec;
				// 阻塞读之前，先释放锁资源 
				read_seq = ++read_seq_;
				guard.reset();
				int readSize = socket_.read_some(*io_buffers,ec);

				if(ec)
				{
					BOOST_ASSERT(readSize == 0);

					if (ec == boost::asio::error::operation_aborted)
					{
						std::string strMsg;
						{
							char bufferMsg[512];
							sprintf(bufferMsg,"IO被cancel掉啦,现在正在释放资源 [%s]",ec.message().c_str());
							strMsg = bufferMsg;
						}

						if(!facade_.bDestroying_ && !facade_.fire_warning_.empty())
							facade_.fire_warning_(conn,strMsg);

						return 0;
					}
					else
					{

						if(!facade_.bDestroying_ && !facade_.fire_error_.empty())
							facade_.fire_error_(conn,ec);

						return -1;
					}

				}

				boost::posix_time::time_duration finishTimes 
					= boost::posix_time::microsec_clock::local_time() - startTime;

				currentReadTimeWithms = finishTimes.total_milliseconds();

				// 恢复异步读 
				guard_sync_read_count.reset();
				read();
				
				return readSize;


			}
			else
				return -1;
		}
		else
		{
			char bufferMsg[512];
			sprintf(bufferMsg,"connection %d 同步读忙",conn->getObjectID());
			if(!facade_.bDestroying_ && !facade_.fire_warning_.empty())
				facade_.fire_warning_(conn,bufferMsg);

			return 0;
		}

	}
	else
	{
		// ToDo  错误报告
		boost::system::error_code ec;
		return -1;
	}

}

AtomicGuard<>::AtomicGuardPtr connection::enable_sync_read(int count)
{
	connection_ptr  conn  = shared_from_this();
	AtomicGuard<>::AtomicGuardPtr guard_sync_read_count;

	if(conn 
		&& conn->type != connection::E_INVALID_HANDLE 
		&& socket_.is_open())
	{
		guard_sync_read_count =	boost::make_shared<AtomicGuard<> >(unfinished_sync_read_count_,count);

		//int unfinished_sync_read_count = conn->get_unfinished_sync_read_count();

		if(0 < *guard_sync_read_count && *guard_sync_read_count < 0x3)
		{
			// 等待以前发起的异步读结束,然后再准备发起同步读 
			if( conn->get_unfinished_read_count() > 0)
			{
				if(conn->type != connection::E_INVALID_HANDLE && conn->socket_.is_open())
					cancel();
				int loopCount = 0;
				// 等待以前发起的异步读结束 
				while(conn->type != connection::E_INVALID_HANDLE &&  
					  conn->socket_.is_open() &&
					  conn->get_unfinished_read_count() > 0
					  && loopCount++ < 20)
				{
					boost::this_thread::sleep(boost::posix_time::milliseconds(200));
				}

			}

		}
	}
	return guard_sync_read_count;
}



bool connection::read(int bufsize)
{
	
	connection_ptr  conn  = shared_from_this();

	if(conn 
		&& conn->type != connection::E_INVALID_HANDLE 
		&& socket_.is_open())
	{
		// 有同步读的情况下，禁止异步读 
		if(conn->get_unfinished_sync_read_count() == 0 && 
			conn->get_unfinished_read_count() < 4)
		{
			// 当前连接在读的不多,代表CPU数据处理能力充足,
			// 可以继续多发起读,消耗更多数据的供应量，加快网络传输  
			// Double check
			boost::detail::lightweight_mutex::scoped_lock guard(lock);
			if(socket_.is_open())
			{
				buffer_ptr read_buffer = boost::make_shared<buffer>(bufsize);
				//read_buffer->resize(bufsize);

				io_buffer_pool_ptr io_buffers = boost::make_shared<io_buffer_pool>( );

				io_buffers->push_back(boost::asio::buffer(&(*read_buffer)[0],read_buffer->size()));

				typedef boost::tuple<buffer_ptr,
									long,
									io_buffer_pool_ptr,
									AtomicGuard<>::AtomicGuardPtr,
									AtomicGuard<unsigned long long>::AtomicGuardPtr,
									boost::posix_time::ptime
									> 
									TypeContext;


				socket_.async_read_some(*io_buffers,
						strand_.wrap(
						boost::bind(&connection::handle_read<TypeContext>, conn,
						boost::asio::placeholders::error,
						boost::asio::placeholders::bytes_transferred,
						boost::make_tuple(read_buffer,
						long(++read_seq_),
						io_buffers,
						boost::make_shared<AtomicGuard<> >(unfinished_read_count_),
						boost::make_shared<AtomicGuard<unsigned long long> >(unfinished_read_bytes_, bufsize),
						boost::posix_time::microsec_clock::local_time()
						)
						)));
				return true;


			}
			else
				return false;
		}
		else
		{
			// 当前连接发起太多的异步读操作,徒然消耗缓存,
			// 无需再发起更多的读操作   
#if 0
			std::string strMsg;
			{
				char bufferMsg[512];
				sprintf(bufferMsg,"当前socket连接 [%d] 总共发起 [%d] 次异步读操作 [%d] 次同步读操作 已经无需再发起新的异步读操作，全部连接数 [%d] ",
					conn->getObjectID(),
					conn->get_unfinished_read_count(),
					conn->get_unfinished_sync_read_count(),
					conn->getObjectCount());
				strMsg = bufferMsg;
			}
			if(!facade_.bDestroying_ && !facade_.fire_warning_.empty())
				facade_.fire_warning_(conn,strMsg);
#endif
			return false;
		}

	}
	else
	{
		// ToDo  错误报告
		//boost::system::error_code ec;
		return false;
	}

}



bool connection::write(buffer_ptr sendBuffer)
{
	connection_ptr  conn  = shared_from_this();

	if(conn && conn->type != connection::E_INVALID_HANDLE 
		&& sendBuffer && sendBuffer->size() && socket_.is_open())
	{
		buffer_pool_ptr sendBuffers = boost::make_shared<buffer_pool>();
		sendBuffers->push_back(sendBuffer);
		return multi_write(sendBuffers);
	}
	else
		return false;
}

bool connection::multi_write(buffer_pool_ptr sendBuffers)
{
	connection_ptr  conn  = shared_from_this();

	if(!conn || conn->type == connection::E_INVALID_HANDLE 
		|| !socket_.is_open()
		|| !sendBuffers || sendBuffers->empty())
		return false;
#if 0
	LOG3(30*1000*1000,notice,"启动multi_write 写 socket [%d] Totoal [%d] 写缓存[%d]个,总共[%d]个字节", 
							  conn->getObjectID(),
							  conn->getObjectCount(), 
							  sendBuffers->size(),
							  get_buffer_pool_size(sendBuffers));
#endif

	io_buffer_pool_ptr buffers = boost::make_shared<io_buffer_pool>( );

	std::size_t write_size = 0;
#if 1
	BOOST_FOREACH(http::server::buffer_ptr sendBuffer , *sendBuffers)
	//for(int i = 0; i < sendBuffers->size();i++)
	{
		std::size_t size = sendBuffer->size();//(*sendBuffers)[i]->size();
		if(size > 0)
		{

			write_size += size;

			buffers->push_back(boost::asio::buffer(&(*sendBuffer)[0],size));

		}
	}
#else
	for(int i = 0; i < sendBuffers->size();i++)
	{
		std::size_t size = (*sendBuffers)[i]->size();
		if((*sendBuffers)[i]->size() > 0)
		{
			
			write_size += size;

			buffers->push_back(boost::asio::buffer(&(*(*sendBuffers)[i])[0],size));

		}
	}
#endif`
	if(write_size > 0)
	{
		if( socket_.is_open())
		{
			// Double check
			boost::detail::lightweight_mutex::scoped_lock guard(lock);
			if(socket_.is_open())
			{

				typedef boost::tuple<buffer_pool_ptr,
									 long,
									 io_buffer_pool_ptr,
									 AtomicGuard<>::AtomicGuardPtr,
									 AtomicGuard<unsigned long long>::AtomicGuardPtr,
									 boost::posix_time::ptime
									> TypeContext;



				boost::asio::async_write(socket_, *buffers, 				
					strand_.wrap(
					boost::bind(&connection::handle_m_write<TypeContext>, 
								conn,
								boost::asio::placeholders::error,
								boost::asio::placeholders::bytes_transferred,
								boost::make_tuple(sendBuffers,
												   long(++write_seq_),
												  buffers,
												  boost::make_shared<AtomicGuard<> >(unfinished_write_count_),
												  boost::make_shared<AtomicGuard<unsigned long long> >(unfinished_write_bytes_,write_size),
												  boost::posix_time::microsec_clock::local_time()
												  )
					)));
#if 0
				buffers.reset();
#endif
				return true;
			}
			else
				return false;
		}
	}
	else
		return false;

}






void connection::handle_read(const boost::system::error_code& err,
				 std::size_t bytes_transferred,
				 buffer_ptr buffer, 
				 long read_seq)
{
	connection_ptr  conn  = shared_from_this();

	if(!conn)
		return;

	//const char* rawBuffer =  boost::asio::buffer_cast<const char *>(buffer->data());

	if (!err)
	{
		if(bytes_transferred == 0)
		{
#if 0
			LOG2(10,informational,"Connection [%d] 读不到数据 全部连接[%d]",getObjectID(),getObjectCount());
#endif
			read(max_read_buf_size_);
			return;
		}
		//  读满，意味着缓存不够长 
		if(bytes_transferred == buffer->size()) //http::server::connection::s_max_read_buf_size)
		{
			if(max_read_buf_size_ < 128*1024)
				max_read_buf_size_ = max_read_buf_size_  > bytes_transferred ? max_read_buf_size_: bytes_transferred + bytes_transferred;
#if 0
			std::string strMsg;
			{
				char bufferMsg[512];
				sprintf(bufferMsg,"缓冲区读满 套接字 [%d] 套接字总数 [%d] 缓冲区增大到 [%d]",
					getObjectID(),
					getObjectCount(),
					max_read_buf_size_);
				strMsg = bufferMsg;
			}

			if(!facade_.bDestroying_ && !facade_.fire_warning_.empty())
				facade_.fire_warning_(conn,strMsg);
#endif


		}
		else if(bytes_transferred < buffer->size())
		{
			max_read_buf_size_ = bytes_transferred > s_min_read_buf_size ? bytes_transferred: s_min_read_buf_size;
#if 0
			std::string strMsg;
			{
				char bufferMsg[512];
				sprintf(bufferMsg,"缓冲区读不满 套接字 [%d] 套接字总数 [%d] 缓冲区减小到 [%d]",
					getObjectID(),
					getObjectCount(),
					max_read_buf_size_);
				strMsg = bufferMsg;
			}

			if(!facade_.bDestroying_ && !facade_.fire_warning_.empty())
				facade_.fire_warning_(conn,strMsg);
#endif




		}
		else
		{
#if 0
			LOG(error,"缓冲区处理的逻辑错误");
#endif
			return;
		}


		if(!facade_.bDestroying_ && !facade_.fire_read_.empty())
			facade_.fire_read_(shared_from_this(),err,bytes_transferred,
			bytes_transferred == buffer->size() ? buffer :
			boost::make_shared<http::server::buffer>(buffer->begin(),
													 buffer->begin() + bytes_transferred),
													 read_seq
													);
		return;
	}
	else 
	{
		BOOST_ASSERT(bytes_transferred == 0);

		if (err == boost::asio::error::operation_aborted)
		{
			std::string strMsg;
			{
				char bufferMsg[512];
				sprintf(bufferMsg,"IO被cancel掉啦,现在正在释放资源 [%s]",err.message().c_str());
				strMsg = bufferMsg;
			}

			if(!facade_.bDestroying_ && !facade_.fire_warning_.empty())
				facade_.fire_warning_(conn,strMsg);



#if 0
			if(bytes_transferred && conn && conn->type != E_INVALID_HANDLE)
			{
				if(!facade_.bDestroying_ && !facade_.fire_read_.empty())
					facade_.fire_read_(shared_from_this(),err,bytes_transferred,
					bytes_transferred == buffer->size() ? buffer :
					boost::make_shared<http::server::buffer>(buffer->begin(),
															 buffer->begin() + bytes_transferred),
					read_seq
					);

			}
#endif

			return;
		}

		if(conn && conn->type != E_INVALID_HANDLE)
		{
			if(!facade_.bDestroying_ && !facade_.fire_error_.empty())
				facade_.fire_error_(conn,err);

			facade_.stop(conn);

		}

	}

}

int connection::get_buffer_pool_size( buffer_pool_ptr buffers )
{
	int bufferSize = 0;
	if(buffers)
	{
		buffer_ptr tmp_buf;
		BOOST_FOREACH(tmp_buf , *buffers)
		{
			bufferSize += tmp_buf->size();
		}

	}
	return bufferSize;
}




void connection::handle_m_write(const boost::system::error_code& err,std::size_t bytes_transferred,
								buffer_pool_ptr buffers,long write_seq)
{

	connection_ptr  conn  = shared_from_this();

	if(!conn)
		return;


	int bufferSize	= get_buffer_pool_size(buffers);

	buffer_ptr bufferLeft =  (bufferSize == bytes_transferred) ?  buffer_ptr() :
		boost::make_shared<buffer>();

	if(bufferLeft)
	{
		bufferLeft->reserve(1024);

		int total_size = 0;
		int prev_total_size	= 0;
		std::size_t size = 0;


		BOOST_FOREACH(buffer_ptr buffer,*buffers)
		//for(int i = 0; i < buffers->size(); i++)
		{
			size = buffer->size();//(*buffers)[i]->size();//boost::asio::buffer_size((*buffers)[i]);

			prev_total_size = total_size;
			total_size += size;
			if( size == 0 || total_size <= bytes_transferred)
				continue;
			else if(prev_total_size < bytes_transferred)
			{
				int consume = bytes_transferred - prev_total_size;

				bufferLeft->insert(bufferLeft->end(),&(*buffer)[0]+consume,&(*buffer)[0]+size);
				//bufferLeft->insert(bufferLeft->end(),&(*(*buffers)[i])[0]+consume,&(*(*buffers)[i])[0]+size);
			}
			else
			{
				//std::size_t temp_size = bufferLeft->size();
				bufferLeft->insert(bufferLeft->end(),&(*buffer)[0], &(*buffer)[0]+size);
			}

		}

	}






	if (!err)
	{

		if(//bufferLeft->size() &&
			!facade_.bDestroying_ && !facade_.fire_write_.empty())
			facade_.fire_write_(shared_from_this(),err,bytes_transferred,
			bufferLeft,write_seq);
		return;
	}
	else 
	{
		BOOST_ASSERT(bytes_transferred == 0);

		if (err == boost::asio::error::operation_aborted)
		{

			std::string strMsg;
			{
				char bufferMsg[512];
				sprintf(bufferMsg,"IO被cancel掉啦,现在正在释放资源 [%s]",err.message().c_str());
				strMsg = bufferMsg;
			}

			if(!facade_.bDestroying_ && !facade_.fire_warning_.empty())
				facade_.fire_warning_(conn,strMsg);

			return;
		}

		if(conn && conn->type != E_INVALID_HANDLE)
		{
			if(!facade_.bDestroying_ && !facade_.fire_error_.empty())
				facade_.fire_error_(conn,err);

			facade_.stop(conn);

		}

	}



}







} // namespace server
} // namespace http

