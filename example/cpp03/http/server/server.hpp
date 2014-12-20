//
// server.hpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_SERVER_HPP
#define HTTP_SERVER_HPP

#include <boost/asio.hpp>
#include <string>
#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>
#include <signal.h>

#include "Singleton2.h"
#include "client.hpp"
//#include "facade.hpp"

namespace http {
namespace server {


template<typename serverType>
class server
  : public client<serverType>
{
public:

	using typename client<serverType>::configType;
	//using client<serverType>::s_io_service_pool_conf;
	//using client<serverType>::s_timer_io_service_pool_conf;

	server();

  explicit server(const std::string& address, 
				  const std::string& port,
				  typename const configType& accept_io_service_pool_conf,
				  typename const configType& io_service_pool_size_conf , 
				  typename const configType& timer_io_service_pool_conf);


  ~server();
  
  void init(const std::string& address= s_server_ip, 
			  const std::string& port	= s_server_port,
			  typename const configType& accept_io_service_pool_conf = s_accept_io_service_pool_conf,
			  typename const configType& io_service_pool_size_conf = s_io_service_pool_conf, 
			  typename const configType& timer_io_service_pool_conf = s_timer_io_service_pool_conf
			);
  /// Run the server's io_service loop.
  void run();

  virtual bool Configure(const char *szConfigFile);

  virtual bool Configure(std::map<std::string,std::map<std::string,std::string> >& cmdParam);	

public:
	//static configType s_accept_io_service_pool_conf;
	//static std::string s_server_ip;
	//static std::string s_server_port;


private:

	std::string host_;
	std::string port_;
	void handle_resolve(const boost::system::error_code& err,  boost::asio::ip::tcp::resolver::iterator endpoint_iterator);

  /// Initiate an asynchronous accept operation.
  void start_accept();

  /// Handle completion of an asynchronous accept operation.
  void handle_accept(typename connection_ptr conn, const boost::system::error_code& e);

  void start();

  void stop_all();
  
  boost::shared_ptr<boost::asio::ip::tcp::resolver> resolver;
  
  io_service_pool io_service_pool_accept_;

  /// Acceptor used to listen for incoming connections.
  boost::shared_ptr<boost::asio::ip::tcp::acceptor> acceptor_;

};


//template<typename serverType>
//typename server<serverType>::configType server<serverType>::s_accept_io_service_pool_conf;


//template<typename serverType>
//std::string server<serverType>::s_server_ip("0.0.0.0");

//template<typename serverType>
//std::string server<serverType>::s_server_port("1003");

template<typename serverType>
void server<serverType>::init(const std::string& address, 
						   const std::string& port,
						   typename const configType& accept_io_service_pool_conf,
						   typename const configType& io_service_pool_size_conf, 
						   typename const configType& timer_io_service_pool_conf
						   )
{

	host_ = address;
	port_ = port;
	if(io_service_pools.size() == 0)
	{
		__super::init(io_service_pool_size_conf,timer_io_service_pool_conf);

		
		resolver =  boost::make_shared<boost::asio::ip::tcp::resolver>(boost::ref(*io_service_pools[E_ALLOC_IOS_TIMER_POOL]->get_io_service()));


		io_service_pools.push_back(boost::make_shared<io_service_pool>(accept_io_service_pool_conf.io_service_pool_size,
			accept_io_service_pool_conf.speed_threads_for_a_io_service));

		acceptor_ = boost::make_shared<boost::asio::ip::tcp::acceptor>(boost::ref(*io_service_pools[E_ALLOC_IOS_ACCEPT_POOL]->get_io_service()));
		
		start();
	}
}

template<typename serverType>
server<serverType>::server()
{

}

template<typename serverType>
server<serverType>::server(const std::string& address, 
								  const std::string& port,
								  typename const configType& accept_io_service_pool_conf,
								  typename const configType& io_service_pool_size_conf, 
								  typename const configType& timer_io_service_pool_conf
								  )
		: client(io_service_pool_size_conf,timer_io_service_pool_conf),
		  host_(address),
		  port_(port),
		  resolver(boost::make_shared<boost::asio::ip::tcp::resolver>(io_service_pools[E_ALLOC_IOS_TIMER_POOL]->get_io_service()))
{

	io_service_pools.push_back(boost::make_shared<io_service_pool>(accept_io_service_pool_conf.io_service_pool_size,
																   accept_io_service_pool_conf.speed_threads_for_a_io_service));

	acceptor_ = boost::make_shared<boost::asio::ip::tcp::acceptor>(boost::ref(io_service_pools[E_ALLOC_IOS_ACCEPT_POOL]->get_io_service()));

	start();
}

template<typename serverType>
void server< serverType>::start()
{
	//boost::asio::ip::tcp::resolver resolver(acceptor_->get_io_service());
	boost::asio::ip::tcp::resolver::query query(host_, port_);
#if 0
	boost::asio::ip::tcp::endpoint endpoint = *(resolver->resolve(query));
	acceptor_->open(endpoint.protocol());
	acceptor_->set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
	acceptor_->set_option(boost::asio::ip::tcp::acceptor::enable_connection_aborted(false));

	acceptor_->bind(endpoint);
	acceptor_->listen();
#else
	resolver->async_resolve(query,
		boost::bind(&server::handle_resolve, this,
		boost::asio::placeholders::error,
		boost::asio::placeholders::iterator));
#endif

}

template<typename serverType>
void server< serverType>::handle_resolve(const boost::system::error_code& err,
										 boost::asio::ip::tcp::resolver::iterator endpoint_iterator)
{

	if (!err)
	{
		boost::asio::ip::tcp::endpoint endpoint = *endpoint_iterator;
#if 0
		LOG(informational, "bind IP [%s] Port [%d]", endpoint.address().to_string().c_str(),endpoint.port());
#endif

		acceptor_->open(endpoint.protocol());
		acceptor_->set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
		acceptor_->set_option(boost::asio::ip::tcp::acceptor::enable_connection_aborted(false));


		acceptor_->bind(endpoint);
		acceptor_->listen();
		start_accept();
	}
	else
	{
#if 0
		LOG(error, "DNS Error: %s", err.message().c_str());
#endif
#if 1
		srand(time(NULL));
		int delayMilliseconds = ( rand() * rand()) % 3000;
#if 0
		LOG2(30,informational,"异步延迟 [%d] 毫秒启动监听Socket",delayMilliseconds);
#endif
		alloc_timer()->start_from_now(boost::bind(&server::start,this),
									  boost::posix_time::milliseconds(delayMilliseconds)
									  );
#else
		boost::this_thread::sleep(boost::posix_time::seconds(3));
		start();
#endif

	}
}




template<typename serverType>
server< serverType>::~server()
{
		handle_stop();
}

template<typename serverType>
void server<serverType>::run()
{
	// The io_service::run() call will block until all asynchronous operations
	// have finished. While the server is running, there is always at least one
	// asynchronous operation outstanding: the asynchronous accept call waiting
	// for new incoming connections.
	__super::run();
	io_service_pools[E_ALLOC_IOS_ACCEPT_POOL]->run();
	//start_accept();
}

template<typename serverType>
void server<serverType>::start_accept()
{

	typename connection_ptr conn = alloc_connection(connection::E_SERVER_HANDLE);

	acceptor_->async_accept(conn->socket(),
		boost::bind(&server::handle_accept, this,conn,
		boost::asio::placeholders::error));
}

template<typename serverType>
void server<serverType>::handle_accept(typename connection_ptr conn,const boost::system::error_code& e)
{
	if (!acceptor_->is_open() )
	{
#if 0
		LOG(error,"无法监听端口 原因:%s",e.message());
#endif
		return;
	}

	start_accept();

	if (!e)
	{

#if 0
		conn->handle_accept(e);
#else
		conn->schedule_defer_func(boost::bind(&connection::handle_accept,conn,e));
#endif

	}

	//start_accept();
}

template<typename serverType>
void server<serverType>::stop_all()
{
	try
	{
#if 1
		// windows 2003 不支持
		acceptor_->cancel();
#endif
		acceptor_->close();
		io_service_pools[E_ALLOC_IOS_ACCEPT_POOL]->stop();
		__super::stop_all();

	}
	catch (std::exception & e)
	{
#if 0
		LOG(error,"关闭时捕获异常 原因:[%s]",e.what());
#endif
	}

}


template<typename serverType>
bool server<serverType>::Configure(const char *szConfigFile)
{

	//if(__super::configure(longevity,szConfigFile))
	{
		pugi::xml_document doc;
		pugi::xml_parse_result result = doc.load_file(szConfigFile);

		if(result)
		{

			configType io_service_pool_conf;

			configType timer_io_service_pool_conf;

			configType accept_io_service_pool_conf;

			std::string server_ip;
			std::string server_port;



			pugi::xpath_node  io_service_cfg_node = doc.select_single_node("/config/io_service[@name='listen']");
			if(io_service_cfg_node)
			{

				accept_io_service_pool_conf.io_service_pool_size	= 
					io_service_cfg_node.node().select_single_node("./io_service_pool_size").node().text().as_int(1);

				accept_io_service_pool_conf.speed_threads_for_a_io_service	= 
					io_service_cfg_node.node().select_single_node("./speed_threads_for_a_io_service").node().text().as_int(9);
			}
			else
			{
				accept_io_service_pool_conf.io_service_pool_size				= 1;
				accept_io_service_pool_conf.speed_threads_for_a_io_service		= 9;
			}


			pugi::xpath_node listen_node = doc.select_single_node("/config/server/listen");
			if(listen_node)
			{
				server_ip	= 
					listen_node.node().select_single_node("./ip").node().text().as_string("127.0.0.1");

				server_port	= 
					listen_node.node().select_single_node("./port").node().text().as_string("10005");


#if 0
				LOG(notice,"监听 ip[%s]port[%s]",server_ip.c_str(),server_port.c_str());
#endif
			}
			else
			{
#if 0
				LOG(warning,"不存在配置项，使用缺省");
#endif
				server_ip		= "127.0.0.1";
				server_port		= "10005";
			}

			init(server_ip,server_port,accept_io_service_pool_conf,io_service_pool_conf,timer_io_service_pool_conf);

			return true;
		}
		else
		{
			return false;
		}

	}
}



template<typename serverType>
bool server<serverType>::Configure(std::map<std::string,std::map<std::string,std::string> >& cmdParam)
{

	configType io_service_pool_conf;

	configType timer_io_service_pool_conf;

	configType accept_io_service_pool_conf;

	if(cmdParam.count("listen"))
	{
		accept_io_service_pool_conf.io_service_pool_size = boost::lexical_cast<int>(cmdParam["listen"]["io_service_pool_size"]);
		accept_io_service_pool_conf.speed_threads_for_a_io_service = boost::lexical_cast<int>(cmdParam["listen"]["speed_threads_for_a_io_service"]);
	}


	if(cmdParam.count("network"))
	{
		io_service_pool_conf.io_service_pool_size = boost::lexical_cast<int>(cmdParam["network"]["io_service_pool_size"]);
		io_service_pool_conf.speed_threads_for_a_io_service = boost::lexical_cast<int>(cmdParam["network"]["speed_threads_for_a_io_service"]);
	}

	if(cmdParam.count("timer"))
	{
		timer_io_service_pool_conf.io_service_pool_size = boost::lexical_cast<int>(cmdParam["timer"]["io_service_pool_size"]);
		timer_io_service_pool_conf.speed_threads_for_a_io_service = boost::lexical_cast<int>(cmdParam["timer"]["speed_threads_for_a_io_service"]);
	}

	std::string address = cmdParam["host"]["address"];
	std::string port	= cmdParam["host"]["port"];
	std::string min_read_buffer_size	= cmdParam["host"]["min_read_buffer_size"];

	if(!min_read_buffer_size.empty())
		connection::s_min_read_buf_size =  boost::lexical_cast<int>(min_read_buffer_size);

	init(address,port,accept_io_service_pool_conf,io_service_pool_conf,timer_io_service_pool_conf);

	return true;
}









} // namespace server
} // namespace http



typedef http::server::server<long> serverFacade;




#endif // HTTP_SERVER_HPP
