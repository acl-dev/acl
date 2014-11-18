#include "lib_acl.h"
#include "lib_protocol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "probe.h"

ACL_ARRAY *var_probe_server_link = NULL;
const char *var_probe_logfile = NULL;
int   var_probe_loglevel = 0;
int   var_probe_statlen = 1024;
int   var_probe_fork_sleep = 1;
ACL_VSTREAM *var_probe_debug_fp = NULL;

static ACL_XINETD_CFG_PARSER *__cfg_parser = NULL;

static char request_format[] = "\
GET %s HTTP/1.1\r\n\
Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, */*\r\n\
Accept-Language: zh-cn\r\n\
User-Agent: Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.0; .NET CLR 1.1.4322)\r\n\
Accept-Encoding: gzip, deflate\r\n\
Host: %s:%d\r\n\r\n";

static int  __parse_url(const char *url, char *host_buf, int hlen,
	int *http_port, char *url_buf, int ulen)
{
	/* url format: http://www.hexun.com[:80]/index.html */
	char *ptr, *url_begin;
	char  buf[1024];
	size_t n;

	snprintf(buf, sizeof(buf) - 1, "%s", url);
	ptr = buf;
	while (*ptr == ' ' || *ptr == '\t')
		ptr++;

	n = strlen("http://");

	/* 是否标准的 http 协议请求格式 */
	if (strncasecmp(ptr, "http://", n) == 0)
		ptr += n;

	if (*ptr == 0)
		return -1;

	url_begin = ptr;
	ptr = strchr(url_begin, '/');
	if (ptr) {
		*ptr++ = 0;
		if (*ptr == 0)
			strcpy(url_buf, "/");
		else
			snprintf(url_buf, ulen, "/%s", ptr);
	} else
		strcpy(url_buf, "/");

	/* 获得 http 协议端口号 */
	if (http_port) {
		ptr = strchr(url_begin, ':');
		if (ptr) {
			*ptr++ = 0;
			n = atoi(ptr);
			if (n <= 0)
				*http_port = 80;
			else
				*http_port = (int) n;
		} else
			*http_port = 80;
	}

	snprintf(host_buf, hlen, "%s", url_begin);
	return 0;
}

static void __build_http_request(PROBE_SERVER *server)
{
	const char *myname = "__build_http_request";
	char  host_refer[256], url_request[256];
	int   port_refer = 80;
	int   ret;

	memset(host_refer, 0, sizeof(host_refer));
	memset(url_request, 0, sizeof(url_request));

	ret = __parse_url(server->url, host_refer, sizeof(host_refer),
			&port_refer, url_request, sizeof(url_request));
	if (ret < 0)
		acl_msg_fatal("%s(%d), %s: invalid url=[%s]",
			__FILE__, __LINE__, myname, server->url);

	snprintf(server->http_request_header,
		sizeof(server->http_request_header) - 1, request_format,
			url_request, host_refer, port_refer);

	server->http_request_len = (int) strlen(server->http_request_header);
}

static void __set_name_value(char *data, PROBE_SERVER *server)
{
	/* data: name=value */
	const char *myname = "__set_name_value";
	char *pname, *pvalue, *ptr;

	pname = data;
	/* skip ' ' and '\t' */
	while (*pname == ' ' || *pname == '\t')
		pname++;
	if (pname == 0) {
		acl_msg_error("%s(%d), %s: invalid data=[%s]",
			__FILE__, __LINE__, myname, data);
		return;
	}

	pvalue = pname;
	/* find '=' */
	while (*pvalue) {
		if (*pvalue == '=') {
			*pvalue++ = 0;
			break;
		}
		pvalue++;
	}

	/* skip ' ' and '\t' */
	while (*pvalue == ' ' || *pvalue == '\t')
		pvalue++;
	if (pvalue == 0) {
		acl_msg_error("%s(%d)->%s: invalid data=[%s]",
			__FILE__, __LINE__, myname, data);
		return;
	}

	ptr = pvalue;
	while (*ptr) {
		if (*ptr == ' ' || *ptr == '\t' || *ptr == ',') {
			*ptr = 0;
			break;
		}
		ptr++;
	}

	if (strcasecmp(pname, VAR_CFG_SERVER_NAME) == 0)
		server->name = acl_mystrdup(pvalue);
	else if (strcasecmp(pname, VAR_CFG_SERVER_ADDR) == 0)
		server->addr = acl_mystrdup(pvalue);
	else if (strcasecmp(pname, VAR_CFG_SERVER_URL) == 0)
		server->url = acl_mystrdup(pvalue);
	else if (strcasecmp(pname, VAR_CFG_SERVER_CONNECT_TIMEOUT) == 0)
		server->connect_timeout = atoi(pvalue);
	else if (strcasecmp(pname, VAR_CFG_SERVER_RW_TIMEOUT) == 0)
		server->rw_timeout = atoi(pvalue);
	else if (strcasecmp(pname, VAR_CFG_SERVER_RETRY_INTER) == 0)
		server->retry_inter = atoi(pvalue);
	else if (strcasecmp(pname, VAR_CFG_SERVER_PROBE_INTER) == 0)
		server->probe_inter = atoi(pvalue);
	else if (strcasecmp(pname, VAR_CFG_SERVER_HTTP_ERRORS) == 0)
		server->http_status_errors = acl_mystrdup(pvalue);
	else if (strcasecmp(pname, VAR_CFG_SERVER_LOG) == 0)
		server->logfile = acl_mystrdup(pvalue);
	else if (strcasecmp(pname, VAR_CFG_SERVER_WARN_TIME) == 0)
		server->warn_time = atoi(pvalue);

	/* only for test */
	printf("name(%s)=value(%s), probe_inter=%d\n",
		pname, pvalue, server->probe_inter);
}

static PROBE_SERVER *__cfg_server_new(char *data)
{
/*
 * data: server_name="博客2", addr=10.0.90.246:80, url=http://blog.hexun.com/index.aspx, connect_timeout=60, rw_timeout=60, retry_inter=5, probe_inter=5
 */
	const char *myname = "__cfg_server_new";
	PROBE_SERVER *server = NULL;
	char *ptr;

	if (data == NULL || *data == 0) {
		acl_msg_error("%s: invalid data input", myname);
		return NULL;
	}

	server = (PROBE_SERVER *) acl_mycalloc(1, sizeof(PROBE_SERVER));
	if (server == NULL)
		acl_msg_fatal("%s: can't calloc, serr=%s",
			myname, strerror(errno));

	while (1) {
		ptr = acl_mystrtok(&data, ",");
		if (ptr == NULL)
			break;

		__set_name_value(ptr, server);
	}

	__build_http_request(server);
	if (server->logfile != NULL) {
		server->logfp = acl_vstream_fopen(server->logfile,
			O_WRONLY | O_CREAT | O_APPEND, 0600, 4096);
		if (server->logfp == NULL)
			acl_msg_fatal("%s: open logfile(%s) error %s ",
				myname, server->logfile, strerror(errno));
	} else
		server->logfp = NULL;

	return server;
}

static void __cfg_parse_one_server(const char *server_data)
{
	const char *myname = "__cfg_parse_one_server";
	char *buf = NULL;
	char *begin, *end;
	PROBE_SERVER *server;

#undef	RETURN
#define	RETURN do { \
	if (buf) \
		acl_myfree(buf); \
	return; \
} while(0)

	if (server_data == NULL || *server_data == 0)
		acl_msg_fatal("%s(%d)->%s: input invalid",
			__FILE__, __LINE__, myname);

	buf = acl_mystrdup(server_data);

	begin = buf;
	while (*begin) {
		if (*begin == '<') {
			*begin++ = 0;
			break;
		}
		begin++;
	}

	if (*begin == 0)
		RETURN;
	end = begin;
	while (*end) {
		if (*end == '>') {
			*end = 0;
			break;
		}
		end++;
	}

	server = __cfg_server_new(begin);
	if (server == NULL)
		acl_msg_fatal("%s(%d)->%s: invalid data=[%s]",
			__FILE__, __LINE__, myname, server_data);
	else
		acl_array_append(var_probe_server_link, (void *) server);

	acl_msg_info("%s(%d)->%s: name=%s, addr=%s, url=%s, connect_timeout=%d,"
		" rw_timeout=%d, retry_inter=%d, probe_inter=%d\n",
		__FILE__, __LINE__, myname, server->name, server->addr,
		server->url, server->connect_timeout, server->rw_timeout,
		server->retry_inter, server->probe_inter);

	RETURN;
}

static void __cfg_server_parse(const ACL_ARRAY *a)
{
	int   i, n;

	if (a == NULL)
		return;

	n = acl_array_size(a);

#if 0
	for (i = 0; i < n; i++)
		acl_msg_info("%s: %s", VAR_CFG_SERVER, (const char *) acl_array_index(a, i));
#endif

	for (i = 0; i < n; i++)
		__cfg_parse_one_server((const char *) acl_array_index(a, i));
}

void probe_cfg_load(void)
{
	const char *myname = "probe_cfg_load";
	ACL_XINETD_CFG_PARSER *xcp;
	const ACL_ARRAY *servers_array;
	const char *ptr;

	if (var_cfg_file == NULL || *var_cfg_file == 0)
		acl_msg_fatal("%s(%d)->%s: __cfg_file null",
			__FILE__, __LINE__, myname);

	xcp = acl_xinetd_cfg_load(var_cfg_file);
	if (xcp == NULL) {
		acl_msg_error("%s(%d)->%s: xinetd_cfg_load return null, file=%s",
			__FILE__, __LINE__, myname, var_cfg_file);
		exit (1);
	}
	__cfg_parser = xcp;

	var_probe_server_link = acl_array_create(50);
	if (var_probe_server_link == NULL)
		acl_msg_fatal("%s(%d)->%s: acl_array_create error, serr=%s",
			__FILE__, __LINE__, myname, strerror(errno));

	servers_array = acl_xinetd_cfg_get_ex(xcp, VAR_CFG_SERVER);
	if (servers_array == NULL)
		acl_msg_fatal("%s(%d)->%s: not fine %s line",
			__FILE__, __LINE__, myname, VAR_CFG_SERVER);

	__cfg_server_parse(servers_array);

	var_probe_logfile = acl_xinetd_cfg_get(xcp, VAR_CFG_LOGFILE);
	if (var_probe_logfile == NULL || *var_probe_logfile == 0)
		acl_msg_warn("%s(%d)->%s: null var_probe_logfile",
			__FILE__, __LINE__, myname);
	ptr = acl_xinetd_cfg_get(xcp, VAR_CFG_LOGLEVEL);
	if (ptr && *ptr)
		var_probe_loglevel = atoi(ptr);
	if (var_probe_loglevel < 0)
		var_probe_loglevel = 0;
	ptr = acl_xinetd_cfg_get(xcp, VAR_CFG_STATLEN);
	if (ptr && *ptr)
		var_probe_statlen = atoi(ptr);
	if (var_probe_statlen <= 0)
		var_probe_statlen = 1024;

	ptr = acl_xinetd_cfg_get(xcp, VAR_CFG_FORK_SLEEP);
	if (ptr && *ptr)
		var_probe_fork_sleep = atoi(ptr);
	if (var_probe_fork_sleep < 0)
		var_probe_fork_sleep = 1;

	ptr = acl_xinetd_cfg_get(xcp, VAR_CFG_DEBUG_FILE);
	if (ptr && *ptr)
		var_probe_debug_fp = acl_vstream_fopen(ptr,
			O_WRONLY | O_CREAT | O_APPEND, 0600, 4096);
	else
		var_probe_debug_fp = NULL;
}
