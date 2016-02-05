#ifndef ACL_DEFINE_UNIX_INCLUDE_H
#define ACL_DEFINE_UNIX_INCLUDE_H

#include "acl_define_linux.h"
#include "acl_define_sunx86.h"
#include "acl_define_bsd.h"
#include "acl_define_macosx.h"

/* __FreeBSD_version version is major+minor */

#if __FreeBSD_version >= 200000
# define HAS_DUPLEX_PIPE
#endif

#ifdef __FreeBSD_kernel__
# define HAS_DUPLEX_PIPE
# define HAS_ISSETUGID
#endif

#ifdef	ACL_UNIX
/*
# include <errno.h>
*/

# ifndef PATH_SEP_C
#  define PATH_SEP_C '/'
# endif
# ifndef PATH_SEP_S
#  define PATH_SEP_S "/"
# endif

/*
# include <pthread.h>
*/
# define ACL_HAS_PTHREAD

#endif /* ACL_UNIX */

#ifdef	ACL_UNIX

# include <assert.h>
# include <sys/types.h>

# ifndef acl_assert
#  define acl_assert assert
# endif

# define ACL_API

# define	ACL_ETIMEDOUT		ETIMEDOUT
# define	ACL_ENOMEM		ENOMEM
# define	ACL_EINVAL		EINVAL

# define	ACL_ECONNREFUSED	ECONNREFUSED
# define	ACL_ECONNRESET		ECONNRESET
# define	ACL_EHOSTDOWN		EHOSTDOWN
# define	ACL_EHOSTUNREACH	EHOSTUNREACH
# define	ACL_EINTR		EINTR
# define	ACL_EAGAIN		EAGAIN
# define	ACL_ENETDOWN		ENETDOWN
# define	ACL_ENETUNREACH		ENETUNREACH
# define	ACL_ENOTCONN		ENOTCONN
# define	ACL_EISCONN		EISCONN
# define	ACL_EWOULDBLOCK		EWOULDBLOCK
# define	ACL_ENOBUFS		ENOBUFS
# define	ACL_ECONNABORTED	ECONNABORTED
# define	ACL_EINPROGRESS		EINPROGRESS

# define	ACL_SOCKET		int
# define	ACL_FILEFD		int
# define	ACL_SOCKET_INVALID	(int) -1
# define	ACL_FILE_HANDLE		int
# define	ACL_FILE_INVALID	(int) -1
# define	ACL_DLL_HANDLE		void*
# define	ACL_DLL_FARPROC		void*

# define	acl_int64	long long int
# define	acl_uint64	unsigned long long int
# define	ACL_FMT_I64D	"%lld"
# define	ACL_FMT_I64U	"%llu"

# define	ACL_PATH_BSHELL	"/bin/sh"

#endif /* ACL_UNIX */

#endif
