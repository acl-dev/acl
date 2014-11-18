#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#ifdef  HP_UX
#define _XOPEN_SOURCE
#define _XOPEN_SOURCE_EXTENDED  1
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef	ACL_UNIX
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#endif

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "net/acl_sane_inet.h"
#include "net/acl_sane_socket.h"

#endif

#ifdef	WIN32
struct SOCK_ADDR {
	union {
		struct sockaddr_in in;
	} sa;
};
#else
struct SOCK_ADDR {
	union {
		struct sockaddr_in in;
		struct sockaddr_un un;
		struct sockaddr sa;
	} sa;
};
#endif

int acl_getpeername(ACL_SOCKET sockfd, char *buf, size_t size)
{
	struct SOCK_ADDR addr;
	struct sockaddr *sa = (struct sockaddr*) &addr;
	socklen_t len = sizeof(addr);
	char  ip[32];
	int   port;

	if (sockfd == ACL_SOCKET_INVALID || buf == NULL || size <= 0)
		return -1;

	memset(&addr, 0, sizeof(addr));
	if (getpeername(sockfd, sa, &len) == -1)
		return -1;

#ifndef	WIN32
	if (sa->sa_family == AF_UNIX) {
		memset(&addr, 0, sizeof(addr));
		len = sizeof(addr);

		if (getsockname(sockfd, sa, &len) == -1)
			return -1;

		snprintf(buf, size, "%s", addr.sa.un.sun_path);
		return 0;
	} else
#endif
	if (sa->sa_family != AF_INET)
		return -1;
	if (acl_inet_ntoa(addr.sa.in.sin_addr, ip, sizeof(ip)) == NULL)
		return -1;
	port = ntohs(addr.sa.in.sin_port);
	snprintf(buf, size, "%s:%d", ip, port);
	return 0;
}

int acl_getsockname(ACL_SOCKET sockfd, char *buf, size_t size)
{
	struct SOCK_ADDR addr;
	struct sockaddr *sa = (struct sockaddr*) &addr;
	socklen_t len = sizeof(addr);
	char  ip[32];
	int   port;

	if (sockfd == ACL_SOCKET_INVALID || buf == NULL || size <= 0)
		return -1;

	memset(&addr, 0, sizeof(addr));

	if (getsockname(sockfd, sa, &len) == -1)
		return -1;

#ifndef	WIN32
	if (sa->sa_family == AF_UNIX) {
		snprintf(buf, size, "%s", addr.sa.un.sun_path);
		return 0;
	} else
#endif
	if (sa->sa_family != AF_INET)
		return -1;
	if (acl_inet_ntoa(addr.sa.in.sin_addr, ip, sizeof(ip)) == NULL)
		return -1;
	port = ntohs(addr.sa.in.sin_port);
	snprintf(buf, size, "%s:%d", ip, port);
	return 0;
}

int acl_getsocktype(ACL_SOCKET sockfd)
{
	struct SOCK_ADDR addr;
	struct sockaddr *sa = (struct sockaddr*) &addr;
	socklen_t len = sizeof(addr);

	if (sockfd == ACL_SOCKET_INVALID)
		return -1;

	if (getsockname(sockfd, sa, &len) == -1)
		return -1;

#ifndef	WIN32
	if (sa->sa_family == AF_UNIX)
		return AF_UNIX;
#endif
	if (sa->sa_family == AF_INET)
		return AF_INET;
	return -1;
}
