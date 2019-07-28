#include "stdafx.h"

#ifdef HAS_PIPE2
#define _GNU_SOURCE
#endif

#include <fcntl.h>
#include <unistd.h>
#include <sys/sendfile.h>

#ifndef __USE_GNU
#define __USE_GNU
#endif
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
typedef int     (*stat_fn)(int, const char*, struct stat*);
typedef int     (*lstat_fn)(int, const char*, struct stat*);
typedef int     (*fstat_fn)(int, int, struct stat*);
typedef int     (*mkdir_fn)(const char*, mode_t);
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
#ifndef __USE_FILE_OFFSET64
typedef ssize_t (*sendfile_fn)(int, int, off_t*, size_t);
#endif
#ifdef  __USE_LARGEFILE64
typedef ssize_t (*sendfile64_fn)(int, int, off64_t*, size_t);
#endif
typedef ssize_t (*pread_fn)(int, void *, size_t, off_t);
typedef ssize_t (*pwrite_fn)(int, const void *, size_t, off_t);
typedef ssize_t (*pread64_fn)(int, void *, size_t, off64_t);
typedef ssize_t (*pwrite64_fn)(int, const void *, size_t, off64_t);

static sleep_fn    __sys_sleep    = NULL;
static pipe_fn     __sys_pipe     = NULL;
#ifdef HAS_PIPE2
static pipe2_fn    __sys_pipe2    = NULL;
#endif
static popen_fn    __sys_popen    = NULL;
static pclose_fn   __sys_pclose   = NULL;
static close_fn    __sys_close    = NULL;
static stat_fn     __sys_stat     = NULL;
static lstat_fn    __sys_lstat    = NULL;
static fstat_fn    __sys_fstat    = NULL;
static mkdir_fn    __sys_mkdir    = NULL;
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

#ifndef __USE_FILE_OFFSET64
static sendfile_fn   __sys_sendfile   = NULL;
#endif
#ifdef __USE_LARGEFILE64
static sendfile64_fn __sys_sendfile64 = NULL;
#endif

static pread_fn      __sys_pread    = NULL;
static pwrite_fn     __sys_pwrite   = NULL;
static pread64_fn    __sys_pread64  = NULL;
static pwrite64_fn   __sys_pwrite64 = NULL;

void hook_io(void)
{
	static acl_pthread_mutex_t __lock = PTHREAD_MUTEX_INITIALIZER;
	static int __called = 0;

	(void) acl_pthread_mutex_lock(&__lock);

	if (__called) {
		(void) acl_pthread_mutex_unlock(&__lock);
		return;
	}

	__called++;

	__sys_sleep      = (sleep_fn) dlsym(RTLD_NEXT, "sleep");
	acl_assert(__sys_sleep);

	__sys_pipe       = (pipe_fn) dlsym(RTLD_NEXT, "pipe");
	acl_assert(__sys_pipe);

#ifdef HAS_PIPE2
	__sys_pipe2      = (pipe2_fn) dlsym(RTLD_NEXT, "pipe2");
	acl_assert(__sys_pipe2);
#endif

	__sys_popen      = (popen_fn) dlsym(RTLD_NEXT, "popen");
	acl_assert(__sys_popen);

	__sys_pclose     = (pclose_fn) dlsym(RTLD_NEXT, "pclose");
	acl_assert(__sys_pclose);

	__sys_close      = (close_fn) dlsym(RTLD_NEXT, "close");
	acl_assert(__sys_close);

	__sys_mkdir      = (mkdir_fn) dlsym(RTLD_NEXT, "mkdir");
	acl_assert(__sys_mkdir);

	__sys_stat       = (stat_fn) dlsym(RTLD_NEXT, "__xstat");
	acl_assert(__sys_stat);

	__sys_fstat      = (fstat_fn) dlsym(RTLD_NEXT, "__fxstat");
	acl_assert(__sys_fstat);

	__sys_read       = (read_fn) dlsym(RTLD_NEXT, "read");
	acl_assert(__sys_read);

	__sys_readv      = (readv_fn) dlsym(RTLD_NEXT, "readv");
	acl_assert(__sys_readv);

	__sys_recv       = (recv_fn) dlsym(RTLD_NEXT, "recv");
	acl_assert(__sys_recv);

	__sys_recvfrom   = (recvfrom_fn) dlsym(RTLD_NEXT, "recvfrom");
	acl_assert(__sys_recvfrom);

	__sys_recvmsg    = (recvmsg_fn) dlsym(RTLD_NEXT, "recvmsg");
	acl_assert(__sys_recvmsg);

	__sys_write      = (write_fn) dlsym(RTLD_NEXT, "write");
	acl_assert(__sys_write);

	__sys_writev     = (writev_fn) dlsym(RTLD_NEXT, "writev");
	acl_assert(__sys_writev);

	__sys_send       = (send_fn) dlsym(RTLD_NEXT, "send");
	acl_assert(__sys_send);

	__sys_sendto     = (sendto_fn) dlsym(RTLD_NEXT, "sendto");
	acl_assert(__sys_sendto);

	__sys_sendmsg    = (sendmsg_fn) dlsym(RTLD_NEXT, "sendmsg");
	acl_assert(__sys_sendmsg);

#ifndef __USE_FILE_OFFSET64
	__sys_sendfile   = (sendfile_fn) dlsym(RTLD_NEXT, "sendfile");
	acl_assert(__sys_sendfile);
#endif

#ifdef __USE_LARGEFILE64
	__sys_sendfile64 = (sendfile64_fn) dlsym(RTLD_NEXT, "sendfile64");
	acl_assert(__sys_sendfile64);
#endif

	__sys_pread = (pread_fn) dlsym(RTLD_NEXT, "pread");
	acl_assert(__sys_pread);
	__sys_pwrite = (pwrite_fn) dlsym(RTLD_NEXT, "pwrite");
	acl_assert(__sys_pwrite);

	__sys_pread64 = (pread64_fn) dlsym(RTLD_NEXT, "pread64");
	acl_assert(__sys_pread64);
	__sys_pwrite64 = (pwrite64_fn) dlsym(RTLD_NEXT, "pwrite64");
	acl_assert(__sys_pwrite64);

	(void) acl_pthread_mutex_unlock(&__lock);
}

unsigned int sleep(unsigned int seconds)
{
	if (!acl_var_hook_sys_api) {
		if (__sys_sleep == NULL)
			hook_io();

		return __sys_sleep(seconds);
	}

	return acl_fiber_sleep(seconds);
}

int pipe(int pipefd[2])
{
	int ret;

	if (__sys_pipe == NULL)
		hook_io();

	ret = __sys_pipe(pipefd);

	if (!acl_var_hook_sys_api)
		return ret;

	if (ret < 0)
		fiber_save_errno();
	return ret;
}

#ifdef HAS_PIPE2
int pipe2(int pipefd[2], int flags)
{
	int ret;

	if (__sys_pipe2 == NULL)
		hook_io();

	ret = __sys_pipe2(pipefd, flags);

	if (!acl_var_hook_sys_api)
		return ret;

	if (ret < 0)
		fiber_save_errno();
	return ret;
}
#endif

FILE *popen(const char *command, const char *type)
{
	FILE *fp;

	if (__sys_popen == NULL)
		hook_io();

	fp = __sys_popen(command, type);

	if (!acl_var_hook_sys_api)
		return fp;

	if (fp == NULL)
		fiber_save_errno();
	return fp;
}

int mkdir(const char *pathname, mode_t mode)
{
	if (__sys_mkdir == NULL)
		hook_io();

	if (!acl_var_hook_sys_api)
		return __sys_mkdir ? __sys_mkdir(pathname, mode) : -1;

	if (__sys_mkdir(pathname, mode) == 0)
		return 0;
	fiber_save_errno();
	return -1;
}

int __xstat(int ver, const char *path, struct stat *buf)
{
	if (__sys_stat == NULL)
		hook_io();

	if (!acl_var_hook_sys_api)
		return __sys_stat ? __sys_stat(ver, path, buf) : -1;

	if (__sys_stat(ver, path, buf) == 0)
		return 0;
	fiber_save_errno();
	return -1;
}

int __lxstat(int ver, const char *path, struct stat *buf)
{
	if (__sys_lstat == NULL)
		hook_io();

	if (!acl_var_hook_sys_api)
		return __sys_lstat ? __sys_lstat(ver, path, buf) : -1;

	if (__sys_lstat(ver, path, buf) == 0)
		return 0;
	fiber_save_errno();
	return -1;
}

int __fxstat(int ver, int fd, struct stat *buf)
{
	if (__sys_fstat == NULL)
		hook_io();

	if (!acl_var_hook_sys_api)
		return __sys_fstat ? __sys_fstat(ver, fd, buf) : -1;

	if (__sys_fstat(ver, fd, buf) == 0)
		return 0;
	fiber_save_errno();
	return -1;
}

int close(int fd)
{
	int ret;

	if (fd < 0) {
		acl_msg_error("%s: invalid fd: %d", __FUNCTION__, fd);
		return -1;
	}

	if (!acl_var_hook_sys_api) {
		if (__sys_close == NULL)
			hook_io();

		return __sys_close(fd);
	}

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
	ACL_FIBER *me;

	if (fd < 0) {
		acl_msg_error("%s: invalid fd: %d", __FUNCTION__, fd);
		return -1;
	}

	if (!acl_var_hook_sys_api) {
		if (__sys_read == NULL)
			hook_io();

		return __sys_read(fd, buf, count);
	}

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
	if (ret >= 0)
		return ret;

	fiber_save_errno();

	me = acl_fiber_running();
	if (acl_fiber_killed(me))
		acl_msg_info("%s(%d), %s: fiber-%u is existing",
			__FILE__, __LINE__, __FUNCTION__, acl_fiber_id(me));

	return ret;
}

inline ssize_t fiber_readv(int fd, const struct iovec *iov, int iovcnt)
{
	ssize_t ret;
	EVENT  *ev;
	ACL_FIBER *me;

	if (fd < 0) {
		acl_msg_error("%s: invalid fd: %d", __FUNCTION__, fd);
		return -1;
	}

	if (!acl_var_hook_sys_api) {
		if (__sys_readv == NULL)
			hook_io();

		return __sys_readv(fd, iov, iovcnt);
	}

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
	if (ret >= 0)
		return ret;

	fiber_save_errno();

	me = acl_fiber_running();
	if (acl_fiber_killed(me))
		acl_msg_info("%s(%d), %s: fiber-%u is existing",
			__FILE__, __LINE__, __FUNCTION__, acl_fiber_id(me));

	return ret;
}

inline ssize_t fiber_recv(int sockfd, void *buf, size_t len, int flags)
{
	ssize_t ret;
	EVENT  *ev;
	ACL_FIBER *me;

	if (sockfd < 0) {
		acl_msg_error("%s: invalid sockfd: %d", __FUNCTION__, sockfd);
		return -1;
	}

	if (!acl_var_hook_sys_api) {
		if (__sys_recv == NULL)
			hook_io();

		return __sys_recv(sockfd, buf, len, flags);
	}

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
	if (ret >= 0)
		return ret;

	fiber_save_errno();

	me = acl_fiber_running();
	if (acl_fiber_killed(me))
		acl_msg_info("%s(%d), %s: fiber-%u is existing",
			__FILE__, __LINE__, __FUNCTION__, acl_fiber_id(me));

	return ret;
}

inline ssize_t fiber_recvfrom(int sockfd, void *buf, size_t len, int flags,
	struct sockaddr *src_addr, socklen_t *addrlen)
{
	ssize_t ret;
	EVENT  *ev;
	ACL_FIBER *me;

	if (sockfd < 0) {
		acl_msg_error("%s: invalid sockfd: %d", __FUNCTION__, sockfd);
		return -1;
	}

	if (!acl_var_hook_sys_api) {
		if (__sys_recvfrom == NULL)
			hook_io();

		return __sys_recvfrom(sockfd, buf, len,
				flags, src_addr, addrlen);
	}

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
	if (ret >= 0)
		return ret;

	fiber_save_errno();

	me = acl_fiber_running();
	if (acl_fiber_killed(me)) {
		acl_msg_info("%s(%d), %s: fiber-%u is existing",
			__FILE__, __LINE__, __FUNCTION__, acl_fiber_id(me));
		return -1;
	}

	return ret;
}

inline ssize_t fiber_recvmsg(int sockfd, struct msghdr *msg, int flags)
{
	ssize_t ret;
	EVENT  *ev;
	ACL_FIBER *me;

	if (sockfd < 0) {
		acl_msg_error("%s: invalid sockfd: %d", __FUNCTION__, sockfd);
		return -1;
	}

	if (!acl_var_hook_sys_api) {
		if (__sys_recvmsg == NULL)
			hook_io();

		return __sys_recvmsg(sockfd, msg, flags);
	}

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
	if (ret >= 0)
		return ret;

	fiber_save_errno();

	me = acl_fiber_running();
	if (acl_fiber_killed(me))
		acl_msg_info("%s(%d), %s: fiber-%u is existing",
			__FILE__, __LINE__, __FUNCTION__, acl_fiber_id(me));

	return ret;
}

#else

inline ssize_t fiber_read(int fd, void *buf, size_t count)
{
	ACL_FIBER *me;

	if (__sys_read == NULL)
		hook_io();

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

		me = acl_fiber_running();
		if (acl_fiber_killed(me))
			acl_msg_info("%s(%d), %s: fiber-%u is existing",
				__FILE__, __LINE__, __FUNCTION__,
				acl_fiber_id(me));
	}
}

inline ssize_t fiber_readv(int fd, const struct iovec *iov, int iovcnt)
{
	ACL_FIBER *me;

	if (__sys_readv == NULL)
		hook_io();

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

		me = acl_fiber_running();
		if (acl_fiber_killed(me))
			acl_msg_info("%s(%d), %s: fiber-%u is existing",
				__FILE__, __LINE__, __FUNCTION__,
				acl_fiber_id(me));
	}
}

inline ssize_t fiber_recv(int sockfd, void *buf, size_t len, int flags)
{
	ACL_FIBER *me;

	if (__sys_recv == NULL)
		hook_io();

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

		me = acl_fiber_running();
		if (acl_fiber_killed(me))
			acl_msg_info("%s(%d), %s: fiber-%u is existing",
				__FILE__, __LINE__, __FUNCTION__,
				acl_fiber_id(me));
	}
}

inline ssize_t fiber_recvfrom(int sockfd, void *buf, size_t len, int flags,
	struct sockaddr *src_addr, socklen_t *addrlen)
{
	ACL_FIBER *me;

	if (__sys_recvfrom == NULL)
		hook_io();

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

		me = acl_fiber_running();
		if (acl_fiber_killed(me))
			acl_msg_info("%s(%d), %s: fiber-%u is existing",
				__FILE__, __LINE__, __FUNCTION__,
				acl_fiber_id(me));
	}
}

inline ssize_t fiber_recvmsg(int sockfd, struct msghdr *msg, int flags)
{
	ACL_FIBER *me;

	if (__sys_recvmsg == NULL)
		hook_io();

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

		me = acl_fiber_running();
		if (acl_fiber_killed(me))
			acl_msg_info("%s(%d), %s: fiber-%u is existing",
				__FILE__, __LINE__, __FUNCTION__,
				acl_fiber_id(me));
	}
}

#endif  /* READ_WAIT_FIRST */

/****************************************************************************/

inline ssize_t fiber_write(int fd, const void *buf, size_t count)
{
	ACL_FIBER *me;

	if (__sys_write == NULL)
		hook_io();

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

		me = acl_fiber_running();
		if (acl_fiber_killed(me)) {
			acl_msg_info("%s(%d), %s: fiber-%u is existing",
				__FILE__, __LINE__, __FUNCTION__,
				acl_fiber_id(me));
			return -1;
		}
	}
}

inline ssize_t fiber_writev(int fd, const struct iovec *iov, int iovcnt)
{
	ACL_FIBER *me;

	if (__sys_writev == NULL)
		hook_io();

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

		me = acl_fiber_running();
		if (acl_fiber_killed(me)) {
			acl_msg_info("%s(%d), %s: fiber-%u is existing",
				__FILE__, __LINE__, __FUNCTION__,
				acl_fiber_id(me));
			return -1;
		}
	}
}

inline ssize_t fiber_send(int sockfd, const void *buf, size_t len, int flags)
{
	ACL_FIBER *me;

	if (__sys_send == NULL)
		hook_io();

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

		me = acl_fiber_running();
		if (acl_fiber_killed(me)) {
			acl_msg_info("%s(%d), %s: fiber-%u is existing",
				__FILE__, __LINE__, __FUNCTION__,
				acl_fiber_id(me));
			return -1;
		}
	}
}

inline ssize_t fiber_sendto(int sockfd, const void *buf, size_t len, int flags,
	const struct sockaddr *dest_addr, socklen_t addrlen)
{
	ACL_FIBER *me;

	if (__sys_sendto == NULL)
		hook_io();

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

		me = acl_fiber_running();
		if (acl_fiber_killed(me)) {
			acl_msg_info("%s(%d), %s: fiber-%u is existing",
				__FILE__, __LINE__, __FUNCTION__,
				acl_fiber_id(me));
			return -1;
		}
	}
}

inline ssize_t fiber_sendmsg(int sockfd, const struct msghdr *msg, int flags)
{
	ACL_FIBER *me;

	if (__sys_sendmsg == NULL)
		hook_io();

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

		me = acl_fiber_running();
		if (acl_fiber_killed(me)) {
			acl_msg_info("%s(%d), %s: fiber-%u is existing",
				__FILE__, __LINE__, __FUNCTION__,
				acl_fiber_id(me));
			return -1;
		}
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

#ifdef ACL_ARM_LINUX

ssize_t recv(int sockfd, void *buf, size_t len, unsigned int flags)
{
	return fiber_recv(sockfd, buf, len, (int) flags);
}

ssize_t recvfrom(int sockfd, void *buf, size_t len, unsigned int flags,
	const struct sockaddr *src_addr, socklen_t *addrlen)
{
	return fiber_recvfrom(sockfd, buf, len, flags,
			(const struct sockaddr*) src_addr, addrlen);
}

ssize_t recvmsg(int sockfd, struct msghdr *msg, unsigned int flags)
{
	return fiber_recvmsg(sockfd, msg, flags);
}

#else

ssize_t recv(int sockfd, void *buf, size_t len, int flags)
{
	return fiber_recv(sockfd, buf, len, (int) flags);
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

#endif

ssize_t write(int fd, const void *buf, size_t count)
{
	return fiber_write(fd, buf, count);
}

ssize_t writev(int fd, const struct iovec *iov, int iovcnt)
{
	return fiber_writev(fd, iov, iovcnt);
}

#ifdef ACL_ARM_LINUX

ssize_t send(int sockfd, const void *buf, size_t len, unsigned int flags)
{
	return fiber_send(sockfd, buf, len, (int) flags);
}

ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
	const struct sockaddr *dest_addr, socklen_t addrlen)
{
	return fiber_sendto(sockfd, buf, len, flags, dest_addr, addrlen);
}

ssize_t sendmsg(int sockfd, const struct msghdr *msg, unsigned int flags)
{
	return fiber_sendmsg(sockfd, msg, (int) flags);
}

#else

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

#endif

/****************************************************************************/

#ifndef __USE_FILE_OFFSET64
ssize_t sendfile(int out_fd, int in_fd, off_t *offset, size_t count)
{
	int ret;

	if (__sys_sendfile == NULL)
		hook_io();
	if (!acl_var_hook_sys_api) 
		return __sys_sendfile(out_fd, in_fd, offset, count);
	ret = __sys_sendfile(out_fd, in_fd, offset, count);
	if (ret < 0)
		fiber_save_errno();
	return ret;

}
#endif

#ifdef  __USE_LARGEFILE64
ssize_t sendfile64(int out_fd, int in_fd, off64_t *offset, size_t count)
{
	ACL_FIBER *me;

	if (__sys_sendfile64 == NULL)
		hook_io();

	while (1) {
		ssize_t n = __sys_sendfile64(out_fd, in_fd, offset, count);
		if (!acl_var_hook_sys_api || n >= 0)
			return n;

		fiber_save_errno();

#if EAGAIN == EWOULDBLOCK
		if (errno != EAGAIN)
#else
		if (errno != EAGAIN && errno != EWOULDBLOCK)
#endif
			return -1;

		fiber_wait_write(out_fd);

		me = acl_fiber_running();
		if (acl_fiber_killed(me)) {
			acl_msg_info("%s(%d), %s: fiber-%u is existing",
				__FILE__, __LINE__, __FUNCTION__,
				acl_fiber_id(me));
			return -1;
		}
	}
}
#endif

#if defined __USE_UNIX98 || defined __USE_XOPEN2K8
# ifndef __USE_FILE_OFFSET64
ssize_t pread(int fd, void *buf, size_t count, off_t offset)
{
	ssize_t ret;

	if (__sys_pread == NULL)
		hook_io();

	ret =  __sys_pread(fd, buf, count, offset);
	if (acl_var_hook_sys_api && ret < 0) 
		fiber_save_errno();
	return ret;
}

ssize_t pwrite(int fd, const void *buf, size_t count, off_t offset)
{
	ssize_t ret;

	if (__sys_pwrite == NULL)
		hook_io();

	ret = __sys_pwrite(fd, buf, count, offset);
	if (acl_var_hook_sys_api && ret < 0) 
		fiber_save_errno();
	return ret;
}
# endif

# ifdef __USE_LARGEFILE64
ssize_t pread64(int fd, void *buf, size_t count, off64_t offset)
{
	ssize_t ret;

	if (__sys_pread64 == NULL)
		hook_io();

	ret =  __sys_pread64(fd, buf, count, offset);
	if (acl_var_hook_sys_api && ret < 0) 
		fiber_save_errno();
	return ret;
}

ssize_t pwrite64(int fd, const void *buf, size_t count, off64_t offset)
{
	ssize_t ret;

	if (__sys_pwrite == NULL)
		hook_io();

	ret = __sys_pwrite64(fd, buf, count, offset);
	if (acl_var_hook_sys_api && ret < 0) 
		fiber_save_errno();
	return ret;
}
# endif
#endif
