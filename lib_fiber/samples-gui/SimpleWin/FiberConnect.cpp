#include "pch.h"
#include "FiberConnect.h"

CFiberConnect::CFiberConnect(const char* ip, int port, int count)
: ip_(ip)
, port_(port)
, count_(count)
{
}

CFiberConnect::~CFiberConnect(void) {}

bool CFiberConnect::Start(void)
{
	struct sockaddr_in in;
	in.sin_family = AF_INET;
	in.sin_port = htons(port_);
	if (inet_pton(AF_INET, ip_.c_str(), &in.sin_addr) == -1) {
		printf("invalid ip = % s\r\n", ip_.c_str());
		return false;
	}

	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) {
		return false;
	}

	if (connect(sock, (const sockaddr*)&in, sizeof(in)) == -1) {
		closesocket(sock);
		printf("connect %s:%d error\r\n", ip_.c_str(), port_);
		return false;
	}

	printf(">>>connect %s:%d ok\r\n", ip_.c_str(), port_);

	const char data[] = "hello world!\r\n";
	char buf[256];
	int i;
	for (i = 0; i < count_; i++) {
		int ret = send(sock, data, sizeof(data) - 1, 0);
		if (ret == -1) {
			break;
		}
		ret = recv(sock, buf, sizeof(buf), 0);
		if (ret <= 0) {
			break;
		}
		if (i % 1000 == 0) {
			printf(">>>current io=%d, thread=%d, fiber=%d\r\n",
				i, GetCurrentThreadId(), acl::fiber::self());
		}
	}

	closesocket(sock);
	printf(">>>All over count=%d, %d\r\n", count_, i);
	return true;
}
