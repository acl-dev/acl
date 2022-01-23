#include "stdafx.h"

#include "msg.h"
#include "sane_socket.h"
#include "iostuff.h"

void tcp_nodelay(socket_t fd, int onoff)
{
	const char *myname = "tcp_nodelay";
	int   on = onoff ? 1 : 0;
	int   n = getsockfamily(fd);

	if (n != AF_INET && n != AF_INET6) {
		return;
	}

	if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY,
		(char *) &on, sizeof(on)) < 0) {

		msg_error("%s(%d): set nodelay error(%s), onoff(%d)",
			myname, __LINE__, last_serror(), onoff);
	}
}
