#include "stdafx.h"
#include "ClientEcho.h"
#include "Listener.h"

CListener::CListener(acl::server_socket& listener)
	: m_listener(listener)
{
}

CListener::~CListener(void)
{
}

void CListener::run(void)
{
	printf("listener fiber run ...\r\n");

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
			acl::fiber* fb = new CClientEcho(conn);
			fb->start();
		}
	}

	delete this;
}