//
// connection.cpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "timer.hpp"
#include <vector>
#include <boost/bind.hpp>
#include "timer_manager.hpp"
#include "facade.hpp"

namespace http {
namespace server {

timer::timer(io_service_ptr& io_service,
			 facade& facade)
  : timer_(new boost::asio::deadline_timer(*io_service)),
    strand_(*io_service),
	ios(io_service),
	waitCount_(0),
	//timeDuration_(new boost::posix_time::time_duration()),
	executeCount_(1),
	bDestroying_(false),
	bStoped_(false),
	facade_(facade)
{
#ifdef _DEBUG
	int len0 = sizeof(timer);
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

}

timer::~timer()
{
#if 0
	LOG(debug,"Timer [%d] Destroyed Total[%d] left",getObjectID(),getObjectCount());
#endif
	if(!bDestroying_)
	{
		boost::mutex::scoped_lock guard(lock);
		if(!bDestroying_)
		{
			bDestroying_ = true;
			stop();
		}
	}
}

void timer::start_from_now(timer_callback callback, boost::posix_time::time_duration timeDuration,int executeCount )
{
	timer_ptr tmp_holder = shared_from_this(); 

	timeDuration_ = timeDuration;
	
	executeCount_ = executeCount;
	attach(callback);
	timer_->expires_from_now(timeDuration);
	timer_->async_wait(
		boost::bind(&timer::on_timeout,shared_from_this()));

	//timer_manager_.add(tmp_holder);
	facade_.add(tmp_holder);
}

// 定时器的时间必须以 UTC 时间为准   
boost::posix_time::ptime timer::utc_time()
{
	return boost::posix_time::microsec_clock::universal_time();
}

boost::posix_time::ptime timer::local_time()
{
	return boost::posix_time::microsec_clock::local_time();
}



boost::posix_time::ptime timer::local_to_utc(boost::posix_time::ptime local_time_)
{
	return local_time_ + (utc_time() - local_time());
}



void timer::start_at(timer_callback callback, boost::posix_time::ptime timeStamp)
{

	timer_ptr tmp_holder = shared_from_this(); 

	attach(callback);
	timer_->expires_at(timeStamp);
	timer_->async_wait(
		boost::bind(&timer::on_timeout,shared_from_this()));
	//timer_manager_.add(tmp_holder);
	facade_.add(tmp_holder);

}


void timer::start_at_once(timer_callback callback)
{
	timer_ptr tmp_holder = shared_from_this(); 
	if(tmp_holder)
	{
		connection::async_callback func = boost::bind(callback,tmp_holder->getObjectID());
#if 1
		timer_->get_io_service().post(strand_.wrap(func));
#else
		timer_->get_io_service().post(func);
#endif
		//BOOST_ASSERT(_HEAPOK == _heapchk());

		facade_.remove(tmp_holder);

		//BOOST_ASSERT(_HEAPOK == _heapchk());


	}
}

// 订阅该定时器的事件
void timer::attach(timer_callback callback)
{
	if(callback)
	{
		++(waitCount_);
		fire_timeout_.connect(getObjectID(),callback);
	}
}

void timer::stop()
{
	if(bDestroying_)
	{
		//BOOST_ASSERT(_HEAPOK == _heapchk());

		timer_->cancel();
		//BOOST_ASSERT(_HEAPOK == _heapchk());

		fire_timeout_.disconnect(getObjectID());
		//BOOST_ASSERT(_HEAPOK == _heapchk());

		condition_.notify_all();
		//BOOST_ASSERT(_HEAPOK == _heapchk());

	}
	else
	{
		//BOOST_ASSERT(_HEAPOK == _heapchk());

		timer_ptr tmp_holder = shared_from_this(); 
		//BOOST_ASSERT(_HEAPOK == _heapchk());

		timer_->cancel();
		fire_timeout_.disconnect(getObjectID());
		condition_.notify_all();
		//BOOST_ASSERT(_HEAPOK == _heapchk());
		if(!facade_.bDestroying_)
			facade_.remove(tmp_holder);
	}
	//BOOST_ASSERT(_HEAPOK == _heapchk());

	bStoped_ = true;

}

void timer::on_timeout()
{

	{
		if(!bStoped_)
		{

			// executeCount_ 为负数的时候，永远执行下去  
			if(executeCount_ > 0)
				executeCount_--;

			//发出回调通知
			if(!fire_timeout_.empty())
				fire_timeout_(getObjectID());

			if(bStoped_)
			{
				return;
			}

			// 通知所有等待的线程
			{
				boost::mutex::scoped_lock guard(lock);
				condition_.notify_all();
			}


			if(executeCount_ != 0)
			{
				//再次调用
				timer_->expires_from_now(timeDuration_);
				timer_->async_wait(
					boost::bind(&timer::on_timeout,shared_from_this()));
			}
			else
			{
				//timer_manager_.stop(shared_from_this());
				facade_.stop(shared_from_this());
			}
		}
	}
}


void timer::wait(boost::posix_time::time_duration timeDuration)
{
		++(waitCount_);
		boost::mutex::scoped_lock guard(lock);
		condition_.timed_wait(guard, timeDuration);
}





} // namespace server
} // namespace http
