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

		//printf("use system gettimeofday\r\n");
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
	int ret, closed;
	if (fd == INVALID_SOCKET) {
		msg_error("%s: invalid fd: %d", __FUNCTION__, fd);
		return -1;
	}

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

#ifdef	HAS_EPOLL
	/* when the fd was closed by epoll_event_close normally, the fd
	 * must be a epoll fd which was created by epoll_create function
	 * hooked in hook_net.c
	 */
	if (epoll_event_close(fd) == 0) {
		return 0;
	}
#endif

	(void) fiber_file_close(fd, &closed);
	if (closed) {
		/* return if the socket has been closed in fiber_file_close */
		return 0;
	}

	ret = (*sys_close)(fd);
	if (ret == 0) {
		return ret;
	}

	fiber_save_errno(acl_fiber_last_error());
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

		fiber_wait_read(fe);
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

	fe = fiber_file_open(fd);
	CLR_POLLING(fe);

	while (1) {
		ssize_t ret;
		int err;

		if (IS_READABLE(fe)) {
			CLR_READABLE(fe);
		} else {
			fiber_wait_read(fe);
		}

		if (acl_fiber_canceled(fe->fiber)) {
			acl_fiber_set_error(fe->fiber->errnum);
			return -1;
		}

		ret = (*sys_read)(fd, buf, count);
		if (ret >= 0) {
			return ret;
		}

		err = acl_fiber_last_error();
		fiber_save_errno(err);

		if (!error_again(err)) {
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

	fe = fiber_file_open(fd);
	CLR_POLLING(fe);

	while (1) {
		ssize_t ret;
		int err;

		if (IS_READABLE(fe)) {
			CLR_READABLE(fe);
		} else {
			fiber_wait_read(fe);
		}

		if (acl_fiber_canceled(fe->fiber)) {
			acl_fiber_set_error(fe->fiber->errnum);
			return -1;
		}

		ret = (*sys_readv)(fd, iov, iovcnt);
		if (ret >= 0) {
			return ret;
		}

		err = acl_fiber_last_error();
		fiber_save_errno(err);

		if (!error_again(err)) {
			return -1;
		}
	}
}
#endif // SYS_UNIX

#ifdef HAS_IOCP
static int fiber_iocp_read(FILE_EVENT *fe, char *buf, int len)
{
	fe->buff = buf;
	fe->size = len;
	fe->len  = 0;

	while (1) {
		int err;

		fe->mask &= ~EVENT_READ;
		fiber_wait_read(fe);
		if (fe->mask & EVENT_ERROR) {
			err = acl_fiber_last_error();
			fiber_save_errno(err);
			return -1;
		}

		if (acl_fiber_canceled(fe->fiber)) {
			acl_fiber_set_error(fe->fiber->errnum);
			return -1;
		}

		if (fe->len >= 0) {
			return fe->len;
		}

		err = acl_fiber_last_error();
		fiber_save_errno(err);

		if (!error_again(err)) {
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

	fe = fiber_file_open(sockfd);
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
		} else {
			fiber_wait_read(fe);
		}

		if (acl_fiber_canceled(fe->fiber)) {
			acl_fiber_set_error(fe->fiber->errnum);
			return -1;
		}

		ret = (int) (*sys_recv)(sockfd, buf, len, flags);
		if (ret >= 0) {
			return ret;
		}

		err = acl_fiber_last_error();
		fiber_save_errno(err);

		if (!error_again(err)) {
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

	fe = fiber_file_open(sockfd);
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
		} else {
			fiber_wait_read(fe);
		}

		if (acl_fiber_canceled(fe->fiber)) {
			acl_fiber_set_error(fe->fiber->errnum);
			return -1;
		}

		ret = (*sys_recvfrom)(sockfd, buf, len, flags, src_addr, addrlen);
		if (ret >= 0) {
			return (int) ret;
		}

		err = acl_fiber_last_error();
		fiber_save_errno(err);

		if (!error_again(err)) {
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

	fe = fiber_file_open(sockfd);
	CLR_POLLING(fe);

	while (1) {
		ssize_t ret;
		int err;

		if (IS_READABLE(fe)) {
			CLR_READABLE(fe);
		} else {
			fiber_wait_read(fe);
		}

		if (acl_fiber_canceled(fe->fiber)) {
			acl_fiber_set_error(fe->fiber->errnum);
			return -1;
		}

		ret = (*sys_recvmsg)(sockfd, msg, flags);
		if (ret >= 0) {
			return ret;
		}

		err = acl_fiber_last_error();
		fiber_save_errno(err);

		if (!error_again(err)) {
			return -1;
		}
	}
}
#endif

/****************************************************************************/

#ifdef SYS_UNIX
ssize_t acl_fiber_write(socket_t fd, const void *buf, size_t count)
{
	if (sys_write == NULL) {
		hook_once();
	}

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

		fe = fiber_file_open(fd);
		CLR_POLLING(fe);

		fiber_wait_write(fe);

		if (acl_fiber_canceled(fe->fiber)) {
			acl_fiber_set_error(fe->fiber->errnum);
			return -1;
		}
	}
}

ssize_t acl_fiber_writev(socket_t fd, const struct iovec *iov, int iovcnt)
{
	if (sys_writev == NULL) {
		hook_once();
	}

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

		fe = fiber_file_open(fd);
		CLR_POLLING(fe);

		fiber_wait_write(fe);

		if (acl_fiber_canceled(fe->fiber)) {
			acl_fiber_set_error(fe->fiber->errnum);
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
	if (sys_send == NULL) {
		hook_once();
		if (sys_send == NULL) {
			msg_error("%s: sys_send NULL", __FUNCTION__);
			return -1;
		}
	}

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

		fe = fiber_file_open(sockfd);
		CLR_POLLING(fe);

		fiber_wait_write(fe);

		if (acl_fiber_canceled(fe->fiber)) {
			acl_fiber_set_error(fe->fiber->errnum);
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
	if (sys_sendto == NULL) {
		hook_once();
		if (sys_sendto == NULL) {
			msg_error("%s: sys_sendto NULL", __FUNCTION__);
			return -1;
		}
	}

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

		fe = fiber_file_open(sockfd);
		CLR_POLLING(fe);

		fiber_wait_write(fe);

		if (acl_fiber_canceled(fe->fiber)) {
			acl_fiber_set_error(fe->fiber->errnum);
			return -1;
		}
	}
}

#ifdef SYS_UNIX
ssize_t acl_fiber_sendmsg(socket_t sockfd, const struct msghdr *msg, int flags)
{
	if (sys_sendmsg == NULL) {
		hook_once();
	}

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

		fe = fiber_file_open(sockfd);
		CLR_POLLING(fe);

		fiber_wait_write(fe);

		if (acl_fiber_canceled(fe->fiber)) {
			acl_fiber_set_error(fe->fiber->errnum);
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
	if (sys_sendfile64 == NULL) {
		hook_once();
	}

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

		fe = fiber_file_open(out_fd);
		CLR_POLLING(fe);

		fiber_wait_write(fe);

		if (acl_fiber_canceled(fe->fiber)) {
			acl_fiber_set_error(fe->fiber->errnum);
			return -1;
		}
	}
}
#endif
