#ifndef __STD_AFX_INCLUDE_H__
#define __STD_AFX_INCLUDE_H__

#ifdef ACL_PREPARE_COMPILE

# include "stdlib/acl_define.h"

# include <string.h>
# include <errno.h>
# include <float.h>			/* DBL_MAX_10_EXP */
# include <ctype.h>
# include <limits.h>			/* CHAR_BIT */

# if defined(_WIN32) || defined(_WIN64)
#  include <process.h>
#  include <stdio.h>
#  include <stdarg.h>
#  if(_MSC_VER >= 1300)
#   include <winsock2.h>
#   include <mswsock.h>
#  else
#   include <winsock.h>
#  endif

#  include <ws2tcpip.h> /* for getaddrinfo */
#  if _MSC_VER >= 1500
#   include <netioapi.h>
#  endif

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
#  ifndef  _GNU_SOURCE
#   define _GNU_SOURCE
#  endif
#  ifndef __USE_UNIX98
#   define __USE_UNIX98
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
#  include <fcntl.h>
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

#if defined (_WIN32) || defined(_WIN64)
/* for vc2003 */
# if _MSC_VER <= 1310
int inet_pton(int af, const char *src, void *dst);
const char *inet_ntop(int af, const void *src, char *dst, socklen_t size);
int WSAAPI WSAPoll(LPWSAPOLLFD fdArray, ULONG fds, INT timeout);
# endif
#endif

#endif /* ACL_PREPARE_COMPILE */

//#define ACL_DEBUG_MIN 0
//#define ACL_DEBUG_MAX	30

#endif

