#ifndef __ACL_DEFINE_WIN32_INCLUDE_H__
#define __ACL_DEFINE_WIN32_INCLUDE_H__

#if defined (MS_VC) || defined(MS_VC6)
# define ACL_MS_WINDOWS
# define ACL_MS_VC
# ifndef _CRT_SECURE_NO_WARNINGS
#  define _CRT_SECURE_NO_WARNINGS
# endif
#elif	defined(BORLAND_CB)
# define ACL_MS_WINDOWS
# define ACL_BCB_COMPILER
#elif	defined(WIN32)
# define ACL_MS_WINDOWS
# define ACL_MS_VC
#endif

#ifdef	ACL_MS_WINDOWS

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
# define	_USE_FAST_MACRO
# define	_USE_HTABLE_SEARCH

# ifndef	c_pathdelim_chr
#  define	c_pathdelim_chr '\\'
# endif

# undef	ACL_HAS_PTHREAD
#endif /* ACL_MS_WINDOWS */

/* errno define */
#ifdef	ACL_MS_WINDOWS
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
#endif /* ACL_MS_WINDOWS */

#endif /* __ACL_DEFINE_WIN32_INCLUDE_H__ */
