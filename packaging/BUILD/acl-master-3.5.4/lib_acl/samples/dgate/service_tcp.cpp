#include "stdafx.h"
#include "service_main.h"

static int respond_data_callback(ACL_ASTREAM *astream acl_unused,
	void *context, const char *data, int len)
{
	const char *myname = "respond_data_callback";
	SERVICE_CTX *respond_ctx = (SERVICE_CTX*) context;
	SERVICE_CTX *request_ctx;
	struct iovec chunks[2];
	unsigned short n;
	int   dlen;
	char  buf[MAX_BUF];

	request_ctx = service_ctx_find(respond_ctx->service,
		SERVICE_CTX_TCP_REQUEST, respond_ctx->id);
	if (request_ctx == NULL) {
		acl_msg_error("%s(%d): request client(%d) exit now",
			myname, __LINE__, respond_ctx->id);
		return (-1);
	}

	n = htons(len);
	dlen = len > MAX_BUF ? MAX_BUF : len;
	memcpy(buf, data, dlen);
	chunks[0].iov_base = &n;
	chunks[0].iov_len = 2;
	chunks[1].iov_base = buf;
	chunks[1].iov_len = dlen;
	acl_aio_writev(request_ctx->stream, chunks, 2);
	acl_aio_iocp_close(request_ctx->stream);
	return (-1);
}

static int respond_len_callback(ACL_ASTREAM *astream, void *context,
	char *data, int len acl_unused)
{
	const char *myname = "respond_len_callback";
	unsigned short n;

	n = ntohs(*(const unsigned short *) data);
	if (n <= 0) {
		acl_msg_error("%s(%d): n(%d) invalid", myname, __LINE__, n);
		return (-1);
	}
	if (n > MAX_BUF) {
		acl_msg_error("%s(%d): n(%d) too long", myname, __LINE__, n);
		return (-1);
	}
	acl_aio_ctl(astream,
		ACL_AIO_CTL_READ_HOOK_ADD, respond_data_callback, context,
		ACL_AIO_CTL_END);
	acl_aio_readn(astream, n);
	return (0);
}

static int write_request_callback(ACL_ASTREAM *astream, void *context)
{
	//SERVICE_CTX *ctx = (SERVICE_CTX*) context;
	acl_aio_ctl(astream,
		ACL_AIO_CTL_READ_HOOK_ADD, respond_len_callback, context,
		ACL_AIO_CTL_END);
	acl_aio_readn(astream, 2);
	return (0);
}

static int respond_connect_callback(ACL_ASTREAM *astream, void *context)
{
	SERVICE_CTX *ctx = (SERVICE_CTX*) context;
	unsigned short len;
	struct iovec chunks[2];

	acl_aio_ctl(astream,
		ACL_AIO_CTL_WRITE_HOOK_ADD, write_request_callback, context,
		ACL_AIO_CTL_END);
	len = htons(ctx->request_len);
	chunks[0].iov_base = &len;
	chunks[0].iov_len = 2;
	chunks[1].iov_base = ctx->request_buf;
	chunks[1].iov_len = ctx->request_len;
	acl_aio_writev(astream, chunks, 2);
	return (0);
}

static int respond_close_callback(ACL_ASTREAM *astream acl_unused, void *context)
{
	SERVICE_CTX *ctx = (SERVICE_CTX*) context;

	service_ctx_free(ctx);
	return (-1);
}

static int request_data_callback(ACL_ASTREAM *astream acl_unused, void *context,
	char *data, int len)
{
	const char *myname = "request_data_callback";
	SERVICE_CTX *request_ctx = (SERVICE_CTX*) context, *respond_ctx;
	SERVICE *service = request_ctx->service;
	ACL_ASTREAM *server;

	server = acl_aio_connect(service->aio, service->dns_addr,
		service->conn_timeout);
	if (server == NULL) {
		acl_msg_error("%s(%d): connect dns(%s) error",
			myname, __LINE__, service->dns_addr);
		return (-1);
	}

	respond_ctx = service_ctx_new(service, server,
		SERVICE_CTX_TCP_RESPOND, request_ctx->id);  // 与请求端共享同一ID
	memcpy(respond_ctx->request_buf, data, len);
	respond_ctx->request_len = len;
	acl_aio_ctl(server,
		ACL_AIO_CTL_CONNECT_HOOK_ADD, respond_connect_callback, respond_ctx,
		ACL_AIO_CTL_CLOSE_HOOK_ADD, respond_close_callback, respond_ctx,
		ACL_AIO_CTL_CTX, respond_ctx,
		ACL_AIO_CTL_END);
	return (0);
}

static int request_len_callback(ACL_ASTREAM *astream, void *context,
	char *data, int len acl_unused)
{
	const char *myname = "request_len_callback";
	//SERVICE_CTX *ctx = (SERVICE_CTX*) context;
	unsigned short n;

	n = ntohs(*(const unsigned short *) data);
	if (n <= 0) {
		acl_msg_error("%s(%d): n(%d) invalid", myname, __LINE__, n);
		return (-1);
	}
	acl_aio_ctl(astream,
		ACL_AIO_CTL_READ_HOOK_ADD, request_data_callback, context,
		ACL_AIO_CTL_END);
	acl_aio_readn(astream, n);
	return (0);
}

static int request_close_callback(ACL_ASTREAM *astream acl_unused, void *context)
{
	SERVICE_CTX *ctx = (SERVICE_CTX*) context;

	service_ctx_free(ctx);
	return (-1);
}

void service_tcp_main(ACL_ASTREAM *client, SERVICE *service)
{
	//const char *myname = "service_tcp_main";
	SERVICE_CTX *ctx;

	ctx = service_ctx_new(service, client, SERVICE_CTX_TCP_REQUEST,
		service->curr_id++);
	acl_aio_ctl(client,
		ACL_AIO_CTL_READ_HOOK_ADD, request_len_callback, ctx,
		ACL_AIO_CTL_CLOSE_HOOK_ADD, request_close_callback, ctx,
		ACL_AIO_CTL_TIMEOUT, service->rw_timeout,
		ACL_AIO_CTL_CTX, ctx,
		ACL_AIO_CTL_END);
	acl_aio_readn(client, 2);  // 读取两个字节
}
