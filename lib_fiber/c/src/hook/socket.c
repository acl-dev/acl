#include "stdafx.h"
#include "common.h"

#include "event.h"
#include "fiber.h"

#ifdef SYS_WIN
typedef socket_t (WINAPI *socket_fn)(int, int, int);
typedef int (WINAPI *listen_fn)(socket_t, int);
typedef socket_t (WINAPI *accept_fn)(socket_t, struct sockaddr *, socklen_t *);
typedef int (WINAPI *connect_fn)(socket_t, const struct sockaddr *, socklen_t);
#else
typedef socket_t (*socket_fn)(int, int, int);
typedef int (*listen_fn)(socket_t, int);
typedef socket_t (*accept_fn)(socket_t, struct sockaddr *, socklen_t *);
typedef int (*connect_fn)(socket_t, const struct sockaddr *, socklen_t);
#endif

static socket_fn  __sys_socket  = NULL;
static listen_fn  __sys_listen  = NULL;
static accept_fn  __sys_accept  = NULL;
static connect_fn __sys_connect = NULL;

static void hook_api(void)
{
#ifdef SYS_UNIX
	__sys_socket     = (socket_fn) dlsym(RTLD_NEXT, "socket");
	assert(__sys_socket);

	__sys_listen     = (listen_fn) dlsym(RTLD_NEXT, "listen");
	assert(__sys_listen);

	__sys_accept     = (accept_fn) dlsym(RTLD_NEXT, "accept");
	assert(__sys_accept);

	__sys_connect    = (connect_fn) dlsym(RTLD_NEXT, "connect");
	assert(__sys_connect);
#elif defined(SYS_WIN)
	__sys_socket  = socket;
	__sys_listen  = listen;
	__sys_accept  = accept;
	__sys_connect = connect;
#endif
}

static pthread_once_t __once_control = PTHREAD_ONCE_INIT;

static void hook_init(void)
{
	if (pthread_once(&__once_control, hook_api) != 0) {
		abort();
	}
}

/***************************************************************************/

socket_t acl_fiber_socket(int domain, int type, int protocol)
{
	socket_t sockfd;

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

	if (sockfd != INVALID_SOCKET) {
		non_blocking(sockfd, NON_BLOCKING);
	} else {
		fiber_save_errno(acl_fiber_last_error());
	}

	return sockfd;
}

int acl_fiber_listen(socket_t sockfd, int backlog)
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

	fiber_save_errno(acl_fiber_last_error());
	return -1;
}

#if FIBER_EAGAIN == FIBER_EWOULDBLOCK
# define error_again(x) ((x) == FIBER_EAGAIN)
#else
# define error_again(x) ((x) == FIBER_EAGAIN || (x) == FIBER_EWOULDBLOCK)
#endif

#define FAST_ACCEPT

socket_t WINAPI acl_fiber_accept(socket_t sockfd, struct sockaddr *addr,
	socklen_t *addrlen)
{
	FILE_EVENT *fe;
	socket_t clifd;
	int  err;

	if (sockfd == INVALID_SOCKET) {
		msg_error("%s: invalid sockfd %d", __FUNCTION__, sockfd);
		return -1;
	}

	if (__sys_accept == NULL) {
		hook_init();
	}

	if (!var_hook_sys_api) {
		return __sys_accept ?
			__sys_accept(sockfd, addr, addrlen) : INVALID_SOCKET;
	}

#ifdef	FAST_ACCEPT

	non_blocking(sockfd, NON_BLOCKING);

	clifd = __sys_accept(sockfd, addr, addrlen);
	if (clifd != INVALID_SOCKET) {
		non_blocking(clifd, NON_BLOCKING);
		tcp_nodelay(clifd, 1);
		return clifd;
	}

	//fiber_save_errno();
	err = acl_fiber_last_error();
#if FIBER_EAGAIN == FIBER_EWOULDBLOCK
	if (err != FIBER_EAGAIN) {
#else
	if (err != FIBER_EAGAIN && err != FIBER_EWOULDBLOCK) {
#endif
		return INVALID_SOCKET;
	}

	fe = fiber_file_open(sockfd);

	while (1) {
		fiber_wait_read(fe);

		if (acl_fiber_killed(fe->fiber)) {
			msg_info("%s(%d), %s: fiber-%u was killed", __FILE__,
				__LINE__, __FUNCTION__, acl_fiber_id(fe->fiber));
			return INVALID_SOCKET;
		}

#ifdef HAS_IOCP
		clifd = fe->iocp_sock;
		if (clifd != INVALID_SOCKET) {
			int ret;
			non_blocking(clifd, NON_BLOCKING);
			tcp_nodelay(clifd, 1);
			fe->iocp_sock = INVALID_SOCKET;
			/* iocp 方式下，需调用下面过程以允许调用
			 * getpeername/getsockname
			 */
			ret = setsockopt(clifd, SOL_SOCKET,
				SO_UPDATE_ACCEPT_CONTEXT,
				(char *)&fe->fd, sizeof(fe->fd));
			if (ret == SOCKET_ERROR) {
				closesocket(clifd);
				continue;
			}
			return clifd;
		}
#endif
		clifd = __sys_accept(sockfd, addr, addrlen);

		if (clifd != INVALID_SOCKET) {
			non_blocking(clifd, NON_BLOCKING);
			tcp_nodelay(clifd, 1);
			return clifd;
		}

		err = acl_fiber_last_error();
		fiber_save_errno(err);

		if (!error_again(err)) {
			return INVALID_SOCKET;
		}
	}
#else
	fe = fiber_file_open(sockfd);

	while(1) {
		if (IS_READABLE(fe)) {
			CLR_READABLE(fe);
		} else {
			fiber_wait_read(fe);

			if (acl_fiber_killed(fe->fiber)) {
				msg_info("%s(%d), %s: fiber-%u was killed",
					__FILE__, __LINE__, __FUNCTION__,
					acl_fiber_id(fe->fiber));
				return INVALID_SOCKET;
			}

		}

#ifdef HAS_IOCP
		clifd = fe->iocp_sock;
		if (clifd != INVALID_SOCKET) {
			non_blocking(clifd, NON_BLOCKING);
			tcp_nodelay(clifd, 1);
			fe->iocp_sock = INVALID_SOCKET;
			return clifd;
		}
#endif
		clifd = __sys_accept(sockfd, addr, addrlen);

		if (clifd != INVALID_SOCKET) {
			non_blocking(clifd, NON_BLOCKING);
			tcp_nodelay(clifd, 1);
			return clifd;
		}

		err = acl_fiber_last_error();
		fiber_save_errno(err);

		if (!error_again(err)) {
			return INVALID_SOCKET;
		}
	}
#endif
}

int WINAPI acl_fiber_connect(socket_t sockfd, const struct sockaddr *addr,
	socklen_t addrlen)
{
	int err, ret;
	socklen_t len;
	FILE_EVENT *fe;
	time_t begin, end;

	if (__sys_connect == NULL)
		hook_init();

	if (!var_hook_sys_api) {
		return __sys_connect ? __sys_connect(sockfd, addr, addrlen) : -1;
	}

	non_blocking(sockfd, NON_BLOCKING);

	ret = __sys_connect(sockfd, addr, addrlen);
	if (ret >= 0) {
		tcp_nodelay(sockfd, 1);
		return ret;
	}

	err = acl_fiber_last_error();
	fiber_save_errno(err);

#if FIBER_EAGAIN == FIBER_EWOULDBLOCK
	if (err != FIBER_EINPROGRESS && err != FIBER_EAGAIN) {
#else
	if (err != FIBER_EINPROGRESS && err != FIBER_EAGAIN
		&& err != FIBER_EWOULDBLOCK) {
#endif
		if (err == FIBER_ECONNREFUSED) {
			msg_error("%s(%d), %s: connect ECONNREFUSED",
				__FILE__, __LINE__, __FUNCTION__);
		} else if (err == FIBER_ECONNRESET) {
			msg_error("%s(%d), %s: connect ECONNRESET",
				__FILE__, __LINE__, __FUNCTION__);
		} else if (err == FIBER_ENETDOWN) {
			msg_error("%s(%d), %s: connect ENETDOWN",
				__FILE__, __LINE__, __FUNCTION__);
		} else if (err == FIBER_ENETUNREACH) {
			msg_error("%s(%d), %s: connect ENETUNREACH",
				__FILE__, __LINE__, __FUNCTION__);
#ifdef SYS_UNIX
		} else if (err == FIBER_EHOSTDOWN) {
			msg_error("%s(%d), %s: connect EHOSTDOWN",
				__FILE__, __LINE__, __FUNCTION__);
#endif
		} else if (err == FIBER_EHOSTUNREACH) {
			msg_error("%s(%d), %s: connect EHOSTUNREACH",
				__FILE__, __LINE__, __FUNCTION__);
#ifdef	ACL_LINUX
		/* Linux returns EAGAIN instead of ECONNREFUSED
		 * for unix sockets if listen queue is full -- see nginx
		 */
		} else if (err == FIBER_EAGAIN) {
			msg_error("%s(%d), %s: connect EAGAIN",
				__FILE__, __LINE__, __FUNCTION__);
#endif
		} else {
			msg_error("%s(%d), %s: connect errno=%d, %s",
				__FILE__, __LINE__, __FUNCTION__, err,
				last_serror());
		}

		return -1;
	}

	fe = fiber_file_open(sockfd);

#ifdef SYS_WIN
	fe->status |= STATUS_CONNECTING;
# ifdef HAS_IOCP
	memcpy(&fe->peer_addr, addr, addrlen);
# endif
#endif

	time(&begin);
	fiber_wait_write(fe);
	time(&end);

#ifdef SYS_WIN
	fe->status &= ~STATUS_CONNECTING;
#endif

	if (acl_fiber_killed(fe->fiber)) {
		msg_info("%s(%d), %s: fiber-%u was killed, %s, spend %ld",
			__FILE__, __LINE__, __FUNCTION__,
			acl_fiber_id(fe->fiber), last_serror(),
			(long) (end - begin));
		return -1;
	}

	len = sizeof(err);
	ret = getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (char *) &err, &len);
	if (ret == 0 && err == 0) {
		SOCK_ADDR saddr;
		struct sockaddr *sa = (struct sockaddr*) &saddr;
		socklen_t n = sizeof(saddr);

		if (getpeername(sockfd, sa, &n) == 0) {
			return 0;
		}

		fiber_save_errno(acl_fiber_last_error());
		msg_error("%s(%d), %s: getpeername error %s, fd: %d, spend %ld",
			__FILE__, __LINE__, __FUNCTION__, last_serror(),
			sockfd, (long)(end - begin));
		return -1;
	}

	acl_fiber_set_error(err);
	msg_error("%s(%d): getsockopt error: %s, ret: %d, err: %d, spend %ld",
		__FUNCTION__, __LINE__, last_serror(), ret, err,
		(long) (end - begin));

	return -1;
}

#ifdef SYS_UNIX

int socket(int domain, int type, int protocol)
{
	return acl_fiber_socket(domain, type, protocol);
}

int listen(int sockfd, int backlog)
{
	return acl_fiber_listen(sockfd, backlog);
}

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
	return acl_fiber_accept(sockfd, addr, addrlen);
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
	return acl_fiber_connect(sockfd, addr, addrlen);
}

#endif
