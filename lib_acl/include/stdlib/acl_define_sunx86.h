#ifndef	ACL_DEFINE_SUNX86_INCLUDE_H
#define	ACL_DEFINE_SUNX86_INCLUDE_H

#ifdef SUNOS5
# define ACL_SUNOS5
# define ACL_UNIX

# include <sys/types.h>
/* # include <features.h> */

# ifdef	ACL_HAVE_NO_STAT64
#  define	acl_stat	stat
#  define	acl_fstat	fstat
# else
#  define	acl_stat	stat64
#  define	acl_fstat	fstat64
# endif

# define ACL_USE_PATHS_H
/* # define ACL_HAS_FLOCK_LOCK */

# define ACL_HAS_FCNTL_LOCK
/*# define ACL_INTERNAL_LOCK	ACL_FLOCK_STYLE_FLOCK */
# define ACL_INTERNAL_LOCK	ACL_FLOCK_STYLE_FCNTL

# define ACL_ROOT_PATH		"/bin:/usr/bin:/sbin:/usr/sbin"
# define ACL_PATH_MAILDIR	"/var/mail"
# define ACL_PATH_DEFPATH	"/usr/bin:/usr/ucb"
# define ACL_PATH_STDPATH	"/usr/bin:/usr/etc:/usr/ucb"

# ifndef ACL_WAIT_STATUS_T
   typedef int ACL_WAIT_STATUS_T;
#  define ACL_NORMAL_EXIT_STATUS(status)      ((status) == 0)
# endif
  
# define	ACL_FIONREAD_IN_SYS_FILIO_H 

#endif

#endif /* __ACL_DEFINE_SUNX86_INCLUDE_H__ */
