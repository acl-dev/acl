#ifndef ACL_DEFINE_WIN32_INCLUDE_H
#define ACL_DEFINE_WIN32_INCLUDE_H

/**
 * _MSC_VER:
 * vc++5.0	VS 5.0	1100
 * vc++6.0	VS 6.0	1200
 * vc++7.0	VS 2003	1310
 * vc++8.0	VS 2005	1400
 * vc++9.0	VS 2008	1500
 * vc++10.0	VS 2010	1600
 * vc++11.0	VS 2012	1700
 */

#if defined (WIN32)
# if _MSC_VER >= 1500
#  ifndef _CRT_SECURE_NO_WARNINGS
#   define _CRT_SECURE_NO_WARNINGS
#  endif
# endif
#elif	defined(BORLAND_CB)
# define ACL_BCB_COMPILER
#endif

#ifdef	WIN32

# ifdef acl_assert
#  undef acl_assert
# endif
# define acl_assert(x) do  \
  {  \
    if (!(x))  \
      abort();  \
  } while(0)

# ifdef ACL_DLL
#  ifdef ACL_EXPORTS
#   define ACL_API __declspec(dllexport)
#  else
#   define ACL_API __declspec(dllimport)
#  endif
# else
#  define ACL_API
# endif

# include <fcntl.h>
# include <sys/stat.h>
# include <sys/types.h>

# ifndef	ACL_WIN32_STDC
#  define	ACL_WIN32_STDC
# endif
# ifndef	FD_SETSIZE
#  define	FD_SETSIZE	4096  /* see WINSOCK2.H, 用户需要预先定义此值，因其默认值为64 */
# endif
/* # include <windows.h> */
/* # include <winsock2.h> */
# if(_MSC_VER >= 1300)
#  include <winsock2.h>
#  include <mswsock.h>
# else
#  include <winsock.h>
# endif

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
#endif /* WIN32 */

/* errno define */
#ifdef	WIN32
# define	ACL_ETIMEDOUT		WSAETIMEDOUT
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

# define	ACL_SOCKET	SOCKET
# define	ACL_FILEFD	unsigned int
# define	socklen_t	int
# define	ACL_SOCKET_INVALID	INVALID_SOCKET
# define	ACL_FILE_HANDLE		HANDLE
# define	ACL_FILE_INVALID	INVALID_HANDLE_VALUE
# define	ACL_DLL_HANDLE		HINSTANCE
# define	ACL_DLL_FARPROC		FARPROC
# ifndef	HAS_SSIZE_T
#  define	ssize_t	long
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

/* 线程局部变量 */
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
#endif /* WIN32 */

#endif /* __ACL_DEFINE_WIN32_INCLUDE_H__ */
