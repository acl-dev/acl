#include "StdAfx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "http/lib_http.h"

typedef struct STATUS_LINE {
	int   status;
	const char *info;
} STATUS_LINE;

static STATUS_LINE __status_lines_100[] = {
	{ 100, "100 Continue" },
	{ 101, "101 Switching Protocols" },
	{ 102, "102 Processing" },
};

static STATUS_LINE __status_lines_200[] = {
	{ 200, "200 OK" },
	{ 201, "201 Created" },
	{ 202, "202 Accepted" },
	{ 203, "203 Non-Authoritative Information" },
	{ 204, "204 No Content" },
	{ 205, "205 Reset Content" },
	{ 206, "206 Partial Content" },
	{ 207, "207 Multi-Status" },
};

static STATUS_LINE __status_lines_300[] = {
	{ 300, "300 Multiple Choices" },
	{ 301, "301 Moved Permanently" },
	{ 302, "302 Found" },
	{ 303, "303 See Other" },
	{ 304, "304 Not Modified" },
	{ 305, "305 Use Proxy" },
	{ 306, "306 unused" },
	{ 307, "307 Temporary Redirect" },
};

static STATUS_LINE __status_lines_400[] = {
	{ 400, "400 Bad Request" },
	{ 401, "401 Authorization Required" },
	{ 402, "402 Payment Required" },
	{ 403, "403 Forbidden" },
	{ 404, "404 Not Found" },
	{ 405, "405 Method Not Allowed" },
	{ 406, "406 Not Acceptable" },
	{ 407, "407 Proxy Authentication Required" },
	{ 408, "408 Request Time-out" },
	{ 409, "409 Conflict" },
	{ 410, "410 Gone" },
	{ 411, "411 Length Required" },
	{ 412, "412 Precondition Failed" },
	{ 413, "413 Request Entity Too Large" },
	{ 414, "414 Request-URI Too Large" },
	{ 415, "415 Unsupported Media Type" },
	{ 416, "416 Requested Range Not Satisfiable" },
	{ 417, "417 Expectation Failed" },
	{ 418, NULL },
	{ 419, NULL },
	{ 420, NULL },
	{ 421, NULL },
	{ 422, "422 Unprocessable Entity" },
	{ 423, "423 Locked" },
	{ 424, "424 Failed Dependency" },
	{ 425, "425 No code" },
	{ 426, "426 Upgrade Required" },
};

static STATUS_LINE __status_lines_500[] = {
	{ 500, "500 Internal Server Error" },
	{ 501, "501 Method Not Implemented" },
	{ 502, "502 Bad Gateway" },
	{ 503, "503 Service Temporarily Unavailable" },
	{ 504, "504 Gateway Time-out" },
	{ 505, "505 HTTP Version Not Supported" },
	{ 506, "506 Variant Also Negotiates" },
	{ 507, "507 Insufficient Storage" },
	{ 508, NULL },
	{ 509, NULL },
	{ 510, "510 Not Extended" },
};

typedef struct STATUS_MAP {
	int   level;
	int   size;
	STATUS_LINE *status_lines;
} STATUS_MAP;

static STATUS_MAP __status_maps[] = {
	{ 100, sizeof(__status_lines_100) / sizeof(STATUS_LINE), __status_lines_100 },
	{ 200, sizeof(__status_lines_200) / sizeof(STATUS_LINE), __status_lines_200 },
	{ 300, sizeof(__status_lines_300) / sizeof(STATUS_LINE), __status_lines_300 },
	{ 400, sizeof(__status_lines_400) / sizeof(STATUS_LINE), __status_lines_400 },
	{ 500, sizeof(__status_lines_500) / sizeof(STATUS_LINE), __status_lines_500 },
};

#define	UNKNOWN_ERROR_INFO	"500 Internal Server Error(unknown)"

const char *http_status_line(int status)
{
	int   i, pos;

	i = status / 100;
	if (i < 1 || i > 5)
		return (UNKNOWN_ERROR_INFO);

	i--;
	pos = status - __status_maps[i].level;

	if (pos >= __status_maps[i].size)
		return (UNKNOWN_ERROR_INFO);

	if (__status_maps[i].status_lines[pos].info == NULL)
		return (UNKNOWN_ERROR_INFO);

	return (__status_maps[i].status_lines[pos].info);
}


