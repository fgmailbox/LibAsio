#include "subscribe_subject.h"

namespace http {
	namespace server {


subscribe_subject::subscribe_subject(void)
{
}

subscribe_subject::~subscribe_subject(void)
{
}


void subscribe_subject::detach_subscriber(subscribe_ptr s)
{
	if(s)
	{
		fire_accept_.disconnect(s->getObjectID());
		fire_connect_.disconnect(s->getObjectID());
		fire_read_.disconnect(s->getObjectID());
		fire_write_.disconnect(s->getObjectID());


		fire_cancel_.disconnect(s->getObjectID());
		fire_error_.disconnect(s->getObjectID());
		fire_warning_.disconnect(s->getObjectID());
		fire_close_.disconnect(s->getObjectID());


	}
}


	} // namespace server
} // namespace http
