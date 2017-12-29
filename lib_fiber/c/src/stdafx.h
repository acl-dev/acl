#ifndef __STD_AFX_INCLUDE_H__
#define __STD_AFX_INCLUDE_H__


#if defined(__linux__)
# define HAS_EPOLL
#elif defined(__FreeBSD__)
# define HAS_KQUEUE
#else
# error "unknown OS"
#endif

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
#include <unistd.h>
#include <dlfcn.h>
#include <time.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <poll.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <signal.h>
#include <limits.h>
#include <pthread.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#if defined(__linux__)
# include <sys/sendfile.h>
# include <sys/epoll.h>
#elif defined(__FreeBSD__)
# include <sys/uio.h>
#else
# error "unknown OS"
#endif

#include "define.h"

#endif
