#include "stdafx.h"
#include "FiberClient.h"
#include "FiberListener.h"

CFiberListener::CFiberListener(acl::server_socket& listener)
: m_listener(listener)
{
}

CFiberListener::~CFiberListener(void)
{
}

void CFiberListener::run(void)
{
	printf("listener fiber run ...\r\n");
	printf("Listener sleep 1000 ms\r\n");
	acl_fiber_delay(1000);
	printf("Listener wakeup now\r\n");

#if 0
	socket_t lfd = m_listener.sock_handle();
	int n = 0;
	while (true) {
		socket_t sock = acl_fiber_accept(lfd, NULL, NULL);
		if (sock == INVALID_SOCKET) {
			printf("accept error %s\r\n", acl::last_serror());
			break;
		}
		//printf("accept one connection, sock=%d, n=%d\r\n", sock, ++n);
		acl::socket_stream* conn = new acl::socket_stream;
		conn->open(sock);
		acl::fiber* fb = new CFiberClient(conn);
		fb->start();
	}

	acl_fiber_close(lfd);

	printf("listening stopped!\r\n");
#else
	while (true) {
		acl::socket_stream* conn = m_listener.accept();
		if (conn) {
			printf("accept one, fd=%u\r\n", conn->sock_handle());
			acl::fiber* fb = new CFiberClient(conn);
			fb->start();
		} else {
			printf("accept error=%s\r\n", acl::last_serror());
			break;
		}
	}
#endif

	delete this;
}
