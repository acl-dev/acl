#ifndef	ACL_DEFINE_LINUX_INCLUDE_H
#define	ACL_DEFINE_LINUX_INCLUDE_H

#ifdef LINUX2
# define ACL_LINUX
# define ACL_UNIX

#include <stddef.h>	/* just for size_t */

/* for O_LARGEFILE flag define */
#ifndef MINGW
# ifndef _GNU_SOURCE
#  ifndef _LARGEFILE64_SOURCE
#   define _LARGEFILE64_SOURCE
#  endif
#  ifndef _FILE_OFFSET_BITS
#   define _FILE_OFFSET_BITS 64
#  endif
# endif
#endif

/*
# include <sys/types.h>
# include <features.h>
# include <sys/stat.h>
# include <unistd.h>
*/

# define ACL_USE_PATHS_H
# ifndef MINGW
#  define ACL_HAS_FLOCK_LOCK
/* #  define ACL_HAS_SPINLOCK */
# endif

# define ACL_HAS_FCNTL_LOCK
# define ACL_INTERNAL_LOCK	ACL_FLOCK_STYLE_FLOCK
# define ACL_ROOT_PATH		"/bin:/usr/bin:/sbin:/usr/sbin"
# define ACL_PATH_MAILDIR	"/var/mail"
# define ACL_PATH_BSHELL	"/bin/sh"
# define ACL_PATH_DEFPATH	"/usr/bin"
# define ACL_PATH_STDPATH	"/usr/bin:/usr/sbin"

# if defined(ACL_HAVE_NO_STAT64) || defined(MINGW)
#  define	acl_stat	stat
#  define	acl_fstat	fstat
# else
#  define	acl_stat	stat64
#  define	acl_fstat	fstat64
# endif

# if defined(MINGW)
typedef long int ssize_t;
# endif

# ifndef ACL_WAIT_STATUS_T
 typedef int ACL_WAIT_STATUS_T;
#  define ACL_NORMAL_EXIT_STATUS(status)      !(status)
# endif

# define ACL_FIONREAD_IN_TERMIOS_H
# define	ACL_HAVE_NO_RWLOCK

#endif

#endif /* __ACL_DEFINE_LINUX_INCLUDE_H__ */

