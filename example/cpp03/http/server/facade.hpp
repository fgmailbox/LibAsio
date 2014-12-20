//
// server.hpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_FACADE_HPP
#define HTTP_FACADE_HPP

#include <string>
#include <map>
#include <vector>
//#include <pair>

#include <boost/detail/lightweight_mutex.hpp>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/tuple/tuple.hpp>

#include "resource_manager.h"

#include "connection.hpp"
#include "timer.hpp"

#include "io_service_pool.hpp"
#include "Subscribe.h"
#include "Singleton2.h"

#include "subscribe_subject.h"

namespace http {
namespace server {

class facade
  : public subscribe_subject
{
public:
	typedef enum
	{
		E_ALLOC_IOS_NET_POOL	= 0,
		E_ALLOC_IOS_TIMER_POOL	= 1,
		E_ALLOC_IOS_ACCEPT_POOL	= 2,
	} AllocIOServicePoolType;



	class connection_manager
		: public resource_manager<connection>,
		private boost::noncopyable
	{
	public:

		void start(	connection_ptr c)
		{
			if(c)
			{
				add(c);
				c->start();
			}
		}


	};

	class timer_manager
		: public resource_manager<timer>,
		private boost::noncopyable
	{
	public:
		void start(timer_ptr c)
		{
			if(c)
				add(c);
		}

	};






  /// Construct the server to listen on the specified TCP address and port, and
  /// serve up files from the given directory.
  explicit facade();

  virtual ~facade();
  
  virtual void run() = 0;

  virtual connection_ptr	alloc_connection(connection::ConnectionType connType = connection::E_CLIENT_HANDLE,
											 int iosPoolType = E_ALLOC_IOS_NET_POOL) = 0 ;

  virtual timer_ptr			alloc_timer(int iosPoolType =  E_ALLOC_IOS_TIMER_POOL) = 0;

  virtual bool Configure(std::map<std::string,std::map<std::string,std::string> >& cmdParam) = 0;	


  virtual void		handle_stop();

  virtual void		stop_all();

  virtual void		start(connection_ptr conn);
  virtual void		start(timer_ptr t);
  virtual void		stop(connection_ptr conn);
  virtual void		stop(timer_ptr t);
  virtual void		add(connection_ptr conn);
  virtual void		add(timer_ptr t);
  virtual void		remove(connection_ptr conn);
  virtual void		remove(timer_ptr t);
 
  virtual void		remove_connection(long connID);
  virtual void		remove_timer(long tID);

  connection_ptr	get_connection(long connID);
  timer_ptr			get_timer(long timerID);


public:
	volatile bool bDestroying_;

protected:
	//boost::recursive_mutex	lock;
	boost::detail::lightweight_mutex lock;

	connection_manager connection_manager_;

	timer_manager timer_manager_;

	std::vector<boost::shared_ptr<io_service_pool> > io_service_pools;

	boost::shared_ptr<boost::asio::signal_set> signals_;



};


// NetSpeedThreads模板参数   用于指定一个用于网络io_service对应多少个加速线程 
// TimerSpeedThreads模板参数 用于指定一个用于定时器的io_service对应多少个加速线程 
class facade_extension
	: public facade
{
public:

	//typedef boost::tuple<std::size_t,std::size_t> configType;
	struct configType
	{
		configType(int io_service_pool_size_  = 1 ,int speed_threads_for_a_io_service_ = 1)
			: io_service_pool_size(io_service_pool_size_),
			  speed_threads_for_a_io_service(speed_threads_for_a_io_service_)
		{
		}
		int io_service_pool_size;
		int speed_threads_for_a_io_service;
	};

	facade_extension();

	explicit facade_extension(const configType& io_service_pool_size_conf,
							  const configType& timer_io_service_pool_conf);

	void init(const configType& io_service_pool_size_conf,
		const configType& timer_io_service_pool_conf);


	virtual ~facade_extension();

	virtual connection_ptr	alloc_connection(connection::ConnectionType connType = connection::E_CLIENT_HANDLE,
											 int iosPoolType = E_ALLOC_IOS_NET_POOL) ;

	virtual timer_ptr			alloc_timer(int iosPoolType =  E_ALLOC_IOS_TIMER_POOL);


	//virtual void		handle_stop();

	virtual void		stop_all();


protected:



};





} // namespace server
} // namespace http

#endif // HTTP_FACADE_HPP
