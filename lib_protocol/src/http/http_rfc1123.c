#include "StdAfx.h"
#include <time.h>

#include "http/lib_http.h"

#define RFC1123_STRFTIME "%a, %d %b %Y %H:%M:%S GMT"

const char *http_mkrfc1123(char *buf, size_t size, time_t t)
{
	struct tm *gmt = gmtime(&t);

	strftime(buf, size - 1, RFC1123_STRFTIME, gmt);
	return (buf);
}

