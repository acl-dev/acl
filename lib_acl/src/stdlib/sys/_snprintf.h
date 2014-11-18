#ifndef	__SNPRINTF_SAFE_INCLUDE_H__
#define	__SNPRINTF_SAFE_INCLUDE_H__

#include <stdarg.h>
#include <stdlib.h>

#ifdef	__cplusplus
extern "C" {
#endif

int acl_secure_snprintf(char *buf, size_t size, const char *fmt, ...);
int acl_secure_vsnprintf(char *buf, size_t size, const char *fmt, va_list ap);

#ifdef	__cplusplus
}
#endif

#endif
