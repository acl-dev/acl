#include "stdafx.h"

#ifdef HAS_PIPE2
#define _GNU_SOURCE
#endif

#include <fcntl.h>
#include <unistd.h>

#define __USE_GNU
#include <dlfcn.h>
#include <sys/stat.h>
#include "fiber.h"

typedef unsigned int (*sleep_fn)(unsigned int seconds);
typedef int     (*pipe_fn)(int pipefd[2]);
#ifdef HAS_PIPE2
typedef int     (*pipe2_fn)(int pipefd[2], int flags);
#endif
typedef FILE   *(*popen_fn)(const char *, const char *);
typedef int     (*pclose_fn)(FILE *);
typedef int     (*close_fn)(int);
typedef ssize_t (*read_fn)(int, void *, size_t);
typedef ssize_t (*readv_fn)(int, const struct iovec *, int);
typedef ssize_t (*recv_fn)(int, void *, size_t, int);
typedef ssize_t (*recvfrom_fn)(int, void *, size_t, int,
	struct sockaddr *, socklen_t *);
typedef ssize_t (*recvmsg_fn)(int, struct msghdr *, int);
typedef ssize_t (*write_fn)(int, const void *, size_t);
typedef ssize_t (*writev_fn)(int, const struct iovec *, int);
typedef ssize_t (*send_fn)(int, const void *, size_t, int);
typedef ssize_t (*sendto_fn)(int, const void *, size_t, int,
	const struct sockaddr *, socklen_t);
typedef ssize_t (*sendmsg_fn)(int, const struct msghdr *, int);

static sleep_fn    __sys_sleep    = NULL;
static pipe_fn     __sys_pipe     = NULL;
#ifdef HAS_PIPE2
static pipe2_fn    __sys_pipe2    = NULL;
#endif
static popen_fn    __sys_popen    = NULL;
static pclose_fn   __sys_pclose   = NULL;
static close_fn    __sys_close    = NULL;
static read_fn     __sys_read     = NULL;
static readv_fn    __sys_readv    = NULL;
static recv_fn     __sys_recv     = NULL;
static recvfrom_fn __sys_recvfrom = NULL;
static recvmsg_fn  __sys_recvmsg  = NULL;

static write_fn    __sys_write    = NULL;
static writev_fn   __sys_writev   = NULL;
static send_fn     __sys_send     = NULL;
static sendto_fn   __sys_sendto   = NULL;
static sendmsg_fn  __sys_sendmsg  = NULL;

void hook_io(void)
{
	static int __called = 0;

	if (__called)
		return;

	__called++;

	__sys_sleep    = (sleep_fn) dlsym(RTLD_NEXT, "sleep");
	__sys_pipe     = (pipe_fn) dlsym(RTLD_NEXT, "pipe");
#ifdef HAS_PIPE2
	__sys_pipe2    = (pipe2_fn) dlsym(RTLD_NEXT, "pipe2");
#endif
	__sys_popen    = (popen_fn) dlsym(RTLD_NEXT, "popen");
	__sys_pclose   = (pclose_fn) dlsym(RTLD_NEXT, "pclose");
	__sys_close    = (close_fn) dlsym(RTLD_NEXT, "close");
	__sys_read     = (read_fn) dlsym(RTLD_NEXT, "read");
	__sys_readv    = (readv_fn) dlsym(RTLD_NEXT, "readv");
	__sys_recv     = (recv_fn) dlsym(RTLD_NEXT, "recv");
	__sys_recvfrom = (recvfrom_fn) dlsym(RTLD_NEXT, "recvfrom");
	__sys_recvmsg  = (recvmsg_fn) dlsym(RTLD_NEXT, "recvmsg");

	__sys_write    = (write_fn) dlsym(RTLD_NEXT, "write");
	__sys_writev   = (writev_fn) dlsym(RTLD_NEXT, "writev");
	__sys_send     = (send_fn) dlsym(RTLD_NEXT, "send");
	__sys_sendto   = (sendto_fn) dlsym(RTLD_NEXT, "sendto");
	__sys_sendmsg  = (sendmsg_fn) dlsym(RTLD_NEXT, "sendmsg");
}

unsigned int sleep(unsigned int seconds)
{
	if (!acl_var_hook_sys_api)
		return __sys_sleep(seconds);

	return acl_fiber_sleep(seconds);
}

int pipe(int pipefd[2])
{
	int ret = __sys_pipe(pipefd);

	if (!acl_var_hook_sys_api)
		return ret;

	if (ret < 0)
		fiber_save_errno();
	return ret;
}

#ifdef HAS_PIPE2
int pipe2(int pipefd[2], int flags)
{
	int ret = __sys_pipe2(pipefd, flags);

	if (!acl_var_hook_sys_api)
		return ret;

	if (ret < 0)
		fiber_save_errno();
	return ret;
}
#endif

FILE *popen(const char *command, const char *type)
{
	FILE *fp = __sys_popen(command, type);

	if (!acl_var_hook_sys_api)
		return fp;

	if (fp == NULL)
		fiber_save_errno();
	return fp;
}

int close(int fd)
{
	int ret;

	if (fd < 0) {
		acl_msg_error("%s: invalid fd: %d", __FUNCTION__, fd);
		return -1;
	}

	if (!acl_var_hook_sys_api)
		return __sys_close(fd);

	fiber_io_close(fd);

	/* when the fd was closed by epoll_event_close normally, the fd
	 * must be a epoll fd which was created by epoll_create function
	 * hooked in hook_net.c
	 */
	if (epoll_event_close(fd) == 0)
		return 0;

	ret = __sys_close(fd);
	if (ret == 0)
		return ret;

	fiber_save_errno();
	return ret;
}

/****************************************************************************/

#define READ_WAIT_FIRST

#ifdef READ_WAIT_FIRST

#if 0
static int check_fdtype(int fd)
{
	struct stat s;

	if (fstat(fd, &s) < 0) {
		acl_msg_info("fd: %d fstat error", fd);
		return -1;
	}

	if (S_ISSOCK(s.st_mode))
		acl_msg_info("fd %d S_ISSOCK", fd);
	else if (S_ISFIFO(s.st_mode))
		acl_msg_info("fd %d S_ISFIFO", fd);
	else if (S_ISCHR(s.st_mode))
		acl_msg_info("fd %d S_ISCHR", fd);

	if (S_ISSOCK(s.st_mode) || S_ISFIFO(s.st_mode) || S_ISCHR(s.st_mode))
		return 0;

	if (S_ISLNK(s.st_mode))
		acl_msg_info("fd %d S_ISLNK", fd);
	else if (S_ISREG(s.st_mode))
		acl_msg_info("fd %d S_ISREG", fd);
	else if (S_ISDIR(s.st_mode))
		acl_msg_info("fd %d S_ISDIR", fd);
	else if (S_ISCHR(s.st_mode))
		acl_msg_info("fd %d S_ISCHR", fd);
	else if (S_ISBLK(s.st_mode))
		acl_msg_info("fd %d S_ISBLK", fd);
	else if (S_ISFIFO(s.st_mode))
		acl_msg_info("fd %d S_ISFIFO", fd);
	else if (S_ISSOCK(s.st_mode))
		acl_msg_info("fd %d S_ISSOCK", fd);
	else
		acl_msg_info("fd: %d, unknoiwn st_mode: %d", fd, s.st_mode);

	return -1;
}
#endif

inline ssize_t fiber_read(int fd, void *buf, size_t count)
{
	ssize_t ret;
	EVENT  *ev;

	if (fd < 0) {
		acl_msg_error("%s: invalid fd: %d", __FUNCTION__, fd);
		return -1;
	}

	if (!acl_var_hook_sys_api)
		return __sys_read(fd, buf, count);

	ev = fiber_io_event();
	if (ev && event_readable(ev, fd)) {
		event_clear_readable(ev, fd);

		ret = __sys_read(fd, buf, count);
		if (ret < 0)
			fiber_save_errno();
		return ret;
	}

	fiber_wait_read(fd);
	if (ev)
		event_clear_readable(ev, fd);

	ret = __sys_read(fd, buf, count);
	if (ret < 0)
		fiber_save_errno();
	return ret;
}

inline ssize_t fiber_readv(int fd, const struct iovec *iov, int iovcnt)
{
	ssize_t ret;
	EVENT  *ev;

	if (fd < 0) {
		acl_msg_error("%s: invalid fd: %d", __FUNCTION__, fd);
		return -1;
	}

	if (!acl_var_hook_sys_api)
		return __sys_readv(fd, iov, iovcnt);

	ev = fiber_io_event();
	if (ev && event_readable(ev, fd)) {
		event_clear_readable(ev, fd);

		ret = __sys_readv(fd, iov, iovcnt);
		if (ret < 0)
			fiber_save_errno();
		return ret;
	}

	fiber_wait_read(fd);
	if (ev)
		event_clear_readable(ev, fd);

	ret = __sys_readv(fd, iov, iovcnt);
	if (ret < 0)
		fiber_save_errno();
	return ret;
}

inline ssize_t fiber_recv(int sockfd, void *buf, size_t len, int flags)
{
	ssize_t ret;
	EVENT  *ev;

	if (sockfd < 0) {
		acl_msg_error("%s: invalid sockfd: %d", __FUNCTION__, sockfd);
		return -1;
	}

	if (!acl_var_hook_sys_api)
		return __sys_recv(sockfd, buf, len, flags);

	ev = fiber_io_event();
	if (ev && event_readable(ev, sockfd)) {
		event_clear_readable(ev, sockfd);

		ret = __sys_recv(sockfd, buf, len, flags);
		if (ret < 0)
			fiber_save_errno();
		return ret;
	}

	fiber_wait_read(sockfd);
	if (ev)
		event_clear_readable(ev, sockfd);

	ret = __sys_recv(sockfd, buf, len, flags);
	if (ret < 0)
		fiber_save_errno();
	return ret;
}

inline ssize_t fiber_recvfrom(int sockfd, void *buf, size_t len, int flags,
	struct sockaddr *src_addr, socklen_t *addrlen)
{
	ssize_t ret;
	EVENT  *ev;

	if (sockfd < 0) {
		acl_msg_error("%s: invalid sockfd: %d", __FUNCTION__, sockfd);
		return -1;
	}

	if (!acl_var_hook_sys_api)
		return __sys_recvfrom(sockfd, buf, len,
				flags, src_addr, addrlen);

	ev = fiber_io_event();
	if (ev && event_readable(ev, sockfd)) {
		event_clear_readable(ev, sockfd);

		ret = __sys_recvfrom(sockfd, buf, len,
				flags, src_addr, addrlen);
		if (ret < 0)
			fiber_save_errno();
		return ret;
	}

	fiber_wait_read(sockfd);
	if (ev)
		event_clear_readable(ev, sockfd);

	ret = __sys_recvfrom(sockfd, buf, len, flags, src_addr, addrlen);
	if (ret < 0)
		fiber_save_errno();
	return ret;
}

inline ssize_t fiber_recvmsg(int sockfd, struct msghdr *msg, int flags)
{
	ssize_t ret;
	EVENT  *ev;

	if (sockfd < 0) {
		acl_msg_error("%s: invalid sockfd: %d", __FUNCTION__, sockfd);
		return -1;
	}

	if (!acl_var_hook_sys_api)
		return __sys_recvmsg(sockfd, msg, flags);

	ev = fiber_io_event();
	if (ev && event_readable(ev, sockfd)) {
		event_clear_readable(ev, sockfd);

		ret = __sys_recvmsg(sockfd, msg, flags);
		if (ret < 0)
			fiber_save_errno();
		return ret;
	}

	fiber_wait_read(sockfd);
	if (ev)
		event_clear_readable(ev, sockfd);

	ret = __sys_recvmsg(sockfd, msg, flags);
	if (ret < 0)
		fiber_save_errno();
	return ret;
}

#else

inline ssize_t fiber_read(int fd, void *buf, size_t count)
{
	while (1) {
		ssize_t n = __sys_read(fd, buf, count);

		if (!acl_var_hook_sys_api)
			return n;

		if (n >= 0)
			return n;

		fiber_save_errno();

#if EAGAIN == EWOULDBLOCK
		if (errno != EAGAIN)
#else
		if (errno != EAGAIN && errno != EWOULDBLOCK)
#endif
			return -1;
		fiber_wait_read(fd);
	}
}

inline ssize_t fiber_readv(int fd, const struct iovec *iov, int iovcnt)
{
	while (1) {
		ssize_t n = __sys_readv(fd, iov, iovcnt);

		if (!acl_var_hook_sys_api)
			return n;

		if (n >= 0)
			return n;

		fiber_save_errno();

#if EAGAIN == EWOULDBLOCK
		if (errno != EAGAIN)
#else
		if (errno != EAGAIN && errno != EWOULDBLOCK)
#endif
			return -1;

		fiber_wait_read(fd);
	}
}

inline ssize_t fiber_recv(int sockfd, void *buf, size_t len, int flags)
{
	while (1) {
		ssize_t n = __sys_recv(sockfd, buf, len, flags);

		if (!acl_var_hook_sys_api)
			return n;

		if (n >= 0)
			return n;

		fiber_save_errno();

#if EAGAIN == EWOULDBLOCK
		if (errno != EAGAIN)
#else
		if (errno != EAGAIN && errno != EWOULDBLOCK)
#endif
			return -1;

		fiber_wait_read(sockfd);
	}
}

inline ssize_t fiber_recvfrom(int sockfd, void *buf, size_t len, int flags,
	struct sockaddr *src_addr, socklen_t *addrlen)
{
	while (1) {
		ssize_t n = __sys_recvfrom(sockfd, buf, len, flags,
				src_addr, addrlen);

		if (!acl_var_hook_sys_api)
			return n;

		if (n >= 0)
			return n;

		fiber_save_errno();

#if EAGAIN == EWOULDBLOCK
		if (errno != EAGAIN)
#else
		if (errno != EAGAIN && errno != EWOULDBLOCK)
#endif
			return -1;

		fiber_wait_read(sockfd);
	}
}

inline ssize_t fiber_recvmsg(int sockfd, struct msghdr *msg, int flags)
{
	while (1) {
		ssize_t n = __sys_recvmsg(sockfd, msg, flags);

		if (!acl_var_hook_sys_api)
			return n;

		if (n >= 0)
			return n;

		fiber_save_errno();

#if EAGAIN == EWOULDBLOCK
		if (errno != EAGAIN)
#else
		if (errno != EAGAIN && errno != EWOULDBLOCK)
#endif
			return -1;

		fiber_wait_read(sockfd);
	}
}

#endif  /* READ_WAIT_FIRST */

/****************************************************************************/

inline ssize_t fiber_write(int fd, const void *buf, size_t count)
{
	while (1) {
		ssize_t n = __sys_write(fd, buf, count);

		if (!acl_var_hook_sys_api)
			return n;

		if (n >= 0)
			return n;

		fiber_save_errno();

#if EAGAIN == EWOULDBLOCK
		if (errno != EAGAIN)
#else
		if (errno != EAGAIN && errno != EWOULDBLOCK)
#endif
			return -1;

		fiber_wait_write(fd);
	}
}

inline ssize_t fiber_writev(int fd, const struct iovec *iov, int iovcnt)
{
	while (1) {
		ssize_t n = __sys_writev(fd, iov, iovcnt);

		if (!acl_var_hook_sys_api)
			return n;

		if (n >= 0)
			return n;

		fiber_save_errno();

#if EAGAIN == EWOULDBLOCK
		if (errno != EAGAIN)
#else
		if (errno != EAGAIN && errno != EWOULDBLOCK)
#endif
			return -1;

		fiber_wait_write(fd);
	}
}

inline ssize_t fiber_send(int sockfd, const void *buf, size_t len, int flags)
{
	while (1) {
		ssize_t n = __sys_send(sockfd, buf, len, flags);

		if (!acl_var_hook_sys_api)
			return n;

		if (n >= 0)
			return n;

		fiber_save_errno();

#if EAGAIN == EWOULDBLOCK
		if (errno != EAGAIN)
#else
		if (errno != EAGAIN && errno != EWOULDBLOCK)
#endif
			return -1;

		fiber_wait_write(sockfd);
	}
}

inline ssize_t fiber_sendto(int sockfd, const void *buf, size_t len, int flags,
	const struct sockaddr *dest_addr, socklen_t addrlen)
{
	while (1) {
		ssize_t n = __sys_sendto(sockfd, buf, len, flags,
				dest_addr, addrlen);

		if (!acl_var_hook_sys_api)
			return n;

		if (n >= 0)
			return n;

		fiber_save_errno();

#if EAGAIN == EWOULDBLOCK
		if (errno != EAGAIN)
#else
		if (errno != EAGAIN && errno != EWOULDBLOCK)
#endif
			return -1;

		fiber_wait_write(sockfd);
	}
}

inline ssize_t fiber_sendmsg(int sockfd, const struct msghdr *msg, int flags)
{
	while (1) {
		ssize_t n = __sys_sendmsg(sockfd, msg, flags);

		if (!acl_var_hook_sys_api)
			return n;

		if (n >= 0)
			return n;

		fiber_save_errno();

#if EAGAIN == EWOULDBLOCK
		if (errno != EAGAIN)
#else
		if (errno != EAGAIN && errno != EWOULDBLOCK)
#endif
			return -1;

		fiber_wait_write(sockfd);
	}
}

/****************************************************************************/

ssize_t read(int fd, void *buf, size_t count)
{
	return fiber_read(fd, buf, count);
}

ssize_t readv(int fd, const struct iovec *iov, int iovcnt)
{
	return fiber_readv(fd, iov, iovcnt);
}

ssize_t recv(int sockfd, void *buf, size_t len, int flags)
{
	return fiber_recv(sockfd, buf, len, flags);
}

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
	struct sockaddr *src_addr, socklen_t *addrlen)
{
	return fiber_recvfrom(sockfd, buf, len, flags, src_addr, addrlen);
}

ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags)
{
	return fiber_recvmsg(sockfd, msg, flags);
}

ssize_t write(int fd, const void *buf, size_t count)
{
	return fiber_write(fd, buf, count);
}

ssize_t writev(int fd, const struct iovec *iov, int iovcnt)
{
	return writev(fd, iov, iovcnt);
}

ssize_t send(int sockfd, const void *buf, size_t len, int flags)
{
	return fiber_send(sockfd, buf, len, flags);
}

ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
	const struct sockaddr *dest_addr, socklen_t addrlen)
{
	return fiber_sendto(sockfd, buf, len, flags, dest_addr, addrlen);
}

ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags)
{
	return fiber_sendmsg(sockfd, msg, flags);
}

/****************************************************************************/
