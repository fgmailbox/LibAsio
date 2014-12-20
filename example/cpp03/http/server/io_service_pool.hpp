//
// io_service_pool.hpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_SERVER2_IO_SERVICE_POOL_HPP
#define HTTP_SERVER2_IO_SERVICE_POOL_HPP

#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>
#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/atomic.hpp>

namespace http {
namespace server {

typedef boost::shared_ptr<boost::asio::io_service> io_service_ptr;
typedef boost::shared_ptr<boost::asio::io_service::work> work_ptr;

// SpeedThread 为每个io_service的加速线程数 
class io_service_pool
  : private boost::noncopyable
{
public:
  /// Construct the io_service pool.
  explicit io_service_pool(std::size_t io_service_pool_size = 1, 
						   std::size_t a_io_service_speed_thread_size = 1);

  /// Run all io_service objects in the pool.
  void run();

  /// Stop all io_service objects in the pool.
  void stop();

  /// Get an io_service to use.
  io_service_ptr get_io_service();

private:

  bool bDestroying;


  boost::thread_group							threads_;
  //std::vector<boost::shared_ptr<boost::thread> > threads_;

  /// The pool of io_services.
  std::vector<io_service_ptr> io_services_;

  /// The work that keeps the io_services running.
  std::vector<work_ptr> work_;

  /// The next io_service to use for a connection.
  //boost::mutex	lock_;
  boost::atomic<long> next_io_service_;
  //volatile std::size_t next_io_service_;
  std::pair<std::size_t,std::size_t> config_;

private:

  void io_service_run(io_service_ptr ios);

};





 









} // namespace server2
} // namespace http

#endif // HTTP_SERVER2_IO_SERVICE_POOL_HPP
