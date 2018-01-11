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

	socket_t lfd = m_listener.sock_handle();
	while (true)
	{
		socket_t sock = fiber_accept(lfd, NULL, NULL);
		if (sock == INVALID_SOCKET)
		{
			printf("accept error %s\r\n", acl::last_serror());
			break;
		}
		printf("accept one connection, sock=%d\r\n", sock);
		acl::socket_stream* conn = new acl::socket_stream;
		conn->open(sock);
		acl::fiber* fb = new CFiberClient(conn);
		fb->start();
	}

	fiber_close(lfd);

	printf("listening stopped!\r\n");
#if 0
	while (true)
	{
		acl_fiber_delay(1000);
		printf("wakeup now\r\n");
	}

	while (true)
	{
		acl::socket_stream* conn = m_listener.accept();
		if (conn)
		{
			printf("accept one\r\n");
			acl::fiber* fb = new CFiberClient(conn);
			fb->start();
		}
	}
#endif

	delete this;
}