//
// server.hpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_CLIENT_HPP
#define HTTP_CLIENT_HPP
#include <string>
#include <vector>
#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include "pugixml.hpp"
#include "facade.hpp"

namespace http {
namespace server {

template<typename ClientType>
class client
  : public facade_extension,
    private boost::noncopyable
{
public:

	using typename facade_extension::configType;

	client();

  explicit client( typename const configType& io_service_pool_size_conf	, 
				   typename const configType& timer_io_service_pool_conf);

  virtual ~client(){};

  void run();

  virtual bool Configure(const char *szConfigFile);

  virtual bool Configure(std::map<std::string,std::map<std::string,std::string> >& cmdParam);	

public:
  ///static  configType s_io_service_pool_conf;
  //static  configType s_timer_io_service_pool_conf;


private:

};

//template<typename ClientType>
//typename client<ClientType>::configType client<ClientType>::s_io_service_pool_conf;

//template<typename ClientType>
//typename client<ClientType>::configType client<ClientType>::s_timer_io_service_pool_conf;

template<typename ClientType>
client<ClientType>::client( typename const configType& io_service_pool_size_conf,
						    typename const configType& timer_io_service_pool_conf)
: facade_extension(io_service_pool_size_conf,timer_io_service_pool_conf)
{
}

template<typename ClientType>
client<ClientType>::client()
						   : facade_extension()
{

}


#if 0
template<int NetSpeedThreads , int TimerSpeedThreads >
client<NetSpeedThreads, TimerSpeedThreads>::~client()
{
	handle_stop();
}
#endif

template<typename ClientType>
void client<ClientType>::run()
{
	io_service_pools[E_ALLOC_IOS_NET_POOL]->run();
	io_service_pools[E_ALLOC_IOS_TIMER_POOL]->run();
	//io_service_pool_.run();
	//io_service_timer_pool_.run();
}

template<typename ClientType>
bool client<ClientType>::Configure( const char *szConfigFile)
{
	configType io_service_pool_conf;

	configType timer_io_service_pool_conf;

	{
		pugi::xml_document doc;
		pugi::xml_parse_result result = doc.load_file(szConfigFile);
		if(result)
		{

			
			pugi::xpath_node io_service_cfg_node = doc.select_single_node("/config/io_service[@name='network']");
			if(io_service_cfg_node)
			{

				io_service_pool_conf.io_service_pool_size	= 
					io_service_cfg_node.node().select_single_node("./io_service_pool_size").node().text().as_int(32);

				io_service_pool_conf.speed_threads_for_a_io_service	= 
					io_service_cfg_node.node().select_single_node("./speed_threads_for_a_io_service").node().text().as_int(9);

			}
			else
			{
				io_service_pool_conf.io_service_pool_size						= 32;
				io_service_pool_conf.speed_threads_for_a_io_service				= 9;
			}


			io_service_cfg_node = doc.select_single_node("/config/io_service[@name='timer']");
			if(io_service_cfg_node)
			{

				timer_io_service_pool_conf.io_service_pool_size	= 
					io_service_cfg_node.node().select_single_node("./io_service_pool_size").node().text().as_int(32);

				timer_io_service_pool_conf.speed_threads_for_a_io_service	= 
					io_service_cfg_node.node().select_single_node("./speed_threads_for_a_io_service").node().text().as_int(9);
			}
			else
			{
				timer_io_service_pool_conf.io_service_pool_size					= 32;
				timer_io_service_pool_conf.speed_threads_for_a_io_service		= 9;
			}

			init(io_service_pool_conf,timer_io_service_pool_conf);

			return true;
		}
		else
			return false;
	}
}


template<typename ClientType>
bool client<ClientType>::Configure( std::map<std::string,std::map<std::string,std::string> >& cmdParam)
{
	configType io_service_pool_conf;

	configType timer_io_service_pool_conf;

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

	std::string min_read_buffer_size	= cmdParam["host"]["min_read_buffer_size"];
	if(!min_read_buffer_size.empty())
		connection::s_min_read_buf_size =  boost::lexical_cast<int>(min_read_buffer_size);


	init(io_service_pool_conf,timer_io_service_pool_conf);

	return true;
}






} // namespace server
} // namespace http




typedef http::server::client<long> clientFacade;






#endif // HTTP_CLIENT_HPP
