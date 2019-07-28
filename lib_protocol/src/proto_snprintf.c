#include "StdAfx.h"
#ifdef __STDC_WANT_SECURE_LIB__
#include <stdarg.h>

int proto_secure_snprintf(char *buf, size_t size, const char *fmt, ...)
{
	va_list ap;
	int   ret;

	va_start(ap, fmt);
	ret = _vsnprintf_s(buf, size, _TRUNCATE, fmt, ap);
	va_end(ap);
	return (ret);
}

int proto_securevsnprintf(char *buf, size_t size, const char *fmt, va_list ap)
{
	int   ret;

	ret = _vsnprintf_s(buf, size, _TRUNCATE, fmt, ap);
	return (ret);
}

#endif
