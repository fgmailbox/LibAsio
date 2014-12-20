#pragma once
#include <vector>
#include <map>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include "connection.hpp"
#include "ObjectHandle.h"

namespace http {
	namespace server {

class subscribe
	: //public boost::enable_shared_from_this<subscribe>,
	  public ObjectHandle<subscribe>
{
public:
	subscribe(void){};
	virtual ~subscribe(void){};

	virtual void on_accept(connection_weak_ptr conn,
		const boost::system::error_code& e) = 0;


	virtual void on_connect(connection_weak_ptr conn,
		const boost::system::error_code& e) = 0;



	virtual void on_read(connection_weak_ptr conn,
				const boost::system::error_code& e,
				std::size_t bytes_transferred,
				buffer_ptr buffer,
				long read_seq) = 0;

	virtual void on_write(connection_weak_ptr conn,
		const boost::system::error_code& e,
		std::size_t bytes_transferred,
		buffer_ptr bufferLeft,
		long write_seq) = 0;

	virtual void on_cancel(connection_weak_ptr conn, buffer_ptr cancel_buffer,long write_seq) = 0;


	virtual void on_error(connection_weak_ptr conn, const boost::system::error_code& e) = 0;

	virtual void on_warning(connection_weak_ptr conn,const std::string & warningMsg) = 0;

	virtual void on_close(long connID, const boost::system::error_code& e,std::string reason) = 0;
	
};

typedef boost::shared_ptr<subscribe> subscribe_ptr;

	} // namespace server
} // namespace http


