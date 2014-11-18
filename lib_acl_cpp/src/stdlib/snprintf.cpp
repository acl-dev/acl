#include "acl_stdafx.hpp"
#ifdef WIN32
#include <stdarg.h>

namespace acl
{

#ifdef __STDC_WANT_SECURE_LIB__

int snprintf(char *buf, size_t size, const char *fmt, ...)
{
	va_list ap;
	int   ret;

	va_start(ap, fmt);
	ret = _vsnprintf_s(buf, size, _TRUNCATE, fmt, ap);
	va_end(ap);
	return (ret);
}

int vsnprintf(char *buf, size_t size, const char *fmt, va_list ap)
{
	int   ret;

	ret = _vsnprintf_s(buf, size, _TRUNCATE, fmt, ap);
	return (ret);
}

#else

int snprintf(char *buf, size_t size, const char *fmt, ...)
{
	va_list ap;
	int   ret;

	va_start(ap, fmt);
	ret = vsnprintf(buf, size, fmt, ap);
	va_end(ap);
	return ret;
}

int vsnprintf(char *buf, size_t size, const char *fmt, va_list ap)
{
	int   ret = _vsnprintf(buf, size, fmt, ap);
	if (ret <= (int) size)
	{
		buf[size - 1] = 0;
		ret = (int) size - 1;
	}
	return ret;
}

#endif // __STDC_WANT_SECURE_LIB__

}

#endif // WIN32
