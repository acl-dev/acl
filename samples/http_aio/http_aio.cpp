// http_aio.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "lib_acl.h"
#include "lib_protocol.h"
#include <assert.h>

typedef struct {
	ACL_ASTREAM *stream;
	HTTP_HDR_RES *hdr_res;
	HTTP_RES *http_res;
} CTX;

int  On_httphdr_notify_cb(int status, void *arg);

static int On_close_cb(ACL_ASTREAM *stream, void *context)
{
	printf("%s(%d)\n", __FUNCTION__, __LINE__);
	return (0);
}

static int On_timeout_cb(ACL_ASTREAM *stream, void *context)
{
	printf("%s(%d)\n", __FUNCTION__, __LINE__);
	return (0);
}

static int On_connect_cb(ACL_ASTREAM *stream, void *context)
{
	const char *request = "GET / HTTP/1.1\r\n"
		"HOST: www.sina.com.cn\r\n"
		"\r\n";
	CTX *ctx = (CTX*) context;

	printf("%s(%d)\n", __FUNCTION__, __LINE__);
	acl_aio_writen(stream, request, (int) strlen(request));
	ctx->hdr_res = http_hdr_res_new();
	http_hdr_res_get_async(ctx->hdr_res, stream, On_httphdr_notify_cb, ctx, 0);
	return (0);
}

static int read_respond_body_ready(int status, char *data, int dlen, void *arg)
{
	static ACL_VSTRING *_buf;

	if (_buf == NULL)
		_buf = acl_vstring_alloc(256);
	acl_vstring_memcpy(_buf, data, dlen);
	ACL_VSTRING_TERMINATE(_buf);
	//printf(">>%s", acl_vstring_str(_buf));
	return (0);
}

int  On_httphdr_notify_cb(int status, void *arg)
{
	CTX *ctx = (CTX*) arg;
	ACL_ASTREAM *stream = ctx->stream;

	http_hdr_print(&ctx->hdr_res->hdr, "---respond---");
	ctx->http_res = http_res_new(ctx->hdr_res);
	http_res_body_get_async(ctx->http_res,
		ctx->stream,
		read_respond_body_ready,
		ctx,
		0);
	//acl_aio_iocp_close(stream);
	return (0);
}

static int On_write_cb(ACL_ASTREAM *stream, void *context)
{
	CTX *ctx = (CTX*) context;
	//ctx->hdr_res = http_hdr_res_new();
	printf("%s(%d)\n", __FUNCTION__, __LINE__);
//	http_hdr_res_get_async(ctx->hdr_res, stream, On_httphdr_notify_cb, ctx, 0);
	return (0);
}

int _tmain(int argc, _TCHAR* argv[])
{
	ACL_AIO *aio = acl_aio_create(ACL_EVENT_SELECT);
	ACL_ASTREAM *stream;
	const char *addr = "218.30.108.184:80";
	CTX *ctx = (CTX*) acl_mycalloc(1, sizeof(CTX));

	acl_init();
	stream = acl_aio_connect(aio, addr, 0);
	assert(stream);
	ctx->stream = stream;
	acl_aio_ctl(stream,
		ACL_AIO_CTL_CONNECT_HOOK_ADD, On_connect_cb, ctx,
		ACL_AIO_CTL_CLOSE_HOOK_ADD, On_close_cb, ctx,
		ACL_AIO_CTL_TIMEO_HOOK_ADD, On_timeout_cb, ctx,
		ACL_AIO_CTL_WRITE_HOOK_ADD, On_write_cb, ctx,
		ACL_AIO_CTL_END);
	while (1) {
		acl_aio_loop(aio);
	}

	return 0;
}

