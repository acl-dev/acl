#include "stdafx.h"
#include "common.h"

#include "fiber.h"

#ifdef SYS_WIN
typedef int      (WINAPI *close_fn)(socket_t);
typedef int      (WINAPI *recv_fn)(socket_t, char *, int, int);
typedef int      (WINAPI *recvfrom_fn)(socket_t, char *, int, int,
	struct sockaddr *, socklen_t *);
typedef int      (WINAPI *send_fn)(socket_t, const char *, int, int);
typedef int      (WINAPI *sendto_fn)(socket_t, const char *, int, int,
	const struct sockaddr *, socklen_t);
#elif !defined(USE_SYSCALL)
typedef unsigned (*sleep_fn)(unsigned int seconds);
typedef int      (*close_fn)(int);
typedef ssize_t  (*read_fn)(socket_t, void *, size_t);
typedef ssize_t  (*readv_fn)(socket_t, const struct iovec *, int);
typedef ssize_t  (*recv_fn)(socket_t, void *, size_t, int);
typedef ssize_t  (*recvfrom_fn)(socket_t, void *, size_t, int,
	struct sockaddr *, socklen_t *);
typedef ssize_t  (*recvmsg_fn)(socket_t, struct msghdr *, int);
typedef ssize_t  (*write_fn)(socket_t, const void *, size_t);
typedef ssize_t  (*writev_fn)(socket_t, const struct iovec *, int);
typedef ssize_t  (*send_fn)(socket_t, const void *, size_t, int);
typedef ssize_t  (*sendto_fn)(socket_t, const void *, size_t, int,
	const struct sockaddr *, socklen_t);
typedef ssize_t  (*sendmsg_fn)(socket_t, const struct msghdr *, int);
# ifdef  __USE_LARGEFILE64
typedef ssize_t  (*sendfile64_fn)(socket_t, int, off64_t*, size_t);
# endif
#endif

#if defined(SYS_WIN)

static close_fn    __sys_close    = NULL;
static recv_fn     __sys_recv     = NULL;
static recvfrom_fn __sys_recvfrom = NULL;

static send_fn     __sys_send     = NULL;
static sendto_fn   __sys_sendto   = NULL;

#elif defined(SYS_UNIX) && !defined(USE_SYSCALL)

static close_fn    __sys_close    = NULL;
static recv_fn     __sys_recv     = NULL;
static recvfrom_fn __sys_recvfrom = NULL;

static send_fn     __sys_send     = NULL;
static sendto_fn   __sys_sendto   = NULL;

static sleep_fn    __sys_sleep    = NULL;
static read_fn     __sys_read     = NULL;
static readv_fn    __sys_readv    = NULL;
static recvmsg_fn  __sys_recvmsg  = NULL;

static write_fn    __sys_write    = NULL;
static writev_fn   __sys_writev   = NULL;
static sendmsg_fn  __sys_sendmsg  = NULL;

# ifdef __USE_LARGEFILE64
static sendfile64_fn __sys_sendfile64 = NULL;
# endif

#endif

#if defined(SYS_UNIX) && defined(USE_SYSCALL)
#include <sys/syscall.h>

static unsigned __sys_sleep(unsigned int seconds)
{
#ifdef SYS_sleep
	return syscall(SYS_sleep, seconds);
#else
	return 0;
#endif
}

static int __sys_close(int fd)
{
	return syscall(SYS_close, fd);
}

static ssize_t __sys_read(socket_t fd, void *buf, size_t count)
{
	return syscall(SYS_read, fd, buf, count);
}

static ssize_t __sys_readv(socket_t fd, const struct iovec *iov, int iovcnt)
{
	return syscall(SYS_readv, fd, iov, iovcnt);
}

static ssize_t __sys_recv(socket_t fd, void *buf, size_t len, int flags)
{
#ifdef SYS_recv
	return syscall(SYS_recv, fd, buf, len, flags);
#else
	return -1;
#endif
}

static ssize_t __sys_recvfrom(int fd, void *buf, size_t len, int flags,
	struct sockaddr *src_addr, socklen_t *addrlen)
{
#ifdef SYS_recvfrom
	return syscall(SYS_recvfrom, fd, buf, len, flags, src_addr, addrlen);
#else
	return -1;
#endif
}

static ssize_t __sys_recvmsg(int fd, struct msghdr *msg, int flags)
{
	return syscall(SYS_recvmsg, fd, msg, flags);
}

static ssize_t __sys_write(int fd, const void *buf, size_t count)
{
	return syscall(SYS_write, fd, buf, count);
}

static ssize_t __sys_writev(int fd, const struct iovec *iov, int iovcnt)
{
	return syscall(SYS_writev, fd, iov, iovcnt);
}

static ssize_t __sys_send(int fd, const void *buf, size_t len, int flags)
{
#ifdef SYS_send
	return syscall(SYS_send, fd, buf, len, flags);
#else
	return -1;
#endif
}

static ssize_t __sys_sendto(int fd, const void *buf, size_t len, int flags,
	const struct sockaddr *dest_addr, socklen_t addrlen)
{
	return syscall(SYS_sendto, fd, buf, len, flags, dest_addr, addrlen);
}

static ssize_t __sys_sendmsg(int fd, const struct msghdr *msg, int flags)
{
	return syscall(SYS_sendmsg, fd, msg, flags);
}

# ifdef  __USE_LARGEFILE64
static ssize_t __sys_sendfile64(int ofd, int ifd, off64_t *offset, size_t count)
{
#ifdef SYS_sendfile64
	return syscall(SYS_sendfile64, ofd, ifd, offset, count);
#else
	return -1;
#endif
}
# endif

#endif

static void hook_api(void)
{
#ifdef SYS_WIN
	__sys_close    = closesocket;
	__sys_recv     = (recv_fn) recv;
	__sys_recvfrom = (recvfrom_fn) recvfrom;
	__sys_send     = (send_fn) send;
	__sys_sendto   = (sendto_fn) sendto;
#elif !defined(USE_SYSCALL)
	__sys_sleep      = (sleep_fn) dlsym(RTLD_NEXT, "sleep");
	assert(__sys_sleep);

	__sys_close      = (close_fn) dlsym(RTLD_NEXT, "close");
	assert(__sys_close);

	__sys_read       = (read_fn) dlsym(RTLD_NEXT, "read");
	assert(__sys_read);

	__sys_readv      = (readv_fn) dlsym(RTLD_NEXT, "readv");
	assert(__sys_readv);

	__sys_recv       = (recv_fn) dlsym(RTLD_NEXT, "recv");
	assert(__sys_recv);

	__sys_recvfrom   = (recvfrom_fn) dlsym(RTLD_NEXT, "recvfrom");
	assert(__sys_recvfrom);

	__sys_recvmsg    = (recvmsg_fn) dlsym(RTLD_NEXT, "recvmsg");
	assert(__sys_recvmsg);

	__sys_write      = (write_fn) dlsym(RTLD_NEXT, "write");
	assert(__sys_write);

	__sys_writev     = (writev_fn) dlsym(RTLD_NEXT, "writev");
	assert(__sys_writev);

	__sys_send       = (send_fn) dlsym(RTLD_NEXT, "send");
	assert(__sys_send);

	__sys_sendto     = (sendto_fn) dlsym(RTLD_NEXT, "sendto");
	assert(__sys_sendto);

	__sys_sendmsg    = (sendmsg_fn) dlsym(RTLD_NEXT, "sendmsg");
	assert(__sys_sendmsg);

# ifdef __USE_LARGEFILE64
	__sys_sendfile64 = (sendfile64_fn) dlsym(RTLD_NEXT, "sendfile64");
	assert(__sys_sendfile64);
# endif
#endif
}

static pthread_once_t __once_control = PTHREAD_ONCE_INIT;

static void hook_init(void)
{
	if (pthread_once(&__once_control, hook_api) != 0) {
		abort();
	}
}

#ifdef SYS_UNIX
unsigned int sleep(unsigned int seconds)
{
	if (!var_hook_sys_api) {
#ifndef USE_SYSCALL
		if (__sys_sleep == NULL) {
			hook_init();
		}
#endif

		//printf("use system gettimeofday\r\n");
		return __sys_sleep(seconds);
	}

	return acl_fiber_sleep(seconds);
}

int close(socket_t fd)
{
	return acl_fiber_close(fd);
}
#endif

#if defined(SYS_WIN)
int WINAPI acl_fiber_close(socket_t fd)
#else
int acl_fiber_close(socket_t fd)
#endif
{
	int ret, closed;
	if (fd == INVALID_SOCKET) {
		msg_error("%s: invalid fd: %d", __FUNCTION__, fd);
		return -1;
	}

#ifndef USE_SYSCALL
	if (__sys_close == NULL) {
		hook_init();
	}
#endif

	if (!var_hook_sys_api) {
		return __sys_close(fd);
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

	ret = __sys_close(fd);
	if (ret == 0) {
		return ret;
	}

	fiber_save_errno(acl_fiber_last_error());
	return ret;
}

/****************************************************************************/

#if FIBER_EAGAIN == FIBER_EWOULDBLOCK
# define error_again(x) ((x) == FIBER_EAGAIN)
#else
# define error_again(x) ((x) == FIBER_EAGAIN || (x) == FIBER_EWOULDBLOCK)
#endif

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

#ifndef	USE_SYSCALL
	if (__sys_read == NULL) {
		hook_init();
	}
#endif

	if (!var_hook_sys_api) {
		return __sys_read(fd, buf, count);
	}

	fe = fiber_file_open(fd);

	while (1) {
		ssize_t n;
		int err;

		n = __sys_read(fd, buf, count);
		if (n >= 0) {
			return n;
		}

		err = acl_fiber_last_error();
		fiber_save_errno(err);

		if (acl_fiber_killed(fe->fiber)) {
			msg_info("%s(%d), %s: fiber-%u is existing",
				__FILE__, __LINE__, __FUNCTION__,
				acl_fiber_id(fe->fiber));
			return -1;
		}
		if (!error_again(err)) {
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

#ifndef	USE_SYSCALL
	if (__sys_read == NULL) {
		hook_init();
	}
#endif

	if (!var_hook_sys_api) {
		return __sys_read(fd, buf, count);
	}

	fe = fiber_file_open(fd);

	while (1) {
		ssize_t ret;
		int err;

		if (IS_READABLE(fe)) {
			CLR_READABLE(fe);
		} else {
			fiber_wait_read(fe);
		}

		ret = __sys_read(fd, buf, count);
		if (ret >= 0) {
			return ret;
		}

		err = acl_fiber_last_error();
		fiber_save_errno(err);

		if (acl_fiber_killed(fe->fiber)) {
			msg_info("%s(%d), %s: fiber-%u is existing",
				__FILE__, __LINE__, __FUNCTION__,
				acl_fiber_id(fe->fiber));
			return -1;
		}

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

#ifndef USE_SYSCALL
	if (__sys_readv == NULL) {
		hook_init();
	}
#endif

	if (!var_hook_sys_api) {
		return __sys_readv(fd, iov, iovcnt);
	}

	fe = fiber_file_open(fd);
	while (1) {
		ssize_t ret;
		int err;

		if (IS_READABLE(fe)) {
			CLR_READABLE(fe);
		} else {
			fiber_wait_read(fe);
		}

		ret = __sys_readv(fd, iov, iovcnt);
		if (ret >= 0) {
			return ret;
		}

		err = acl_fiber_last_error();
		fiber_save_errno(err);

		if (acl_fiber_killed(fe->fiber)) {
			msg_info("%s(%d), %s: fiber-%u is existing",
				__FILE__, __LINE__, __FUNCTION__,
				acl_fiber_id(fe->fiber));
			return -1;
		}

		if (!error_again(err)) {
			return -1;
		}
	}
}
#endif

static int fiber_iocp_read(FILE_EVENT *fe, char *buf, int len)
{
	fe->buf  = buf;
	fe->size = len;
	fe->len  = 0;

	while (1) {
		int err;

		fiber_wait_read(fe);
		if (fe->mask & EVENT_ERROR) {
			return -1;
		}

		err = acl_fiber_last_error();
		fiber_save_errno(err);

		if (acl_fiber_killed(fe->fiber)) {
			msg_info("%s(%d), %s: fiber-%u is existing",
				__FILE__, __LINE__, __FUNCTION__,
				acl_fiber_id(fe->fiber));
			return -1;
		}

		return fe->len;
	}
}

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

#ifndef USE_SYSCALL
	if (__sys_recv == NULL) {
		hook_init();
	}
#endif

	if (!var_hook_sys_api) {
		return __sys_recv(sockfd, buf, len, flags);
	}

	fe = fiber_file_open(sockfd);

	if (EVENT_IS_IOCP(fiber_io_event())) {
		return fiber_iocp_read(fe, buf, (int) len);
	}

	while (1) {
		int ret;
		int err;

		if (IS_READABLE(fe)) {
			CLR_READABLE(fe);
		} else {
			fiber_wait_read(fe);
		}

		ret = (int) __sys_recv(sockfd, buf, len, flags);
		if (ret >= 0) {
			return ret;
		}

		err = acl_fiber_last_error();
		fiber_save_errno(err);

		if (acl_fiber_killed(fe->fiber)) {
			msg_info("%s(%d), %s: fiber-%u is existing",
				__FILE__, __LINE__, __FUNCTION__,
				acl_fiber_id(fe->fiber));
			return -1;
		}

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

#ifndef USE_SYSCALL
	if (__sys_recvfrom == NULL) {
		hook_init();
	}
#endif

	if (!var_hook_sys_api) {
		return __sys_recvfrom(sockfd, buf, len, flags,
				src_addr, addrlen);
	}

	fe = fiber_file_open(sockfd);

	if (EVENT_IS_IOCP(fiber_io_event())) {
		return fiber_iocp_read(fe, buf, (int) len);
	}

	while (1) {
		ssize_t ret;
		int err;

		if (IS_READABLE(fe)) {
			CLR_READABLE(fe);
		} else {
			fiber_wait_read(fe);
		}

		ret = __sys_recvfrom(sockfd, buf, len, flags,
				src_addr, addrlen);
		if (ret >= 0) {
			return ret;
		}

		err = acl_fiber_last_error();
		fiber_save_errno(err);

		if (acl_fiber_killed(fe->fiber)) {
			msg_info("%s(%d), %s: fiber-%u is existing",
				__FILE__, __LINE__, __FUNCTION__,
				acl_fiber_id(fe->fiber));
			return -1;
		}

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

#ifdef USE_SYSCALL
	if (__sys_recvmsg == NULL) {
		hook_init();
	}
#endif

	if (!var_hook_sys_api) {
		return __sys_recvmsg(sockfd, msg, flags);
	}

	fe = fiber_file_open(sockfd);

	while (1) {
		ssize_t ret;
		int err;

		if (IS_READABLE(fe)) {
			CLR_READABLE(fe);
		} else {
			fiber_wait_read(fe);
		}

		ret = __sys_recvmsg(sockfd, msg, flags);
		if (ret >= 0) {
			return ret;
		}

		err = acl_fiber_last_error();
		fiber_save_errno(err);

		if (acl_fiber_killed(fe->fiber)) {
			msg_info("%s(%d), %s: fiber-%u is existing",
				__FILE__, __LINE__, __FUNCTION__,
				acl_fiber_id(fe->fiber));
			return -1;
		}

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
#ifndef USE_SYSCALL
	if (__sys_write == NULL) {
		hook_init();
	}
#endif

	while (1) {
		ssize_t n = __sys_write(fd, buf, count);
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
		fiber_wait_write(fe);

		if (acl_fiber_killed(fe->fiber)) {
			msg_info("%s(%d), %s: fiber-%u is existing",
				__FILE__, __LINE__, __FUNCTION__,
				acl_fiber_id(fe->fiber));
			return -1;
		}
	}
}

ssize_t acl_fiber_writev(socket_t fd, const struct iovec *iov, int iovcnt)
{
#ifndef USE_SYSCALL
	if (__sys_writev == NULL) {
		hook_init();
	}
#endif

	while (1) {
		int n = (int) __sys_writev(fd, iov, iovcnt);
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
		fiber_wait_write(fe);

		if (acl_fiber_killed(fe->fiber)) {
			msg_info("%s(%d), %s: fiber-%u is existing",
				__FILE__, __LINE__, __FUNCTION__,
				acl_fiber_id(fe->fiber));
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
#ifndef USE_SYSCALL
	if (__sys_send == NULL) {
		hook_init();
	}
#endif

	while (1) {
		int n = (int) __sys_send(sockfd, buf, len, flags);
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
		fiber_wait_write(fe);

		if (acl_fiber_killed(fe->fiber)) {
			msg_info("%s(%d), %s: fiber-%u is existing",
				__FILE__, __LINE__, __FUNCTION__,
				acl_fiber_id(fe->fiber));
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
#ifndef USE_SYSCALL
	if (__sys_sendto == NULL) {
		hook_init();
	}
#endif

	while (1) {
		int n = (int) __sys_sendto(sockfd, buf, len, flags,
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
		fiber_wait_write(fe);

		if (acl_fiber_killed(fe->fiber)) {
			msg_info("%s(%d), %s: fiber-%u is existing",
				__FILE__, __LINE__, __FUNCTION__,
				acl_fiber_id(fe->fiber));
			return -1;
		}
	}
}

#ifdef SYS_UNIX
ssize_t acl_fiber_sendmsg(socket_t sockfd, const struct msghdr *msg, int flags)
{
#ifndef USE_SYSCALL
	if (__sys_sendmsg == NULL) {
		hook_init();
	}
#endif

	while (1) {
		ssize_t n = __sys_sendmsg(sockfd, msg, flags);
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
		fiber_wait_write(fe);

		if (acl_fiber_killed(fe->fiber)) {
			msg_info("%s(%d), %s: fiber-%u is existing",
				__FILE__, __LINE__, __FUNCTION__,
				acl_fiber_id(fe->fiber));
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
	if (__sys_sendfile64 == NULL) {
		hook_init();
	}

	while (1) {
		ssize_t n = __sys_sendfile64(out_fd, in_fd, offset, count);
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
		fiber_wait_write(fe);

		if (acl_fiber_killed(fe->fiber)) {
			msg_info("%s(%d), %s: fiber-%u is existing",
				__FILE__, __LINE__, __FUNCTION__,
				acl_fiber_id(fe->fiber));
			return -1;
		}
	}
}
#endif
