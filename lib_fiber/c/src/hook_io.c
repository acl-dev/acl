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

void fiber_hook_io(void)
{
	static int __called = 0;

	if (__called)
		return;

	__called++;

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

int pipe(int pipefd[2])
{
	int ret = __sys_pipe(pipefd);

	if (ret < 0)
		fiber_save_errno();
	return ret;
}

#ifdef HAS_PIPE2
int pipe2(int pipefd[2], int flags)
{
	int ret = __sys_pipe2(pipefd, flags);

	if (ret < 0)
		fiber_save_errno();
	return ret;
}
#endif

FILE *popen(const char *command, const char *type)
{
	FILE *fp = __sys_popen(command, type);

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

	fiber_io_close(fd);

	ret = __sys_close(fd);
	if (ret == 0)
		return ret;

	fiber_save_errno();
	return ret;
}

#define READ_WAIT_FIRST

#ifdef READ_WAIT_FIRST

ssize_t read(int fd, void *buf, size_t count)
{
	ssize_t ret;

	if (fd < 0) {
		acl_msg_error("%s: invalid fd: %d", __FUNCTION__, fd);
		return -1;
	}

	fiber_wait_read(fd);

	ret = __sys_read(fd, buf, count);
	if (ret > 0)
		return ret;

	fiber_save_errno();
	return ret;
}

ssize_t readv(int fd, const struct iovec *iov, int iovcnt)
{
	ssize_t ret;

	if (fd < 0) {
		acl_msg_error("%s: invalid fd: %d", __FUNCTION__, fd);
		return -1;
	}

	fiber_wait_read(fd);
	ret = __sys_readv(fd, iov, iovcnt);
	if (ret > 0)
		return ret;

	fiber_save_errno();
	return ret;
}

ssize_t recv(int sockfd, void *buf, size_t len, int flags)
{
	ssize_t ret;

	if (sockfd < 0) {
		acl_msg_error("%s: invalid sockfd: %d", __FUNCTION__, sockfd);
		return -1;
	}

	fiber_wait_read(sockfd);
	ret = __sys_recv(sockfd, buf, len, flags);
	if (ret > 0)
		return ret;

	fiber_save_errno();
	return ret;
}

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
	struct sockaddr *src_addr, socklen_t *addrlen)
{
	ssize_t ret;

	if (sockfd < 0) {
		acl_msg_error("%s: invalid sockfd: %d", __FUNCTION__, sockfd);
		return -1;
	}

	fiber_wait_read(sockfd);
	ret = __sys_recvfrom(sockfd, buf, len, flags, src_addr, addrlen);
	if (ret > 0)
		return ret;

	fiber_save_errno();
	return ret;
}

ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags)
{
	ssize_t ret;

	if (sockfd < 0) {
		acl_msg_error("%s: invalid sockfd: %d", __FUNCTION__, sockfd);
		return -1;
	}

	fiber_wait_read(sockfd);
	ret = __sys_recvmsg(sockfd, msg, flags);
	if (ret > 0)
		return ret;

	fiber_save_errno();
	return ret;
}

#else

ssize_t read(int fd, void *buf, size_t count)
{
	while (1) {
		ssize_t n = __sys_read(fd, buf, count);

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

ssize_t readv(int fd, const struct iovec *iov, int iovcnt)
{
	while (1) {
		ssize_t n = __sys_readv(fd, iov, iovcnt);

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

ssize_t recv(int sockfd, void *buf, size_t len, int flags)
{
	while (1) {
		ssize_t n = __sys_recv(sockfd, buf, len, flags);

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

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
	struct sockaddr *src_addr, socklen_t *addrlen)
{
	while (1) {
		ssize_t n = __sys_recvfrom(sockfd, buf, len, flags,
				src_addr, addrlen);

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

ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags)
{
	while (1) {
		ssize_t n = __sys_recvmsg(sockfd, msg, flags);

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

#endif

ssize_t write(int fd, const void *buf, size_t count)
{
	while (1) {
		ssize_t n = __sys_write(fd, buf, count);

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

ssize_t writev(int fd, const struct iovec *iov, int iovcnt)
{
	while (1) {
		ssize_t n = __sys_writev(fd, iov, iovcnt);

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

ssize_t send(int sockfd, const void *buf, size_t len, int flags)
{
	while (1) {
		ssize_t n = __sys_send(sockfd, buf, len, flags);

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

ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
	const struct sockaddr *dest_addr, socklen_t addrlen)
{
	while (1) {
		ssize_t n = __sys_sendto(sockfd, buf, len, flags,
				dest_addr, addrlen);

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

ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags)
{
	while (1) {
		ssize_t n = __sys_sendmsg(sockfd, msg, flags);

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
