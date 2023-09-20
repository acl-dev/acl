#pragma once
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/un.h>
#include <vector>

typedef struct SOCK_ADDR {
	union {
		struct sockaddr_storage ss;
#ifdef AF_INET6
		struct sockaddr_in6 in6;
#endif
		struct sockaddr_in in;
#ifdef ACL_UNIX
		struct sockaddr_un un;
#endif
		struct sockaddr sa;
	} sa;
} SOCK_ADDR;

struct addrinfo *host_addrinfo(const char *addr);
