#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif
#include "stdlib/acl_stdlib.h"
#include "net/acl_net.h"
#include "net/acl_sane_inet.h"
#include "aio/acl_aio.h"

#endif

#include "../event/events_define.h" /* just for ACL_EVENTS_STYLE_IOCP define */
#include "aio.h"

#define	WRITE_SAFE_ENABLE(x, callback) do {  \
	if (((x)->flag & ACL_AIO_FLAG_ISWR) == 0) {  \
		(x)->flag |= ACL_AIO_FLAG_ISWR;  \
		acl_event_enable_write((x)->aio->event, (x)->stream,  \
			(x)->timeout, callback, (x));  \
	}  \
} while (0)

#define WRITE_SAFE_DIABLE(x) do {  \
	if (((x)->flag & ACL_AIO_FLAG_ISWR) != 0) {  \
		(x)->flag &= ~ACL_AIO_FLAG_ISWR;  \
		acl_event_disable_write((x)->aio->event, (x)->stream);  \
	}  \
} while (0)

#ifdef ACL_WINDOWS
static void __connect_notify_callback(int event_type, ACL_EVENT *event,
	ACL_VSTREAM *stream, void *context);

static void ConnectTimer(int event_type acl_unused,
	ACL_EVENT *event acl_unused, void *ctx)
{
	ACL_ASTREAM *conn = (ACL_ASTREAM*) ctx;

	if (conn->aio->event_mode != ACL_EVENT_WMSG) {
		acl_msg_fatal("event_mode(%d) != ACL_EVENT_WMSG(%d)",
			conn->aio->event_mode, ACL_EVENT_WMSG);
	}
	__connect_notify_callback(ACL_EVENT_RW_TIMEOUT, NULL, NULL, ctx);
}
#endif

static void __connect_notify_callback(int event_type, ACL_EVENT *event,
	ACL_VSTREAM *stream acl_unused, void *context)
{
	const char *myname = "__connect_notify_callback";
	ACL_ASTREAM *conn = (ACL_ASTREAM *) context;
	int   ret, err;
	socklen_t errlen;

	WRITE_SAFE_DIABLE(conn);

	/* 先判断是否是超时导致返回 */
	if ((event_type & ACL_EVENT_RW_TIMEOUT) != 0) {
		if (aio_timeout_callback(conn) < 0) {
			acl_aio_iocp_close(conn);
		} else if (conn->flag & ACL_AIO_FLAG_IOCP_CLOSE) {
			/* 该流正处于IO延迟关闭状态，因为本次写IO已经成功完成，
			 * 所以需要完成流的IO延迟关闭过程
			 */
			acl_aio_iocp_close(conn);
		} else {
			acl_event_enable_write(event, conn->stream,
				conn->timeout, __connect_notify_callback,
				conn);
		}
		return;
	}

#ifdef ACL_WINDOWS
	/* 如果是基于 win32 窗口消息的事件引擎则需要取消之前设置的超时定时器 */
	if (conn->aio->event_mode == ACL_EVENT_WMSG) {
		acl_aio_cancel_timer(conn->aio, ConnectTimer, conn);
	}
#endif

	if ((event_type & ACL_EVENT_XCPT) != 0) {
		acl_aio_iocp_close(conn);
		return;
	}

	errlen = sizeof(err);
	ret = getsockopt(ACL_VSTREAM_SOCK(acl_aio_vstream(conn)),
			SOL_SOCKET, SO_ERROR, (char *) &err, &errlen);
	if (ret >= 0) {
		acl_set_error(err);
	}

#if defined(ACL_SUNOS5)
	/*
	 * Solaris 2.4's socket emulation doesn't allow you
	 * to determine the error from a failed non-blocking
	 * connect and just returns EPIPE.  Create a fake
	 * error message for connect.   -- fenner@parc.xerox.com
	 */
	if (ret < 0 && errno == EPIPE) {
		acl_set_error(ACL_ENOTCONN);
	}
#endif

	if (errno == 0 || errno == ACL_EISCONN) {
		event_type = ACL_EVENT_CONNECT;
	} else if ((event_type & ACL_EVENT_CONNECT) == 0) {
		event_type |= ACL_EVENT_XCPT;
	}

	if ((event_type & ACL_EVENT_XCPT) != 0) {
		conn->flag |= ACL_AIO_FLAG_DEAD;
		acl_aio_iocp_close(conn);
		return;
	}

	if ((event_type & ACL_EVENT_CONNECT) == 0) {
		acl_msg_fatal("%s: unknown event: %d", myname, event_type);
	}

	/* 将引用计数加1以防止在 connect_fn 内部调用了关闭过程，connect_fn
	 * 可通过返回-1，在回调返回后真正关闭
	 */
	conn->nrefer++;

	if (conn->connect_handles) {
		ACL_ITER iter;
		ACL_FIFO connect_handles;

		/* 必须将各个回调句柄从回调队列中一一提出置入一个单独队列中,
		 * 因为 ACL_AIO 在回调过程中有可能发生嵌套，防止被重复调用
		 */

		acl_fifo_init(&connect_handles);
		acl_foreach_reverse(iter, conn->connect_handles) {
			AIO_CONNECT_HOOK *handle = (AIO_CONNECT_HOOK*) iter.data;
			if (handle->disable) {
				continue;
			}
			acl_fifo_push(&connect_handles, handle);
		}

		while (1) {
			AIO_CONNECT_HOOK *handle = acl_fifo_pop(&connect_handles);
			if (handle == NULL) {
				break;
			}
			ret = handle->callback(conn, handle->ctx);
			if (ret == 0) {
				continue;
			}

			conn->nrefer--;
			if (ret < 0 || conn->flag  & ACL_AIO_FLAG_IOCP_CLOSE) {
				acl_aio_iocp_close(conn);
			}
			return;
		}
	}

	conn->nrefer--;

	if (ret < 0) {
		acl_aio_iocp_close(conn);
	} else if ((conn->flag  & ACL_AIO_FLAG_IOCP_CLOSE)) {
		/* 之前该流已经被设置了IO完成延迟关闭标志位，
		 * 则再次启动IO完成延迟关闭过程
		 */
		acl_aio_iocp_close(conn);
	}
}

ACL_ASTREAM *acl_aio_connect(ACL_AIO *aio, const char *addr, int timeout)
{
	const char *myname = "acl_aio_connect";
	ACL_ASTREAM *conn;
	ACL_VSTREAM *cstream;

	if (aio == NULL || addr == NULL || *addr == 0) {
		acl_msg_fatal("%s: input invalid", myname);
	}

#ifdef ACL_EVENTS_STYLE_IOCP
	if (aio->event_mode == ACL_EVENT_KERNEL) {
		ACL_SOCKET connfd = WSASocket(AF_INET, SOCK_STREAM,
			IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);

		cstream = acl_vstream_fdopen(connfd, ACL_VSTREAM_FLAG_RW,
				aio->rbuf_size, timeout, ACL_VSTREAM_TYPE_SOCK);
		acl_assert(cstream);
		acl_vstream_set_peer(cstream, addr);
	} else {
		cstream = acl_vstream_connect(addr, ACL_NON_BLOCKING,
				0, 0, aio->rbuf_size);
	}
#else
	cstream = acl_vstream_connect(addr, ACL_NON_BLOCKING,
			0, 0, aio->rbuf_size);
#endif

	if (cstream == NULL) {
		acl_msg_error("%s: connect addr(%s) error", myname, addr);
		return NULL;
	}

	cstream->flag |= ACL_VSTREAM_FLAG_CONNECTING;

	conn = acl_aio_open(aio, cstream);
	if (conn == NULL) {
		acl_msg_fatal("%s: open connection error", myname);
	}

#ifdef ACL_WINDOWS
	if (timeout > 0 && aio->event_mode == ACL_EVENT_WMSG) {
		acl_aio_request_timer(aio, ConnectTimer, conn,
			timeout * 1000000, 0);
	}
#endif
	conn->error = acl_last_error();
	acl_aio_ctl(conn, ACL_AIO_CTL_TIMEOUT, timeout, ACL_AIO_CTL_END);

	WRITE_SAFE_ENABLE(conn, __connect_notify_callback);
	return conn;
}

/*--------------------------------------------------------------------------*/

struct ACL_ASTREAM_CTX {
	ACL_SOCKADDR ns_addr;
	ACL_SOCKADDR serv_addr;
	ACL_ASTREAM *conn;
	int status;
	void *ctx;
};

typedef struct {
	ACL_AIO *aio;
	ACL_SOCKADDR ns_addr;
	ACL_SOCKADDR serv_addr;
	ACL_ASTREAM *conn;
	ACL_AIO_CONNECT_ADDR_FN callback;
	void *context;
	int   port;
	int   timeout;
	ACL_ARGV *ip_list;
	int   ip_next;
} RESOLVE_CTX;

static void resolve_ctx_free(RESOLVE_CTX *ctx)
{
	acl_argv_free(ctx->ip_list);
	acl_myfree(ctx);
}

static int connect_callback(ACL_ASTREAM *conn, void *context);
static int connect_timeout(ACL_ASTREAM *conn, void *context);
static ACL_ASTREAM *try_connect_one(RESOLVE_CTX *ctx);

static int connect_failed(ACL_ASTREAM *conn acl_unused, void *context)
{
	RESOLVE_CTX *ctx = (RESOLVE_CTX *) context;
	ACL_ASTREAM_CTX conn_ctx;

	/* 因为在 connect_callback 和 connect_timeout 清除了关闭回调，所以当 
	 * 本函数被调用时，一定是连接失败所导致的，而不是由连接超时所致。
	 */

	/* 如果 DNS 解析出多个 IP 地址，则尝试连接下一个 IP 地址 */
	if (try_connect_one(ctx) != NULL) {
		/* 返回 -1 仅关闭当前超时的连接对象流，用新的连接流继续等待连接成功 */
		return -1;
	}

	acl_set_error(ACL_ECONNREFUSED);

	memset(&conn_ctx, 0, sizeof(conn_ctx));
	conn_ctx.conn = NULL;
	conn_ctx.ctx  = ctx->context;
	memcpy(&conn_ctx.ns_addr, &ctx->ns_addr, sizeof(ACL_SOCKADDR));
	memcpy(&conn_ctx.serv_addr, &ctx->serv_addr, sizeof(ACL_SOCKADDR));
	conn_ctx.status = ACL_ASTREAM_STATUS_CONNECT_ERROR;

	ctx->callback(&conn_ctx);
	resolve_ctx_free(ctx);
	return -1;
}

static int connect_timeout(ACL_ASTREAM *conn, void *context)
{
	RESOLVE_CTX *ctx = (RESOLVE_CTX *) context;
	ACL_ASTREAM_CTX conn_ctx;

	 /* 按 acl aio 的设计，当超时回调函数被调用且返回 -1 时，则所注册的
	  * 关闭回调接着会被调用，通过在此处清除域名解析后异步连接所注册的关
	  * 闭回调，从而禁止 connect_failed 再被调用。
	  */
	acl_aio_del_close_hook(conn, connect_failed, context);

	/* 如果 DNS 解析出多个 IP 地址，则尝试连接下一个 IP 地址 */
	if (try_connect_one(ctx) != NULL) {
		/* 返回 -1 仅关闭当前超时的连接对象流，用新的连接流继续等待连接成功 */
		return -1;
	}

	acl_set_error(ACL_ETIMEDOUT);

	memset(&conn_ctx, 0, sizeof(conn_ctx));
	conn_ctx.conn = NULL;
	conn_ctx.ctx  = ctx->context;
	memcpy(&conn_ctx.ns_addr, &ctx->ns_addr, sizeof(ACL_SOCKADDR));
	memcpy(&conn_ctx.serv_addr, &ctx->serv_addr, sizeof(ACL_SOCKADDR));
	conn_ctx.status = ACL_ASTREAM_STATUS_CONNECT_TIMEOUT;

	ctx->callback(&conn_ctx);
	resolve_ctx_free(ctx);
	return -1;
}

static int connect_callback(ACL_ASTREAM *conn, void *context)
{
	RESOLVE_CTX *ctx = (RESOLVE_CTX *) context;
	ACL_ASTREAM_CTX conn_ctx;

	/* 清除在域名解析后进行连接时注册的回调函数，这样当 IO 超时或关闭时
	 * 只回调应用注册的回调函数
	 */
	acl_aio_del_connect_hook(conn, connect_callback, context);
	acl_aio_del_timeo_hook(conn, connect_timeout, context);
	acl_aio_del_close_hook(conn, connect_failed, context);

	acl_set_error(0);

	memset(&conn_ctx, 0, sizeof(conn_ctx));
	conn_ctx.conn = conn;
	conn_ctx.ctx  = ctx->context;
	memcpy(&conn_ctx.ns_addr, &ctx->ns_addr, sizeof(ACL_SOCKADDR));
	memcpy(&conn_ctx.serv_addr, &ctx->serv_addr, sizeof(ACL_SOCKADDR));
	conn_ctx.status = ACL_ASTREAM_STATUS_OK;

	ctx->callback(&conn_ctx);
	resolve_ctx_free(ctx);
	return 0;
}

static ACL_ASTREAM *try_connect_one(RESOLVE_CTX *ctx)
{
	int n = acl_argv_size(ctx->ip_list);

	while (ctx->ip_next < n) {
		char  addr[128];
		const char *ip = acl_argv_index(ctx->ip_list,  ctx->ip_next);
		ACL_SOCKADDR sa;

		ctx->ip_next++;
		acl_assert(ip && *ip);
		snprintf(addr, sizeof(addr), "%s|%d", ip, ctx->port);
		if (acl_sane_pton(addr, (struct sockaddr *)&sa) == 0) {
			continue;
		}

		memcpy(&ctx->serv_addr, &sa, sizeof(sa));
		ctx->conn = acl_aio_connect(ctx->aio, addr, ctx->timeout);
		if (ctx->conn == NULL) {
			continue;
		}

		acl_aio_add_connect_hook(ctx->conn, connect_callback, ctx);
		acl_aio_add_timeo_hook(ctx->conn, connect_timeout, ctx);
		acl_aio_add_close_hook(ctx->conn, connect_failed, ctx);
		return ctx->conn;
	}

	return NULL;
}

static void dns_lookup_callback(ACL_DNS_DB *db, void *context, int errnum)
{
	RESOLVE_CTX *ctx  = (RESOLVE_CTX *) context;
	ACL_ITER     iter;
	ACL_ASTREAM_CTX conn_ctx;

	memset(&conn_ctx, 0, sizeof(conn_ctx));
	conn_ctx.ctx = ctx->context;

	if (db == NULL) {
		acl_set_error(errnum);
		conn_ctx.status = ACL_ASTREAM_STATUS_NS_ERROR;
		ctx->callback(&conn_ctx);
		resolve_ctx_free(ctx);
		return;
	}

	memcpy(&conn_ctx.ns_addr, &db->ns_addr, sizeof(conn_ctx.ns_addr));
	memcpy(&ctx->ns_addr, &db->ns_addr, sizeof(ctx->ns_addr));

	acl_foreach(iter, db) {
		const ACL_HOST_INFO *info = (const ACL_HOST_INFO *) iter.data;
		acl_argv_add(ctx->ip_list, info->ip, NULL);
	}

	if (acl_argv_size(ctx->ip_list) <= 0) {
		acl_set_error(errnum);
		conn_ctx.status = ACL_ASTREAM_STATUS_NS_ERROR;
		ctx->callback(&conn_ctx);
		resolve_ctx_free(ctx);
		return;
	}

	if (try_connect_one(ctx) == NULL) {
		acl_set_error(ACL_ECONNREFUSED);
		conn_ctx.status = ACL_ASTREAM_STATUS_CONNECT_ERROR;
		ctx->callback(&conn_ctx);
		resolve_ctx_free(ctx);
	}
}

int acl_aio_connect_addr(ACL_AIO *aio, const char *addr, int timeout,
	ACL_AIO_CONNECT_ADDR_FN callback, void *context)
{
	char buf[128], *ptr;
	int  port;
	RESOLVE_CTX *ctx;

	ACL_SAFE_STRNCPY(buf, addr, sizeof(buf));
	ptr = strrchr(buf, '|');
	if (ptr == NULL) {
		ptr = strrchr(buf, ':');
	}
	if (ptr == NULL || *(ptr + 1) == 0) {
		acl_msg_error("%s(%d), %s: invalid addr=%s",
			__FILE__, __LINE__, __FUNCTION__, addr);
		return -1;
	}
	*ptr++ = 0;
	port = atoi(ptr);
	if (port <= 0 || port > 65535) {
		acl_msg_error("%s(%d), %s: invalid port=%d, addr=%s",
			__FILE__, __LINE__, __FUNCTION__, port, addr);
		return -1;
	}

	ctx = (RESOLVE_CTX *) acl_mycalloc(1, sizeof(RESOLVE_CTX));
	ctx->aio      = aio;
	ctx->callback = callback;
	ctx->context  = context;
	ctx->port     = port;
	ctx->timeout  = timeout;
	ctx->ip_list  = acl_argv_alloc(5);

	if (acl_is_ip(buf) || acl_valid_unix(buf)) {
		ACL_SOCKADDR sa;
		ACL_ASTREAM *conn;

		if (acl_sane_pton(addr, (struct sockaddr *)&sa) == 0) {
			acl_msg_error("%s(%d): invalid addr=%s",
				__FUNCTION__, __LINE__, addr);
			resolve_ctx_free(ctx);
			return -1;
		}

		if ((conn = acl_aio_connect(aio, addr, timeout)) == NULL) {
			resolve_ctx_free(ctx);
			return -1;
		}

		memcpy(&ctx->serv_addr, &sa, sizeof(ACL_SOCKADDR));

		acl_aio_add_connect_hook(conn, connect_callback, ctx);
		acl_aio_add_timeo_hook(conn, connect_timeout, ctx);
		acl_aio_add_close_hook(conn, connect_failed, ctx);

		return 0;
	} else if (aio->dns == NULL) {
		acl_msg_error("%s(%d), %s: call acl_aio_set_dns first",
			__FILE__, __LINE__, __FUNCTION__);
		return -1;
	} else {
		acl_dns_lookup(aio->dns, buf, dns_lookup_callback, ctx);
		return 0;
	}
}

int acl_astream_get_status(const ACL_ASTREAM_CTX *ctx)
{
	return ctx ? ctx->status : ACL_ASTREAM_STATUS_INVALID;
}

const ACL_SOCKADDR *acl_astream_get_ns_addr(const ACL_ASTREAM_CTX *ctx)
{
	return ctx ? &ctx->ns_addr : NULL;
}

const ACL_SOCKADDR *acl_astream_get_serv_addr(const ACL_ASTREAM_CTX *ctx)
{
	return ctx ? &ctx->serv_addr : NULL;
}

ACL_ASTREAM *acl_astream_get_conn(const ACL_ASTREAM_CTX *ctx)
{
	return ctx ? ctx->conn : NULL;
}

void *acl_astream_get_ctx(const ACL_ASTREAM_CTX *ctx)
{
	return ctx ? ctx->ctx : NULL;
}
