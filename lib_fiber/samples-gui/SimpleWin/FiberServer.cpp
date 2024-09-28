#include "pch.h"
#include "FiberClient.h"
#include "FiberServer.h"

CFiberServer::CFiberServer(bool autoDestroy /* = true */)
: m_sock(INVALID_SOCKET)
, m_autoDestroy(autoDestroy)
{
}

CFiberServer::~CFiberServer(void)
{
	if (m_sock != INVALID_SOCKET) {
		closesocket(m_sock);
	}
}

bool CFiberServer::BindAndListen(int port, const std::string& addr)
{
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) {
		return false;
	}

	struct sockaddr_in in;
	in.sin_family = AF_INET;
	in.sin_port = htons(port);
	if (inet_pton(AF_INET, addr.c_str(), &in.sin_addr) == -1) {
		closesocket(sock);
		return false;
	}

	if (bind(sock, (const sockaddr*)&in, sizeof(in)) == -1) {
		closesocket(sock);
		return false;
	}

	if (listen(sock, 128) == -1) {
		return false;
	}

	m_sock = sock;
	return true;
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

	if (m_autoDestroy) {
		delete this;
	}
}
