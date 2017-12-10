/* System library. */
#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#ifdef	ACL_UNIX
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#endif

#include "stdlib/acl_define.h"
#include "stdlib/acl_sys_patch.h"
#include "stdlib/acl_msg.h"
#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_iostuff.h"
#include "net/acl_host_port.h"
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

	on = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
		(const void *) &on, sizeof(on)) < 0) {

		acl_msg_warn("%s(%d): setsockopt(SO_REUSEADDR): %s",
			__FILE__, __LINE__, acl_last_serror());
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
static ACL_SOCKET acl_unix_bind(const char *addr, unsigned flag)
{
#undef sun
	struct sockaddr_un sun;
	int len = strlen(addr);
	ACL_SOCKET sock;

	/*
	 * Translate address information to internal form.
	 */
	if (len >= (int) sizeof(sun.sun_path)) {
		acl_msg_error("%s: addr too long: %s", __FUNCTION__, addr);
		return ACL_SOCKET_INVALID;
	}

	memset((char *) &sun, 0, sizeof(sun));
	sun.sun_family = AF_UNIX;
#ifdef HAS_SUN_LEN
	sun.sun_len    = len + 1;
#endif
	memcpy(sun.sun_path, addr, len + 1);

	/*
	 * Create a listener socket. Do whatever we can so we don't run into
	 * trouble when this process is restarted after crash.
	 */
	if ((sock = socket(AF_UNIX, SOCK_DGRAM, 0)) == ACL_SOCKET_INVALID) {
		acl_msg_error("%s: create socket error %s",
			__FUNCTION__, acl_last_serror());
		return ACL_SOCKET_INVALID;
	}

	(void) unlink(addr);

	if (bind(sock, (struct sockaddr *) & sun, sizeof(sun)) < 0) {
		acl_msg_error("%s: bind %s error %s",
			__FUNCTION__, addr, acl_last_serror());
		acl_socket_close(sock);
		return ACL_SOCKET_INVALID;
	}

#ifdef FCHMOD_UNIX_SOCKETS
	if (fchmod(sock, 0666) < 0) {
		acl_msg_fatal("%s: fchmod socket %s: %s",
			__FUNCTION__, addr, acl_last_serror());
		acl_socket_close(sock);
		return ACL_SOCKET_INVALID;
	}
#else
	if (chmod(addr, 0666) < 0) {
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
	struct addrinfo *res0, *res;
	ACL_SOCKET fd;

#ifdef ACL_UNIX
	const char udp_suffix[] = "@udp";

	if (acl_strrncasecmp(addr, udp_suffix, sizeof(udp_suffix) - 1) == 0) {
		char *buf = acl_mystrdup(addr), *at = strchr(buf, '@');
		*at = 0;
		if (*buf == 0) {
			acl_msg_error("%s(%d): invalid addr=%s",
				__FILE__, __LINE__, addr);
			acl_myfree(buf);
			return ACL_SOCKET_INVALID;
		}
		fd = acl_unix_bind(buf, flag);
		printf("bind fd=%d, buf=%s\r\n", fd, buf);
		acl_myfree(buf);
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
		if (fd != ACL_SOCKET_INVALID)
			break;
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
