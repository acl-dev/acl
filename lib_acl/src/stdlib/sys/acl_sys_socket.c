#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#include <errno.h>
#ifdef  WIN32
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
#include "stdlib/acl_sys_patch.h"

#endif

#ifdef WIN32

static int __socket_inited = 0;
static int __socket_ended = 0;

int acl_socket_init(void)
{
	const char *myname = "acl_socket_init";
	WORD version = 0;
	WSADATA data;
	char  ebuf[256];

	if (__socket_inited) {
		acl_msg_warn("%s(%d): has been inited", myname, __LINE__);
		return 0;
	}

	__socket_inited = 1;

	FillMemory(&data, sizeof(WSADATA), 0);

	version = MAKEWORD(2, 0);

	if (WSAStartup(version, &data) != 0) {
		acl_msg_error("%s(%d): WSAStartup error(%s)",
			myname, __LINE__, acl_last_strerror(ebuf, sizeof(ebuf)));
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
	int timeout acl_unused, ACL_VSTREAM *fp acl_unused,
	void *arg acl_unused)
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
	int ret = recv(fd, buf, size, 0);
	if (ret <= 0)
		errno = acl_last_error();
	return ret;
#endif
}

int acl_socket_write(ACL_SOCKET fd, const void *buf, size_t size,
	int timeout acl_unused, ACL_VSTREAM *fp acl_unused,
	void *arg acl_unused)
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
	int ret = send(fd, buf, size, 0);

	if (ret <= 0)
		errno = acl_last_error();
	return ret;
#endif
}

int acl_socket_writev(ACL_SOCKET fd, const struct iovec *vec, int count,
	int timeout acl_unused, ACL_VSTREAM *fp acl_unused,
	void *arg acl_unused)
{
	int   i, n, ret;

	n = 0;
	for (i = 0; i < count; i++)	{
		ret = send(fd, vec[i].iov_base,
				vec[i].iov_len, 0);
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

int acl_socket_read(ACL_SOCKET fd, void *buf, size_t size,
	int timeout acl_unused, ACL_VSTREAM *fp acl_unused,
	void *arg acl_unused)
{
	return read(fd, buf, size);
}

int acl_socket_write(ACL_SOCKET fd, const void *buf, size_t size,
	int timeout acl_unused, ACL_VSTREAM *fp acl_unused,
	void *arg acl_unused)
{
	return write(fd, buf, size);
}

int acl_socket_writev(ACL_SOCKET fd, const struct iovec *vec, int count,
	int timeout acl_unused, ACL_VSTREAM *fp acl_unused,
	void *arg acl_unused)
{
	return writev(fd, vec, count);
}

#else
# error "unknown OS type"
#endif
