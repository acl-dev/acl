#ifndef	__DEFINE_INCLUDE_H__
#define	__DEFINE_INCLUDE_H__

#if defined(__linux__)
# define LINUX
# define SYS_UNIX
# define HAS_SELECT
# define HAS_POLL
# define HAS_EPOLL
#elif defined(__FreeBSD__)
# define SYS_UNIX
# define HAS_SELECT
# define HAS_POLL
# define HAS_KQUEUE
#elif defined(__APPLE__)
# define SYS_UNIX
# define HAS_SELECT
# define HAS_POLL
# define HAS_KQUEUE
# define _XOPEN_SOURCE
#elif defined(_WIN32) || defined(_WIN64)

# if(_MSC_VER >= 1300)
#  undef FD_SETSIZE
#  define FD_SETSIZE 10240
#  include <winsock2.h>
#  include <mswsock.h>
# else
#  include <winsock.h>
# endif

# if _MSC_VER >= 1500
#  include <netioapi.h>
# endif

# include <ws2tcpip.h> /* for getaddrinfo */
# include <process.h>
# include <stdint.h>

# define SYS_WIN
# define HAS_SELECT
# if(_WIN32_WINNT >= 0x0600)
#  define HAS_POLL
# endif
# define HAS_WMSG
# define HAS_IOCP
# define __thread __declspec(thread)

typedef unsigned long nfds_t;

#else
# error "unknown OS"
#endif

#ifdef SYS_UNIX
# ifndef WINAPI
#  define WINAPI
# endif
#endif

#ifndef fiber_unused
# ifdef	__GNUC__
#  define fiber_unused	__attribute__ ((__unused__))
# else
#  define fiber_unused  /* Ignore */
# endif
#endif

#if	__GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
#define	PRINTF(format_idx, arg_idx) \
	__attribute__((__format__ (__printf__, (format_idx), (arg_idx))))
#define	SCANF(format_idx, arg_idx) \
	__attribute__((__format__ (__scanf__, (format_idx), (arg_idx))))
#define	NORETURN __attribute__((__noreturn__))
#define	UNUSED __attribute__((__unused__))
#else
#define	PRINTF(format_idx, arg_idx)
#define	SCANF
#define	NORETURN
#define	UNUSED
#endif	/* __GNUC__ */

#if	__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1)
#define	DEPRECATED __attribute__((__deprecated__))
#elif	defined(_MSC_VER) && (_MSC_VER >= 1300)
#define	DEPRECATED __declspec(deprecated)
#else
#define	DEPRECATED
#endif	/* __GNUC__ */

#if	__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5)
#define	DEPRECATED_FOR(f) __attribute__((deprecated("Use " #f " instead")))
#elif	defined(_MSC_FULL_VER) && (_MSC_FULL_VER > 140050320)
#define	DEPRECATED_FOR(f) __declspec(deprecated("is deprecated. Use '" #f "' instead"))
#else
#define	DEPRECATED_FOR(f)	DEPRECATED
#endif	/* __GNUC__ */

#endif /* __DEFINE_INCLUDE_H__ */
