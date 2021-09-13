#include "pch.h"
#include "FiberClient.h"
#include "FiberServer.h"

CFiberServer::CFiberServer(SOCKET sock)
	: m_sock(sock)
{
}

CFiberServer::~CFiberServer(void)
{
	closesocket(m_sock);
}

void CFiberServer::run(void)
{
	while (true) {
		SOCKET s = accept(m_sock, NULL, NULL);
		if (s == INVALID_SOCKET) {
			break;
		}

		acl::fiber* fb = new CFiberClient(s);
		fb->start();
	}

	delete this;
}
