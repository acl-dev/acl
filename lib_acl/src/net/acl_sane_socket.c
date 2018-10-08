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

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "net/acl_sane_inet.h"
#include "net/acl_sane_socket.h"

#endif

int acl_getpeername(ACL_SOCKET fd, char *buf, size_t size)
{
	ACL_SOCKADDR addr;
	struct sockaddr *sa = (struct sockaddr*) &addr;
	socklen_t len = sizeof(addr);

	if (fd == ACL_SOCKET_INVALID || buf == NULL || size <= 0) {
		return -1;
	}

	memset(&addr, 0, sizeof(addr));

	if (getpeername(fd, sa, &len) == -1) {
		return -1;
	}

#ifdef	ACL_UNIX
	if (sa->sa_family == AF_UNIX) {
		memset(&addr, 0, sizeof(addr));
		len = sizeof(addr);

		if (getsockname(fd, sa, &len) == -1) {
			return -1;
		}
	}
#endif

	if (acl_inet_ntop(sa, buf, size) > 0) {
		return 0;
	} else {
		return -1;
	}
}

int acl_getsockname(ACL_SOCKET fd, char *buf, size_t size)
{
	ACL_SOCKADDR addr;
	struct sockaddr *sa = (struct sockaddr*) &addr;
	socklen_t len = sizeof(addr);

	if (fd == ACL_SOCKET_INVALID || buf == NULL || size <= 0) {
		return -1;
	}

	memset(&addr, 0, sizeof(addr));

	if (getsockname(fd, sa, &len) == -1) {
		return -1;
	}

	if (acl_inet_ntop(sa, buf, size) > 0) {
		return 0;
	} else {
		return -1;
	}
}

int acl_getsocktype(ACL_SOCKET fd)
{
	ACL_SOCKADDR addr;
	struct sockaddr *sa = (struct sockaddr*) &addr;
	socklen_t len = sizeof(addr);

	if (fd == ACL_SOCKET_INVALID) {
		return -1;
	}

	if (getsockname(fd, sa, &len) == -1) {
		return -1;
	}

#ifdef	ACL_UNIX
	if (sa->sa_family == AF_UNIX) {
		return AF_UNIX;
	}
#endif

#ifdef AF_INET6
	if (sa->sa_family == AF_INET || sa->sa_family == AF_INET6) {
#else
	if (sa->sa_family == AF_INET) {
#endif
		return sa->sa_family;
	}
	return -1;
}

int acl_check_socket(ACL_SOCKET fd)
{
	int val, ret;
	socklen_t len = sizeof(val);

	ret = getsockopt(fd, SOL_SOCKET, SO_ACCEPTCONN, (void*) &val, &len);
	if (ret == -1) {
		return -1;
	} else if (val) {
		return 1;
	} else {
		return 0;
	}
}

int acl_is_listening_socket(ACL_SOCKET fd)
{
	return acl_check_socket(fd) == 1;
}
