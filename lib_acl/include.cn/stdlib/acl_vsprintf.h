#ifndef	ACL_VSPRINTF_INCLUDE_H
#define	ACL_VSPRINTF_INCLUDE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "acl_define.h"
#include <stdarg.h>
#include <stdlib.h>

ACL_API int acl_vsnprintf(char *buf, size_t size, const char *fmt, va_list args);
ACL_API int acl_snprintf(char * buf, size_t size, const char *fmt, ...);
ACL_API int acl_vsprintf(char *buf, const char *fmt, va_list args);
ACL_API int acl_sprintf(char * buf, const char *fmt, ...);

#ifdef	__cplusplus
}
#endif

#endif
