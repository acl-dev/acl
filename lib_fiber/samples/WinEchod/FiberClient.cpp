#include "stdafx.h"
#include "FiberClient.h"

CFiberClient::CFiberClient(acl::socket_stream* conn)
	: m_conn(conn)
{
}

CFiberClient::~CFiberClient(void)
{
}

void CFiberClient::run(void)
{
	socket_t sock = m_conn->sock_handle();
	while (true)
	{
		char buf[1024];
		int ret = fiber_recv(sock, buf, sizeof(buf) - 1, 0);
		if (ret <= 0)
		{
			int err0 = acl_fiber_last_error();
			int err1 = acl::last_error();
			printf("recv error: %s, %d, %d\r\n",
				acl::last_serror(), err0, err1);
			break;
		}
		buf[ret] = 0;
		//printf("recv=%d, [%s]\r\n", ret, buf);
		if (fiber_send(sock, buf, ret, 0) == -1)
		{
			printf("write error %s\r\n", acl::last_serror());
			break;
		}
	}

	fiber_close(sock);
	m_conn->unbind();
	delete m_conn;
	delete this;
#if 0
	acl::string buf;
	while (true)
	{
		if (m_conn->gets(buf) == false)
		{
			printf("gets error\r\n");
			break;
		}
		if (m_conn->write(buf) == -1)
		{
			printf("write error\r\n");
			break;
		}
	}
	delete m_conn;
	delete this;
#endif
}
