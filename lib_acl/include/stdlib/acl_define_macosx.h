#ifndef	__ACL_DEFINE_MACOSX_INCLUDE_H__
#define	__ACL_DEFINE_MACOSX_INCLUDE_H__

#if defined(MACOSX) || defined(__APPLE__)
# define ACL_UNIX
# define ACL_MACOSX

# include <sys/types.h>
# define ACL_USE_PATHS_H
# define ACL_HAS_FLOCK_LOCK
# define ACL_HAS_FCNTL_LOCK
# define ACL_INTERNAL_LOCK	ACL_FLOCK_STYLE_FLOCK
# define ACL_ROOT_PATH		"/bin:/usr/bin:/sbin:/usr/sbin"
# define ACL_PATH_MAILDIR	"/var/mail"
# define ACL_PATH_BSHELL	"/bin/sh"
# define ACL_PATH_DEFPATH	"/usr/bin:/usr/bsd"
# define ACL_PATH_STDPATH	"/usr/bin:/usr/sbin:/usr/bsd"

# define	ACL_HAVE_NO_STAT64

# ifdef	ACL_HAVE_NO_STAT64
#  define	acl_stat	stat
#  define	acl_fstat	fstat
# else
#  define	acl_stat	stat64
#  define	acl_fstat	fstat64
# endif

# ifndef ACL_WAIT_STATUS_T
   typedef int ACL_WAIT_STATUS_T;
#  define ACL_NORMAL_EXIT_STATUS(status)      !(status)
# endif

# define ACL_FIONREAD_IN_TERMIOS_H
# define	ACL_HAVE_NO_RWLOCK

#endif

#endif /* __ACL_DEFINE_MACOSX_INCLUDE_H__ */

