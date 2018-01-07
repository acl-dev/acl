#include "stdafx.h"
#include "fiber/lib_fiber.h"
#include "sane_socket.h"

int is_listen_socket(socket_t fd)
{
	int val, ret;
#ifdef SYS_WIN
	int len = sizeof(val);
#else
	socklen_t len = sizeof(val);
#endif

	ret = getsockopt(fd, SOL_SOCKET, SO_ACCEPTCONN, (void*) &val, &len);
	if (ret == -1) {
		return 0;
	} else if (val) {
		return 1;
	} else {
		return 0;
	}
}
