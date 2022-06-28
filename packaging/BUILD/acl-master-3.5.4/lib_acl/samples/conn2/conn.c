#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>

int main(int argc, const char* argv[])
{
	if (argc != 4) {
		printf("usage: %s addr nic port\r\n", argv[0]);
		printf("for example: %s fe80::ca1f:66ff:fef6:c495 eth0 8887\r\n", argv[0]);
		return 1;
	}

	int port = atoi(argv[3]);

	struct sockaddr_in6 in6;
	memset(&in6, 0, sizeof(in6));

	in6.sin6_family   = AF_INET6;
	in6.sin6_port     = htons(port);
	in6.sin6_scope_id = if_nametoindex(argv[2]);
	if (inet_pton(AF_INET6, argv[1], &in6.sin6_addr) <= 0) {
		printf("inet_pton %s error %s\r\n", argv[1], strerror(errno));
		return 1;
	}

	int sock = socket(AF_INET6, SOCK_STREAM, 0);
	if (sock == -1) {
		printf("socket error %s\r\n", strerror(errno));
		return 1;
	}

	if (connect(sock, (const struct sockaddr *)& in6, sizeof(in6)) == -1) {
		printf("connect %s|%d error %s\r\n", argv[1], port, strerror(errno));
		return 1;
	}

	printf("connect %s|%d ok\r\n", argv[1], port);
	return 0;
}
