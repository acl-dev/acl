#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "stdafx.h"
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include "fiber/lib_fiber.h"
#include "event.h"
#include "fiber.h"

typedef int (*socket_fn)(int, int, int);
typedef int (*socketpair_fn)(int, int, int, int sv[2]);
typedef int (*bind_fn)(int, const struct sockaddr *, socklen_t);
typedef int (*listen_fn)(int, int);
typedef int (*accept_fn)(int, struct sockaddr *, socklen_t *);
typedef int (*connect_fn)(int, const struct sockaddr *, socklen_t);

static socket_fn          __sys_socket          = NULL;
static socketpair_fn      __sys_socketpair      = NULL;
static bind_fn            __sys_bind            = NULL;
static listen_fn          __sys_listen          = NULL;
static accept_fn          __sys_accept          = NULL;
static connect_fn         __sys_connect         = NULL;

static void hook_init(void)
{
	static acl_pthread_mutex_t __lock = PTHREAD_MUTEX_INITIALIZER;
	static int __called = 0;

	(void) acl_pthread_mutex_lock(&__lock);

	if (__called) {
		(void) acl_pthread_mutex_unlock(&__lock);
		return;
	}

	__called++;

	__sys_socket     = (socket_fn) dlsym(RTLD_NEXT, "socket");
	acl_assert(__sys_socket);

	__sys_socketpair = (socketpair_fn) dlsym(RTLD_NEXT, "socketpair");
	acl_assert(__sys_socketpair);

	__sys_bind       = (bind_fn) dlsym(RTLD_NEXT, "bind");
	acl_assert(__sys_bind);

	__sys_listen     = (listen_fn) dlsym(RTLD_NEXT, "listen");
	acl_assert(__sys_listen);

	__sys_accept     = (accept_fn) dlsym(RTLD_NEXT, "accept");
	acl_assert(__sys_accept);

	__sys_connect    = (connect_fn) dlsym(RTLD_NEXT, "connect");
	acl_assert(__sys_connect);

	(void) acl_pthread_mutex_unlock(&__lock);
}

int socket(int domain, int type, int protocol)
{
	int sockfd;

	assert(0);
	if (__sys_socket == NULL) {
		hook_init();
	} 

	if (__sys_socket == NULL) {
		return -1;
	}

	sockfd = __sys_socket(domain, type, protocol);

	if (!acl_var_hook_sys_api) {
		return sockfd;
	}

	if (sockfd >= 0) {
		acl_non_blocking(sockfd, ACL_NON_BLOCKING);
	} else {
		fiber_save_errno();
	}

	return sockfd;
}

int listen(int sockfd, int backlog)
{
	assert(0);
	if (__sys_listen == NULL) {
		hook_init();
	}

	if (!acl_var_hook_sys_api) {
		return __sys_listen ? __sys_listen(sockfd, backlog) : -1;
	}

	acl_non_blocking(sockfd, ACL_NON_BLOCKING);
	if (__sys_listen(sockfd, backlog) == 0) {
		return 0;
	}

	fiber_save_errno();
	return -1;
}

#define FAST_ACCEPT

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
	FILE_EVENT *fe;
	int clifd;

	if (sockfd < 0) {
		acl_msg_error("%s: invalid sockfd %d", __FUNCTION__, sockfd);
		return -1;
	}

	if (__sys_accept == NULL) {
		hook_init();
	}

	if (!acl_var_hook_sys_api) {
		return __sys_accept ? __sys_accept(sockfd, addr, addrlen) : -1;
	}

#ifdef	FAST_ACCEPT

	acl_non_blocking(sockfd, ACL_NON_BLOCKING);

	clifd = __sys_accept(sockfd, addr, addrlen);
	if (clifd >= 0) {
		acl_non_blocking(clifd, ACL_NON_BLOCKING);
		acl_tcp_nodelay(clifd, 1);
		return clifd;
	}

	fiber_save_errno();
#if EAGAIN == EWOULDBLOCK
	if (errno != EAGAIN) {
#else
	if (errno != EAGAIN && errno != EWOULDBLOCK) {
#endif
		return -1;
	}

	fe = fiber_file_event(sockfd);
	fiber_wait_read(fe);

	if (acl_fiber_killed(fe->fiber)) {
		acl_msg_info("%s(%d), %s: fiber-%u was killed", __FILE__,
			__LINE__, __FUNCTION__, acl_fiber_id(fe->fiber));
		return -1;
	}

	clifd = __sys_accept(sockfd, addr, addrlen);

	if (clifd >= 0) {
		acl_non_blocking(clifd, ACL_NON_BLOCKING);
		acl_tcp_nodelay(clifd, 1);
		return clifd;
	}

	fiber_save_errno();
	return clifd;
#else
	file_event_init(&fe, sockfd);
	fiber_wait_read(&fe);

	if (acl_fiber_killed(fe.fiber)) {
		acl_msg_info("%s(%d), %s: fiber-%u was killed", __FILE__,
			__LINE__, __FUNCTION__, acl_fiber_id(fe.fiber));
		return -1;
	}

	clifd = __sys_accept(sockfd, addr, addrlen);

	if (clifd >= 0) {
		acl_non_blocking(clifd, ACL_NON_BLOCKING);
		acl_tcp_nodelay(clifd, 1);
		return clifd;
	}

	fiber_save_errno();
	return clifd;
#endif
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
	int err;
	socklen_t len;
	FILE_EVENT *fe;

	if (__sys_connect == NULL)
		hook_init();

	if (!acl_var_hook_sys_api) {
		return __sys_connect ? __sys_connect(sockfd, addr, addrlen) : -1;
	}

	acl_non_blocking(sockfd, ACL_NON_BLOCKING);

	int ret = __sys_connect(sockfd, addr, addrlen);
	if (ret >= 0) {
		acl_tcp_nodelay(sockfd, 1);
		return ret;
	}

	fiber_save_errno();

	if (errno != EINPROGRESS) {
		if (errno == ECONNREFUSED) {
			acl_msg_error("%s(%d), %s: connect ECONNREFUSED",
				__FILE__, __LINE__, __FUNCTION__);
		} else if (errno == ECONNRESET) {
			acl_msg_error("%s(%d), %s: connect ECONNRESET",
				__FILE__, __LINE__, __FUNCTION__);
		} else if (errno == ENETDOWN) {
			acl_msg_error("%s(%d), %s: connect ENETDOWN",
				__FILE__, __LINE__, __FUNCTION__);
		} else if (errno == ENETUNREACH) {
			acl_msg_error("%s(%d), %s: connect ENETUNREACH",
				__FILE__, __LINE__, __FUNCTION__);
		} else if (errno == EHOSTDOWN) {
			acl_msg_error("%s(%d), %s: connect EHOSTDOWN",
				__FILE__, __LINE__, __FUNCTION__);
		} else if (errno == EHOSTUNREACH) {
			acl_msg_error("%s(%d), %s: connect EHOSTUNREACH",
				__FILE__, __LINE__, __FUNCTION__);
#ifdef	ACL_LINUX
		/* Linux returns EAGAIN instead of ECONNREFUSED
		 * for unix sockets if listen queue is full -- see nginx
		 */
		} else if (errno == EAGAIN) {
			acl_msg_error("%s(%d), %s: connect EAGAIN",
				__FILE__, __LINE__, __FUNCTION__);
#endif
		} else {
			acl_msg_error("%s(%d), %s: connect errno=%d, %s",
				__FILE__, __LINE__, __FUNCTION__, errno,
				acl_last_serror());
		}

		return -1;
	}

	fe = fiber_file_event(sockfd);
	fiber_wait_write(fe);

	if (acl_fiber_killed(fe->fiber)) {
		acl_msg_info("%s(%d), %s: fiber-%u was killed, %s",
			__FILE__, __LINE__, __FUNCTION__,
			acl_fiber_id(fe->fiber), acl_last_serror());
		return -1;
	}

	len = sizeof(err);
	ret = getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (char *) &err, &len);
	if (ret == 0 && err == 0) {
		char peer[256];
		len = sizeof(peer);
		if (acl_getpeername(sockfd, peer, len) == 0) {
			return 0;
		}

		fiber_save_errno();
		acl_msg_error("%s(%d), %s: getpeername error %s, fd: %d",
			__FILE__, __LINE__, __FUNCTION__,
			acl_last_serror(), sockfd);
		return -1;
	}

	acl_set_error(err);
	acl_msg_error("%s(%d): getsockopt error: %s, ret: %d, err: %d",
		__FUNCTION__, __LINE__, acl_last_serror(), ret, err);

	return -1;
}
