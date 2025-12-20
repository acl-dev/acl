#ifndef ACL_DEFINE_WIN32_INCLUDE_H
#define ACL_DEFINE_WIN32_INCLUDE_H

/**
 * _MSC_VER:
 * see: https://docs.microsoft.com/zh-cn/cpp/preprocessor/predefined-macros?view=msvc-160&viewFallbackFrom=vs-2019
 * vc++5.0	VS 5.0	1100
 * vc++6.0	VS 6.0	1200
 * vc++7.0	VS 2003	1310
 * vc++8.0	VS 2005	1400
 * vc++9.0	VS 2008	1500
 * vc++10.0	VS 2010	1600
 * vc++11.0	VS 2012	1700
 * vc++14.0	VS 2015	1900
 * vc++15.0	VS 2017	1911
 * vc++16.0	VS 2019	1920
 * vc++16.1	VS 2019	1921
 * vc++16.2	VS 2019	1922
 * vc++16.3	VS 2019	1923
 * vc++16.4	VS 2019	1924
 * vc++16.5	VS 2019	1925
 * vc++16.6	VS 2019	1926
 * vc++16.7	VS 2019	1927
 */

#if defined (_WIN32) || defined(_WIN64)
# define ACL_WINDOWS
# if _MSC_VER >= 1500
#  ifndef _CRT_SECURE_NO_WARNINGS
#   define _CRT_SECURE_NO_WARNINGS
#  endif
# if _MSC_VER >= 1900
#   ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
#    define _WINSOCK_DEPRECATED_NO_WARNINGS
#   endif
# endif
# endif
#elif	defined(BORLAND_CB)
# define ACL_BCB_COMPILER
#endif

#if defined(_WIN32) || defined(_WIN64)

# ifdef acl_assert
#  undef acl_assert
# endif
# define acl_assert(x) do  \
  {  \
    if (!(x))  \
      abort();  \
  } while(0)

#ifdef ACL_LIB
# ifndef ACL_API
#  define ACL_API
# endif
#elif defined(ACL_DLL) /* || defined(_WINDLL) */
# if defined(ACL_EXPORTS) || defined(acl_EXPORTS)
#  ifndef ACL_API
#   define ACL_API __declspec(dllexport)
#  endif
# elif !defined(ACL_API)
#  define ACL_API __declspec(dllimport)
# endif
#elif !defined(ACL_API)
# define ACL_API
#endif

/**
 * see WINSOCK2.H, user needs to predefine this value. Default value is 64.
 * Additionally, this value should not be too large.
 * When compiling on X64 platform, problems may occur. Refer to fd_set structure definition:
 * typedef struct fd_set {
 *   u_int fd_count;
 *   SOCKET  fd_array[FD_SETSIZE];
 * } fd_set;
 * Under x64 platform, when FD_SETSIZE=50000, this structure occupies space size:
 * 8 + sizeof(SOCKET) * FD_SETSIZE = 400008
 * In events_select_thr.c's function event_loop, if multiple fd_set variables
 * are defined, this function's stack space size > 1MB. Since VC's default
 * stack size is 1MB, when calling this function, thread stack overflow will occur.
 * When FD_SSETSIZE needs to be very large, need to increase executable program's
 * stack space size. In program link options, select larger stack size, or
 * select a smaller value.
 */
# ifndef	FD_SETSIZE
#  define	FD_SETSIZE	40000
# endif

# include <fcntl.h>
# include <sys/stat.h>
# include <sys/types.h>

# ifndef	ACL_WIN32_STDC
#  define	ACL_WIN32_STDC
# endif

/* # include <windows.h> */
/* # include <winsock2.h> */
# if(_MSC_VER >= 1300)
#  include <winsock2.h>
#  include <mswsock.h>
# else
#  include <winsock.h>
# endif

# include <ws2tcpip.h> /* for getaddrinfo */
/*# include <netioapi.h>*/

# ifdef	ACL_BCB_COMPILER
#  pragma hdrstop
# endif
# define _USE_FAST_MACRO
# define _USE_HTABLE_SEARCH

# ifndef PATH_SEP_C
#  define PATH_SEP_C '\\'
# endif
# ifndef PATH_SEP_S
#  define PATH_SEP_S "\\"
# endif

# undef	ACL_HAS_PTHREAD
#endif /* _WIN32 */

/* errno define */
#if defined(_WIN32) || defined(_WIN64)
# define	ACL_ETIMEDOUT		WSAETIMEDOUT
# define	ACL_ETIME		WSAETIMEDOUT
# define	ACL_ENOMEM		WSAENOBUFS
# define	ACL_EINVAL		WSAEINVAL

# define	ACL_ECONNREFUSED	WSAECONNREFUSED
# define	ACL_ECONNRESET		WSAECONNRESET
# define	ACL_EHOSTDOWN		WSAEHOSTDOWN
# define	ACL_EHOSTUNREACH	WSAEHOSTUNREACH
# define	ACL_EINTR		WSAEINTR
# define	ACL_ENETDOWN		WSAENETDOWN
# define	ACL_ENETUNREACH		WSAENETUNREACH
# define	ACL_ENOTCONN		WSAENOTCONN
# define	ACL_EISCONN		WSAEISCONN
# define	ACL_EWOULDBLOCK		WSAEWOULDBLOCK
# define	ACL_EAGAIN		ACL_EWOULDBLOCK	/* xxx */
# define	ACL_ENOBUFS		WSAENOBUFS
# define	ACL_ECONNABORTED	WSAECONNABORTED
# define	ACL_EINPROGRESS		WSAEINPROGRESS
# define	ACL_EMFILE		WSAEMFILE

# define	ACL_SOCKET	SOCKET
# define	ACL_FILEFD	unsigned int
# ifndef	HAS_SOCKLEN_T
#  define	HAS_SOCKLEN_T
typedef int socklen_t;
# endif
# define	ACL_SOCKET_INVALID	INVALID_SOCKET
# define	ACL_FILE_HANDLE		HANDLE
# define	ACL_FILE_INVALID	INVALID_HANDLE_VALUE
# define	ACL_DLL_HANDLE		HINSTANCE
# define	ACL_DLL_FARPROC		FARPROC
# ifndef	HAS_SSIZE_T
#  define	HAS_SSIZE_T
/* typedef intptr_t ssize_t; */
#  if defined(_WIN64)
typedef __int64 ssize_t;
#  elif defined(_WIN32)
typedef int ssize_t;
#  else
typedef long ssize_t;
#  endif
# endif

# define ACL_INTERNAL_LOCK	ACL_FLOCK_STYLE_FLOCK

# define	acl_int64	__int64
# define	acl_uint64	unsigned __int64
# define	ACL_FMT_I64D	"%I64d"
# define	ACL_FMT_I64U	"%I64u"

# ifdef	ACL_BCB_COMPILER
/* #  define	acl_stat	_tstati64 */
#  define	acl_stat	stati64
# else
#  ifdef	ACL_HAVE_NO_STAT64
#   define	acl_stat	_stat
#   define	acl_fstat	_fstat
#  else
#   define	acl_stat	_stati64
#  endif
# endif

ACL_API int acl_fstat(ACL_FILE_HANDLE fh, struct acl_stat *buf);

/* Thread local storage */
# if defined(ACL_BCB_COMPILER)
#  define	__thread
# else
#  define	__thread	__declspec(thread)
# endif

/*
# ifdef	ACL_BCB_COMPILER
#  define	offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
# endif
*/
#endif /* _WIN32 / _WIN64 */

#endif /* __ACL_DEFINE_WIN32_INCLUDE_H__ */
