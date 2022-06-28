#include "stdafx.h"
#include "common.h"

#include "event.h"
#include "fiber.h"
#include "hook.h"

socket_t WINAPI acl_fiber_socket(int domain, int type, int protocol)
{
	socket_t sockfd;

	if (sys_socket == NULL) {
		hook_once();
		if (sys_socket == NULL) {
			return -1;
		}
	} 

	sockfd = (*sys_socket)(domain, type, protocol);

	if (!var_hook_sys_api) {
		return sockfd;
	}

#if 0
	/* We shouldn't set NON_BLOCKING where because the NON_BLOCKING will
	 * be checked in acl_fiber_connect(). -- zsx, 2022.01.21
	 */
	if (sockfd != INVALID_SOCKET) {
		non_blocking(sockfd, NON_BLOCKING);
	} else {
		fiber_save_errno(acl_fiber_last_error());
	}
#else
	if (sockfd == INVALID_SOCKET) {
		fiber_save_errno(acl_fiber_last_error());
	}
#endif

	return sockfd;
}

int WINAPI acl_fiber_listen(socket_t sockfd, int backlog)
{
	if (sys_listen == NULL) {
		hook_once();
		if (sys_listen == NULL) {
			msg_error("%s: sys_listen NULL", __FUNCTION__);
			return -1;
		}
	}

	if (!var_hook_sys_api) {
		return sys_listen ? (*sys_listen)(sockfd, backlog) : -1;
	}

	non_blocking(sockfd, NON_BLOCKING);
	if ((*sys_listen)(sockfd, backlog) == 0) {
		return 0;
	}

	fiber_save_errno(acl_fiber_last_error());
	return -1;
}

#ifdef SYS_WIN
socket_t WSAAPI acl_fiber_WSAAccept(
    socket_t s,
    struct sockaddr FAR * addr,
    LPINT addrlen,
    LPCONDITIONPROC lpfnCondition,
    DWORD_PTR dwCallbackData)
{
	return acl_fiber_accept(s, addr, addrlen);
}
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

#ifdef SYS_WSA_API
	if (sys_WSAAccept == NULL) {
		hook_once();
		if (sys_WSAAccept == NULL) {
			msg_error("%s: sys_accept NULL", __FUNCTION__);
			return -1;
		}
	}
#else
	if (sys_accept == NULL) {
		hook_once();
		if (sys_accept == NULL) {
			msg_error("%s: sys_accept NULL", __FUNCTION__);
			return -1;
		}
	}
#endif

	if (!var_hook_sys_api) {
#ifdef SYS_WSA_API
		return sys_WSAAccept ?
			(*sys_WSAAccept)(sockfd, addr, addrlen, 0, 0) : INVALID_SOCKET;
#else
		return sys_accept ?
			(*sys_accept)(sockfd, addr, addrlen) : INVALID_SOCKET;
#endif
	}

#ifdef	FAST_ACCEPT

	non_blocking(sockfd, NON_BLOCKING);

#ifdef SYS_WSA_API
	clifd = (*sys_WSAAccept)(sockfd, addr, addrlen, 0, 0);
#else
	clifd = (*sys_accept)(sockfd, addr, addrlen);
#endif

	if (clifd != INVALID_SOCKET) {
		non_blocking(clifd, NON_BLOCKING);
		tcp_nodelay(clifd, 1);
		return clifd;
	}

	//fiber_save_errno();
	err = acl_fiber_last_error();
	if (!error_again(err)) {
		return INVALID_SOCKET;
	}

	fe = fiber_file_open_read(sockfd);

	while (1) {
		if (fiber_wait_read(fe) < 0) {
			msg_error("%s(%d): fiber_wait_read error=%s, fd=%d",
				__FUNCTION__, __LINE__, last_serror(),
				(int) sockfd);
			return INVALID_SOCKET;
		}

		if (acl_fiber_killed(fe->fiber_r)) {
			msg_info("%s(%d), %s: fiber-%u was killed", __FILE__,
				__LINE__, __FUNCTION__, acl_fiber_id(fe->fiber_r));
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

#ifdef SYS_WSA_API
		clifd = (*sys_WSAAccept)(sockfd, addr, addrlen, 0, 0);
#else
		clifd = (*sys_accept)(sockfd, addr, addrlen);
#endif

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
			if (fiber_wait_read(fe) < 0) {
				msg_error("%s(%d): fiber_wait_read error=%s, fd=%d",
					__FUNCTION__, __LINE__, last_serror(),
					(int) sockfd);
				return INVALID_SOCKET;
			}

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
		clifd = (*sys_accept)(sockfd, addr, addrlen);

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

extern int event_iocp_connect(EVENT* ev, FILE_EVENT* fe);

int WINAPI acl_fiber_connect(socket_t sockfd, const struct sockaddr *addr,
	socklen_t addrlen)
{
	int err, ret, nblock;
	socklen_t len;
	FILE_EVENT *fe;
	time_t begin, end;

	if (sys_connect == NULL) {
		hook_once();
		if (sys_connect == NULL) {
			msg_error("%s: sys_connect NULL", __FUNCTION__);
			return -1;
		}
	}

	if (!var_hook_sys_api) {
		return sys_connect ? (*sys_connect)(sockfd, addr, addrlen) : -1;
	}

	fe = fiber_file_open_write(sockfd);

	SET_NDUBLOCK(fe);

#ifdef SYS_WIN
	SET_CONNECTING(fe);
# ifdef HAS_IOCP
	memcpy(&fe->peer_addr, addr, addrlen);
# endif
#endif

	// The socket must be set to in no blocking status to avoid to be
	// blocked by the sys_connect API. If sys_connect returns an error
	// which is FIBER_EINPROGRESS or FIBER_EAGAIN and the original status
	// of the socket is blocking, the socket should be be in waiting for
	// writable by calling fiber_wait_write, which is just like the
	// connecting process being in blocking mode.
	nblock = is_non_blocking(sockfd);
	if (!nblock) {
		non_blocking(sockfd, NON_BLOCKING);
	}

#ifdef HAS_IOCP
	if (EVENT_IS_IOCP(fiber_io_event())) {
		EVENT *ev = fiber_io_event();
		fe->type = TYPE_SPIPE;
		ret = event_iocp_connect(ev, fe);
	} else {
		ret = (*sys_connect)(sockfd, addr, addrlen);
	}
#else
	ret = (*sys_connect)(sockfd, addr, addrlen);
#endif

	if (ret >= 0) {
		tcp_nodelay(sockfd, 1);
		CLR_CONNECTING(fe);
		return ret;
	}

	err = acl_fiber_last_error();
	fiber_save_errno(err);

	if (err != FIBER_EINPROGRESS && !error_again(err)) {
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

	/* If the non-blocking has bee set, we should return and don't wait */
	if (nblock) {
		return -1;
	}

	time(&begin);
	if (fiber_wait_write(fe) < 0) {
		time(&end);
		msg_error("%s(%d): fiber_wait_write error=%s, fd=%d, cost=%ld",
			__FUNCTION__, __LINE__, last_serror(), (int) sockfd,
			(long)(end - begin));
		return -1;
	}
	time(&end);

#ifdef SYS_WIN
	CLR_CONNECTING(fe);
#endif

	if (acl_fiber_killed(fe->fiber_w)) {
		msg_info("%s(%d), %s: fiber-%u was killed, %s, spend %ld",
			__FILE__, __LINE__, __FUNCTION__,
			acl_fiber_id(fe->fiber_w), last_serror(),
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

typedef struct TIMEOUT_CTX {
	ACL_FIBER *fiber;
	int        sockfd;
	unsigned   id;
} TIMEOUT_CTX;

static void fiber_timeout(ACL_FIBER *fiber UNUSED, void *ctx)
{
	TIMEOUT_CTX *tc = (TIMEOUT_CTX*) ctx;
	FILE_EVENT *fe = fiber_file_get(tc->sockfd);

	// we must check the fiber carefully here.
	if (fe == NULL || tc->fiber != fe->fiber_r
		|| tc->fiber->id != fe->fiber_r->id) {

		mem_free(ctx);
		return;
	}

	// we can kill the fiber only if the fiber is waiting
	// for readable ore writable of IO process.
	if (fe->fiber_r->status == FIBER_STATUS_WAIT_READ
		|| fe->fiber_w->status == FIBER_STATUS_WAIT_WRITE) {

		tc->fiber->errnum = FIBER_EAGAIN;
		acl_fiber_signal(tc->fiber, SIGINT);
	}

	mem_free(ctx);
}

int setsockopt(int sockfd, int level, int optname,
	const void *optval, socklen_t optlen)
{
	size_t val;
	TIMEOUT_CTX *ctx;
	const struct timeval *tm;

	if (sys_setsockopt == NULL) {
		hook_once();
	}

	if (!var_hook_sys_api || (optname != SO_RCVTIMEO
				&& optname != SO_SNDTIMEO)) {
		return sys_setsockopt ? (*sys_setsockopt)(sockfd, level,
			optname, optval, optlen) : -1;
	}

	if (sys_setsockopt == NULL) {
		msg_error("sys_setsockopt null");
		return -1;
	}

	switch (optlen) {
	case 0:
		msg_error("optlen is 0");
		return -1;
	case 1:
		val = *((const char*) optval);
		break;
	case 2:
		val = *((const short*) optval);
		break;
	case 4:
		val = *((const int*) optval);
		break;
	case 8:
		val = *((const long long*) optval);
		break;
	case 16:
		tm = (const struct timeval*) optval;
		val = tm->tv_sec + tm->tv_usec / 1000000;
		break;
	default:
		msg_error("invalid optlen=%d", (int) optlen);
		return -1;
	}

	ctx = (TIMEOUT_CTX*) mem_malloc(sizeof(TIMEOUT_CTX));
	ctx->fiber  = acl_fiber_running();
	ctx->sockfd = sockfd;
	acl_fiber_create_timer((unsigned) val * 1000, 64000, fiber_timeout, ctx);
	return 0;
}

#endif
