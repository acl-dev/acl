#include "stdafx.h"
#include "common.h"

#include "fiber.h"
#include "hook.h"

#ifdef SYS_UNIX
unsigned int sleep(unsigned int seconds)
{
	if (!var_hook_sys_api) {
		if (sys_sleep == NULL) {
			hook_once();
		}

		return (*sys_sleep)(seconds);
	}

	return acl_fiber_sleep(seconds);
}

int close(socket_t fd)
{
	return acl_fiber_close(fd);
}
#endif

int WINAPI acl_fiber_close(socket_t fd)
{
	int ret;
	FILE_EVENT *fe;
	EVENT *ev;

	if (sys_close == NULL) {
		hook_once();
		if (sys_close == NULL) {
			msg_error("%s: sys_close NULL", __FUNCTION__);
			return -1;
		}
	}

	if (!var_hook_sys_api) {
		return (*sys_close)(fd);
	}

	if (fd == INVALID_SOCKET) {
		msg_error("%s(%d): invalid fd=%u", __FUNCTION__, __LINE__, fd);
		return -1;
	} else if (fd >= (socket_t) var_maxfd) {
		msg_error("%s(%d): too large fd=%u", __FUNCTION__, __LINE__, fd);
		return (*sys_close)(fd);
	}

#ifdef	HAS_EPOLL
	/* when the fd was closed by epoll_event_close normally, the fd
	 * must be a epoll fd which was created by epoll_create function
	 * hooked in hook_net.c
	 */
	if (epoll_event_close(fd) == 0) {
		return 0;
	}
#endif

	// The fd should be closed directly if no fe hoding it.
	fe = fiber_file_get(fd);
	if (fe == NULL) {
		ret = (*sys_close)(fd);
		if (ret != 0) {
			fiber_save_errno(acl_fiber_last_error());
		}
		return ret;
	}

	// If the fd is in the status waiting for IO ready, the current fiber
	// is trying to close the other fiber's fd, so, we should wakeup the
	// suspending fiber and wait for its returning back, the process is:
	// ->killer kill the fiber which is suspending and holding the fd;
	// ->suspending fiber wakeup and return;
	// ->killer closing the fd and free the fe.

	// If the fd isn't in waiting status, we don't know which fiber is
	// holding the fd, but we can close it and free the fe with it.
	// If the current fiber is holding the fd, we just close it ok, else
	// if the other fiber is holding it, the IO API such as acl_fiber_read
	// should hanlding the fd carefully, the process is:
	// ->killer closing the fd and free fe owned by itself or other fiber;
	// ->if fd was owned by the other fiber, calling API like acl_fiber_read
	//   will return -1 after fiber_wait_read returns.

	fiber_file_close(fe);

	ev = fiber_io_event();
	if (ev && ev->close_sock) {
		ret = ev->close_sock(ev, fe);
		if (ret == 0) {
			ret = (*sys_close)(fd);
		}
	} else {
		ret = (*sys_close)(fd);
	}

	fiber_file_free(fe);

	if (ret != 0) {
		fiber_save_errno(acl_fiber_last_error());
	}
	return ret;
}

/****************************************************************************/

#ifdef SYS_UNIX

//# define READ_FIRST

# ifdef READ_FIRST
ssize_t acl_fiber_read(socket_t fd, void *buf, size_t count)
{
	FILE_EVENT* fe;

	if (fd == INVALID_SOCKET) {
		msg_error("%s: invalid fd: %d", __FUNCTION__, fd);
		return -1;
	}

	if (sys_read == NULL) {
		hook_once();
	}

	if (!var_hook_sys_api) {
		return (*sys_read)(fd, buf, count);
	}

	fe = fiber_file_open(fd);
	CLR_POLLING(fe);

	while (1) {
		ssize_t n;
		int err;

		if (acl_fiber_canceled(fe->fiber)) {
			acl_fiber_set_error(fe->fiber->errnum);
			return -1;
		}

		n = (*sys_read)(fd, buf, count);
		if (n >= 0) {
			return n;
		}

		err = acl_fiber_last_error();
		fiber_save_errno(err);

		if (!error_again(err)) {
			return -1;
		}

		if (fiber_wait_read(fe) < 0) {
			msg_error("%s(%d): fiber_wait_read error=%s, fd=%d",
				__FUNCTION__, __LINE__, last_serror(), (int) fd);
			return -1;
		}
	}
}
# else
ssize_t acl_fiber_read(socket_t fd, void *buf, size_t count)
{
	FILE_EVENT* fe;

	if (fd == INVALID_SOCKET) {
		msg_error("%s: invalid fd: %d", __FUNCTION__, fd);
		return -1;
	}

	if (sys_read == NULL) {
		hook_once();
	}

	if (!var_hook_sys_api) {
		return (*sys_read)(fd, buf, count);
	}

	fe = fiber_file_open_read(fd);
	CLR_POLLING(fe);

	while (1) {
		ssize_t ret;
		int err;

		if (IS_READABLE(fe)) {
			CLR_READABLE(fe);
		}

		// The fiber_wait_read will return three status:
		// 1: The fd is a valid socket/pipe/fifo, which can be
		//    monitored by event engine, such as epoll, select or poll;
		// 0: The fd isn't a socket/pipe/fifo, maybe a file, and can't
		//    be monitored by event engine and can read directly;
		// -1: The fd isn't a valid descriptor, just return error, and
		//   the fe should be freed.
		else if (fiber_wait_read(fe) < 0) {
			msg_error("%s(%d): fiber_wait_read error=%s, fd=%d, fe=%p",
				__FUNCTION__, __LINE__, last_serror(), (int) fd, fe);
			fiber_file_free(fe);
			return -1;
		}

		// If the suspending fiber wakeup for the reason that it was
		// killed by the other fiber which called acl_fiber_kill and
		// want to close the fd owned by the current fiber, we just
		// set the errno status and return -1, and the killer fiber
		// will close the fd in acl_fiber_close API.
		if (acl_fiber_canceled(fe->fiber_r)) {
			acl_fiber_set_error(fe->fiber_r->errnum);
			return -1;
		}

		ret = (*sys_read)(fd, buf, count);
		if (ret >= 0) {
			return ret;
		}

		err = acl_fiber_last_error();
		fiber_save_errno(err);

		if (!error_again(err)) {
			// If the fd is a descriptor but not a socket, the
			// above fiber_wait_read() must return 0, so we must
			// free the fe here because the fd isn't monitored by
			// the event engine.
			if (fe->type != TYPE_SPIPE) {
				fiber_file_free(fe);
			}
			return -1;
		}
	}
}
# endif // READ_FIRST

ssize_t acl_fiber_readv(socket_t fd, const struct iovec *iov, int iovcnt)
{
	FILE_EVENT *fe;

	if (fd == INVALID_SOCKET) {
		msg_error("%s: invalid fd: %d", __FUNCTION__, fd);
		return -1;
	}

	if (sys_readv == NULL) {
		hook_once();
	}

	if (!var_hook_sys_api) {
		return (*sys_readv)(fd, iov, iovcnt);
	}

	fe = fiber_file_open_read(fd);
	CLR_POLLING(fe);

	while (1) {
		ssize_t ret;
		int err;

		if (IS_READABLE(fe)) {
			CLR_READABLE(fe);
		} else if (fiber_wait_read(fe) < 0) {
			msg_error("%s(%d): fiber_wait_read error=%s, fd=%d",
				__FUNCTION__, __LINE__, last_serror(), (int) fd);
			return -1;
		}

		if (acl_fiber_canceled(fe->fiber_r)) {
			acl_fiber_set_error(fe->fiber_r->errnum);
			return -1;
		}

		ret = (*sys_readv)(fd, iov, iovcnt);
		if (ret >= 0) {
			return ret;
		}

		err = acl_fiber_last_error();
		fiber_save_errno(err);

		if (!error_again(err)) {
			if (fe->type != TYPE_SPIPE) {
				fiber_file_free(fe);
			}
			return -1;
		}
	}
}
#endif // SYS_UNIX

#ifdef HAS_IOCP
static int fiber_iocp_read(FILE_EVENT *fe, char *buf, int len)
{
	/* If the socket type is UDP, We must check the fixed buffer first,
	 * which maybe used in iocp_add_read() and set for polling read status.
	 */
	if (fe->sock_type == SOCK_DGRAM && fe->buff == fe->packet && fe->len > 0) {
		if (fe->len < len) {
			len = fe->len;
		}
		memcpy(buf, fe->packet, len);
		fe->buff = NULL;
		fe->len  = 0;
		return len;
	}

	fe->buff = buf;
	fe->size = len;
	fe->len  = 0;

	while (1) {
		int err;

		fe->mask &= ~EVENT_READ;

		if (fiber_wait_read(fe) < 0) {
			msg_error("%s(%d): fiber_wait_read error=%s, fd=%d",
				__FUNCTION__, __LINE__, last_serror(), (int) fe->fd);
			return -1;
		}

		if (fe->mask & EVENT_ERR) {
			err = acl_fiber_last_error();
			fiber_save_errno(err);
			return -1;
		}

		if (acl_fiber_canceled(fe->fiber_r)) {
			acl_fiber_set_error(fe->fiber_r->errnum);
			return -1;
		}

		if (fe->len >= 0) {
			return fe->len;
		}

		err = acl_fiber_last_error();
		fiber_save_errno(err);

		if (!error_again(err)) {
			if (fe->type != TYPE_SPIPE) {
				fiber_file_free(fe);
			}
			return -1;
		}
	}
}
#endif

#ifdef SYS_WIN
int WINAPI acl_fiber_WSARecv(socket_t sockfd,
	LPWSABUF lpBuffers,
	DWORD dwBufferCount,
	LPDWORD lpNumberOfBytesRecvd,
	LPDWORD lpFlags,
	LPWSAOVERLAPPED lpOverlapped,
	LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
	return acl_fiber_recv(sockfd, lpBuffers->buf, dwBufferCount, *lpFlags);
}
#endif

#ifdef SYS_WIN
int WINAPI acl_fiber_recv(socket_t sockfd, char *buf, int len, int flags)
#else
ssize_t acl_fiber_recv(socket_t sockfd, void *buf, size_t len, int flags)
#endif
{
	FILE_EVENT *fe;

	if (sockfd == INVALID_SOCKET) {
		msg_error("%s: invalid sockfd: %d", __FUNCTION__, sockfd);
		return -1;
	}

	if (sys_recv == NULL) {
		hook_once();
		if (sys_recv == NULL) {
			msg_error("%s: sys_recv NULL", __FUNCTION__);
			return -1;
		}
	}

	if (!var_hook_sys_api) {
		return (*sys_recv)(sockfd, buf, len, flags);
	}

	fe = fiber_file_open_read(sockfd);
	CLR_POLLING(fe);

#ifdef HAS_IOCP
	if (EVENT_IS_IOCP(fiber_io_event())) {
		return fiber_iocp_read(fe, buf, (int) len);
	}
#endif

	while (1) {
		int ret;
		int err;

		if (IS_READABLE(fe)) {
			CLR_READABLE(fe);
		} else if (fiber_wait_read(fe) < 0) {
			msg_error("%s(%d): fiber_wait_read error=%s, fd=%d",
				__FUNCTION__, __LINE__,last_serror(), (int) sockfd);
			return -1;
		}

		if (acl_fiber_canceled(fe->fiber_r)) {
			acl_fiber_set_error(fe->fiber_r->errnum);
			return -1;
		}

		ret = (int) (*sys_recv)(sockfd, buf, len, flags);
		if (ret >= 0) {
			return ret;
		}

		err = acl_fiber_last_error();
		fiber_save_errno(err);

		if (!error_again(err)) {
			if (fe->type != TYPE_SPIPE) {
				fiber_file_free(fe);
			}
			return -1;
		}
	}
}

#ifdef SYS_WIN
int WINAPI acl_fiber_recvfrom(socket_t sockfd, char *buf, int len,
	int flags, struct sockaddr *src_addr, socklen_t *addrlen)
#else
ssize_t acl_fiber_recvfrom(socket_t sockfd, void *buf, size_t len,
	int flags, struct sockaddr *src_addr, socklen_t *addrlen)
#endif
{
	FILE_EVENT *fe;

	if (sockfd == INVALID_SOCKET) {
		msg_error("%s: invalid sockfd: %d", __FUNCTION__, sockfd);
		return -1;
	}

	if (sys_recvfrom == NULL) {
		hook_once();
		if (sys_recvfrom == NULL) {
			msg_error("%s: sys_recvfrom NULL", __FUNCTION__);
			return -1;
		}
	}

	if (!var_hook_sys_api) {
		return (*sys_recvfrom)(sockfd, buf, len, flags,
				src_addr, addrlen);
	}

	fe = fiber_file_open_read(sockfd);
	CLR_POLLING(fe);

#ifdef HAS_IOCP
	if (EVENT_IS_IOCP(fiber_io_event())) {
		return fiber_iocp_read(fe, buf, (int) len);
	}
#endif

	while (1) {
		ssize_t ret;
		int err;

		if (IS_READABLE(fe)) {
			CLR_READABLE(fe);
		} else if (fiber_wait_read(fe) < 0) {
			msg_error("%s(%d): fiber_wait_read error=%s, fd=%d",
				__FUNCTION__, __LINE__, last_serror(), (int) sockfd);
			return -1;
		}

		if (acl_fiber_canceled(fe->fiber_r)) {
			acl_fiber_set_error(fe->fiber_r->errnum);
			return -1;
		}

		ret = (*sys_recvfrom)(sockfd, buf, len, flags, src_addr, addrlen);
		if (ret >= 0) {
			return (int) ret;
		}

		err = acl_fiber_last_error();
		fiber_save_errno(err);

		if (!error_again(err)) {
			if (fe->type != TYPE_SPIPE) {
				fiber_file_free(fe);
			}
			return -1;
		}
	}
}

#ifdef SYS_UNIX
ssize_t acl_fiber_recvmsg(socket_t sockfd, struct msghdr *msg, int flags)
{
	FILE_EVENT *fe;

	if (sockfd == INVALID_SOCKET) {
		msg_error("%s: invalid sockfd: %d", __FUNCTION__, sockfd);
		return -1;
	}

	if (sys_recvmsg == NULL) {
		hook_once();
	}

	if (!var_hook_sys_api) {
		return (*sys_recvmsg)(sockfd, msg, flags);
	}

	fe = fiber_file_open_read(sockfd);
	CLR_POLLING(fe);

	while (1) {
		ssize_t ret;
		int err;

		if (IS_READABLE(fe)) {
			CLR_READABLE(fe);
		} else if (fiber_wait_read(fe) < 0) {
			msg_error("%s(%d): fiber_wait_read error=%s, fd=%d",
				__FUNCTION__, __LINE__, last_serror(), (int) sockfd);
			return -1;
		}

		if (acl_fiber_canceled(fe->fiber_r)) {
			acl_fiber_set_error(fe->fiber_r->errnum);
			return -1;
		}

		ret = (*sys_recvmsg)(sockfd, msg, flags);
		if (ret >= 0) {
			return ret;
		}

		err = acl_fiber_last_error();
		fiber_save_errno(err);

		if (!error_again(err)) {
			if (fe->type != TYPE_SPIPE) {
				fiber_file_free(fe);
			}
			return -1;
		}
	}
}
#endif

/****************************************************************************/

// In the API connect() being hooked in hook/socket.c, the STATUS_NDUBLOCK
// flag was set and the fd was in non-block status in order to return imaginary
// from connecting process.
#define CHECK_SET_NBLOCK(_fd) do { \
	if (var_hook_sys_api) { \
		FILE_EVENT *fe = fiber_file_get(_fd); \
		if (fe && IS_NDUBLOCK(fe)) { \
			non_blocking(_fd, NON_BLOCKING); \
			CLR_NDUBLOCK(fe); \
		} \
	} \
} while (0)

#ifdef SYS_UNIX
ssize_t acl_fiber_write(socket_t fd, const void *buf, size_t count)
{
	if (fd == INVALID_SOCKET) {
		msg_error("%s: invalid fd: %d", __FUNCTION__, fd);
		return -1;
	}

	if (sys_write == NULL) {
		hook_once();
	}

	CHECK_SET_NBLOCK(fd);

	while (1) {
		ssize_t n = (*sys_write)(fd, buf, count);
		FILE_EVENT *fe;
		int err;

		if (!var_hook_sys_api) {
			return n;
		}

		if (n >= 0) {
			return n;
		}

		err = acl_fiber_last_error();
		fiber_save_errno(err);

		if (!error_again(err)) {
			return -1;
		}

		fe = fiber_file_open_write(fd);
		CLR_POLLING(fe);

		if (fiber_wait_write(fe) < 0) {
			msg_error("%s(%d): fiber_wait_write error=%s, fd=%d",
				__FUNCTION__, __LINE__, last_serror(), (int) fd);
			fiber_file_free(fe);
			return -1;
		}

		if (acl_fiber_canceled(fe->fiber_w)) {
			acl_fiber_set_error(fe->fiber_w->errnum);
			return -1;
		}
	}
}

ssize_t acl_fiber_writev(socket_t fd, const struct iovec *iov, int iovcnt)
{
	if (fd == INVALID_SOCKET) {
		msg_error("%s: invalid fd: %d", __FUNCTION__, fd);
		return -1;
	}

	if (sys_writev == NULL) {
		hook_once();
	}

	CHECK_SET_NBLOCK(fd);

	while (1) {
		int n = (int) (*sys_writev)(fd, iov, iovcnt);
		FILE_EVENT *fe;
		int err;

		if (!var_hook_sys_api) {
			return n;
		}

		if (n >= 0) {
			return n;
		}

		err = acl_fiber_last_error();
		fiber_save_errno(err);

		if (!error_again(err)) {
			return -1;
		}

		fe = fiber_file_open_write(fd);
		CLR_POLLING(fe);

		if (fiber_wait_write(fe) < 0) {
			msg_error("%s(%d): fiber_wait_write error=%s, fd=%d",
				__FUNCTION__, __LINE__, last_serror(), (int) fd);
			fiber_file_free(fe);
			return -1;
		}

		if (acl_fiber_canceled(fe->fiber_w)) {
			acl_fiber_set_error(fe->fiber_w->errnum);
			return -1;
		}
	}
}
#endif

#ifdef SYS_WIN
int WINAPI acl_fiber_send(socket_t sockfd, const char *buf,
	int len, int flags)
#else
ssize_t acl_fiber_send(socket_t sockfd, const void *buf,
	size_t len, int flags)
#endif
{
	if (sockfd == INVALID_SOCKET) {
		msg_error("%s: invalid fd: %d", __FUNCTION__, sockfd);
		return -1;
	}

	if (sys_send == NULL) {
		hook_once();
		if (sys_send == NULL) {
			msg_error("%s: sys_send NULL", __FUNCTION__);
			return -1;
		}
	}

	CHECK_SET_NBLOCK(sockfd);

	while (1) {
		int n = (int) (*sys_send)(sockfd, buf, len, flags);
		FILE_EVENT *fe;
		int err;

		if (!var_hook_sys_api) {
			return n;
		}

		if (n >= 0) {
			return n;
		}

		err = acl_fiber_last_error();
		fiber_save_errno(err);

		if (!error_again(err)) {
			return -1;
		}

		fe = fiber_file_open_write(sockfd);
		CLR_POLLING(fe);

		if (fiber_wait_write(fe) < 0) {
			msg_error("%s(%d): fiber_wait_write error=%s, fd=%d",
				__FUNCTION__, __LINE__, last_serror(), (int) sockfd);
			fiber_file_free(fe);
			return -1;
		}

		if (acl_fiber_canceled(fe->fiber_w)) {
			acl_fiber_set_error(fe->fiber_w->errnum);
			return -1;
		}
	}
}

#ifdef SYS_WIN
int WINAPI acl_fiber_sendto(socket_t sockfd, const char *buf, int len,
	int flags, const struct sockaddr *dest_addr, socklen_t addrlen)
#else
ssize_t acl_fiber_sendto(socket_t sockfd, const void *buf, size_t len,
	int flags, const struct sockaddr *dest_addr, socklen_t addrlen)
#endif
{
	if (sockfd == INVALID_SOCKET) {
		msg_error("%s: invalid fd: %d", __FUNCTION__, sockfd);
		return -1;
	}

	if (sys_sendto == NULL) {
		hook_once();
		if (sys_sendto == NULL) {
			msg_error("%s: sys_sendto NULL", __FUNCTION__);
			return -1;
		}
	}

	CHECK_SET_NBLOCK(sockfd);

	while (1) {
		int n = (int) (*sys_sendto)(sockfd, buf, len, flags,
				dest_addr, addrlen);
		FILE_EVENT *fe;
		int err;

		if (!var_hook_sys_api) {
			return n;
		}

		if (n >= 0) {
			return n;
		}

		err = acl_fiber_last_error();
		fiber_save_errno(err);

		if (!error_again(err)) {
			return -1;
		}

		fe = fiber_file_open_write(sockfd);
		CLR_POLLING(fe);

		if (fiber_wait_write(fe) < 0) {
			msg_error("%s(%d): fiber_wait_write error=%s, fd=%d",
				__FUNCTION__, __LINE__, last_serror(), (int) sockfd);
			fiber_file_free(fe);
			return -1;
		}

		if (acl_fiber_canceled(fe->fiber_w)) {
			acl_fiber_set_error(fe->fiber_w->errnum);
			return -1;
		}
	}
}

#ifdef SYS_UNIX
ssize_t acl_fiber_sendmsg(socket_t sockfd, const struct msghdr *msg, int flags)
{
	if (sockfd == INVALID_SOCKET) {
		msg_error("%s: invalid fd: %d", __FUNCTION__, sockfd);
		return -1;
	}

	if (sys_sendmsg == NULL) {
		hook_once();
	}

	CHECK_SET_NBLOCK(sockfd);

	while (1) {
		ssize_t n = (*sys_sendmsg)(sockfd, msg, flags);
		FILE_EVENT *fe;
		int err;

		if (!var_hook_sys_api) {
			return n;
		}

		if (n >= 0) {
			return n;
		}

		err = acl_fiber_last_error();
		fiber_save_errno(err);

		if (!error_again(err)) {
			return -1;
		}

		fe = fiber_file_open_write(sockfd);
		CLR_POLLING(fe);

		if (fiber_wait_write(fe) < 0) {
			msg_error("%s(%d): fiber_wait_write error=%s, fd=%d",
				__FUNCTION__, __LINE__, last_serror(), (int) sockfd);
			fiber_file_free(fe);
			return -1;
		}

		if (acl_fiber_canceled(fe->fiber_w)) {
			acl_fiber_set_error(fe->fiber_w->errnum);
			return -1;
		}
	}
}
#endif

/****************************************************************************/

#if defined(SYS_UNIX) && !defined(DISABLE_HOOK_IO)
ssize_t read(socket_t fd, void *buf, size_t count)
{
	return acl_fiber_read(fd, buf, count);
}

ssize_t readv(socket_t fd, const struct iovec *iov, int iovcnt)
{
	return acl_fiber_readv(fd, iov, iovcnt);
}

ssize_t recv(socket_t sockfd, void *buf, size_t len, int flags)
{
	return acl_fiber_recv(sockfd, buf, len, (int) flags);
}

ssize_t recvfrom(socket_t sockfd, void *buf, size_t len, int flags,
		 struct sockaddr *src_addr, socklen_t *addrlen)
{
	return acl_fiber_recvfrom(sockfd, buf, len, flags, src_addr, addrlen);
}

ssize_t recvmsg(socket_t sockfd, struct msghdr *msg, int flags)
{
	return acl_fiber_recvmsg(sockfd, msg, flags);
}
#endif  // SYS_UNIX

#if defined(SYS_UNIX) && !defined(DISABLE_HOOK_IO)
ssize_t write(socket_t fd, const void *buf, size_t count)
{
	return acl_fiber_write(fd, buf, count);
}

ssize_t writev(socket_t fd, const struct iovec *iov, int iovcnt)
{
	return acl_fiber_writev(fd, iov, iovcnt);
}

ssize_t send(socket_t sockfd, const void *buf, size_t len, int flags)
{
	return acl_fiber_send(sockfd, buf, len, flags);
}

ssize_t sendto(socket_t sockfd, const void *buf, size_t len, int flags,
	       const struct sockaddr *dest_addr, socklen_t addrlen)
{
	return acl_fiber_sendto(sockfd, buf, len, flags, dest_addr, addrlen);
}

ssize_t sendmsg(socket_t sockfd, const struct msghdr *msg, int flags)
{
	return acl_fiber_sendmsg(sockfd, msg, flags);
}
#endif // SYS_UNIX

/****************************************************************************/

#if defined(__USE_LARGEFILE64) && !defined(DISABLE_HOOK_IO)
ssize_t sendfile64(socket_t out_fd, int in_fd, off64_t *offset, size_t count)
{
	if (out_fd == INVALID_SOCKET) {
		msg_error("%s: invalid fd: %d", __FUNCTION__, out_fd);
		return -1;
	}

	if (sys_sendfile64 == NULL) {
		hook_once();
	}

	CHECK_SET_NBLOCK(out_fd);

	while (1) {
		ssize_t n = (*sys_sendfile64)(out_fd, in_fd, offset, count);
		FILE_EVENT *fe;
		int err;

		if (!var_hook_sys_api || n >= 0) {
			return n;
		}

		err = acl_fiber_last_error();
		fiber_save_errno(err);

		if (!error_again(err)) {
			return -1;
		}

		fe = fiber_file_open_write(out_fd);
		CLR_POLLING(fe);

		if (fiber_wait_write(fe) < 0) {
			msg_error("%s(%d): fiber_wait_write error=%s, fd=%d",
				__FUNCTION__, __LINE__, last_serror(), (int) out_fd);
			fiber_file_free(fe);
			return -1;
		}

		if (IS_CLOSING(fe)) {
			msg_info("%s(%d): fd=%d being closing",
				__FUNCTION__, __LINE__, (int) out_fd);
			return 0;
		}

		if (acl_fiber_canceled(fe->fiber_w)) {
			acl_fiber_set_error(fe->fiber_w->errnum);
			return -1;
		}
	}
}
#endif
