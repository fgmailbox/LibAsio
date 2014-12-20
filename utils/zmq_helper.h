#ifndef __ZHELPERS_H_INCLUDED__
#define __ZHELPERS_H_INCLUDED__
// Include a bunch of headers that we will need in the examples

// Bring Windows MSVC up to C99 scratch
#if (defined (_WINDOWS_))
typedef unsigned long ulong;
typedef unsigned int uint;
typedef __int64 int64_t;
#endif
// Provide random number from 0..(num-1)
#include <stdlib.h>
#include <string>

#define within(num) (int) ((float) (num) * rand () / (RAND_MAX + 1.0))

std::string zmq_helper_version (void);

std::string zmq_helper_recv (void *socket);

int zmq_helper_send (void *socket, char *string, int len = 0);

int zmq_helper_sendmore (void *socket, char *string, int len = 0);

void zmq_helper_dump (void *socket);

void zmq_helper_set_id (void *socket);

#endif