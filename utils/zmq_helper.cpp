#include "ZeroMQ/zmq.h"
#include "ZeroMQ/zmq_utils.h"
#include "zmq_helper.h"
#include <boost/smart_ptr.hpp>
#include "log.h"


std::string zmq_helper_version (void)
{
	int major, minor, patch;
	zmq_version (&major, &minor, &patch);
	char buf[64];
	sprintf(buf,"0MQ version %d.%d.%d", major, minor, patch);
	return buf;
}


std::string zmq_helper_recv (void *socket) 
{
	zmq_msg_t message;
	zmq_msg_init (&message);
	if (zmq_recvmsg (socket, &message, 0) < 0)
		return "";
	
	int size = zmq_msg_size (&message);

	boost::scoped_array<char> string (new char[size + 1]);

	memcpy (string.get(), zmq_msg_data (&message), size);
	zmq_msg_close (&message);
	string [size] = 0;
	return (string.get());
}




// Convert C string to 0MQ string and send to socket
int zmq_helper_send (void *socket, char *string,int len) 
{
	int rc;
	zmq_msg_t message;
	zmq_msg_init_size (&message, strlen (string));
	memcpy (zmq_msg_data (&message), string, len > 0 ? len : strlen (string));
	rc = zmq_sendmsg (socket, &message, 0);
	zmq_msg_close (&message);
	return (rc);
}
// Sends string as 0MQ string, as multipart non-terminal
int zmq_helper_sendmore (void *socket, char *string, int len) 
{
	int rc;
	zmq_msg_t message;
	zmq_msg_init_size (&message, strlen (string));
	memcpy (zmq_msg_data (&message), string, len > 0 ? len : strlen (string));
	rc = zmq_sendmsg (socket, &message, ZMQ_SNDMORE);
	zmq_msg_close (&message);
	return (rc);
}
// Receives all message parts from socket, prints neatly
//
void zmq_helper_dump (void *socket)
{
	puts ("----------------------------------------");
	while (1) {
		// Process all parts of the message
		zmq_msg_t message;
		zmq_msg_init (&message);
		zmq_recvmsg (socket, &message, 0);
		// Dump the message as text or binary
		char *data = (char *)zmq_msg_data (&message);
		int size = zmq_msg_size (&message);
		int is_text = 1;
		int char_nbr;
		for (char_nbr = 0; char_nbr < size; char_nbr++)
			if ((unsigned char) data [char_nbr] < 32
				|| (unsigned char) data [char_nbr] > 127)
				is_text = 0;
		printf ("[%03d] ", size);
		for (char_nbr = 0; char_nbr < size; char_nbr++) {
			if (is_text)
				printf ("%c", data [char_nbr]);
			else
				printf ("%02X", (unsigned char) data [char_nbr]);
		}
		printf ("\n");
		int64_t more; // Multipart detection
		size_t more_size = sizeof (more);
		zmq_getsockopt (socket, ZMQ_RCVMORE, &more, &more_size);
		zmq_msg_close (&message);
		if (!more)
			break; // Last message part
	}
}
// Set simple random printable identity on socket
//
void zmq_helper_set_id (void *socket)
{
	char identity [10];
	sprintf (identity, "%04X-%04X", within (0x10000), within (0x10000));
	zmq_setsockopt (socket, ZMQ_IDENTITY, identity, strlen (identity));
}

