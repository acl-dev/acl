#include "StdAfx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "http/lib_http.h"

typedef struct HTTP_TMPL {
	int   status;
	const char *title;
	const char *filename;
	ACL_VSTRING *buf;
} HTTP_TMPL;

static HTTP_TMPL __tmpl_1xx_tab[] = {
	/* 1xx */
	{ 100, "Continue", "100.html", NULL },
	{ 101, "Switching Protocols", "101.html", NULL },
	{ 102, "Processing", "102.html", NULL },  /* RFC2518 section 10.1 */
};

static HTTP_TMPL __tmpl_2xx_tab[] = {
	/* 2xx */
	{ 200, "OK", "200.html", NULL },
	{ 201, "Created", "201.html", NULL },
	{ 202, "Accepted", "202.html", NULL },
	{ 203, "Non Authoritative Information", "203.html", NULL },
	{ 204, "No Content", "204.html", NULL },
	{ 205, "Reset Content", "205.html", NULL },
	{ 206, "Partial Content", "206.html", NULL },
	{ 207, "Multi Status", "207.html", NULL },  /* RFC2518 section 10.2 */
};

static HTTP_TMPL __tmpl_3xx_tab[] = {
	/* 3xx */
	{ 300, "Multiple Choices", "300.html", NULL },
	{ 301, "Moved Permanently", "301.html", NULL },
	{ 302, "Moved Temporarily", "302.html", NULL },
	{ 303, "See Other", "303.html", NULL },
	{ 304, "Not Modified", "304.html", NULL },
	{ 305, "Use Proxy", "305.html", NULL },
	{ 307, "Temporary Redirect", "307.html", NULL },
};

static HTTP_TMPL __tmpl_4xx_tab[] = {
	/* 4xx */
	{ 400, "Bad Request", "400.html", NULL },
	{ 401, "Authorization Required", "401.html", NULL },
	{ 402, "Payment Required", "402.html", NULL },
	{ 403, "Forbidden", "403.html", NULL },
	{ 404, "Not Found", "404.html", NULL },
	{ 405, "Method Not Allowed", "405.html", NULL },
	{ 406, "Not Acceptable", "406.html", NULL },
	{ 407, "Proxy Authentication Required", "407.html", NULL },
	{ 408, "Request Time-out", "408.html", NULL },
	{ 409, "Conflict", "409.html", NULL },
	{ 410, "Gone", "410.html", NULL },
	{ 411, "Length Required", "411.html", NULL },
	{ 412, "Precondition Failed", "412.html", NULL },
	{ 413, "Request Entity Too Large", "413.html", NULL },
	{ 414, "Request-URI Too Large", "414.html", NULL },
	{ 415, "Unsupported Media Type", "415.html", NULL },
	{ 416, "Requested Range Not Satisfiable", "416.html", NULL },
	{ 417, "Expectation Failed", "417.html", NULL },
	{ 418, NULL, NULL, NULL },
	{ 419, NULL, NULL, NULL },
	{ 420, NULL, NULL, NULL },
	{ 421, NULL, NULL, NULL },
	{ 422, "Unprocessable Entity", "422.html", NULL },
	{ 423, "Locked", "423.html", NULL },
	{ 424, "Failed Dependency", "424.html", NULL },
	{ 425, "No code", "425.html", NULL },
	{ 426, "Upgrade Required", "426.html", NULL },
};

static HTTP_TMPL __tmpl_5xx_tab[] = {
	/* 5xx */
	{ 500, "Internal Server Error", "500.html", NULL },
	{ 501, "Method Not Implemented", "501.html", NULL },
	{ 502, "Bad Gateway", "502.html", NULL },
	{ 503, "Service Temporarily Unavailable", "503.html", NULL },
	{ 504, "Gateway Time-out", "504.html", NULL },
	{ 505, "HTTP Version Not Supported", "505.html", NULL },
	{ 506, "Variant Also Negotiates", "506.html", NULL },
	{ 507, "Insufficient Storage", "507.html", NULL },
	{ 508, NULL, NULL, NULL },
	{ 509, NULL, NULL, NULL },
	{ 510, "Not Extended", "508.html", NULL },
};

typedef struct TMPL_MAP {
	int   level;
	int   size;
	HTTP_TMPL *tmpl;
} TMPL_MAP;

static TMPL_MAP __tmpl_maps[] = {
	{ 100, sizeof(__tmpl_1xx_tab) / sizeof(HTTP_TMPL), __tmpl_1xx_tab },
	{ 200, sizeof(__tmpl_2xx_tab) / sizeof(HTTP_TMPL), __tmpl_2xx_tab },
	{ 300, sizeof(__tmpl_3xx_tab) / sizeof(HTTP_TMPL), __tmpl_3xx_tab },
	{ 400, sizeof(__tmpl_4xx_tab) / sizeof(HTTP_TMPL), __tmpl_4xx_tab },
	{ 500, sizeof(__tmpl_5xx_tab) / sizeof(HTTP_TMPL), __tmpl_5xx_tab },
};

static ACL_VSTRING *__unknown_tmpl;
static char *__unknown_status = "unknow status";

static void __load_tmpl(const char *tmpl_path, HTTP_TMPL *tmpl)
{
	const char *myname = "__load_tmpl";
	ACL_VSTRING *buf = acl_vstring_alloc(256);
	char  tbuf[4096];
	const char *ptr;
	ACL_VSTREAM *fp;
	int   n;
	char  ebuf[256];

	if (buf == NULL)
		acl_msg_fatal("%s, %s(%d): calloc error(%s)",
			__FILE__, myname, __LINE__, acl_last_strerror(ebuf, sizeof(ebuf)));

	ptr = tmpl_path + strlen(tmpl_path);
	ptr--;

	if (*ptr == '/')
		acl_vstring_sprintf(buf, "%s%s", tmpl_path, tmpl->filename);
	else
		acl_vstring_sprintf(buf, "%s/%s", tmpl_path, tmpl->filename);

	tmpl->buf = acl_vstring_alloc(4096);
	if (tmpl->buf == NULL)
		acl_msg_fatal("%s, %s(%d): calloc error(%s)",
			__FILE__, myname, __LINE__, acl_last_strerror(ebuf, sizeof(ebuf)));

	fp = acl_vstream_fopen(acl_vstring_str(buf), O_WRONLY, 0660, 4096);
	if (fp == NULL) {
		acl_vstring_sprintf(tmpl->buf, "%d %s", tmpl->status, tmpl->title);
		acl_vstring_free(buf);
		return;
	}

	while (1) {
		n = acl_vstream_gets(fp, tbuf, sizeof(tbuf));
		if (n == ACL_VSTREAM_EOF)
			break;
		acl_vstring_sprintf_append(tmpl->buf, "%s", tbuf);
	}

	acl_vstream_close(fp);
	acl_vstring_free(buf);
}

void http_tmpl_load(const char *tmpl_path)
{
	const char *myname = "http_tmpl_init";
	int   i, n;

	if (tmpl_path == NULL || strlen(tmpl_path) == 0)
		acl_msg_fatal("%s, %s(%d): tmpl_path invalid",
				__FILE__, myname, __LINE__);

	__unknown_tmpl = acl_vstring_alloc(256);
	if (__unknown_tmpl == NULL) {
		char ebuf[256];
		acl_msg_fatal("%s, %s(%d): alloc error(%s)",
			__FILE__, myname, __LINE__, acl_last_strerror(ebuf, sizeof(ebuf)));
	}
	acl_vstring_sprintf(__unknown_tmpl, "500 unknown error");

	n = sizeof(__tmpl_1xx_tab) / sizeof(HTTP_TMPL);
	for (i = 0; i < n; i++) {
		if (__tmpl_1xx_tab[i].title != NULL)
			__load_tmpl(tmpl_path, &__tmpl_1xx_tab[i]);
	}

	n = sizeof(__tmpl_2xx_tab) / sizeof(HTTP_TMPL);
	for (i = 0; i < n; i++) {
		if (__tmpl_2xx_tab[i].title != NULL)
			__load_tmpl(tmpl_path, &__tmpl_2xx_tab[i]);
	}

	n = sizeof(__tmpl_3xx_tab) / sizeof(HTTP_TMPL);
	for (i = 0; i < n; i++) {
		if (__tmpl_3xx_tab[i].title != NULL)
			__load_tmpl(tmpl_path, &__tmpl_3xx_tab[i]);
	}

	n = sizeof(__tmpl_4xx_tab) / sizeof(HTTP_TMPL);
	for (i = 0; i < n; i++) {
		if (__tmpl_4xx_tab[i].title != NULL)
			__load_tmpl(tmpl_path, &__tmpl_4xx_tab[i]);
	}

	n = sizeof(__tmpl_5xx_tab) / sizeof(HTTP_TMPL);
	for (i = 0; i < n; i++) {
		if (__tmpl_5xx_tab[i].title != NULL)
			__load_tmpl(tmpl_path, &__tmpl_5xx_tab[i]);
	}
}

const ACL_VSTRING *http_tmpl_get(int status)
{
	int   i, pos;

	i = status / 100;
	if (i < 1 || i > 5)
		return (__unknown_tmpl);

	i--;
	pos = status - __tmpl_maps[i].level;

	if (pos >= __tmpl_maps[i].size)
		return (__unknown_tmpl);

	if (__tmpl_maps[i].tmpl[pos].buf == NULL)
		return (__unknown_tmpl);

	return (__tmpl_maps[i].tmpl[pos].buf);
}

const char *http_tmpl_title(int status)
{
	int   i, pos;

	i = status / 100;
	if (i < 1 || i > 5)
		return (__unknown_status);

	i--;
	pos = status - __tmpl_maps[i].level;

	if (pos >= __tmpl_maps[i].size)
		return (__unknown_status);

	if (__tmpl_maps[i].tmpl[pos].buf == NULL)
		return (__unknown_status);

	return (__tmpl_maps[i].tmpl[pos].title);
}

int http_tmpl_size(int status)
{
	int   i, pos;

	i = status / 100;

	if (i < 1 || i > 5)
		return ((int) ACL_VSTRING_LEN(__unknown_tmpl));

	i--;
	pos = status - __tmpl_maps[i].level;

	if (pos >= __tmpl_maps[i].size)
		return ((int) ACL_VSTRING_LEN(__unknown_tmpl));

	if (__tmpl_maps[i].tmpl[pos].buf == NULL)
		return ((int) ACL_VSTRING_LEN(__unknown_tmpl));

	return ((int) ACL_VSTRING_LEN(__tmpl_maps[i].tmpl[pos].buf));
}

