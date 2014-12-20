//
// io_service_pool.hpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "io_service_pool.hpp"
//#include "log.h"

namespace http {
namespace server {

	io_service_pool::io_service_pool(std::size_t io_service_pool_size,std::size_t a_io_service_speed_thread_size)
		: next_io_service_(0),
		  config_(io_service_pool_size,a_io_service_speed_thread_size),
		  bDestroying(false)
	{
		if (io_service_pool_size == 0 || a_io_service_speed_thread_size == 0)
			throw std::runtime_error("io_service_pool size is 0");

		// Give all the io_services work to do so that their run() functions will not
		// exit until they are explicitly stopped.
		for (std::size_t i = 0; i < io_service_pool_size; ++i)
		{
			io_service_ptr io_service(new boost::asio::io_service);
			work_ptr work(new boost::asio::io_service::work(*io_service));
			io_services_.push_back(io_service);
			work_.push_back(work);
		}
	}

	void io_service_pool::run()
	{

		// Create a pool of threads to run all of the io_services.
		for (std::size_t i = 0; i < io_services_.size(); ++i)
		{
			for (std::size_t j = 0; j < config_.second ; ++j)
			{
				//threads_.create_thread(boost::bind(&boost::asio::io_service::run, io_services_[i]));
				threads_.create_thread(boost::bind(&io_service_pool::io_service_run, this, io_services_[i]));
			}
		}
	}

	void io_service_pool::io_service_run(io_service_ptr io_service)
	{
		while(!bDestroying)
		{
			try
			{
				io_service->run();
			}
			catch (std::exception& e)
			{
//				LOG(error, "exception: %s", e.what() );
#if 0
				HeapCompact(GetProcessHeap(),0);
#endif			
			}
		}


	}


	void io_service_pool::stop()
	{
		bDestroying = true;
		// Explicitly stop all io_services.
		for (std::size_t i = 0; i < io_services_.size(); ++i)
			io_services_[i]->stop();

		threads_.join_all();
	}

	io_service_ptr io_service_pool::get_io_service()
	{

		// Use a round-robin scheme to choose the next io_service to use.
		long	curIdx = next_io_service_++;
		io_service_ptr io_service = io_services_[ curIdx % io_services_.size() ];
		return io_service;
	}









} // namespace server2
} // namespace http

