#include "pch.h"
#include "Resource.h"
#include "WinFiberDlg.h"
#include "afxdialogex.h"
#include "FiberConnect.h"

CFiberConnect::CFiberConnect(CWinFiberDlg& hWin, const char* serverAddr, int count)
: m_hWin(hWin)
, m_serverAddr(serverAddr)
, m_count(count)
{
}

CFiberConnect::~CFiberConnect(void)
{
}

void CFiberConnect::run(void)
{
#if 0
	acl::string serverAddr(m_serverAddr);
	char *addr = serverAddr.c_str();
	char *port_s = strchr(addr, ':');

	ASSERT(port_s && *(port_s + 1));
	*port_s++ = 0;

	struct sockaddr_in sa;
	int len = sizeof(sa);
	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port   = htons(atoi(port_s));
	sa.sin_addr.s_addr = inet_addr(addr);

	socket_t sock = acl_fiber_socket(AF_INET, SOCK_STREAM, 0);

	if (acl_fiber_connect(sock, (const struct sockaddr*) &sa, len) < 0)
		printf("connect %s error %s\r\n", m_serverAddr.c_str(),
			acl::last_serror());
	else
		doEcho(sock);

	acl_fiber_close(m_sock);
#else
	acl::socket_stream conn;
	if (!conn.open(m_serverAddr, 2, 0)) {
		printf("connect %s error %s\r\n", m_serverAddr.c_str(),
			acl::last_serror());
	} else {
		doEcho(conn);
	}
#endif

	m_hWin.OnFiberConnectExit(this);
}

void CFiberConnect::doEcho(socket_t sock)
{
	char buf[1024];
	const char* s = "hello world\r\n";

	int i;
	for (i = 0; i < m_count; i++) {
		if (acl_fiber_send(sock, s, (int) strlen(s), 0) < 0) {
			printf("%s(%d): send error %s\r\n",
				__FILE__, __LINE__, acl::last_serror());
			break;
		}
		int n = acl_fiber_recv(sock, buf, sizeof(buf) - 1, 0);
		if (n <= 0) {
			printf("%s(%d): read error %s\r\n",
				__FILE__, __LINE__, acl::last_serror());
			break;
		}
		buf[n] = 0;
		printf(">>read=%s\r\n", buf);
	}
	printf("Echo over, fd=%u, total=%d, count=%d\r\n", (int) sock, m_count, i);
}

void CFiberConnect::doEcho(acl::socket_stream& conn)
{
	char buf[1024];
	const char* s = "hello world\r\n";

	int delay = 100;
	printf("Begin sleep %d ms\r\n", delay);
	acl::fiber::delay(delay);
	printf("Begin do echo ...\r\n");

	int i;
	for (i = 0; i < m_count; i++) {
		if (conn.write(s, strlen(s)) == -1) {
			printf("%s(%d): write error %s\r\n",
				__FILE__, __LINE__, acl::last_serror());
			break;
		}
		int n = conn.read(buf, sizeof(buf) - 1, false);
		if (n == -1) {
			printf("%s(%d): read error %s\r\n",
				__FILE__, __LINE__, acl::last_serror());
			break;
		}
		buf[n] = 0;
		//printf(">>read=%s\r\n", buf);
	}

	printf("%s(%d): echo over, fd=%u, total=%d, count=%d\r\n",
		__FILE__, __LINE__, (int) conn.sock_handle(), m_count, i);
}
