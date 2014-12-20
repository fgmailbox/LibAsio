//
// connection.hpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_TIMER_HPP
#define HTTP_TIMER_HPP

//#define BOOST_THREAD_USE_DLL

#define  BOOST_DATE_TIME_SOURCE
#define  BOOST_DATE_TIME_NO_LIB

#define  BOOST_ATOMIC_SOURCE
#define  BOOST_ATOMIC_NO_LIB


#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/function.hpp>
#include <boost/signals2.hpp>
#include <boost/date_time.hpp>
#include <boost/detail/atomic_count.hpp>
#include <boost/noncopyable.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include "ObjectHandle.h"




#include <vector>
#include <boost/bind.hpp>
//#include "timer_manager.hpp"
//#include "facade.hpp"




namespace http {
namespace server {

class timer_manager;
class timer;

typedef boost::shared_ptr<timer> timer_ptr;
typedef boost::weak_ptr<timer> timer_weak_ptr;
typedef boost::shared_ptr<boost::asio::io_service> io_service_ptr;

class facade;


/// Represents a single connection from a client.
class timer
	: public boost::enable_shared_from_this<timer>,
    public ObjectHandle<timer>,
    private boost::noncopyable
{
public:

	typedef boost::function<void (long timerID)> timer_callback;


  /// Construct a connection with the given io_service.
  explicit timer(io_service_ptr io_service,
				 facade& facade);

  ~timer();

  static boost::posix_time::ptime utc_time();

  static boost::posix_time::ptime local_time();

  static boost::posix_time::ptime local_to_utc(boost::posix_time::ptime local_time);


  void start_from_now(timer_callback callback, boost::posix_time::time_duration timeDuration,int executeCount = 1);

  void start_at(timer_callback callback, boost::posix_time::ptime timeStamp);

  void start_at_once(timer_callback callback);

  void wait(boost::posix_time::time_duration timeDuration);

  /// Stop all asynchronous operations associated with the connection.
  void stop();

  void attach(timer_callback callback);

public:

	volatile bool bStoped_;
	volatile bool bDestroying_;

//private:
	
	facade&	facade_;
	io_service_ptr	ios;




  /// Strand to ensure the connection's handlers are not called concurrently.
    boost::asio::io_service::strand		strand_;

	boost::detail::atomic_count		waitCount_;
	int									executeCount_;
	boost::posix_time::time_duration	timeDuration_;

	
	boost::condition_variable_any		condition_;
	boost::mutex						lock;
	//boost::detail::lightweight_mutex	lock;

	boost::scoped_ptr<boost::asio::deadline_timer>		timer_;

	boost::signals2::signal<void (long timerID)> fire_timeout_;

	void	on_timeout();


};


} // namespace server
} // namespace http

#endif // HTTP_TIMER_HPP
