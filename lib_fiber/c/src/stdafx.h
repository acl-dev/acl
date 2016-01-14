#ifndef __STD_AFX_INCLUDE_H__
#define __STD_AFX_INCLUDE_H__


#ifdef	LINUX
#define	HAS_EPOLL
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
#if	defined(LINUX)
#include <sys/sendfile.h>
#elif	defined(FREEBSD)
#endif
#include <sys/stat.h>
#include <sys/select.h>
#include <poll.h>
#ifdef	LINUX
#include <sys/epoll.h>
#endif
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

#include "define.h"

#endif
