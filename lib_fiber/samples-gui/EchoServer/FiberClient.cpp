#include "pch.h"
#include "FiberClient.h"

CFiberClient::CFiberClient(SOCKET s)
: m_sock(s)
{
}

CFiberClient::~CFiberClient(void)
{
	closesocket(m_sock);
}

void CFiberClient::run(void)
{
	char buf[8192];
	while (true) {
		int ret = recv(m_sock, buf, sizeof(buf), 0);
		if (ret <= 0) {
			break;
		}

		if (send(m_sock, buf, ret, 0) == -1) {
			break;
		}
	}

	delete this;
}
