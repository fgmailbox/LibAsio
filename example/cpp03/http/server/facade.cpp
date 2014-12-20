//
// server.cpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/bind.hpp>
#include <signal.h>
#include "Singleton2.h"
#include "facade.hpp"

namespace http {
namespace server {


facade::facade()
  : connection_manager_(),
	timer_manager_(),
	bDestroying_(false)
{
}

facade::~facade()
{
#if 0
	handle_stop();
#endif
}


void facade::stop_all()
{
	timer_manager_.stop_all();
	connection_manager_.stop_all();

	io_service_pools[E_ALLOC_IOS_TIMER_POOL]->stop();
	io_service_pools[E_ALLOC_IOS_NET_POOL]->stop();

}



void facade::handle_stop()
{
	if(!bDestroying_)
	{
		//boost::recursive_mutex::scoped_lock guard(lock);
		boost::detail::lightweight_mutex::scoped_lock guard(lock);
		if(!bDestroying_)
		{
			bDestroying_ = true;
			stop_all();
		}
	}
}


void	facade::start(connection_ptr conn)
{
	connection_manager_.start(conn);
}
void	facade::start(timer_ptr t)
{
	timer_manager_.start(t);
}
void	facade::stop(connection_ptr conn)
{
	connection_manager_.stop(conn);

}
void	facade::stop(timer_ptr t)
{
	timer_manager_.stop(t);
}


void	facade::add(connection_ptr conn)
{
	connection_manager_.add(conn);

}
void	facade::add(timer_ptr t)
{
	timer_manager_.add(t);
}

void	facade::remove(connection_ptr conn)
{
	connection_manager_.remove(conn);

}
void	facade::remove(timer_ptr t)
{
	timer_manager_.remove(t);
}

connection_ptr	facade::get_connection(long connID)
{
	return connection_manager_.get(connID);
}

timer_ptr	facade::get_timer(long timerID)
{
	return timer_manager_.get(timerID);
}

void	facade::remove_connection(long connID)
{
	connection_manager_.remove_by_id(connID);
}
void	facade::remove_timer(long tID)
{
	timer_manager_.remove_by_id(tID);

}

facade_extension::facade_extension()
{

}


void facade_extension::init(	const configType& io_service_pool_size_conf,	const configType& timer_io_service_pool_conf )
{

	if(io_service_pools.size() == 0)
	{
		io_service_pools.push_back(boost::make_shared<io_service_pool>(io_service_pool_size_conf.io_service_pool_size,
			io_service_pool_size_conf.speed_threads_for_a_io_service));

		io_service_pools.push_back(boost::make_shared<io_service_pool>(timer_io_service_pool_conf.io_service_pool_size,
			timer_io_service_pool_conf.speed_threads_for_a_io_service));

		signals_ = boost::make_shared<boost::asio::signal_set>(boost::ref(*io_service_pools[E_ALLOC_IOS_NET_POOL]->get_io_service()));

		signals_->add(SIGINT);
		signals_->add(SIGTERM);
	#if defined(SIGQUIT)
		signals_->add(SIGQUIT);
	#endif // defined(SIGQUIT)
		signals_->async_wait(boost::bind(&facade::handle_stop, this));
	}
}




facade_extension::facade_extension(	const configType& io_service_pool_size_conf,	const configType& timer_io_service_pool_conf )
{
	init(io_service_pool_size_conf,timer_io_service_pool_conf);
}

facade_extension::~facade_extension()
{
	handle_stop();
}



void facade_extension::stop_all()
{
/*
	timer_manager_.stop_all();
	connection_manager_.stop_all();
*/
	__super::stop_all();

}


connection_ptr facade_extension::alloc_connection(connection::ConnectionType connType,int iosPoolType)
{
	int ios_size = io_service_pools.size();
	connection_ptr tmp_connection =  boost::make_shared<connection>(boost::ref(io_service_pools[iosPoolType % ios_size]->get_io_service()),
																	boost::ref(io_service_pools[iosPoolType % ios_size]->get_io_service()),
																	boost::ref(*this),connType);

	if(!tmp_connection )
		throw std::runtime_error("alloc_connection failed");

	return tmp_connection;
}

timer_ptr facade_extension::alloc_timer(int iosPoolType)
{
	int ios_size = io_service_pools.size();

	timer_ptr tmp_timer = boost::make_shared<timer>(
														boost::ref(io_service_pools[iosPoolType % ios_size]->get_io_service()),
														boost::ref(*this)
													);
#ifdef _DEBUG
	int len0 = sizeof(timer);
	int len = sizeof(http::server::timer);
	int len1 = sizeof(boost::enable_shared_from_this<http::server::timer>);
	int len2 = sizeof(ObjectHandle<http::server::timer>);
	int len3 = sizeof(boost::noncopyable);
	int len4 = offsetof(http::server::timer,bStoped_);
	int len5 = offsetof(http::server::timer,strand_);
	int len5_1 = offsetof(http::server::timer,condition_);
	int len6 = offsetof(http::server::timer,fire_timeout_);

#endif
	if(!tmp_timer)
		throw std::runtime_error("alloc_timer failed");

	//timer_manager_.start(tmp_timer);
	start(tmp_timer);

	return tmp_timer;
}










} // namespace server
} // namespace http
