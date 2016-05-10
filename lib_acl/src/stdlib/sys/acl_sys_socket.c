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

#ifdef ACL_WINDOWS

static int __socket_inited = 0;
static int __socket_ended = 0;

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
	return closesocket(fd);
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
	if (ret == SOCKET_ERROR)
		return -1;
	if (dwBytes == 0)
		return -1;
	return dwBytes;
#else
	if (fp != NULL && fp->read_ready)
		fp->read_ready = 0;
	else if (timeout > 0 && acl_read_wait(fd, timeout) < 0)
		return -1;

	return recv(fd, buf, (int) size, 0);
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
	if (ret == SOCKET_ERROR)
		return (-1);
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

	ret = send(fd, buf, (int) size, 0);
	if (ret <= 0)
		errno = acl_last_error();
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
		ret = send(fd, vec[i].iov_base, (int) vec[i].iov_len, 0);
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
	return close(fd);
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
	if (ret > 0)
		return ret;

	if (timeout <= 0)
		return ret;

	error = acl_last_error();

#if ACL_EWOULDBLOCK == ACL_EAGAIN
	if (error != ACL_EWOULDBLOCK)
#else
	if (error != ACL_EWOULDBLOCK && error != ACL_EAGAIN)
#endif
		return ret;

	if (acl_read_wait(fd, timeout) < 0)
		return -1;

	return read(fd, buf, size);
}

#else

int acl_socket_read(ACL_SOCKET fd, void *buf, size_t size,
	int timeout, ACL_VSTREAM *fp, void *arg acl_unused)
{
	if (fp != NULL && fp->read_ready)
		fp->read_ready = 0;
	else if (timeout > 0 && acl_read_wait(fd, timeout) < 0)
		return -1;

	return read(fd, buf, size);
}

#endif

int acl_socket_write(ACL_SOCKET fd, const void *buf, size_t size,
	int timeout, ACL_VSTREAM *fp acl_unused, void *arg acl_unused)
{
	int ret, error;

	ret = write(fd, buf, size);
	if (ret > 0)
		return ret;

	if (timeout <= 0)
		return ret;

	error = acl_last_error();

#if ACL_EWOULDBLOCK == ACL_EAGAIN
	if (error != ACL_EWOULDBLOCK)
#else
	if (error != ACL_EWOULDBLOCK && error != ACL_EAGAIN)
#endif
		return ret;

#ifdef ACL_WRITEABLE_CHECK
	if (acl_write_wait(fd, timeout) < 0)
		return -1;

	ret = write(fd, buf, size);
#endif

	return ret;
}

int acl_socket_writev(ACL_SOCKET fd, const struct iovec *vec, int count,
	int timeout, ACL_VSTREAM *fp acl_unused, void *arg acl_unused)
{
	int ret, error;

	ret = writev(fd, vec, count);
	if (ret > 0)
		return ret;

	if (timeout <= 0)
		return ret;

	error = acl_last_error();

#if ACL_EWOULDBLOCK == ACL_EAGAIN
	if (error != ACL_EWOULDBLOCK)
#else
	if (error != ACL_EWOULDBLOCK && error != ACL_EAGAIN)
#endif
		return ret;

#ifdef ACL_WRITEABLE_CHECK
	if (acl_write_wait(fd, timeout) < 0)
		return -1;

	ret = writev(fd, vec, count);
#endif

	return ret;
}

#else
# error "unknown OS type"
#endif
