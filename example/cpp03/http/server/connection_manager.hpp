//
// connection_manager.hpp
// ~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_CONNECTION_MANAGER_HPP
#define HTTP_CONNECTION_MANAGER_HPP

//#include <map>
#include <boost/noncopyable.hpp>
//#include <boost/thread.hpp>
#include "connection.hpp"
#include "resource_manager.h"

namespace http {
namespace server {

/// Manages open connections so that they may be cleanly stopped when the server
/// needs to shut down.
class connection_manager
  : public resource_manager<connection>,
    private boost::noncopyable
{
public:

	void start(connection_ptr c)
	{
		if(c)
		{
			add(c);
			c->start();
		}
	}


};

} // namespace server
} // namespace http

#endif // HTTP_CONNECTION_MANAGER_HPP
