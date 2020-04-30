/* System library. */
#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#ifdef	ACL_UNIX
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#endif

#include "stdlib/acl_sys_patch.h"
#include "stdlib/acl_msg.h"
#include "stdlib/acl_mystring.h"
#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_iostuff.h"
#include "net/acl_host_port.h"
#include "net/acl_valid_hostname.h"
#include "net/acl_listen.h"
#endif

ACL_SOCKET acl_inet_bind(const struct addrinfo *res, unsigned flag)
{
	ACL_SOCKET fd;
	int        on;

	fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (fd == ACL_SOCKET_INVALID) {
		acl_msg_error("%s(%d): create socket %s",
			__FILE__, __LINE__, acl_last_serror());
		return ACL_SOCKET_INVALID;
	}

	if (flag & ACL_INET_FLAG_EXCLUSIVE) {
#if defined(SO_EXCLUSIVEADDRUSE)
		on = 1;
		if (setsockopt(fd, SOL_SOCKET, SO_EXCLUSIVEADDRUSE,
			(const void *) &on, sizeof(on)) < 0) {

			acl_msg_warn("%s(%d): setsockopt(SO_EXCLUSIVEADDRUSE)"
				": %s", __FILE__, __LINE__, acl_last_serror());
		}
#endif
	} else {
		on = 1;
		if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
			(const void *) &on, sizeof(on)) < 0) {

			acl_msg_warn("%s(%d): setsockopt(SO_REUSEADDR): %s",
				__FILE__, __LINE__, acl_last_serror());
		}
	}

#if defined(SO_REUSEPORT)
	on = 1;
	if (flag & ACL_INET_FLAG_REUSEPORT) {
		int ret = setsockopt(fd, SOL_SOCKET, SO_REUSEPORT,
			(const void *) &on, sizeof(on));
		if (ret < 0)
			acl_msg_warn("%s(%d): setsocket(SO_REUSEPORT): %s",
				__FILE__, __LINE__, acl_last_serror());
	}
#else
	(void) flag;
#endif

#ifdef ACL_WINDOWS
	if (bind(fd, res->ai_addr, (int) res->ai_addrlen) < 0) {
#else
	if (bind(fd, res->ai_addr, res->ai_addrlen) < 0) {
#endif
		acl_socket_close(fd);
		acl_msg_error("%s(%d): bind error %s",
			__FILE__, __LINE__, acl_last_serror());
		return ACL_SOCKET_INVALID;
	}

	return fd;
}

#ifdef ACL_UNIX
ACL_SOCKET acl_unix_dgram_bind(const char *addr, unsigned flag)
{
	struct sockaddr_un sun;
	size_t len, size;
	char  *path = sun.sun_path;
	ACL_SOCKET sock;

	memset((char *) &sun, 0, sizeof(sun));
	sun.sun_family = AF_UNIX;
	size = sizeof(sun.sun_path);

#ifdef	ACL_LINUX
	/* for Linux abstract unix path, we should skip first '@' which was
	 * marked astract unix in the first of the path by acl.
	 */
	if (*addr == '@') {
		addr++;
		size--;
		*path++ = 0;
	}
	len = strlen(addr);
#else
	len = strlen(addr);
#endif

	/* Translate address information to internal form. */
	if (len >= size || len == 0) {
		acl_msg_error("%s(%d), %s: invalid addr len=%ld, unix path=%s",
			__FILE__, __LINE__, __FUNCTION__, (long) len, addr);
		return ACL_SOCKET_INVALID;
	}

#ifdef HAS_SUN_LEN
	sun.sun_len = len + 1;
#endif
	memcpy(path, addr, len + 1);

	/*
	 * Create a listener socket. Do whatever we can so we don't run into
	 * trouble when this process is restarted after crash.
	 */
	if ((sock = socket(AF_UNIX, SOCK_DGRAM, 0)) == ACL_SOCKET_INVALID) {
		acl_msg_error("%s: create socket error %s",
			__FUNCTION__, acl_last_serror());
		return ACL_SOCKET_INVALID;
	}

	if (path == sun.sun_path) {
		(void) unlink(addr);
	}

	size = sizeof(sun.sun_family) + strlen(addr) + 1;
	if (bind(sock, (struct sockaddr *) & sun, (socklen_t) size) < 0) {
		acl_msg_error("%s: bind %s error %s",
			__FUNCTION__, addr, acl_last_serror());
		acl_socket_close(sock);
		return ACL_SOCKET_INVALID;
	}

#ifdef FCHMOD_UNIX_SOCKETS
	if (path == sun.sun_path && fchmod(sock, 0666) < 0) {
		acl_msg_fatal("%s: fchmod socket %s: %s",
			__FUNCTION__, addr, acl_last_serror());
		acl_socket_close(sock);
		return ACL_SOCKET_INVALID;
	}
#else
	if (path == sun.sun_path && chmod(addr, 0666) < 0) {
		acl_msg_error("%s: chmod socket error %s, addr=%s",
			__FUNCTION__, acl_last_serror(), addr);
		acl_socket_close(sock);
		return ACL_SOCKET_INVALID;
	}
#endif
	acl_non_blocking(sock, flag & ACL_INET_FLAG_NBLOCK ?
		ACL_NON_BLOCKING : ACL_BLOCKING);

	return sock;
}
#endif

ACL_SOCKET acl_udp_bind(const char *addr, unsigned flag)
{
	return acl_udp_bind3(addr, flag, NULL);
}

ACL_SOCKET acl_udp_bind3(const char *addr, unsigned flag, int *family)
{
	struct addrinfo *res0, *res;
	ACL_SOCKET fd;

	if (family) {
		*family = 0;
	}

#ifdef ACL_UNIX
	if (!acl_valid_ipv4_hostaddr(addr, 0)
		&& !acl_valid_ipv6_hostaddr(addr, 0)) {

		fd = acl_unix_dgram_bind(addr, flag);
		if (fd >= 0 && family) {
			*family = AF_UNIX;
		}
		return fd;
	}
#endif

	res0 = acl_host_addrinfo(addr, SOCK_DGRAM);
	if (res0 == NULL) {
		acl_msg_error("%s(%d): host_addrinfo NULL, addr=%s",
			__FILE__, __LINE__, addr);
		return ACL_SOCKET_INVALID;
	}

	fd = ACL_SOCKET_INVALID;

	for (res = res0; res != NULL; res = res->ai_next) {
		fd = acl_inet_bind(res, flag);
		if (fd != ACL_SOCKET_INVALID) {
			if (family) {
				*family = res->ai_family;
			}
			break;
		}
	}

	freeaddrinfo(res0);

	if (fd == ACL_SOCKET_INVALID) {
		acl_msg_error("%s(%d): bind %s error %s",
			__FILE__, __LINE__, addr, acl_last_serror());
		return ACL_SOCKET_INVALID;
	}

	acl_non_blocking(fd, flag & ACL_INET_FLAG_NBLOCK ?
		ACL_NON_BLOCKING : ACL_BLOCKING);

	return fd;
}
