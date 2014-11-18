#ifndef __STD_AFX_INCLUDE_H__
#define __STD_AFX_INCLUDE_H__

# ifdef	WIN32
#  include <stdio.h>
#  include <stdarg.h>
#  ifdef __STDC_WANT_SECURE_LIB__
int tls_secure_snprintf(char *buf, size_t size, const char *fmt, ...);
int tls_secure_vsnprintf(char *buf, size_t size, const char *fmt, va_list ap);
#   define snprintf tls_secure_snprintf
#   define vsnprintf tls_secure_vsnprintf
#  else
#  ifndef snprintf
#   define snprintf _snprintf
#  endif
#  ifndef vsnprintf
#   define vsnprintf _vsnprintf
#  endif
#  endif
# endif  /* WIN2 */
#include "lib_acl.h"
#endif
