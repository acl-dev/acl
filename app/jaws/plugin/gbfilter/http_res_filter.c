#include "lib_acl.h"
#include <string.h>
#include "lib_protocol.h"
#include "http_plugin.h"

typedef struct {
	ACL_VSTREAM *client;
	ACL_VSTREAM *server;
	HTTP_HDR_REQ *hdr_req;
	HTTP_HDR_RES *hdr_res;
	HTTP_RES *res;
	void *ctx;
} FWD_RES;

int http_respond_filter(ACL_VSTREAM *client acl_unused, ACL_VSTREAM *server acl_unused,
	HTTP_HDR_REQ *hdr_req acl_unused, HTTP_HDR_RES *hdr_res,
	void **ctx_ptr acl_unused)
{
	char *ptr;

	if (var_cfg_data_clone)
		return (0);

	ptr = http_hdr_entry_value(&hdr_res->hdr, "Content-Encoding");
	if (ptr) {
		if (acl_strcasestr(ptr, "gzip") != 0)
			acl_msg_info("%s(%d)", __FUNCTION__, __LINE__);
		else if (acl_strcasestr(ptr, "deflate") != 0)
			return (0);
	}

	ptr = http_hdr_entry_value(&hdr_res->hdr, "Content-Type");
	if (ptr == NULL)
		return (0);

	/* acl_strcasestr(ptr, "javascript") == 0) */
	if (acl_strcasestr(ptr, "text/html") == 0)
		return (0);

	return (1);
}

static void forward_free(FWD_RES *fwd)
{
	acl_vstream_close(fwd->client);
	acl_vstream_close(fwd->server);
	http_hdr_req_free(fwd->hdr_req);
	http_hdr_res_free(fwd->hdr_res);
	if (fwd->res) {
		fwd->res->hdr_res = NULL;  /* 防止 hdr_res 被重复释放 */
		http_res_free(fwd->res);
	}
	acl_myfree(fwd);
}

static void forward_thread(void *ctx)
{
	FWD_RES *fwd = (FWD_RES*) ctx;
	ACL_VSTRING *vbuf = acl_vstring_alloc(8192);
	int   ret;
	char  buf[8192];

	http_hdr_entry_off(&fwd->hdr_res->hdr, "Content-Length");
	http_hdr_entry_off(&fwd->hdr_res->hdr, "Transfer-Encoding");

	http_hdr_build(&fwd->hdr_res->hdr, vbuf);
	ret = acl_vstream_writen(fwd->client, STR(vbuf), LEN(vbuf));
	if (ret == ACL_VSTREAM_EOF) {
		acl_debug(DBG_RES, 2) ("%s(%d): write hdr to client error(%s)",
			__FUNCTION__, __LINE__, acl_last_serror());
		forward_free(fwd);
		acl_vstring_free(vbuf);
		return;
	}

	ACL_VSTRING_RESET(vbuf);
	fwd->res = http_res_new(fwd->hdr_res);
	while (1) {
		ret = (int) http_res_body_get_sync(fwd->res, fwd->server,
				buf, sizeof(buf) - 1);
		if (ret <= 0)
			break;
		acl_vstring_strncat(vbuf, buf, ret);

		ret = acl_vstream_writen(fwd->client, buf, ret);
		if (ret == ACL_VSTREAM_EOF)
			acl_debug(DBG_RES, 2) ("%s(%d): write to client error(%s)",
				__FUNCTION__, __LINE__, acl_last_serror());
	}

	forward_free(fwd);
	acl_vstring_free(vbuf);
}

static void forward_rewrite_thread(void *ctx)
{
	FWD_RES *fwd = (FWD_RES*) ctx;
	ACL_VSTRING *vbuf = acl_vstring_alloc(102400);
	int   ret;
	char  buf[8192];

	http_hdr_entry_off(&fwd->hdr_res->hdr, "Content-Length");
	http_hdr_entry_off(&fwd->hdr_res->hdr, "Transfer-Encoding");

	http_hdr_build(&fwd->hdr_res->hdr, vbuf);
	ret = acl_vstream_writen(fwd->client, STR(vbuf), LEN(vbuf));
	if (ret == ACL_VSTREAM_EOF) {
		acl_debug(DBG_RES, 2) ("%s(%d): write hdr to client error(%s)",
			__FUNCTION__, __LINE__, acl_last_serror());
		forward_free(fwd);
		acl_vstring_free(vbuf);
		return;
	}

	ACL_VSTRING_RESET(vbuf);
	fwd->res = http_res_new(fwd->hdr_res);

	while (1) {
		ret = (int) http_res_body_get_sync(fwd->res, fwd->server,
				buf, sizeof(buf) - 1);
		if (ret <= 0)
			break;
		acl_vstring_strncat(vbuf, buf, ret);
	}

	ACL_VSTRING_TERMINATE(vbuf);

	/* 简体转繁体 */
	acl_gbjt2ft(STR(vbuf), LEN(vbuf), STR(vbuf), LEN(vbuf));

	ret = acl_vstream_writen(fwd->client, STR(vbuf), LEN(vbuf));
	if (ret == ACL_VSTREAM_EOF)
		acl_debug(DBG_RES, 2) ("%s(%d): write to client error(%s)",
			__FUNCTION__, __LINE__, acl_last_serror());

	forward_free(fwd);
	acl_vstring_free(vbuf);
}

void http_respond_forward(ACL_VSTREAM *client, ACL_VSTREAM *server,
	HTTP_HDR_REQ *hdr_req, HTTP_HDR_RES *hdr_res, void *ctx)
{
	FWD_RES *fwd = (FWD_RES*) acl_mycalloc(1, sizeof(FWD_RES));

	fwd->client = client;
	fwd->server = server;
	fwd->hdr_req = hdr_req;
	fwd->hdr_res = hdr_res;
	fwd->ctx = ctx;

	if (var_cfg_rewrite_enable)
		http_plugin_pool_append(forward_rewrite_thread, fwd);
	else
		http_plugin_pool_append(forward_thread, fwd);
}

char *http_respond_dat_filter(const char *data, int dlen,
                int *ret, int *stop, void *ctx acl_unused)
{
	const char *filename = var_cfg_log_name;
	static ACL_FILE *fp = NULL;
	int   n;

	if (!var_cfg_data_clone) {
		*stop = 1;
		*ret = dlen;
		return ((char*) data);
	}

	if (fp == NULL) {
		fp = acl_fopen(filename, "a");
		if (fp == NULL) {
			acl_msg_error("open %s error(%s)",
				filename, acl_last_serror());
			*stop = 1;
			*ret = dlen;
			return ((char*) data);
		}
	}

	n = (int) acl_fwrite(data, dlen, 1, fp);
	if (n == EOF) {
		acl_msg_error("write to %s error(%s)",
			filename, acl_last_serror());
		*stop = 1;
		return ((char*) data);
	}

	*stop = 0;
	*ret = dlen;
	return ((char*) data);
}

void http_respond_dat_free(void *buf, void *ctx acl_unused)
{
	if (!var_cfg_data_clone)
		return;
	acl_myfree(buf);
}
