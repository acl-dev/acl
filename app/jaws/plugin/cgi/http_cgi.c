#include "lib_acl.h"
#include "http_plugin.h"

typedef struct {
	ACL_VSTRING *alias;
	ACL_VSTRING *cgi;
} CGI_MAP;

static ACL_HTABLE *__table = NULL;

static CGI_MAP *cgi_map_new(const char *alias, const char *cgi)
{
	CGI_MAP *map = (CGI_MAP*) acl_mycalloc(1, sizeof(CGI_MAP));

	map->alias = acl_vstring_alloc(256);
	map->cgi = acl_vstring_alloc(256);
	acl_vstring_strcpy(map->alias, alias);
	acl_vstring_strcpy(map->cgi, var_cfg_cgi_bin);
	acl_vstring_strcpy(map->cgi, cgi);
	return (map);
}

static void cgi_map_free(CGI_MAP *map)
{
	acl_vstring_free(map->alias);
	acl_vstring_free(map->cgi);
	acl_myfree(map);
}

void http_cgi_init()
{
	__table = acl_htable_create(10, 0);
}

void http_cgi_end()
{
	acl_htable_free(__table, (void (*)(void*)) cgi_map_free);
}

void http_cgi_add2(const char *alias, const char *cgi)
{
	CGI_MAP *map;

	map = (CGI_MAP*) acl_htable_find(__table, alias);
	if (map != NULL) {
		acl_msg_warn("%s(%d): alias(%s) already exist",
			__FUNCTION__, __LINE__, alias);
		return;
	}
	map = cgi_map_new(alias, cgi);
	acl_htable_enter(__table, alias, map);
}

void http_cgi_add1(const ACL_ARGV *args)
{
	if (args->argc < 2) {
		acl_msg_error("%s(%d): argc(%d) < 2",
			__FUNCTION__, __LINE__, args->argc);
		return;
	}

	http_cgi_add2(args->argv[0], args->argv[1]);
}

CGI *http_cgi_path(const char *alias)
{
	CGI *cgi;
	CGI_MAP *map = (CGI_MAP*) acl_htable_find(__table, alias);

	if (map == NULL)
		return (NULL);

	cgi = (CGI*) acl_mycalloc(1, sizeof(CGI));
	cgi->alias = acl_mystrdup(alias);
	cgi->cgi = acl_mystrdup(STR(map->cgi));
	return (cgi);
}

static void cgi_free(CGI *cgi)
{
	if (cgi == NULL)
		return;
	if (cgi->client)
		acl_vstream_close(cgi->client);
	if (cgi->hdr_req)
		http_hdr_req_free(cgi->hdr_req);
	if (cgi->req) {
		cgi->req->hdr_req = NULL;
		http_req_free(cgi->req);
	}
	if (cgi->hdr_res)
		http_hdr_res_free(cgi->hdr_res);
	if (cgi->cgi)
		acl_myfree(cgi->cgi);
	if (cgi->alias)
		acl_myfree(cgi->alias);
	acl_myfree(cgi);
}

static void run_cgi(CGI *cgi, ACL_ARGV *env)
{
	char *command = cgi->cgi;
	ACL_VSTREAM *stream;
	ACL_VSTRING *vbuf;
	ACL_ARGV *cgi_hdrs;
	char  buf[4096];
	int   ret, errnum = 200;
	char **cpp;

	/* 打开与CGI程序的管道数据流 */
	stream = acl_vstream_popen(O_RDWR,
			ACL_VSTREAM_POPEN_COMMAND, command,
			ACL_VSTREAM_POPEN_ENV, env->argv,
			ACL_VSTREAM_POPEN_END);

	if (stream == NULL) {
		acl_msg_error("%s(%d): open command(%s) error(%s)",
			__FUNCTION__, __LINE__, command, acl_last_serror());
		return;
	}

	cgi->req = http_req_new(cgi->hdr_req);
	if (cgi->hdr_req->hdr.content_length > 0) {
		while (1) {
			ret = (int) http_req_body_get_sync(cgi->req, cgi->client,
					buf, sizeof(buf) - 1);
			if (ret <= 0)
				break;
			if (acl_vstream_writen(stream, buf, ret) == ACL_VSTREAM_EOF) {
				acl_vstream_close(stream);
				return;
			}
		}
	}

	/*
	 * http_hdr_set_keepalive(cgi->hdr_req, cgi->hdr_res);
	 */

	cgi_hdrs = acl_argv_alloc(10);

	/* 读CGI发送的响应头 */
	while (1) {
		char *pname, *ptr;

		ret = acl_vstream_gets_nonl(stream, buf, sizeof(buf));
		if (ret == ACL_VSTREAM_EOF) {
			acl_msg_error("%s(%d): cgi exit exception",
				__FUNCTION__, __LINE__);
			errnum = 500;
			break;
		}
		if (ret == 0) {
			break;
		}
		ptr = buf;
		pname = acl_mystrtok(&ptr, ":\t ");
		if (ptr == NULL) {
			acl_msg_warn("%s(%d): cgi reply hdr(%s) invalid",
				__FUNCTION__, __LINE__, buf);
			continue;
		}
		acl_argv_add(cgi_hdrs, pname, ptr, NULL);
	}

	vbuf = acl_vstring_alloc(256);

	cgi->hdr_res = http_hdr_res_static(errnum);

	for (cpp = cgi_hdrs->argv; *cpp; cpp += 2) {
		http_hdr_put_str(&cgi->hdr_res->hdr, cpp[0], cpp[1]);
	}

	http_hdr_entry_replace(&cgi->hdr_res->hdr, "Connection", "close", 1);
	http_hdr_build(&cgi->hdr_res->hdr, vbuf);

	ret = acl_vstream_writen(cgi->client, STR(vbuf), LEN(vbuf));
	if (ret > 0) {
		/* 读CGI的响应数据体并发送至浏览器 */
		while (1) {
			ret = acl_vstream_read(stream, buf, sizeof(buf));
			if (ret == ACL_VSTREAM_EOF)
				break;
			if (acl_vstream_writen(cgi->client, buf, ret) == ACL_VSTREAM_EOF)
				break;
		}
	}

	acl_vstring_free(vbuf);
	acl_vstream_pclose(stream);
}

void http_cgi_thread(void *ctx)
{
	CGI *cgi = (CGI*) ctx;
	HTTP_HDR_REQ *hdr_req = cgi->hdr_req;
	ACL_ARGV *env = acl_argv_alloc(10);
	ACL_ITER iter;

	const char *reply = "HTTP/1.1 200 OK\r\n"
		"Date: Sun, 15 Nov 2009 07:06:22 GMT\r\n"
		"Server: Apache/2.2.9 (Unix)\r\n"
		"Connection: close\r\n"
		"Content-Type: text/html\r\n\r\n";
	acl_foreach(iter, hdr_req->hdr.entry_lnk) {
		HTTP_HDR_ENTRY *entry = (HTTP_HDR_ENTRY*) iter.data;

		if (entry->off)
			continue;
		acl_argv_add(env, entry->name, entry->value, NULL);
	}

	if (1)
		run_cgi(cgi, env);
	else {
		acl_vstream_writen(cgi->client, reply, sizeof(reply));
	}
	acl_argv_free(env);
	cgi_free(cgi);
}
