// http_aio.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <assert.h>

typedef struct {
	ACL_ASTREAM  *stream;
	HTTP_HDR_RES *hdr_res;
	HTTP_RES     *http_res;
} CTX;

static char __server_addr[128];
static bool __stop = false;

static int on_close(ACL_ASTREAM *stream acl_unused, void *context)
{
	CTX *ctx = (CTX *) context;

	http_res_free(ctx->http_res);
	acl_myfree(ctx);

	printf("%s(%d)\n", __FUNCTION__, __LINE__);
	__stop = true;
	return 0;
}

static int on_timeout(ACL_ASTREAM *stream acl_unused, void *context acl_unused)
{
	printf("%s(%d)\n", __FUNCTION__, __LINE__);
	return 0;
}

static int read_respond_body_ready(int status, char *data, int dlen,
	void *arg acl_unused)
{
	ACL_VSTRING *buf = acl_vstring_alloc(256);

	acl_vstring_memcpy(buf, data, dlen);
	ACL_VSTRING_TERMINATE(buf);
	printf("status=%d >>%s", status, acl_vstring_str(buf));
	acl_vstring_free(buf);

	return 0;
}

static int on_http_hdr(int status, void *arg)
{
	CTX *ctx = (CTX*) arg;

	printf("%s: status=%d\r\n", __FUNCTION__, status);

	http_hdr_print(&ctx->hdr_res->hdr, "---respond---");

	// 构建 HTTP 响应体
	ctx->http_res = http_res_new(ctx->hdr_res);

	// 异步读取 HTTP 服务端的响应体
	http_res_body_get_async(ctx->http_res, ctx->stream,
		read_respond_body_ready, ctx, 0);

	return 0;
}

static int on_connect(ACL_ASTREAM *stream, void *context)
{
	const char *addr = (const char*) context;

	if (stream == NULL) {
		int err = acl_last_error();
		printf("connect %s failed, errno=%d, %s\r\n", addr, err,
			err < 0 ? acl_dns_serror(err) : acl_last_serror());
		__stop = true;
		return -1;
	}

	printf(">>>> connect %s ok!\r\n", addr);

	char request[1024];
	snprintf(request, sizeof(request),
		"GET / HTTP/1.1\r\n"
		"HOST: %s\r\n"
		"Connection: close\r\n"
		"\r\n", __server_addr);

	CTX *ctx = (CTX*) acl_mycalloc(1, sizeof(CTX));
	ctx->stream = stream;

	// 设置读超时、关闭回调函数
	acl_aio_add_timeo_hook(stream, on_timeout, ctx);
	acl_aio_add_close_hook(stream, on_close, ctx);

	printf("%s(%d)\n", __FUNCTION__, __LINE__);

	// 发送 HTTP 请求
	acl_aio_writen(stream, request, (int) strlen(request));

	// 创建 HTTP 响应对象
	ctx->hdr_res = http_hdr_res_new();

	// 异步读取服务端响应的 HTTP 头
	http_hdr_res_get_async(ctx->hdr_res, stream, on_http_hdr, ctx, 0);

	return 0;
}

static void usage(const char *procname)
{
	printf("usage: %s -h[help]\r\n"
		" -s server_addr\r\n"
		" -N name_server\r\n"
		" -t connect_timeout\r\n"
		" -T dns_timeout\r\n",
		procname);
}

int main(int argc, char* argv[])
{
	ACL_AIO *aio = acl_aio_create(ACL_EVENT_SELECT);
	char name_server[128];
	int ch, connect_timeout = 5, dns_timeout = 1;

	acl_aio_set_keep_read(aio, 1);

	snprintf(__server_addr, sizeof(__server_addr), "www.baidu.com:80");
	snprintf(name_server, sizeof(name_server), "8.8.8.8:53");

	while ((ch = getopt(argc, argv, "hs:N:t:T:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			snprintf(__server_addr, sizeof(__server_addr), "%s", optarg);
			break;
		case 'N':
			snprintf(name_server, sizeof(name_server), "%s", optarg);
			break;
		case 't':
			connect_timeout = atoi(optarg);
			break;
		case 'T':
			dns_timeout = atoi(optarg);
			break;
		default:
			usage(argv[0]);
			return 1;
		}
	}

	// 设置域名解析服务器地址
	acl_aio_set_dns(aio, name_server, dns_timeout);

	// 异步连接指定 WEB 服务器
	if (acl_aio_connect_addr(aio, __server_addr, connect_timeout,
		on_connect, __server_addr) == -1) {

		printf("connect %s error\r\n", __server_addr);
		return 1;
	}

	// 异步事件循环
	while (!__stop) {
		acl_aio_loop(aio);
	}

	acl_aio_free(aio);
	return 0;
}

