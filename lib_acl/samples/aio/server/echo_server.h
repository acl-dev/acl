#ifndef	__AIO_ECHO_SERVER_INCLUDE_H__
#define	__AIO_ECHO_SERVER_INCLUDE_H__

#include "lib_acl.h"

void echo_server_log_fn(void (*write_fn)(void *, const char *fmt, ...),
	void *write_arg, void (*fflush_fn)(void *), void *fflush_arg);
void echo_server_init(char *data, int dlen, int echo_src, int line_length);
ACL_AIO *echo_server_start(ACL_VSTREAM *sstream, int accept_auto, int event_kernel);

#endif

