#include "StdAfx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "http/lib_http.h"

void http_hdr_put_str(HTTP_HDR *hdr, const char *name, const char *value)
{
	HTTP_HDR_ENTRY *entry;

	entry = http_hdr_entry_build(name, value);
	if (entry)
		http_hdr_append_entry(hdr, entry);
}

void http_hdr_put_int(HTTP_HDR *hdr, const char *name, int value)
{
	char  buf[32];
	HTTP_HDR_ENTRY *entry;

	snprintf(buf, sizeof(buf) - 1, "%d", value);
	entry = http_hdr_entry_build(name, buf);
	if (entry)
		http_hdr_append_entry(hdr, entry);
}

void http_hdr_put_fmt(HTTP_HDR *hdr, const char *name, const char *fmt, ...)
{
	char  myname[] = "http_hdr_put_fmt";
	va_list  ap;
	HTTP_HDR_ENTRY *entry;
	ACL_VSTRING *strbuf = acl_vstring_alloc(1024);

	if (strbuf == NULL) {
		char ebuf[256];
		acl_msg_fatal("%s, %s(%d): calloc error(%s)",
			__FILE__, myname, __LINE__, acl_last_strerror(ebuf, sizeof(ebuf)));
	}

	va_start(ap, fmt);
	acl_vstring_vsprintf_append(strbuf, fmt, ap);
	va_end(ap);

	entry = http_hdr_entry_build(name, acl_vstring_str(strbuf));
	if (entry)
		http_hdr_append_entry(hdr, entry);

	acl_vstring_free(strbuf);
}

void http_hdr_put_time(HTTP_HDR *hdr, const char *name, time_t t)
{
	char  buf[128];
	HTTP_HDR_ENTRY *entry;

	buf[sizeof(buf) - 1] = '\0';

	(void) http_mkrfc1123(buf, sizeof(buf) - 1, t);
	entry = http_hdr_entry_build(name, buf);
	if (entry)
		http_hdr_append_entry(hdr, entry);
}

int http_hdr_set_keepalive(const HTTP_HDR_REQ *req, HTTP_HDR_RES *res)
{
#if 0
	int   left;

	if (req->hdr.keep_alive == 0
	    || req->server == NULL
	    || req->server->keep_alive_timeout <= 0) {
		http_hdr_put_str(&res->hdr, "Connection", "close");
		return (0);
	}

	left = req->server->keep_alive_max - req->nkeepalive;

	if (req->server->keep_alive_max > 0)
		http_hdr_put_fmt(&res->hdr,
				"Keep-Alive",
				"timeout=%d, max=%d",
				req->server->keep_alive_timeout,
				left);
	else
		http_hdr_put_int(&res->hdr,
				"Keep-Alive",
				req->server->keep_alive_timeout);

	http_hdr_put_str(&res->hdr, "Connection", "Keep-Alive");

	return (1);
#else
	const char *ptr;

	ptr = http_hdr_entry_value(&req->hdr, "Connection");

	if (ptr == NULL || strcasecmp(ptr, "keep-alive") != 0) {
		http_hdr_put_str(&res->hdr, "Connection", "close");
		res->hdr.keep_alive = 0;
		return (0);
	}

	http_hdr_put_str(&res->hdr, "Connection", "Keep-Alive");
	res->hdr.keep_alive = 1;

	return (1);
#endif
}

void http_hdr_res_init(HTTP_HDR_RES *hdr_res, int status)
{
	const char *ptr;

	ptr = http_status_line(status);
	http_hdr_put_str(&hdr_res->hdr, "HTTP/1.1", ptr);
	http_hdr_put_str(&hdr_res->hdr, "Server", "acl_httpd");
}

HTTP_HDR_RES *http_hdr_res_static(int status)
{
	HTTP_HDR_RES *res_hdr;

	res_hdr = http_hdr_res_new();
	http_hdr_res_init(res_hdr, status);
	http_hdr_put_time(&res_hdr->hdr, "Date", time(NULL));
	return (res_hdr);
}

HTTP_HDR_RES *http_hdr_res_error(int status)
{
	int   n;
	HTTP_HDR_RES *res_hdr;

	res_hdr = http_hdr_res_new();
	http_hdr_res_init(res_hdr, status);
	http_hdr_put_time(&res_hdr->hdr, "Date", time(NULL));
	http_hdr_put_str(&res_hdr->hdr, "Content-Type", "text/html");
	http_hdr_put_str(&res_hdr->hdr, "Connection", "close");

	n = http_tmpl_size(status);
	http_hdr_put_int(&res_hdr->hdr, "Content-Length", n);

	return (res_hdr);
}

void http_hdr_build(const HTTP_HDR *hdr, ACL_VSTRING *strbuf)
{
	ACL_ARRAY *entries;
	HTTP_HDR_ENTRY *entry;
	int   i, n;

	entries = hdr->entry_lnk;
	n = acl_array_size(entries);

	entry = (HTTP_HDR_ENTRY *) acl_array_index(entries, 0);
	acl_vstring_sprintf(strbuf, "%s %s\r\n", entry->name, entry->value);

	for (i = 1; i < n; i++) {
		entry = (HTTP_HDR_ENTRY *) acl_array_index(entries, i);
		if (entry == NULL)
			break;
		if (entry->off)
			continue;
		acl_vstring_sprintf_append(strbuf, "%s: %s\r\n", entry->name, entry->value);
	}

	acl_vstring_strcat(strbuf, "\r\n");
}

void http_hdr_build_request(const HTTP_HDR_REQ *hdr_req, ACL_VSTRING *strbuf)
{
	ACL_ARRAY *entries;
	HTTP_HDR_ENTRY *entry;
	int   i, n;

	entries = hdr_req->hdr.entry_lnk;
	n = acl_array_size(entries);

	entry = (HTTP_HDR_ENTRY *) acl_array_index(entries, 0);
#if 0
	acl_vstring_sprintf(strbuf, "%s %s\r\n", entry->name, entry->value);
#else
	acl_vstring_sprintf(strbuf, "%s %s HTTP/%d.%d\r\n", entry->name,
		acl_vstring_str(hdr_req->url_part),
		hdr_req->hdr.version.major, hdr_req->hdr.version.minor);
#endif

	for (i = 1; i < n; i++) {
		entry = (HTTP_HDR_ENTRY *) acl_array_index(entries, i);
		if (entry == NULL)
			break;
		if (entry->off)
			continue;
		acl_vstring_sprintf_append(strbuf, "%s: %s\r\n", entry->name, entry->value);
	}

	acl_vstring_strcat(strbuf, "\r\n");
}
