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
#if 0
	socket_t sock = m_conn->sock_handle();
	while (true) {
		char buf[1024];
		int ret = acl_fiber_recv(sock, buf, sizeof(buf) - 1, 0);
		if (ret <= 0) {
			int err0 = acl_fiber_last_error();
			int err1 = acl::last_error();
			printf("recv error: %s, %d, %d\r\n",
				acl::last_serror(), err0, err1);
			break;
		}
		buf[ret] = 0;
		//printf("recv=%d, [%s]\r\n", ret, buf);
		if (acl_fiber_send(sock, buf, ret, 0) == -1) {
			printf("write error %s\r\n", acl::last_serror());
			break;
		}
	}

	acl_fiber_close(sock);
	m_conn->unbind();
	delete m_conn;
	delete this;
#else
	acl::string buf;
	int n = 0;
	m_conn->set_rw_timeout(10);
	while (true) {
		if (m_conn->read(buf, false) == false) {
			printf("read error %s, count=%d\r\n",
				acl::last_serror(), n);
			break;
		}
		//printf("read: %s\r\n", buf.c_str());
		if (m_conn->write(buf) == -1) {
			printf("write error %s\r\n", acl::last_serror());
			break;
		}
		n++;
	}
	delete m_conn;
	printf("curr id=%u, %u\r\n", get_id(), acl::fiber::self());
	delete this;
#endif
}
