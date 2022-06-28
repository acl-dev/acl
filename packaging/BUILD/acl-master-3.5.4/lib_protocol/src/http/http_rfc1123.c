#include "StdAfx.h"
#include <time.h>

#include "http/lib_http.h"

#define RFC1123_STRFTIME "%a, %d %b %Y %H:%M:%S GMT"

const char *http_mkrfc1123(char *buf, size_t size, time_t t)
{
#ifdef ACL_WINDOWS
# if _MSC_VER >= 1500
	struct tm gmt_buf, *gmt = &gmt_buf;

	if (gmtime_s(gmt, &t) != 0)
		gmt = NULL;
# else
	struct tm *gmt = gmtime(&t);
# endif
#else
	struct tm gmt_buf, *gmt = gmtime_r(&t, &gmt_buf);
#endif

	buf[0] = 0;

	if (gmt != NULL)
		strftime(buf, size - 1, RFC1123_STRFTIME, gmt);
	return (buf);
}

