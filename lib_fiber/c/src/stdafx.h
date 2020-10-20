#ifndef __STD_AFX_INCLUDE_H__
#define __STD_AFX_INCLUDE_H__

#include "define.h"

#if 1
#define LIKELY(x)	__builtin_expect(!!(x), 1)
#define UNLIKELY(x)	__builtin_expect(!!(x), 0)
#else
#define	LIKELY
#define	UNLIKELY
#endif

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <setjmp.h>
#include <signal.h>

#if defined(SYS_UNIX)
#include <net/if.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/select.h>
#include <poll.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <limits.h>
#include <pthread.h>
#include <sys/resource.h>
#include <ucontext.h>

//#if defined(__APPLE__)
# include <sys/types.h>
# include <sys/uio.h>
//#endif

#define STRDUP strdup
#define GETPID getpid

#elif defined(SYS_WIN)

#define STRDUP _strdup
#define GETPID _getpid
#endif

#if defined(__linux__)
# include <sys/sendfile.h>
# include <sys/epoll.h>

/*
# if !defined(__aarch64__) && !defined(__arm__)
#  define USE_FAST_TIME
# endif
*/

#elif defined(__FreeBSD__)
# include <sys/uio.h>
# include <pthread_np.h>
#endif

typedef union {
	struct sockaddr_storage ss;
#ifdef AF_INET6
	struct sockaddr_in6 in6;
#endif
	struct sockaddr_in in;
#ifdef ACL_UNIX
	struct sockaddr_un un;
#endif
	struct sockaddr sa;
} SOCK_ADDR;

/*
#ifndef USE_SYSCALL
# define USE_SYSCALL
#endif
*/

#endif
