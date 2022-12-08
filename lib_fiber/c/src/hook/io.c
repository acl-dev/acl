#include "stdafx.h"
#include "common.h"

#include "fiber.h"
#include "hook.h"
#include "io.h"

#ifdef SYS_UNIX
#define IS_INVALID(fd) (fd <= INVALID_SOCKET)
#elif defined(_WIN32) || defined(_WIN64)
#define IS_INVALID(fd) (fd == INVALID_SOCKET)
#endif

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
		// In thread mode, we only need to free the fe, because
		// no fiber was bound with the fe.
		fe = fiber_file_get(fd);
		if (fe) {
			fiber_file_free(fe);
		}
		return (*sys_close)(fd);
	}

	if (IS_INVALID(fd)) {
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
#ifdef	HAS_IO_URING
	} else if (EVENT_IS_IO_URING(ev) /* && (fe->type & TYPE_FILE) */) {
		// Don't call file_close() here, because we has called
		// canceling the IO in fiber_file_close() for IO URING event.
		// ret = file_close(ev, fe);
		ret = (*sys_close)(fd);
#endif
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

ssize_t acl_fiber_read(socket_t fd, void *buf, size_t count)
{
	FILE_EVENT* fe;

	if (IS_INVALID(fd)) {
		return -1;
	}

	if (sys_read == NULL) {
		hook_once();
	}

	if (!var_hook_sys_api) {
		return (*sys_read)(fd, buf, count);
	}

	fe = fiber_file_open_read(fd);
	return fiber_read(fe, buf, count);
}

ssize_t acl_fiber_readv(socket_t fd, const struct iovec *iov, int iovcnt)
{
	FILE_EVENT *fe;

	if (IS_INVALID(fd)) {
		return -1;
	}

	if (sys_readv == NULL) {
		hook_once();
	}

	if (!var_hook_sys_api) {
		return (*sys_readv)(fd, iov, iovcnt);
	}

	fe = fiber_file_open_read(fd);
	return fiber_readv(fe, iov, iovcnt);
}

#endif // SYS_UNIX

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

	if (IS_INVALID(sockfd)) {
		return -1;
	}

	if (sys_recv == NULL) {
		hook_once();
	}

	if (!var_hook_sys_api) {
		return (*sys_recv)(sockfd, buf, len, flags);
	}

	fe = fiber_file_open_read(sockfd);
	return fiber_recv(fe, buf, len, flags);
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

	if (IS_INVALID(sockfd)) {
		return -1;
	}

	if (sys_recvfrom == NULL) {
		hook_once();
	}

	if (!var_hook_sys_api) {
		return (*sys_recvfrom)(sockfd, buf, len, flags,
				src_addr, addrlen);
	}

	fe = fiber_file_open_read(sockfd);
	return fiber_recvfrom(fe, buf, len, flags, src_addr, addrlen);
}

#ifdef SYS_UNIX

ssize_t acl_fiber_recvmsg(socket_t sockfd, struct msghdr *msg, int flags)
{
	FILE_EVENT *fe;

	if (IS_INVALID(sockfd)) {
		return -1;
	}

	if (sys_recvmsg == NULL) {
		hook_once();
	}

	if (!var_hook_sys_api) {
		return (*sys_recvmsg)(sockfd, msg, flags);
	}

	fe = fiber_file_open_read(sockfd);
	return fiber_recvmsg(fe, msg, flags);
}
#endif

/****************************************************************************/

#ifdef SYS_UNIX
ssize_t acl_fiber_write(socket_t fd, const void *buf, size_t count)
{
	FILE_EVENT *fe;

	if (IS_INVALID(fd)) {
		return -1;
	}

	if (sys_write == NULL) {
		hook_once();
	}

	if (!var_hook_sys_api) {
		return (*sys_write)(fd, buf, count);
	}

	fe = fiber_file_open_write(fd);
	return fiber_write(fe, buf, count);
}

ssize_t acl_fiber_writev(socket_t fd, const struct iovec *iov, int iovcnt)
{
	FILE_EVENT *fe;

	if (IS_INVALID(fd)) {
		return -1;
	}

	if (sys_writev == NULL) {
		hook_once();
	}

	if (!var_hook_sys_api) {
		return (*sys_writev)(fd, iov, iovcnt);
	}

	fe = fiber_file_open_write(fd);
	return fiber_writev(fe, iov, iovcnt);
}
#endif

#ifdef SYS_WIN
int WINAPI acl_fiber_send(socket_t sockfd, const char *buf, int len, int flags)
#else
ssize_t acl_fiber_send(socket_t sockfd, const void *buf, size_t len, int flags)
#endif
{
	FILE_EVENT *fe;

	if (IS_INVALID(sockfd)) {
		return -1;
	}

	if (sys_send == NULL) {
		hook_once();
	}

	if (!var_hook_sys_api) {
		return (int) (*sys_send)(sockfd, buf, len, flags);
	}

	fe = fiber_file_open_write(sockfd);
	return fiber_send(fe, buf, len, flags);
}

#ifdef SYS_WIN
int WINAPI acl_fiber_sendto(socket_t sockfd, const char *buf, int len,
	int flags, const struct sockaddr *dest_addr, socklen_t addrlen)
#else
ssize_t acl_fiber_sendto(socket_t sockfd, const void *buf, size_t len,
	int flags, const struct sockaddr *dest_addr, socklen_t addrlen)
#endif
{
	FILE_EVENT *fe;

	if (IS_INVALID(sockfd)) {
		msg_error("%s: invalid fd: %d", __FUNCTION__, sockfd);
		return -1;
	}

	if (sys_sendto == NULL) {
		hook_once();
	}

	if (!var_hook_sys_api) {
		return (int) (*sys_sendto)(sockfd, buf, len, flags,
				dest_addr, addrlen);
	}

	fe = fiber_file_open_write(sockfd);
	return fiber_sendto(fe, buf, len, flags, dest_addr, addrlen);
}

#ifdef SYS_UNIX
ssize_t acl_fiber_sendmsg(socket_t sockfd, const struct msghdr *msg, int flags)
{
	FILE_EVENT *fe;

	if (IS_INVALID(sockfd)) {
		return -1;
	}

	if (sys_sendmsg == NULL) {
		hook_once();
	}

	if (!var_hook_sys_api) {
		return (*sys_sendmsg)(sockfd, msg, flags);
	}

	fe = fiber_file_open_write(sockfd);
	return fiber_sendmsg(fe, msg, flags);
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
	if (IS_INVALID(out_fd)) {
		msg_error("%s: invalid fd: %d", __FUNCTION__, out_fd);
		return -1;
	}

	if (sys_sendfile64 == NULL) {
		hook_once();
	}

	if (!var_hook_sys_api) {                                             \
		return (*sys_sendfile64)(out_fd, in_fd, offset, count);
	}

	return fiber_sendfile64(out_fd, in_fd, offset, count);
}

#endif
