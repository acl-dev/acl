#ifndef __STD_AFX_INCLUDE_H__
#define __STD_AFX_INCLUDE_H__

#ifdef ACL_PREPARE_COMPILE

# include "stdlib/acl_define.h"

# include <string.h>
# include <errno.h>
# include <float.h>			/* DBL_MAX_10_EXP */
# include <ctype.h>
# include <limits.h>			/* CHAR_BIT */

#if defined(_WIN32) || defined(_WIN64)
#  include <process.h>
#  include <stdio.h>
#  include <stdarg.h>
#  ifdef __STDC_WANT_SECURE_LIB__
int acl_secure_snprintf(char *buf, size_t size, const char *fmt, ...);
int acl_secure_vsnprintf(char *buf, size_t size, const char *fmt, va_list ap);
#   define snprintf acl_secure_snprintf
#   define vsnprintf acl_secure_vsnprintf
#  else
#   define snprintf _snprintf
#   define vsnprintf _vsnprintf
#  endif  /* __STDC_WANT_SECURE_LIB__ */
# endif  /* _WIN32 */

# ifdef	ACL_UNIX
#  ifndef  __USE_XOPEN2K
#   define __USE_XOPEN2K
#  endif
#  ifndef  _GNU_SOURCE
#   define _GNU_SOURCE
#  endif

#  include <stdio.h>
#  include <stdlib.h>
#  include <unistd.h>
#  include <signal.h>
#  include <sys/types.h>
#  include <sys/wait.h>
#  include <pthread.h>
#  include <dlfcn.h>
#  include <dirent.h>
#  include <netinet/in.h>
#  include <netinet/ip.h>
#  include <netinet/tcp.h>
#  include <netdb.h>
#  include <stdarg.h>
#  include <pthread.h>
#  include <signal.h>
#  include <sys/stat.h>
#  include <sys/socket.h>
#  include <sys/un.h>
#  include <sys/mman.h>
#  ifdef ACL_FREEBSD
#   include <netinet/in_systm.h>
#   include <netinet/in.h>
#  endif
# endif  /* ACL_UNIX */

# include "lib_acl.h"

#endif /* ACL_PREPARE_COMPILE */

#endif

