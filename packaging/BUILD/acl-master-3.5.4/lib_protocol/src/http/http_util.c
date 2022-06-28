#include "StdAfx.h"
#include <string.h>
#include "http/lib_http_struct.h"
#include "http/lib_http.h"
#include "http/lib_http_util.h"

void http_util_free(HTTP_UTIL *http_util)
{
	if ((http_util->flag & HTTP_UTIL_FLAG_SET_DUMP_FILE)) {
		if (http_util->dump_stream)
			acl_vstream_close(http_util->dump_stream);
	}

	if (http_util->stream)
		acl_vstream_close(http_util->stream);
	if (http_util->req_buf)
		acl_vstring_free(http_util->req_buf);
	if (http_util->hdr_req)
		http_hdr_req_free(http_util->hdr_req);
	if (http_util->http_res)
		http_res_free(http_util->http_res);
	else if (http_util->hdr_res)
		http_hdr_res_free(http_util->hdr_res);
	acl_myfree(http_util);
}

HTTP_UTIL *http_util_req_new(const char *url, const char *method)
{
	const char *myname = "http_util_req_new";
	HTTP_UTIL *http_util = (HTTP_UTIL*) acl_mycalloc(1, sizeof(HTTP_UTIL));
	const char *ptr, *host;

	if (url == NULL || *url == 0) {
		acl_msg_error("%s(%d): url invalid", myname, __LINE__);
		return (NULL);
	}
	if (method == NULL || *method == 0) {
		acl_msg_error("%s(%d): method invalid", myname, __LINE__);
		return (NULL);
	}

	if (acl_strncasecmp(url, "http://", sizeof("http://") - 1) != 0) {
		acl_msg_error("%s(%d): url(%s) invalid", myname, __LINE__, url);
		return (NULL);
	}

	if (strcmp(method, "GET") != 0 && strcmp(method, "POST") != 0
		&& strcmp(method, "HEAD") != 0 && strcmp(method, "CONNECT") != 0)
	{
		acl_msg_error("%s(%d): method(%s) invalid", myname, __LINE__, method);
		return (NULL);
	}

	http_util->hdr_req = http_hdr_req_create(url, method, "HTTP/1.1");
	http_util->hdr_res = http_hdr_res_new();
	http_util->http_res = http_res_new(http_util->hdr_res);
	http_util->req_buf = acl_vstring_alloc(4096);

	host = http_hdr_req_host(http_util->hdr_req);
	if (host) {
		ptr = strchr(host, ':');
		if (ptr == NULL)
			snprintf(http_util->server_addr,
				sizeof(http_util->server_addr), "%s:80", host);
		else
			ACL_SAFE_STRNCPY(http_util->server_addr, host,
				sizeof(http_util->server_addr));
	}

	http_util->conn_timeout = 10;
	http_util->rw_timeout = 10;
	return (http_util);
}

HTTP_UTIL *http_util_res_new(int status)
{
	HTTP_UTIL *http_util = (HTTP_UTIL*) acl_mycalloc(1, sizeof(HTTP_UTIL));

	http_util->hdr_res = http_hdr_res_static(status);
	return (http_util);
}

void http_util_set_req_entry(HTTP_UTIL *http_util, const char *name, const char *value)
{
	if (name && *name && value && *value)
		http_hdr_entry_replace(&http_util->hdr_req->hdr, name, value, 1);
}

void http_util_off_req_entry(HTTP_UTIL *http_util, const char *name)
{
	if (name && *name)
		http_hdr_entry_off(&http_util->hdr_req->hdr, name);
}

char *http_util_get_req_value(HTTP_UTIL *http_util, const char *name)
{
	if (name == NULL || *name == 0)
		return (NULL);
	return (http_hdr_entry_value(&http_util->hdr_req->hdr, name));
}

HTTP_HDR_ENTRY *http_util_get_req_entry(HTTP_UTIL *http_util, const char *name)
{
	if (name == NULL || *name == 0)
		return (NULL);
	return (http_hdr_entry(&http_util->hdr_req->hdr, name));
}

void http_util_set_req_content_length(HTTP_UTIL *http_util, int len)
{
	char  buf[32];

	if (len < 0)
		return;

	snprintf(buf, sizeof(buf), "%d", len);
	http_hdr_entry_replace(&http_util->hdr_req->hdr, "Content-Length", buf, 1);
}

void http_util_set_req_keep_alive(HTTP_UTIL *http_util, int timeout)
{
	char  buf[32];

	snprintf(buf, sizeof(buf), "%d", timeout);
	http_hdr_entry_replace(&http_util->hdr_req->hdr, "Connection", "keep-alive", 1);
	http_hdr_entry_replace(&http_util->hdr_req->hdr, "Keep-Alive", buf, 1);
}

void http_util_set_req_connection(HTTP_UTIL *http_util, const char *value)
{
	if (value == NULL || *value == 0)
		return;
	http_hdr_entry_replace(&http_util->hdr_req->hdr, "Connection", value, 1);
}

void http_util_set_req_refer(HTTP_UTIL *http_util, const char *refer)
{
	if (refer == NULL || *refer == 0)
		return;
	http_hdr_entry_replace(&http_util->hdr_req->hdr, "Referer", refer, 1);
}

void http_util_set_req_cookie(HTTP_UTIL *http_util, const char *name, const char *value)
{
	HTTP_HDR_ENTRY *hdr_entry;
	char *ptr;

	if (name == NULL || *name == 0 || value == NULL)
		return;
	hdr_entry = http_hdr_entry(&http_util->hdr_req->hdr, name);
	if (hdr_entry == NULL) {
		http_hdr_put_str(&http_util->hdr_req->hdr, name, value);
		return;
	}

	ptr = acl_concatenate(hdr_entry->value, "; ", name, "=", value, NULL);
	acl_myfree(hdr_entry->value);
	hdr_entry->value = ptr;
}

void http_util_set_req_proxy(HTTP_UTIL *http_util, const char *proxy)
{
	if (proxy && *proxy)
		ACL_SAFE_STRNCPY(http_util->server_addr, proxy,
			sizeof(http_util->server_addr));
}

void http_util_set_dump_stream(HTTP_UTIL *http_util, ACL_VSTREAM *stream)
{
	const char *myname = "http_util_set_dump_stream";

	if (stream == NULL)
		return;

	if ((http_util->flag & HTTP_UTIL_FLAG_SET_DUMP_FILE)) {
		acl_msg_error("%s(%d): You've called http_util_set_dump_file before!",
			myname, __LINE__);
		return;
	}

	http_util->dump_stream = stream;
	http_util->flag |= HTTP_UTIL_FLAG_SET_DUMP_STREAM;
}

int http_util_set_dump_file(HTTP_UTIL *http_util, const char *filename)
{
	const char *myname = "http_util_set_dump_file";

	if (filename == NULL || *filename == 0) {
		acl_msg_error("%s(%d): filename invalid", myname, __LINE__);
		return (-1);
	}
	if ((http_util->flag & HTTP_UTIL_FLAG_SET_DUMP_STREAM)) {
		acl_msg_error("%s(%d): You've called http_util_set_dump_stream before!",
			myname, __LINE__);
		return (-1);
	}

	http_util->dump_stream = acl_vstream_fopen(filename,
			O_CREAT | O_TRUNC | O_WRONLY, 0600, 4096);
	if (http_util->dump_stream == NULL) {
		acl_msg_error("%s(%d): open dump file(%s) error(%s)",
			myname, __LINE__, filename, acl_last_serror());
		return (-1);
	}
	http_util->flag |= HTTP_UTIL_FLAG_SET_DUMP_FILE;
	return (0);
}

int http_util_req_open(HTTP_UTIL *http_util)
{
	const char *myname = "http_util_req_open";
	int   ret;

	/* 连接远程 http 服务器 */

	http_util->stream = acl_vstream_connect(http_util->server_addr,
			ACL_BLOCKING /* 采用阻塞方式 */,
			http_util->conn_timeout /* 连接超时时间 */,
			http_util->rw_timeout /* 网络 IO 操作超时时间 */,
			4096 /* stream 流缓冲区大小为 4096 字节 */);
	if (http_util->stream == NULL) {
		acl_msg_error("%s(%d): connect %s error(%s)",
			myname, __LINE__, http_util->server_addr,
			acl_last_serror());
		return (-1);
	}

	/* 构建 HTTP 请求头数据 */

	http_hdr_build_request(http_util->hdr_req, http_util->req_buf);

	/* 向 HTTP 服务器发送请求 */

	ret = acl_vstream_writen(http_util->stream,
			acl_vstring_str(http_util->req_buf),
			ACL_VSTRING_LEN(http_util->req_buf));
	if (ret == ACL_VSTREAM_EOF) {
		acl_msg_error("%s(%d): send request to server(%s) error(%s)",
			myname, __LINE__, http_util->server_addr,
			acl_last_serror());
		return (-1);
	}

	return (0);
}

int http_util_put_req_data(HTTP_UTIL *http_util, const char *data, size_t dlen)
{
	const char *myname = "http_util_put_req_data";

	if (data == NULL || dlen == 0) {
		acl_msg_info("%s(%d): data %s, dlen %d invalid",
			myname, __LINE__, data ? "not null" : "null", (int) dlen);
		return (-1);
	}
	if (acl_vstream_writen(http_util->stream, data, dlen) == ACL_VSTREAM_EOF)
		return (-1);
	return ((int) dlen);
}

int http_util_get_res_hdr(HTTP_UTIL *http_util)
{
	const char *myname = "http_util_get_res_hdr";
	int   ret;

	/* 读取 HTTP 服务器响应头*/

	ret = http_hdr_res_get_sync(http_util->hdr_res,
			http_util->stream, http_util->rw_timeout);
	if (ret < 0) {
		acl_msg_error("%s(%d): get respond header error(%s)",
			myname, __LINE__, acl_last_serror());
		return (-1);
	} else if (http_hdr_res_parse(http_util->hdr_res) < 0) {
		acl_msg_error("%s(%d): parse respond header error",
			myname, __LINE__);
		return (-1);
	} else
		return (0);
}

char *http_util_get_res_value(HTTP_UTIL *http_util, const char *name)
{
	if (name == NULL || *name == 0)
		return (NULL);
	return (http_hdr_entry_value(&http_util->hdr_res->hdr, name));
}

HTTP_HDR_ENTRY *http_util_get_res_entry(HTTP_UTIL *http_util, const char *name)
{
	if (name == NULL || *name == 0)
		return (NULL);
	return (http_hdr_entry(&http_util->hdr_res->hdr, name));
}

void http_util_set_res_entry(HTTP_UTIL *http_util, const char *name, const char *value)
{
	if (name == NULL || *name == 0 || value == NULL || *value == 0)
		return;
	http_hdr_entry_replace(&http_util->hdr_res->hdr, name, value, 1);
}

void http_util_off_res_entry(HTTP_UTIL *http_util, const char *name)
{
	if (name == NULL || *name == 0)
		return;
	http_hdr_entry_off(&http_util->hdr_res->hdr, name);
}

int http_util_has_res_body(HTTP_UTIL *http_util)
{
	if (http_util->hdr_res->hdr.content_length == 0
		|| (http_util->hdr_res->hdr.content_length == -1
			&& !http_util->hdr_res->hdr.chunked
			&& http_util->hdr_res->reply_status > 300
			&& http_util->hdr_res->reply_status < 400))
	{
		http_util->flag |= HTTP_UTIL_FLAG_NO_RES_BODY;
		http_util->flag &= ~HTTP_UTIL_FLAG_HAS_RES_BODY;
		return (0);
	} else {
		http_util->flag |= HTTP_UTIL_FLAG_HAS_RES_BODY;
		http_util->flag &= ~HTTP_UTIL_FLAG_NO_RES_BODY;
		return (1);
	}
}

int http_util_get_res_body(HTTP_UTIL *http_util, char *buf, size_t size)
{
	const char *myname = "http_util_get_res_body";
	int   ret;

	if (buf == NULL || size == 0) {
		acl_msg_error("%s(%d): buf(%s), size(%d) invalid",
			myname, __LINE__, buf ? "not null" : "null", (int) size);
		return (-1);
	}

	if ((http_util->flag & (HTTP_UTIL_FLAG_HAS_RES_BODY
		| HTTP_UTIL_FLAG_NO_RES_BODY)) == 0)
	{
		if (!http_util_has_res_body(http_util))
			return (http_util->res_body_dlen);
	}

	ret = (int) http_res_body_get_sync(http_util->http_res,
			http_util->stream, buf, (int) size);
	if (ret <= 0)
		return (ret);
	http_util->res_body_dlen += ret;
	if (http_util->dump_stream == NULL)
		return (ret);

	if (acl_vstream_writen(http_util->dump_stream, buf, ret) == ACL_VSTREAM_EOF)
	{
		/* 如果有一次不能转储数据至文件或流则关闭该功能不再进行转储 */

		acl_msg_error("%s(%d): dump to stream(%s) error(%s)",
			myname, __LINE__, ACL_VSTREAM_PATH(http_util->dump_stream),
			acl_last_serror());
		if ((http_util->flag & HTTP_UTIL_FLAG_SET_DUMP_FILE)) {
			if (http_util->dump_stream)
				acl_vstream_close(http_util->dump_stream);
			http_util->flag &= ~HTTP_UTIL_FLAG_SET_DUMP_FILE;
		} else
			http_util->flag &= ~HTTP_UTIL_FLAG_SET_DUMP_STREAM;

		http_util->dump_stream = NULL;
	}

	return (ret);
}

int http_util_dump_url(const char *url, const char *dump)
{
	const char *myname = "http_util_dump_url";
	HTTP_UTIL *http_util = http_util_req_new(url, "GET");
	char  buf[4096];
	int   ret;

	if (http_util == NULL)
		return (-1);
	if (dump == NULL || *dump == 0) {
		acl_msg_error("%s(%d): dump invalid", myname, __LINE__);
		return (-1);
	}

	if (http_util_set_dump_file(http_util, dump) < 0) {
		acl_msg_error("%s(%d): open dump file(%s) error(%s)",
			myname, __LINE__, dump, acl_last_serror());
		http_util_free(http_util);
		return (-1);
	}

	if (http_util_req_open(http_util) < 0) {
		acl_msg_error("%s(%d): open url(%s) error(%s)",
			myname, __LINE__, url, acl_last_serror());
		http_util_free(http_util);
		return (-1);
	}

	if (http_util_get_res_hdr(http_util) < 0) {
		acl_msg_error("%s(%d): url(%s)'s respond error",
			myname, __LINE__, url);
		http_util_free(http_util);
		return (-1);
	}

	while (1) {
		if (http_util_get_res_body(http_util, buf, sizeof(buf)) <= 0)
			break;
	}

	ret = http_util->res_body_dlen;
	http_util_free(http_util);
	return (ret);
}

int http_util_dump_url_to_stream(const char *url, ACL_VSTREAM *stream)
{
	const char *myname = "http_util_dump_url_to_stream";
	HTTP_UTIL *http_util = http_util_req_new(url, "GET");
	char  buf[4096];
	int   ret;

	if (http_util == NULL)
		return (-1);
	if (stream == NULL) {
		acl_msg_error("%s(%d): dump invalid", myname, __LINE__);
		return (-1);
	}

	http_util_set_dump_stream(http_util, stream);

	if (http_util_req_open(http_util) < 0) {
		acl_msg_error("%s(%d): open url(%s) error(%s)",
			myname, __LINE__, url, acl_last_serror());
		http_util_free(http_util);
		return (-1);
	}

	if (http_util_get_res_hdr(http_util) < 0) {
		acl_msg_error("%s(%d): url(%s)'s respond error",
			myname, __LINE__, url);
		http_util_free(http_util);
		return (-1);
	}

	while (1) {
		if (http_util_get_res_body(http_util, buf, sizeof(buf)) <= 0)
			break;
	}

	ret = http_util->res_body_dlen;
	http_util_free(http_util);
	return (ret);
}
