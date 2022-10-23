#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#if defined(_WIN32) || defined(_WIN64)
#include <winsock2.h>
#else
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#endif
#include "fiber/libfiber.h"
#include "patch.h"

void socket_init(void)
{
#if defined(_WIN32) || defined(_WIN64)
	WORD version = 0;
	WSADATA data;

	FillMemory(&data, sizeof(WSADATA), 0);
	version = MAKEWORD(2, 0);

	if (WSAStartup(version, &data) != 0) {
		abort();
	}
#else
	signal(SIGPIPE, SIG_IGN);
#endif
}

void socket_end(void)
{
#if defined(_WIN32) || defined(_WIN64)
	WSACleanup();
#endif
}

void socket_close(SOCKET fd)
{
	acl_fiber_close(fd);
}

SOCKET socket_listen(const char *ip, int port)
{
	SOCKET fd;
	int on;
	struct sockaddr_in sa;

	memset(&sa, 0, sizeof(sa));
	sa.sin_family      = AF_INET;
	sa.sin_port        = htons(port);
	sa.sin_addr.s_addr = inet_addr(ip);

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == INVALID_SOCKET) {
		printf("create socket error, %s\r\n", strerror(errno));
		getchar();
		exit (1);
	}

#if defined(_WIN32) || defined(_WIN64)
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char *) &on, sizeof(on))) {
#else
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) {
#endif
		printf("setsockopt error %s\r\n", strerror(errno));
		exit (1);
	}

	if (bind(fd, (struct sockaddr *) &sa, sizeof(struct sockaddr)) < 0) {
		printf("bind error %s\r\n", strerror(errno));
		getchar();
		exit (1);
	}

	if (acl_fiber_listen(fd, 128) < 0) {
		printf("listen error %s\r\n", strerror(errno));
		getchar();
		exit (1);
	}

	return fd;
}

SOCKET socket_accept(SOCKET fd)
{
	SOCKET cfd;
	struct sockaddr_in sa;
	int len = sizeof(sa);
	
	cfd = acl_fiber_accept(fd, (struct sockaddr *)& sa, (socklen_t *)& len);
	return cfd;
}

SOCKET socket_connect(const char *ip, int port)
{
	SOCKET  fd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in sa;
	socklen_t len = (socklen_t) sizeof(sa);

	if (fd == INVALID_SOCKET) {
		printf("create socket error %s\r\n", strerror(errno));
		return INVALID_SOCKET;
	}

	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port   = htons(port);
	sa.sin_addr.s_addr = inet_addr(ip);

	if (acl_fiber_connect(fd, (const struct sockaddr *) &sa, len) < 0) {
		acl_fiber_close(fd);
		printf("%s: connect %s:%d erorr %s\r\n",
			__FUNCTION__, ip, port, strerror(errno));
		return INVALID_SOCKET;
	}
	return fd;
}

#if defined(_WIN32) || defined(_WIN64)

int set_non_blocking(SOCKET fd, int on)
{
	unsigned long n = on;
	int flags = 0;

	if (ioctlsocket(fd, FIONBIO, &n) < 0) {
		printf("ioctlsocket(fd,FIONBIO) failed\r\n");
		return -1;
	}
	return flags;
}

int socket_is_non_blocking(SOCKET fd)
{
	printf("%s: Not support, fd=%d\r\n", __FUNCTION__, fd);
	return -1;
}

#else

# ifndef O_NONBLOCK
#  define PATTERN       FNDELAY
# else
#  define PATTERN       O_NONBLOCK
# endif

int set_non_blocking(SOCKET fd, int on)
{
	int   flags;
	int   nonb = PATTERN;

	/*
	** NOTE: consult ALL your relevant manual pages *BEFORE* changing
	**	 these ioctl's.  There are quite a few variations on them,
	**	 as can be seen by the PCS one.  They are *NOT* all the same.
	**	 Heed this well. - Avalon.
	*/
#ifdef	NBLOCK_POSIX
	nonb |= O_NONBLOCK;
#endif
#ifdef	NBLOCK_BSD
	nonb |= O_NDELAY;
#endif

	if ((flags = fcntl(fd, F_GETFL)) == -1) {
		printf("%s(%d), %s: fcntl(%d, F_GETFL) error: %s\r\n",
			__FILE__, __LINE__, __FUNCTION__,
			fd, acl_fiber_last_serror());
		return -1;
	}
	if (fcntl(fd, F_SETFL, on ? flags | nonb : flags & ~nonb) < 0) {
		printf("%s(%d), %s: fcntl(%d, F_SETL, nonb) error: %s\r\n",
			__FILE__, __LINE__, __FUNCTION__,
			fd, acl_fiber_last_serror());
		return -1;
	}

	return (flags & PATTERN) ? 1 : 0;
}

int socket_is_non_blocking(SOCKET fd)
{
	int flags;

	if ((flags = fcntl(fd, F_GETFL)) == -1) {
		printf("%s(%d), %s: fcntl(%d, F_GETFL) error: %s\r\n",
			__FILE__, __LINE__, __FUNCTION__,
			fd, acl_fiber_last_serror());
		return 0;
	}

	return (flags & PATTERN) ? 1 : 0;
}

#endif // !_WIN32 && !_WIN64
