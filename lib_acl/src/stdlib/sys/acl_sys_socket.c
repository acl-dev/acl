#include "StdAfx.h"

#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#include <errno.h>
#ifdef  ACL_WINDOWS
#include <io.h>
#include <stdarg.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <time.h>

#endif

#ifdef	ACL_UNIX
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <signal.h>
#endif

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "stdlib/acl_msg.h"
#include "stdlib/acl_vstream.h"
#include "stdlib/acl_iostuff.h"
#include "stdlib/acl_sys_patch.h"

#endif

#ifdef ACL_UNIX
static acl_read_fn   __sys_read   = read;
static acl_write_fn  __sys_write  = write;
static acl_writev_fn __sys_writev = (acl_writev_fn) writev;

void acl_set_read(acl_read_fn fn)
{
	__sys_read = fn;
}

void acl_set_write(acl_write_fn fn)
{
	__sys_write = fn;
}

void acl_set_writev(acl_writev_fn fn)
{
	__sys_writev = fn;
}

static acl_close_socket_fn  __sys_close = close;
#elif defined(ACL_WINDOWS)
static acl_close_socket_fn  __sys_close = closesocket;
#endif

static acl_recv_fn   __sys_recv   = (acl_recv_fn) recv;
static acl_send_fn   __sys_send   = (acl_send_fn) send;

void acl_set_close_socket(acl_close_socket_fn fn)
{
	__sys_close = fn;
}

void acl_set_recv(acl_recv_fn fn)
{
	__sys_recv = fn;
}

void acl_set_send(acl_send_fn fn)
{
	__sys_send = fn;
}

#ifdef ACL_WINDOWS

static int __socket_inited = 0;
static int __socket_ended  = 0;

int acl_socket_init(void)
{
	const char *myname = "acl_socket_init";
	WORD version = 0;
	WSADATA data;

	if (__socket_inited) {
		acl_msg_warn("%s(%d): has been inited", myname, __LINE__);
		return 0;
	}

	__socket_inited = 1;

	FillMemory(&data, sizeof(WSADATA), 0);

	version = MAKEWORD(2, 0);

	if (WSAStartup(version, &data) != 0) {
		acl_msg_error("%s(%d): WSAStartup error(%s)",
			myname, __LINE__, acl_last_serror());
		return -1;
	}
	if (LOBYTE(data.wVersion) != 2 || HIBYTE(data.wVersion) != 0) {
		WSACleanup();
		acl_msg_error("%s(%d): LOBYTE(data.wVersion) = %d"
			", HIBYTE(data.wVersion) = %d", myname, __LINE__,
			LOBYTE(data.wVersion), HIBYTE(data.wVersion));
		return -1;
	}

	__socket_ended = 0;
	return (0);
}

int acl_socket_end(void)
{
	const char *myname = "acl_socket_end";

	if (__socket_ended) {
		acl_msg_warn("%s(%d): has been ended", myname, __LINE__);
		return 0;
	}
	__socket_ended = 1;

	if (WSACleanup() == SOCKET_ERROR) {
		acl_msg_error("%s(%d): WSACleanup error(%s)",
			myname, __LINE__, acl_last_serror());
		return -1;
	}

	__socket_inited = 0;
	return 0;
}

int acl_socket_close(ACL_SOCKET fd)
{
	return __sys_close(fd);
}

int acl_socket_read(ACL_SOCKET fd, void *buf, size_t size,
	int timeout, ACL_VSTREAM *fp, void *arg acl_unused)
{
#if 0
	WSABUF wsaData;
	DWORD dwBytes = 0;
	DWORD flags = 0;
	int   ret;

	wsaData.len = (u_long) size;
	wsaData.buf = (char*) buf;
	ret = WSARecv(fd, &wsaData, 1, &dwBytes, &flags, NULL, NULL);
	if (ret == SOCKET_ERROR) {
		return -1;
	}
	if (dwBytes == 0) {
		return -1;
	}
	return dwBytes;
#else
	if (fp != NULL && fp->read_ready) {
		fp->read_ready = 0;
	} else if (timeout > 0 && acl_read_wait(fd, timeout) < 0) {
		return -1;
	}

	return __sys_recv(fd, buf, (int) size, 0);
#endif
}

int acl_socket_write(ACL_SOCKET fd, const void *buf, size_t size,
	int timeout, ACL_VSTREAM *fp acl_unused, void *arg acl_unused)
{
#if 0
	WSABUF wsaData;
	DWORD dwBytes = 0;
	int   ret;

	wsaData.len = (u_long) size;
	wsaData.buf = (char*) buf;

	ret = WSASend(fd, &wsaData, 1, &dwBytes, 0, NULL, NULL);
	if (ret == SOCKET_ERROR) {
		return (-1);
	}
	return dwBytes;
#else
	int   ret;

#ifdef ACL_WRITEABLE_CHECK
	if (timeout > 0 && acl_write_wait(fd, timeout) < 0) {
		errno = acl_last_error();
		return -1;
	}
#else
	(void) timeout;
#endif

	ret = __sys_send(fd, buf, (int) size, 0);
	if (ret <= 0) {
		errno = acl_last_error();
	}
	return ret;
#endif
}

int acl_socket_writev(ACL_SOCKET fd, const struct iovec *vec, int count,
	int timeout, ACL_VSTREAM *fp acl_unused, void *arg acl_unused)
{
	int   i, n, ret;

#ifdef ACL_WRITEABLE_CHECK
	if (timeout > 0 && acl_write_wait(fd, timeout) < 0) {
		errno = acl_last_error();
		return -1;
	}
#else
	(void) timeout;
#endif

	n = 0;
	for (i = 0; i < count; i++)	{
		ret = __sys_send(fd, vec[i].iov_base, (int) vec[i].iov_len, 0);
		if (ret == SOCKET_ERROR) {
			errno = acl_last_error();
			return -1;
		} else if (ret < (int) vec[i].iov_len) {
			n += ret;
			break;
		}
		n += ret;
	}
	return n;
}

/* for vc2003 */

#if _MSC_VER <= 1310
#include <winsock2.h>
#include <ws2tcpip.h>

int WSAAPI WSAPoll(LPWSAPOLLFD fds, ULONG nfds, INT timeout) {
	int ret;
	unsigned long i;
	struct timeval tv, *ptv;

	FD_SET rset;
	FD_SET wset;
	FD_SET eset;

	FD_ZERO(&rset);
	FD_ZERO(&wset);
	FD_ZERO(&eset);

	for (i = 0; i < nfds; i++) {
		if (fds[i].events & POLLRDNORM) {
			FD_SET(fds[i].fd, &rset);
		}
		if (fds[i].events & POLLWRNORM) {
			FD_SET(fds[i].fd, &wset);
		}
		FD_SET(fds[i].fd, &eset);
	}

	if (timeout >= 0) {
		tv.tv_sec  = timeout / 1000;
		tv.tv_usec = (timeout % 1000) * 1000;
		ptv = &tv;
	} else {
		ptv = NULL;
	}

	ret = select((int) fds, &rset, &wset, &eset, ptv);
	if (ret == SOCKET_ERROR) {
		return SOCKET_ERROR;
	}

	for(i = 0; i < nfds; i++) {
		if ((fds[i].events & POLLRDNORM) && FD_ISSET(fds[i].fd, &rset)) {
			fds[i].revents |= POLLRDNORM;
		}
		if ((fds[i].events & POLLWRNORM) && FD_ISSET(fds[i].fd, &wset)) {
			fds[i].revents |= POLLWRNORM;
		}
		if (FD_ISSET(fds[i].fd, &eset)) {
			fds[i].revents |= POLLERR;
		}
	}

	return ret;
}

int inet_pton(int af, const char *src, void *dst)
{
	struct sockaddr_storage ss;
	int size = sizeof(ss);
	char src_copy[INET6_ADDRSTRLEN+1];

	ZeroMemory(&ss, sizeof(ss));
	/* stupid non-const API */
	strncpy (src_copy, src, INET6_ADDRSTRLEN+1);
	src_copy[INET6_ADDRSTRLEN] = 0;

	if (WSAStringToAddress(src_copy, af, NULL, (struct sockaddr *)&ss, &size) == 0) {
		switch(af) {
		case AF_INET:
			*(struct in_addr *)dst = ((struct sockaddr_in *)&ss)->sin_addr;
			return 1;
		case AF_INET6:
			*(struct in6_addr *)dst = ((struct sockaddr_in6 *)&ss)->sin6_addr;
			return 1;
		}
	}
	return 0;
}

const char *inet_ntop(int af, const void *src, char *dst, socklen_t size)
{
	struct sockaddr_storage ss;
	unsigned long s = size;

	ZeroMemory(&ss, sizeof(ss));
	ss.ss_family = af;

	switch(af) {
	case AF_INET:
		((struct sockaddr_in *)&ss)->sin_addr = *(struct in_addr *)src;
		break;
	case AF_INET6:
		((struct sockaddr_in6 *)&ss)->sin6_addr = *(struct in6_addr *)src;
		break;
	default:
		return NULL;
	}
	/* cannot direclty use &size because of strict aliasing rules */
	return (WSAAddressToString((struct sockaddr *)&ss, sizeof(ss),
		NULL, dst, &s) == 0) ? dst : NULL;
}

#endif

#elif defined(ACL_UNIX)

int acl_socket_init(void)
{
	signal(SIGPIPE, SIG_IGN);
	return 0;
}

int acl_socket_end(void)
{
	return 0;
}

int acl_socket_close(ACL_SOCKET fd)
{
	return __sys_close(fd);
}

#if 0

int acl_socket_read(ACL_SOCKET fd, void *buf, size_t size,
	int timeout, ACL_VSTREAM *fp, void *arg acl_unused)
{
	int ret, error;

	if (fp != NULL && fp->read_ready) {
		fp->read_ready = 0;
		return read(fd, buf, size);
	}

	ret = read(fd, buf, size);
	if (ret > 0) {
		return ret;
	}

	if (timeout <= 0) {
		return ret;
	}

	error = acl_last_error();

#if ACL_EWOULDBLOCK == ACL_EAGAIN
	if (error != ACL_EWOULDBLOCK) {
#else
	if (error != ACL_EWOULDBLOCK && error != ACL_EAGAIN) {
#endif
		return ret;
	}

	if (acl_read_wait(fd, timeout) < 0) {
		return -1;
	}

	return read(fd, buf, size);
}

#else

int acl_socket_read(ACL_SOCKET fd, void *buf, size_t size,
	int timeout, ACL_VSTREAM *fp, void *arg acl_unused)
{
	if (fp != NULL && fp->read_ready) {
		fp->read_ready = 0;
	} else if (timeout > 0 && acl_read_wait(fd, timeout) < 0) {
		return -1;
	}

	return (int) __sys_read(fd, buf, size);
}

#endif

int acl_socket_write(ACL_SOCKET fd, const void *buf, size_t size,
	int timeout, ACL_VSTREAM *fp acl_unused, void *arg acl_unused)
{
	int ret, error;

	ret = (int) __sys_write(fd, buf, size);
	if (ret > 0) {
		return ret;
	}

	if (timeout <= 0) {
		return ret;
	}

	error = acl_last_error();

#if ACL_EWOULDBLOCK == ACL_EAGAIN
	if (error != ACL_EWOULDBLOCK) {
#else
	if (error != ACL_EWOULDBLOCK && error != ACL_EAGAIN) {
#endif
		return ret;
	}

#ifdef ACL_WRITEABLE_CHECK
	if (acl_write_wait(fd, timeout) < 0) {
		return -1;
	}

	ret = __sys_write(fd, buf, size);
#endif

	return ret;
}

int acl_socket_writev(ACL_SOCKET fd, const struct iovec *vec, int count,
	int timeout, ACL_VSTREAM *fp acl_unused, void *arg acl_unused)
{
	int ret, error;

	ret = (int) __sys_writev(fd, vec, count);
	if (ret > 0) {
		return ret;
	}

	if (timeout <= 0) {
		return ret;
	}

	error = acl_last_error();

#if ACL_EWOULDBLOCK == ACL_EAGAIN
	if (error != ACL_EWOULDBLOCK) {
#else
	if (error != ACL_EWOULDBLOCK && error != ACL_EAGAIN) {
#endif
		return ret;
	}

#ifdef ACL_WRITEABLE_CHECK
	if (acl_write_wait(fd, timeout) < 0) {
		return -1;
	}

	ret = __sys_writev(fd, vec, count);
#endif

	return ret;
}

#else
# error "unknown OS type"
#endif

int acl_socket_shutdown(ACL_SOCKET fd, int how)
{
	return shutdown(fd, how);
}

int acl_socket_alive(ACL_SOCKET fd)
{
	char  buf[16];
	int   ret = acl_readable(fd);

	if (ret == -1) {
		return 0;
	}
	if (ret == 0) {
		return 1;
	}
	ret = (int) __sys_recv(fd, buf, sizeof(buf), MSG_PEEK);
	if (ret == 0 || (ret < 0 && acl_last_error() != ACL_EWOULDBLOCK)) {
		return 0;
	}
	return 1;
}
