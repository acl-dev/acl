#include "stdafx.h"
#include "common.h"

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

#ifdef SYS_UNIX

static void hook_init(void)
{
	static pthread_mutex_t __lock = PTHREAD_MUTEX_INITIALIZER;
	static int __called = 0;

	(void) pthread_mutex_lock(&__lock);

	if (__called) {
		(void) pthread_mutex_unlock(&__lock);
		return;
	}

	__called++;

	__sys_socket     = (socket_fn) dlsym(RTLD_NEXT, "socket");
	assert(__sys_socket);

	__sys_socketpair = (socketpair_fn) dlsym(RTLD_NEXT, "socketpair");
	assert(__sys_socketpair);

	__sys_bind       = (bind_fn) dlsym(RTLD_NEXT, "bind");
	assert(__sys_bind);

	__sys_listen     = (listen_fn) dlsym(RTLD_NEXT, "listen");
	assert(__sys_listen);

	__sys_accept     = (accept_fn) dlsym(RTLD_NEXT, "accept");
	assert(__sys_accept);

	__sys_connect    = (connect_fn) dlsym(RTLD_NEXT, "connect");
	assert(__sys_connect);

	(void) pthread_mutex_unlock(&__lock);
}

int socket(int domain, int type, int protocol)
{
	int sockfd;

	if (__sys_socket == NULL) {
		hook_init();
	} 

	if (__sys_socket == NULL) {
		return -1;
	}

	sockfd = __sys_socket(domain, type, protocol);

	if (!var_hook_sys_api) {
		return sockfd;
	}

	if (sockfd >= 0) {
		non_blocking(sockfd, NON_BLOCKING);
	} else {
		fiber_save_errno();
	}

	return sockfd;
}

int listen(int sockfd, int backlog)
{
	if (__sys_listen == NULL) {
		hook_init();
	}

	if (!var_hook_sys_api) {
		return __sys_listen ? __sys_listen(sockfd, backlog) : -1;
	}

	non_blocking(sockfd, NON_BLOCKING);
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
		msg_error("%s: invalid sockfd %d", __FUNCTION__, sockfd);
		return -1;
	}

	if (__sys_accept == NULL) {
		hook_init();
	}

	if (!var_hook_sys_api) {
		return __sys_accept ? __sys_accept(sockfd, addr, addrlen) : -1;
	}

#ifdef	FAST_ACCEPT

	non_blocking(sockfd, NON_BLOCKING);

	clifd = __sys_accept(sockfd, addr, addrlen);
	if (clifd >= 0) {
		non_blocking(clifd, NON_BLOCKING);
		tcp_nodelay(clifd, 1);
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

	fe = fiber_file_open(sockfd);
	fiber_wait_read(fe);

	if (acl_fiber_killed(fe->fiber)) {
		msg_info("%s(%d), %s: fiber-%u was killed", __FILE__,
			__LINE__, __FUNCTION__, acl_fiber_id(fe->fiber));
		return -1;
	}

	clifd = __sys_accept(sockfd, addr, addrlen);

	if (clifd >= 0) {
		non_blocking(clifd, NON_BLOCKING);
		tcp_nodelay(clifd, 1);
		return clifd;
	}

	fiber_save_errno();
	return clifd;
#else
	file_event_init(&fe, sockfd);
	fiber_wait_read(&fe);

	if (acl_fiber_killed(fe.fiber)) {
		msg_info("%s(%d), %s: fiber-%u was killed", __FILE__,
			__LINE__, __FUNCTION__, acl_fiber_id(fe.fiber));
		return -1;
	}

	clifd = __sys_accept(sockfd, addr, addrlen);

	if (clifd >= 0) {
		non_blocking(clifd, NON_BLOCKING);
		tcp_nodelay(clifd, 1);
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

	if (!var_hook_sys_api) {
		return __sys_connect ? __sys_connect(sockfd, addr, addrlen) : -1;
	}

	non_blocking(sockfd, NON_BLOCKING);

	int ret = __sys_connect(sockfd, addr, addrlen);
	if (ret >= 0) {
		tcp_nodelay(sockfd, 1);
		return ret;
	}

	fiber_save_errno();

	if (errno != EINPROGRESS) {
		if (errno == ECONNREFUSED) {
			msg_error("%s(%d), %s: connect ECONNREFUSED",
				__FILE__, __LINE__, __FUNCTION__);
		} else if (errno == ECONNRESET) {
			msg_error("%s(%d), %s: connect ECONNRESET",
				__FILE__, __LINE__, __FUNCTION__);
		} else if (errno == ENETDOWN) {
			msg_error("%s(%d), %s: connect ENETDOWN",
				__FILE__, __LINE__, __FUNCTION__);
		} else if (errno == ENETUNREACH) {
			msg_error("%s(%d), %s: connect ENETUNREACH",
				__FILE__, __LINE__, __FUNCTION__);
		} else if (errno == EHOSTDOWN) {
			msg_error("%s(%d), %s: connect EHOSTDOWN",
				__FILE__, __LINE__, __FUNCTION__);
		} else if (errno == EHOSTUNREACH) {
			msg_error("%s(%d), %s: connect EHOSTUNREACH",
				__FILE__, __LINE__, __FUNCTION__);
#ifdef	ACL_LINUX
		/* Linux returns EAGAIN instead of ECONNREFUSED
		 * for unix sockets if listen queue is full -- see nginx
		 */
		} else if (errno == EAGAIN) {
			msg_error("%s(%d), %s: connect EAGAIN",
				__FILE__, __LINE__, __FUNCTION__);
#endif
		} else {
			msg_error("%s(%d), %s: connect errno=%d, %s",
				__FILE__, __LINE__, __FUNCTION__, errno,
				last_serror());
		}

		return -1;
	}

	fe = fiber_file_open(sockfd);
	fiber_wait_write(fe);

	if (acl_fiber_killed(fe->fiber)) {
		msg_info("%s(%d), %s: fiber-%u was killed, %s",
			__FILE__, __LINE__, __FUNCTION__,
			acl_fiber_id(fe->fiber), last_serror());
		return -1;
	}

	len = sizeof(err);
	ret = getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (char *) &err, &len);
	if (ret == 0 && err == 0) {
		struct SOCK_ADDR saddr;
		struct sockaddr *sa = (struct sockaddr*) &saddr;
		socklen_t n = sizeof(saddr);

		if (getpeername(sockfd, sa, &n) == 0) {
			return 0;
		}

		fiber_save_errno();
		msg_error("%s(%d), %s: getpeername error %s, fd: %d",
			__FILE__, __LINE__, __FUNCTION__,
			last_serror(), sockfd);
		return -1;
	}

	set_error(err);
	msg_error("%s(%d): getsockopt error: %s, ret: %d, err: %d",
		__FUNCTION__, __LINE__, last_serror(), ret, err);

	return -1;
}

#endif
