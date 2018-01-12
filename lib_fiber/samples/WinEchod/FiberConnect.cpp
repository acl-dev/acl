#include "stdafx.h"
#include "FiberConnect.h"

CFiberConnect::CFiberConnect(UINT count)
	: m_count(count)
	, m_sock(INVALID_SOCKET)
{
}

CFiberConnect::~CFiberConnect(void)
{
	if (m_sock != INVALID_SOCKET)
		acl_fiber_close(m_sock);
}

void CFiberConnect::run(void)
{
	struct sockaddr_in sa;
	int len = sizeof(sa);
	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port   = htons(m_serverPort);
	sa.sin_addr.s_addr = inet_addr(m_serverIP.GetString());

	m_sock = acl_fiber_socket(AF_INET, SOCK_STREAM, 0);
	if (acl_fiber_connect(m_sock, (const struct sockaddr*) &sa, len) < 0)
		printf("connect %s:%d error %s\r\n", m_serverIP.GetString(),
			m_serverPort, acl::last_serror());
	else
		doEcho();
	delete this;
}

void CFiberConnect::doEcho(void)
{
	char buf[1024];
	const char* s = "hello world\r\n";

	for (UINT i = 0; i < m_count; i++)
	{
		if (acl_fiber_send(m_sock, s, strlen(s), 0) < 0)
		{
			printf("send error %s\r\n", acl::last_serror());
			break;
		}
		int n = acl_fiber_recv(m_sock, buf, sizeof(buf) - 1, 0);
		if (n <= 0)
		{
			printf("read error %s\r\n", acl::last_serror());
			break;
		}
	}
}
