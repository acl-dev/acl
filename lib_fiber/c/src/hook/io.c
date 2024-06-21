#include "stdafx.h"

#if defined(SYS_UNIX) && defined(USE_TCMALLOC)
# include <unistd.h>
# include <sys/syscall.h>
# include <sys/types.h>
#endif

#include "common.h"

#include "fiber.h"
#include "hook.h"
#include "io.h"

#ifdef SYS_UNIX
# define IS_INVALID(fd) (fd <= INVALID_SOCKET)
#elif defined(_WIN32) || defined(_WIN64)
# define IS_INVALID(fd) (fd == INVALID_SOCKET)
#endif

#if defined(SYS_UNIX) && !defined(DISABLE_HOOK)

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


#ifndef USE_TCMALLOC
	if (sys_close == NULL) {
		hook_once();
	}
#endif

	if (!var_hook_sys_api) {
#ifdef USE_TCMALLOC
		return syscall(SYS_close, fd);
#else
		// In thread mode, we only need to free the fe, because
		// no fiber was bound with the fe.
		fe = fiber_file_get(fd);
		if (fe) {
			fiber_file_free(fe);
		}

		return (*sys_close)(fd);
#endif
	}

#ifdef USE_TCMALLOC
	if (sys_close == NULL) {
		hook_once();
	}
#endif

	if (IS_INVALID(fd)) {
		msg_error("%s(%d): invalid fd=%u", __FUNCTION__, __LINE__, fd);
		return -1;
	} else if (fd >= (socket_t) var_maxfd) {
		msg_error("%s(%d): too large fd=%u", __FUNCTION__, __LINE__, fd);
		return (*sys_close)(fd);
	}

#ifdef	HAS_EPOLL
	/* when the fd was closed by epoll_close normally, the fd
	 * must be a epoll fd which was created by epoll_create function
	 * hooked in epoll.c
	 */
	if (epoll_close(fd) == 0) {
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

	ret = fiber_file_close(fe);

	// If the return value more than 0, the fe has just been bound with
	// the other fiber, we just return here and the fe will really be
	// closed and freed by the last one binding the fe.
	if (ret > 0) {
		return 0;
	}

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

# ifdef USE_TCMALLOC
	if (!var_hook_sys_api) {
		return syscall(SYS_read, fd, buf, count);
	}

	if (sys_read == NULL) {
		hook_once();
	}
# else
	if (sys_read == NULL) {
		hook_once();
	}

	if (!var_hook_sys_api) {
		return (*sys_read)(fd, buf, count);
	}
# endif

	fe = fiber_file_open(fd);
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

	fe = fiber_file_open(fd);
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

#ifdef MSG_DONTWAIT
	if ((flags & MSG_DONTWAIT)) {
		return (*sys_recv)(sockfd, buf, len, flags);
	}
#endif

#ifdef MSG_PEEK
	if ((flags & MSG_PEEK)) {
		return (*sys_recv)(sockfd, buf, len, flags);
	}
#endif

	fe = fiber_file_open(sockfd);
#ifdef SYS_WIN
	return (int) fiber_recv(fe, buf, len, flags);
#else
	return fiber_recv(fe, buf, len, flags);
#endif
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

#ifdef MSG_DONTWAIT
	if ((flags & MSG_DONTWAIT)) {
		return (*sys_recvfrom)(sockfd, buf, len, flags,
		       		src_addr, addrlen);
	}
#endif

#ifdef MSG_PEEK
	if ((flags & MSG_PEEK)) {
		return (*sys_recvfrom)(sockfd, buf, len, flags,
		       		src_addr, addrlen);
	}
#endif

	fe = fiber_file_open(sockfd);

#ifdef SYS_WIN
	return (int) fiber_recvfrom(fe, buf, len, flags, src_addr, addrlen);
#else
	return fiber_recvfrom(fe, buf, len, flags, src_addr, addrlen);
#endif
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

# ifdef MSG_DONTWAIT
	if ((flags & MSG_DONTWAIT)) {
		return (*sys_recvmsg)(sockfd, msg, flags);
	}
# endif

# ifdef MSG_PEEK
	if ((flags & MSG_PEEK)) {
		return (*sys_recvmsg)(sockfd, msg, flags);
	}
# endif

	fe = fiber_file_open(sockfd);
	return fiber_recvmsg(fe, msg, flags);
}

# ifdef HAS_MMSG
int acl_fiber_recvmmsg(int sockfd, struct mmsghdr *msgvec, unsigned int vlen,
	int flags, const struct timespec *timeout)
{
	FILE_EVENT *fe;

	if (IS_INVALID(sockfd)) {
		return -1;
	}

	if (sys_recvmmsg == NULL) {
		hook_once();
	}

	if (sys_recvmmsg == NULL) {
		printf("%s(%d): sys_recvmmsg NULL\r\n", __FUNCTION__, __LINE__);
		return -1;
	}

	if (!var_hook_sys_api) {
		return (*sys_recvmmsg)(sockfd, msgvec, vlen, flags, timeout);
	}

	fe = fiber_file_open(sockfd);
	return fiber_recvmmsg(fe, msgvec, vlen, flags, timeout);
}
# endif  // HAS_MMSG

#endif   // SYS_UNIX

/****************************************************************************/

#ifdef SYS_UNIX

ssize_t acl_fiber_write(socket_t fd, const void *buf, size_t count)
{
	FILE_EVENT *fe;

	if (IS_INVALID(fd)) {
		return -1;
	}

# ifdef USE_TCMALLOC
	if (!var_hook_sys_api) {
		return syscall(SYS_write, fd, buf, count);
	}

	if (sys_write == NULL) {
		hook_once();
	}
# else
	if (sys_write == NULL) {
		hook_once();
	}

	if (!var_hook_sys_api) {
		return (*sys_write)(fd, buf, count);
	}
# endif

	fe = fiber_file_open(fd);
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

	fe = fiber_file_open(fd);
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

	fe = fiber_file_open(sockfd);

#ifdef SYS_WIN
	return (int) fiber_send(fe, buf, len, flags);
#else
	return fiber_send(fe, buf, len, flags);
#endif
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

	fe = fiber_file_open(sockfd);

#ifdef SYS_WIN
	return (int) fiber_sendto(fe, buf, len, flags, dest_addr, addrlen);
#else
	return fiber_sendto(fe, buf, len, flags, dest_addr, addrlen);
#endif
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

	fe = fiber_file_open(sockfd);
	return fiber_sendmsg(fe, msg, flags);
}

# ifdef HAS_MMSG
int acl_fiber_sendmmsg(int sockfd, struct mmsghdr *msgvec, unsigned int vlen,
	int flags)
{
	FILE_EVENT *fe;

	if (IS_INVALID(sockfd)) {
		return -1;
	}

	if (sys_sendmmsg == NULL) {
		hook_once();
	}

	if (sys_sendmmsg == NULL) {
		printf("%s(%d): sys_sendmmsg NULL\r\n", __FUNCTION__, __LINE__);
		return -1;
	}

	if (!var_hook_sys_api) {
		return (*sys_sendmmsg)(sockfd, msgvec, vlen, flags);
	}

	fe = fiber_file_open(sockfd);
	return fiber_sendmmsg(fe, msgvec, vlen, flags);
}
# endif

#endif

/****************************************************************************/

#if defined(SYS_UNIX) && !defined(DISABLE_HOOK)

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

#if defined(SYS_UNIX) && !defined(DISABLE_HOOK)

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

# ifdef HAS_MMSG
#  if defined(HOOK_MMSG_UBUNTU)
int recvmmsg(int sockfd, struct mmsghdr *msgvec, unsigned int vlen,
	int flags, struct timespec *timeout)
{
	return acl_fiber_recvmmsg(sockfd, msgvec, vlen, flags, timeout);
}
#  elif defined(HOOK_MMSG_CENTOS)
int recvmmsg(int sockfd, struct mmsghdr *msgvec, unsigned int vlen,
	int flags, const struct timespec *timeout)
{
	return acl_fiber_recvmmsg(sockfd, msgvec, vlen, flags, timeout);
}
#  endif

int sendmmsg(int sockfd, struct mmsghdr *msgvec, unsigned int vlen, int flags)
{
	return acl_fiber_sendmmsg(sockfd, msgvec, vlen, flags);
}
# endif

#endif // SYS_UNIX

/****************************************************************************/

#if defined(__USE_LARGEFILE64) && !defined(DISABLE_HOOK)

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
