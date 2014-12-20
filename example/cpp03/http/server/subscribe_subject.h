#pragma once

#include <boost/signals2.hpp>
#include "Subscribe.h"

namespace http {
	namespace server {


class subscribe_subject
{
public:
	typedef enum 
	{
		Err_Socket,
		Err_Timer,
		Err_File,
	} ErrorCategoryType;



	subscribe_subject(void);

	virtual ~subscribe_subject(void);

	//template<typename DerivedPtr>
	//void	attach_subscriber(boost::shared_ptr<DerivedPtr> s);

	template<typename Derived>
	void attach_subscriber(boost::shared_ptr<Derived> s);

	template<typename Derived, typename ExtContext>
	void attach_subscriber(boost::shared_ptr<Derived> s, ExtContext context);



	void	detach_subscriber(subscribe_ptr s);

	boost::signals2::signal<void (connection_weak_ptr conn,
		const boost::system::error_code& e,
		std::size_t bytes_transferred,
		buffer_ptr buffer,
		long read_seq) > fire_read_;

	boost::signals2::signal<void (connection_weak_ptr conn,
		const boost::system::error_code& e,
		std::size_t bytes_transferred,
		buffer_ptr bufferLeft,
		long write_seq) > fire_write_;

	boost::signals2::signal<  void (connection_weak_ptr conn,
		const boost::system::error_code& e) > fire_connect_;

	boost::signals2::signal<  void (connection_weak_ptr conn,
		const boost::system::error_code& e) > fire_accept_;

	boost::signals2::signal<  void (connection_weak_ptr conn, buffer_ptr cancel_buffer, long write_seq) > fire_cancel_;

	boost::signals2::signal<  void (connection_weak_ptr conn, const boost::system::error_code& e) > fire_error_;

	boost::signals2::signal<  void (long connID,const boost::system::error_code& e,std::string reason) > fire_close_;

	boost::signals2::signal<  void (connection_weak_ptr conn,const std::string & warningMsg) > fire_warning_;

};

template<typename Derived>
void subscribe_subject::attach_subscriber(boost::shared_ptr<Derived> s)
{
	if(s)
	{
		fire_accept_.connect(s->getObjectID(),boost::bind(&Derived::on_accept,s,_1,_2));
		fire_connect_.connect(s->getObjectID(),boost::bind(&Derived::on_connect,s,_1,_2));
		fire_read_.connect(s->getObjectID(),boost::bind(&Derived::on_read,s,_1,_2,_3,_4, _5));
		fire_write_.connect(s->getObjectID(),boost::bind(&Derived::on_write,s,_1,_2,_3,_4, _5));


		fire_cancel_.connect(s->getObjectID(),boost::bind(&Derived::on_cancel,s,_1,_2, _3));
		fire_error_.connect(s->getObjectID(),boost::bind(&Derived::on_error,s,_1,_2));
		fire_warning_.connect(s->getObjectID(),boost::bind(&Derived::on_warning,s,_1,_2));
		fire_close_.connect(s->getObjectID(),boost::bind(&Derived::on_close,s,_1,_2,_3));

	}
}


template<typename Derived, typename ExtContext>
void subscribe_subject::attach_subscriber(boost::shared_ptr<Derived> s, ExtContext context)
{
	if(s)
	{
		fire_accept_.connect(s->getObjectID(),boost::bind(&Derived::on_accept,s,_1,_2,context));
		fire_connect_.connect(s->getObjectID(),boost::bind(&Derived::on_connect,s,_1,_2,context));
		fire_read_.connect(s->getObjectID(),boost::bind(&Derived::on_read,s,_1,_2,_3,_4, _5, context));
		fire_write_.connect(s->getObjectID(),boost::bind(&Derived::on_write,s,_1,_2,_3,_4, _5,context));
		
		fire_cancel_.connect(s->getObjectID(),boost::bind(&Derived::on_cancel,s,_1,_2, _3,context));
		fire_error_.connect(s->getObjectID(),boost::bind(&Derived::on_error,s,_1,_2,context));
		fire_warning_.connect(s->getObjectID(),boost::bind(&Derived::on_warning,s,_1,_2,context));
		fire_close_.connect(s->getObjectID(),boost::bind(&Derived::on_close,s,_1,_2,_3,context));

	}
}




} // namespace server
} // namespace http
