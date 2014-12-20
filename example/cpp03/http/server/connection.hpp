//
// connection.hpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_CONNECTION_HPP
#define HTTP_CONNECTION_HPP
//#define BOOST_THREAD_USE_DLL
#include <list>
#include <boost/detail/lightweight_mutex.hpp>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/noncopyable.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/enable_shared_from_this.hpp>
#include "ObjectHandle.h"
//#include "log.h"

namespace http {
namespace server {

class connection_manager;
class facade;
class connection;

#if 0
class buffer
	:public std::vector<char>,
	 public ObjectHandle<buffer>
{
public:
	buffer();

	buffer(const char *begin,const char *end);

	virtual ~buffer();
};
#else
typedef std::vector<char> buffer;
#endif
typedef boost::shared_ptr<boost::asio::io_service> io_service_ptr;

typedef  boost::shared_ptr<buffer> buffer_ptr;

typedef  boost::weak_ptr<buffer> buffer_weak_ptr;

typedef  std::list<buffer_weak_ptr>  weak_buffer_pool;

typedef  boost::shared_ptr<weak_buffer_pool> weak_buffer_pool_ptr;


typedef  std::list<buffer_ptr>  buffer_pool;

typedef  boost::shared_ptr<buffer_pool> buffer_pool_ptr;

//typedef  boost::asio::const_buffer	io_buffer;
typedef  boost::asio::mutable_buffer	io_buffer;

typedef std::list<io_buffer> io_buffer_pool; 

typedef boost::shared_ptr<io_buffer_pool> io_buffer_pool_ptr;

typedef boost::shared_ptr<connection> connection_ptr;
typedef boost::weak_ptr<connection> connection_weak_ptr;

typedef std::vector<http::server::connection_ptr> connection_pool; 
typedef boost::shared_ptr<connection_pool> connection_pool_ptr;


// connection的上下文 
class connection_context
{
public:
	virtual ~connection_context()
	{
	};

};

typedef boost::shared_ptr<connection_context> connection_context_ptr;


/// Represents a single connection from a client.
class connection
  : public boost::enable_shared_from_this<connection>,
    public ObjectHandle<connection>,
    private boost::noncopyable
{
public:
	typedef boost::function<void ()> async_callback;

	typedef enum
	{
		E_INVALID_HANDLE,
		E_CLIENT_HANDLE,
		E_SERVER_HANDLE,
	}
	ConnectionType;

	ConnectionType type;
  /// Construct a connection with the given io_service.
  explicit connection(io_service_ptr io_service,
					  io_service_ptr io_service_work,
					  facade& facade, ConnectionType connType = E_CLIENT_HANDLE);

  ~connection();
  /// Get the socket associated with the connection.
  boost::asio::ip::tcp::socket& socket();

  void set_context(connection_context_ptr context)
  {
	  context_ = context;
  }

  connection_context_ptr get_context( )
  {
	  return context_;
  }

  template<typename Derived>
  boost::shared_ptr<Derived> get_context( )
  {
	  return boost::dynamic_pointer_cast<Derived>(get_context());
  }


  void schedule_defer_func(async_callback callback);

  void connect(const std::string& address, const std::string& port);

  /// Start the first asynchronous operation for the connection.
  void start();

  bool read(int bufsize = s_min_read_buf_size);

  int  sync_read(int bufsize, char* read_buffer, long& read_seq, boost::system::error_code& ec);

  AtomicGuard<>::AtomicGuardPtr enable_sync_read(int count = 1);


  bool multi_write(buffer_pool_ptr sendBuffers);

  void cancel();

  bool write(buffer_ptr  sendBuffer);

  // 返回未完成的同步读事件计数 
  long connection::get_unfinished_sync_read_count();

  // 返回未完成的读事件计数 
  long get_unfinished_read_count();
  // 返回未完成的写事件计数 
  long get_unfinished_write_count();

  // 返回未完成的读字节计数 
  long get_unfinished_read_bytes();
  // 返回未完成的写字节计数 
  long get_unfinished_write_bytes();

  void stop();


public:

	// 读事件序列号  
	boost::atomic<long>		read_seq_;
	// 写事件序列号 
	boost::atomic<long>		write_seq_;

	int						max_read_buf_size_;

	static volatile int		s_min_read_buf_size;

	facade&					facade_;

  void handle_accept(const boost::system::error_code& e);

private:
  
	connection_context_ptr context_;



	void handle_resolve(const boost::system::error_code& err,  boost::asio::ip::tcp::resolver::iterator endpoint_iterator);


	template<typename TypeContext>
	void handle_resolve(const boost::system::error_code& err,
						 boost::asio::ip::tcp::resolver::iterator endpoint_iterator,
						 TypeContext context);
	


	void handle_connect(const boost::system::error_code& err);

	template<typename TypeContext>
	void handle_connect(const boost::system::error_code& err,
						TypeContext context);
	


  /// Handle completion of a read operation.

  void handle_read(const boost::system::error_code& e,
				  std::size_t bytes_transferred,
				  buffer_ptr buffer, 
				  long read_seq);

  template<typename TypeContext>
  void handle_read(const boost::system::error_code& e,
				   std::size_t bytes_transferred,
				   TypeContext context);
  


  /// Handle completion of a write operation.

  void handle_m_write(const boost::system::error_code& e,
	  std::size_t bytes_transferred,
	  buffer_pool_ptr buffers, 
	  long write_seq);



  template<typename TypeContext>
  void handle_m_write(const boost::system::error_code& err,std::size_t bytes_transferred,
					  TypeContext context);

  int get_buffer_pool_size( buffer_pool_ptr buffers );

public:

  // 未完成的同步读事件计数 
  AtomicGuard<>::AtomicTypePtr		unfinished_sync_read_count_;
	
  // 未完成的读事件计数 
  AtomicGuard<>::AtomicTypePtr		unfinished_read_count_;
  // 未完成的写事件计数 
  AtomicGuard<>::AtomicTypePtr		unfinished_write_count_;
  // 未完成的读字节计数 
  AtomicGuard<unsigned long long>::AtomicTypePtr		unfinished_read_bytes_;
  // 未完成的写字节计数 
  AtomicGuard<unsigned long long>::AtomicTypePtr		unfinished_write_bytes_;
  // 当前连接读入的字节总数 
  boost::atomic<unsigned long long>				current_total_read_bytes_;
  // 当前连接写出的字节总数 
  boost::atomic<unsigned long long>				current_total_write_bytes_;

  volatile long						currentReadTimeWithms;		//!<当前socket完成一次读操作所需要的毫秒数
  volatile long						currentWriteTimeWithms;		//!<当前socket完成一次写操作所需要的毫秒数
  volatile long						currentConnectTimeWithms;	//!<当前socket完成一次连接操作所需要的毫秒数
  volatile long						currentDNSResolveTimeWithms;//!<当前socket完成一次域名解析操作所需要的毫秒数


  static boost::atomic<unsigned long long>		s_system_total_read_bytes_;

  static boost::atomic<unsigned long long>		s_system_total_write_bytes_;

  bool bDestroying_;

  //boost::recursive_mutex	lock;
  boost::detail::lightweight_mutex lock;
  /// Buffer for incoming data.
  //boost::array<char, 8192> buffer_;
  //boost::shared_ptr<std::vector<char> >			 buffer_;

  io_service_ptr	ios;

  io_service_ptr	ios_work;
 
  boost::asio::ip::tcp::resolver resolver;
  /// Strand to ensure the connection's handlers are not called concurrently.
  boost::asio::io_service::strand strand_;

  boost::asio::io_service::strand strand_work_;

  /// Socket for the connection.
  boost::asio::ip::tcp::socket socket_;

};

template<typename TypeContext>
void connection::handle_resolve(const boost::system::error_code& err,
					boost::asio::ip::tcp::resolver::iterator endpoint_iterator,
					TypeContext context)
{
	boost::posix_time::time_duration finishTimes 
		= boost::posix_time::microsec_clock::local_time() - boost::tuples::get<0>(context);

	currentDNSResolveTimeWithms = finishTimes.total_milliseconds();

#if 0
	if(finishTimes.total_milliseconds() > 30*1000)
	{
		std::string strMsg;
		{
			char bufferMsg[512];
			sprintf(bufferMsg,"DNS spend time [%lld] milliseconds", finishTimes.total_milliseconds());
			strMsg = bufferMsg;
		}

		connection_ptr  conn  = shared_from_this();

		if(!facade_.bDestroying_ && !facade_.fire_warning_.empty())
			facade_.fire_warning_(conn,strMsg);
	}
#endif 


	//LOG2(1000, informational, "spend time [%lld] milliseconds", finishTimes.total_milliseconds());

	handle_resolve(err,endpoint_iterator);

}



template<typename TypeContext>
void connection::handle_connect(const boost::system::error_code& err,
					TypeContext context)
{
	boost::posix_time::time_duration finishTimes 
		= boost::posix_time::microsec_clock::local_time() - boost::tuples::get<0>(context);

	currentConnectTimeWithms  = finishTimes.total_milliseconds();	
#if 0
	if(finishTimes.total_milliseconds() > 30*1000)
	{
		std::string strMsg;
		{
			char bufferMsg[512];
			sprintf(bufferMsg,"Connect spend time [%lld] milliseconds", finishTimes.total_milliseconds());
			strMsg = bufferMsg;
		}

		connection_ptr  conn  = shared_from_this();

		if(!facade_.bDestroying_ && !facade_.fire_warning_.empty())
			facade_.fire_warning_(conn,strMsg);
	}
#endif



	//if( finishTimes.total_milliseconds() > 5000)
	//	LOG2(1000, warning, "spend time [%lld] milliseconds", finishTimes.total_milliseconds());

	handle_connect(err);
}






template<typename TypeContext>
void connection::handle_read(const boost::system::error_code& e,
				 std::size_t bytes_transferred,
				 TypeContext context)
{
	if(!e)
	{

		s_system_total_read_bytes_.fetch_add(bytes_transferred);
		current_total_read_bytes_.fetch_add(bytes_transferred);
		boost::posix_time::time_duration finishTimes 
			= boost::posix_time::microsec_clock::local_time() - boost::tuples::get<5>(context);

		currentReadTimeWithms = finishTimes.total_milliseconds();

#if 0
		if(currentReadTimeWithms > 30*1000)
		{
			std::string strMsg;
			{
				char bufferMsg[512];
				sprintf(bufferMsg,"花了 [%lld] 毫秒 读了 [%d] 字节", finishTimes.total_milliseconds(), bytes_transferred);
				strMsg = bufferMsg;
			}

			connection_ptr  conn  = shared_from_this();

			if(!facade_.bDestroying_ && !facade_.fire_warning_.empty())
				facade_.fire_warning_(conn,strMsg);
		}
#endif

	}





	//if(finishTimes.total_milliseconds() > 5000)
	//	  LOG2(5000, warning, "花了 [%lld] 毫秒 读了 [%d] 字节", finishTimes.total_milliseconds(), bytes_transferred);

	handle_read(e,bytes_transferred,boost::tuples::get<0>(context), boost::tuples::get<1>(context));
}




template<typename TypeContext>
void connection::handle_m_write(const boost::system::error_code& err,std::size_t bytes_transferred,
					TypeContext context)
{
	

	if(!err)
	{

;
		s_system_total_write_bytes_.fetch_add(bytes_transferred);
		current_total_write_bytes_.fetch_add(bytes_transferred);

		boost::posix_time::time_duration finishTimes 
		= boost::posix_time::microsec_clock::local_time() - boost::tuples::get<5>(context);

		currentWriteTimeWithms = finishTimes.total_milliseconds();
#if 0	
		if( currentWriteTimeWithms > 30*1000)
		{
			std::string strMsg;
			{
				char bufferMsg[512];
				sprintf(bufferMsg,"写数据花了太多时间 [%lld] milliseconds", finishTimes.total_milliseconds());
				strMsg = bufferMsg;
			}

			connection_ptr  conn  = shared_from_this();

			if(!facade_.bDestroying_ && !facade_.fire_warning_.empty())
				facade_.fire_warning_(conn,strMsg);
		}
#endif


	}

	//if(finishTimes.total_milliseconds() > 5000)
	//	LOG2(1000,warning, "spend time [%lld] milliseconds", finishTimes.total_milliseconds());


	buffer_pool_ptr buffers = boost::tuples::get<0>(context);


	long write_seq = boost::tuples::get<1>(context);

	 handle_m_write(err,bytes_transferred,buffers,write_seq);


}




} // namespace server
} // namespace http

#endif // HTTP_CONNECTION_HPP
