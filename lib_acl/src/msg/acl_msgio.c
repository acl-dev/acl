#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_ring.h"
#include "stdlib/acl_msg.h"
#include "stdlib/acl_vstream.h"
#include "stdlib/acl_vstring.h"
#include "thread/acl_pthread.h"
#include "net/acl_vstream_net.h"
#include "net/acl_listen.h"
#include "msg/acl_msgio.h"

#endif

typedef struct MSGIO_CTX MSGIO_CTX;

/* IO 消息句柄 */
struct ACL_MSGIO {
	ACL_AIO *aio;
	ACL_RING msg_list;
	union {
		ACL_ASTREAM *async;
		ACL_VSTREAM *sync;
	} stream;
	int   rw_timeout;
	char  addr[256];
	MSGIO_CTX *ctx;
	int   type;
#define ACL_MSGIO_TYPE_CLIENT	0
#define ACL_MSGIO_TYPE_SERVER	1
#define ACL_MSGIO_TYPE_ACCEPT	2

	int   keep_alive;
};

/* 消息信息载体结构定义 */
typedef struct MSG_ITEM {
	int   id;		/* 消息 ID */
	int   inherit;		/* 是否允许消息继承，主要用于在创建客户端
				 * 消息对象后克隆监听对象的消息集合时
				 */
	ACL_RING entry;		/* 连接进 ACL_MSGIO.msg_list */
	ACL_RING call_list;	/* MSG_CALL 对象集合 */
} MSG_ITEM;

/* 消息回调处理过程载体结构定义 */
typedef struct MSG_CALL {
	MSG_ITEM  *msg;		/* 指向消息 */
	ACL_RING entry;		/* 连接进 MSG_ITEM.call_list */
	ACL_MSGIO_NOTIFY_FN notify_fn; /* 消息回调函数 */
	void *arg;		/* 消息回调函数的参数 */
} MSG_CALL;

struct MSGIO_CTX {
	ACL_MSGIO *mio;
	ACL_MSGIO_INFO info;
	char addr[256];
};

static ACL_MSGIO *__global_mio = NULL;
static acl_pthread_mutex_t __global_mutex;

/*----------------------------------------------------------------------------*/

static MSGIO_CTX *msgio_ctx_new(ACL_MSGIO *mio, const char *addr)
{
	MSGIO_CTX *ctx;

	ctx = acl_mycalloc(1, sizeof(MSGIO_CTX));
	ctx->info.body.buf = acl_vstring_alloc(256);
	ctx->mio = mio;
	ACL_SAFE_STRNCPY(ctx->addr, addr, sizeof(ctx->addr));
	mio->ctx = ctx;
	return (ctx);
}

static void msg_ctx_free(MSGIO_CTX *ctx)
{
	acl_vstring_free(ctx->info.body.buf);
	acl_myfree(ctx);
}

/*----------------------------------------------------------------------------*/

/* 创建新的消息回调处理对象 */

static MSG_CALL *msg_call_new(MSG_ITEM *msg, ACL_MSGIO_NOTIFY_FN notify_fn, void *arg)
{
	MSG_CALL *call = acl_mycalloc(1, sizeof(MSG_CALL));

	call->msg = msg;
	call->notify_fn = notify_fn;
	call->arg = arg;
	return (call);
}

/* 释放消息回调处理对象 */

static void msg_call_free(MSG_CALL *call)
{
	acl_myfree(call);
}

/*----------------------------------------------------------------------------*/

/* 创建新的消息对象 */

static MSG_ITEM *msg_new(int id, int inherit)
{
	MSG_ITEM *msg;

	msg = acl_mycalloc(1, sizeof(MSG_ITEM));
	msg->id = id;
	msg->inherit = inherit;
	acl_ring_init(&msg->call_list);
	return (msg);
}

/* 向消息的处理对象集合中添加新的处理对象 */
  
static void msg_add(MSG_ITEM *msg, MSG_CALL *call)
{
	acl_ring_append(&msg->call_list, &call->entry);
}

/* 释放消息对象 */

static void msg_free(MSG_ITEM *msg)
{
	MSG_CALL *call;
	ACL_RING *tmp;

	while ((tmp = acl_ring_pop_head(&msg->call_list)) != NULL) {
		call = ACL_RING_TO_APPL(tmp, MSG_CALL, entry);
		msg_call_free(call);
	}
	acl_myfree(msg);
}

/* 向消息处理对象集合中添加新的处理对象 */

static void msg_append(MSG_ITEM *msg, ACL_MSGIO_NOTIFY_FN notify_fn, void *arg)
{
	ACL_RING_ITER iter;
	MSG_CALL *call;

	acl_ring_foreach(iter, &msg->call_list) {
		call = ACL_RING_TO_APPL(iter.ptr, MSG_CALL, entry);
		if (call->notify_fn == notify_fn) {
			return;
		}
	}

	call = msg_call_new(msg, notify_fn, arg);
	acl_ring_append(&msg->call_list, &call->entry);
}

/* 拷贝克隆某消息的处理对象集合 */

static void msg_clone(MSG_ITEM *msg_from, MSG_ITEM *msg_to)
{
	MSG_CALL *call_from, *call_to;
	ACL_RING_ITER iter;

	acl_ring_foreach(iter, &msg_from->call_list) {
		call_from = ACL_RING_TO_APPL(iter.ptr, MSG_CALL, entry);
		call_to = msg_call_new(msg_to, call_from->notify_fn,
				call_from->arg);
		msg_add(msg_to, call_to);
	}
}

/* 拷贝克隆消息集合及消息处理对象集合 */

static void msg_list_clone(ACL_MSGIO *mio_from, ACL_MSGIO *mio_to)
{
	MSG_ITEM *msg_from, *msg_to;
	ACL_RING_ITER iter;

	acl_ring_foreach(iter, &mio_from->msg_list) {
		msg_from = ACL_RING_TO_APPL(iter.ptr, MSG_ITEM, entry);
		if (!msg_from->inherit)
			continue;
		msg_to = msg_new(msg_from->id, msg_from->inherit);
		acl_ring_append(&mio_to->msg_list, &msg_to->entry);
		msg_clone(msg_from, msg_to);
	}
}

/* 根据消息ID查询消息对象 */

static MSG_ITEM *msg_find(ACL_MSGIO *mio, int id)
{
	MSG_ITEM *msg;
	ACL_RING_ITER iter;

	acl_ring_foreach(iter, &mio->msg_list) {
		msg = ACL_RING_TO_APPL(iter.ptr, MSG_ITEM, entry);
		if (msg->id == id)
			return (msg);
	}

	return (NULL);
}

/* 取消某消息的某个处理过程 */

static void msg_unreg(MSG_ITEM *msg, ACL_MSGIO_NOTIFY_FN notify_fn)
{
	ACL_RING_ITER iter;
	MSG_CALL *call;

	acl_ring_foreach(iter, &msg->call_list) {
		call = ACL_RING_TO_APPL(iter.ptr, MSG_CALL, entry);
		if (call->notify_fn == notify_fn) {
			acl_ring_detach(&call->entry);
			msg_call_free(call);
			break;
		}
	}
}

/* 取消某消息的所有处理过程并释放消息对象 */

static void msg_unreg_all(MSG_ITEM *msg)
{
	msg_free(msg);
}

/*----------------------------------------------------------------------------*/

static ACL_MSGIO *msgio_new(void)
{
	const char *myname = "acl_msgio_new";
	ACL_MSGIO *mio;
	
	mio = acl_mycalloc(1, sizeof(ACL_MSGIO));
	if (mio == NULL) {
		char  ebuf[256];
		acl_msg_fatal("%s: calloc error(%s)",
			myname, acl_last_strerror(ebuf, sizeof(ebuf)));
	}

	acl_ring_init(&mio->msg_list);
	
	return (mio);
}

void acl_msgio_close(ACL_MSGIO *mio)
{
	acl_msgio_unreg_all(mio);
	if (mio->aio)
		acl_aio_iocp_close(mio->stream.async);
	else
		acl_vstream_close(mio->stream.sync);
}

void acl_msgio_init(void)
{
	const char *myname = "acl_msgio_init";

	if (__global_mio != NULL)
		acl_msg_fatal("%s: be called more than twice", myname);

	acl_pthread_mutex_init(&__global_mutex, NULL);
	__global_mio = msgio_new();
}

static void msgio_reg(ACL_MSGIO *mio, int id,
	ACL_MSGIO_NOTIFY_FN callback, void *arg, int inherit)
{
	const char *myname= "acl_msgio_reg";
	MSG_ITEM *msg;

	if (mio == NULL)
		mio = __global_mio;

	if (mio == NULL)
		acl_msg_fatal("%s: call acl_msgio_init first", myname);

	msg = msg_find(mio, id);

	/* 先查询某个消息是否存在，如果存在则向该消息对象添加处理过程,
	 * 否则，创建并添加新的消息对象及处理过程
	 */

	if (msg) {
		msg_append(msg, callback, arg);
	} else {
		MSG_CALL *call;

		msg = msg_new(id, inherit);
		call = msg_call_new(msg, callback, arg);
		msg_add(msg, call);
		acl_ring_append(&mio->msg_list, &msg->entry);
	}
}

void acl_msgio_reg(ACL_MSGIO *mio, int id,
	ACL_MSGIO_NOTIFY_FN callback, void *arg)
{
	msgio_reg(mio, id, callback, arg, 0);
}

void acl_msgio_listen_reg(ACL_MSGIO *mio, int id,
	ACL_MSGIO_NOTIFY_FN callback, void *arg, int inherit)
{
	msgio_reg(mio, id, callback, arg, inherit);
}

void acl_msgio_unreg(ACL_MSGIO *mio, int id, ACL_MSGIO_NOTIFY_FN callback)
{
	const char *myname = "acl_msgio_unreg";
	MSG_ITEM *msg;

	if (mio == NULL)
		mio = __global_mio;

	if (mio == NULL)
		acl_msg_fatal("%s: call acl_msgio_init first", myname);

	msg = msg_find(mio, id);
	if (msg != NULL)
		msg_unreg(msg, callback);
}

void acl_msgio_unreg_id(ACL_MSGIO *mio, int id)
{
	const char *myname = "acl_msgio_unreg_id";
	MSG_ITEM *msg;

	if (mio == NULL)
		mio = __global_mio;

	if (mio == NULL)
		acl_msg_fatal("%s: call acl_msgio_init first", myname);

	msg = msg_find(mio, id);
	if (msg != NULL) {
		acl_ring_detach(&msg->entry);
		msg_unreg_all(msg);
	}
}

void acl_msgio_unreg_all(ACL_MSGIO *mio)
{
	const char *myname = "acl_msgio_unreg_all";
	MSG_ITEM *msg;
	ACL_RING *tmp;

	if (mio == NULL)
		mio = __global_mio;

	if (mio == NULL)
		acl_msg_fatal("%s: call acl_msgio_init first", myname);

	while (1) {
		tmp = acl_ring_pop_head(&mio->msg_list);
		if (tmp == NULL)
			break;

		msg = ACL_RING_TO_APPL(tmp, MSG_ITEM, entry);
		msg_unreg_all(msg);
	}
}

/* 调用某个 ACL_MSGIO 句柄中某个消息的所有处理过程 */

static int dispatch_foreach(ACL_MSGIO *mio, const ACL_MSGIO_INFO *info, int id)
{
	const char *myname = "dispatch_foreach";
	ACL_RING_ITER iter;
	MSG_CALL *call;
	MSG_ITEM *msg;
	int   ret = 0;

	/* 找出注册该消息 (id) 的对象集合 */
	msg = msg_find(mio, id);
	if (msg == NULL) {
		if (id == ACL_MSGIO_QUIT) {
			acl_msg_info("%s: not found quit handler,"
				" msgio quit(%d) now", myname, id);
			return (-1);
		}
		return (0);
	}

	acl_ring_foreach(iter, &msg->call_list) {
		call = ACL_RING_TO_APPL(iter.ptr, MSG_CALL, entry);
		ret = call->notify_fn(id, mio, info, call->arg);
		if (ret != 0)
			break;
	}

	if (id == ACL_MSGIO_QUIT) {
		acl_msg_info("msgio quit now");
		return (-1);
	}

	return (ret);
}

/* 发送消息, 调用消息回调函数 */

static int message_dispatch(MSGIO_CTX *ctx)
{
	const char *myname = "message_dispatch";
	int   ret;

	ret = dispatch_foreach(ctx->mio, &ctx->info, ctx->info.hdr.type);
	if (ret < 0) {
		acl_msg_error("%s: dispatch_foreach error, type=%d",
			myname, ctx->info.hdr.type);
		acl_msgio_close(ctx->mio);
		return (-1);
	} else if (ret == 0 && __global_mio) {
		ret = dispatch_foreach(__global_mio, &ctx->info,
				ctx->info.hdr.type);
		if (ret < 0) {
			acl_msg_error("%s: dispatch_foreach error, type=%d",
				myname, ctx->info.hdr.type);
			acl_msgio_close(ctx->mio);
			return (-1);
		}
	}
	return (ret);
}

/* 异步读消息体回调函数 */

static int read_body_callback(ACL_ASTREAM *astream acl_unused, void *arg,
	char *data, int dlen)
{
	const char *myname = "read_body_callback";
	MSGIO_CTX *ctx = (MSGIO_CTX *) arg;

	if (dlen != ctx->info.hdr.dlen) {
		acl_msg_fatal("%s: dlen=%d, hdr.dlen=%d",
			myname, dlen, ctx->info.hdr.dlen);
	}

	/* 拷贝消息体数据 */
	acl_vstring_memcpy(ctx->info.body.buf, data, dlen);

	/* 发送消息至各个注册函数 */
	if (message_dispatch(ctx) < 0)
		return (-1);

	/* 异步等待下一个消息 */
	return (acl_msgio_wait(ctx->mio));
}

/* 异步读消息头回调函数 */

static int read_hdr_callback(ACL_ASTREAM *astream, void *arg,
	char *data, int dlen)
{
	const char *myname = "read_hdr_callback";
	MSGIO_CTX *ctx = (MSGIO_CTX *) arg;
	const ACL_MSGIO_INFO *info = (const ACL_MSGIO_INFO *) data;

	/* 校验消息头长度 */
	if (dlen != sizeof(ctx->info.hdr)) {
		acl_msg_fatal("%s: dlen=%d, size=%d",
			myname, dlen, (int) sizeof(ctx->info.hdr));
	}

	ctx->info.hdr.type = info->hdr.type;
	ctx->info.hdr.dlen = info->hdr.dlen;

	/* 如果该消息有消息体则读消息体 */
	if (ctx->info.hdr.dlen > 0) {
		acl_aio_add_read_hook(astream, read_body_callback, ctx);
		/* 开始读消息体 */
		acl_aio_readn(astream, ctx->info.hdr.dlen);
		return (0);
	}

	/* 该消息没有消息体，则开始派发消息 */
	if (message_dispatch(ctx) < 0) {
		acl_msg_error("%s: message_dispatch error", myname);
		return (-1);
	}

	/* 异步等待下一个消息 */
	return (acl_msgio_wait(ctx->mio));
}

/* 异步方式等待IO消息 */

static int async_wait_msg(ACL_MSGIO *mio)
{
	/* 注册回调函数 */
	acl_aio_ctl(mio->stream.async,
		ACL_AIO_CTL_READ_HOOK_ADD, read_hdr_callback, mio->ctx,
		ACL_AIO_CTL_END);

	/* 异步读消息头 */
	acl_aio_readn(mio->stream.async, sizeof(mio->ctx->info.hdr));
	return (0);
}

/* 同步方式等待IO消息 */

static int sync_wait_msg(ACL_MSGIO *mio)
{
	const char *myname = "sync_wait_msg";
	MSGIO_CTX *ctx = mio->ctx;
	char  buf[1024];
	int   dlen, n;

	/* 同步读消息头 */
	if (acl_vstream_readn(mio->stream.sync,
		&ctx->info.hdr, sizeof(ctx->info.hdr)) == ACL_VSTREAM_EOF)
	{
		acl_msg_error("%s: vstream read error", myname);
		acl_vstream_close(mio->stream.sync);
		return (-1);
	}

	if (ctx->info.hdr.dlen <= 0)
		return (message_dispatch(ctx));  /* 发送消息到各个注册函数 */

	dlen = ctx->info.hdr.dlen;
	/* 同步读消息体 */
	while (dlen > 0) {
		n = acl_vstream_read(mio->stream.sync, buf, sizeof(buf));
		if (n == ACL_VSTREAM_EOF) {
			acl_msg_error("%s: read msg body error(%s)",
				myname, acl_last_serror());
			acl_vstream_close(mio->stream.sync);
			return (-1);
		}
		acl_vstring_strncat(ctx->info.body.buf, buf, n);
		dlen -= n;
	}

	/* 发送消息到各个注册函数 */
	return (message_dispatch(ctx));
}

int acl_msgio_wait(ACL_MSGIO *mio_client)
{
	const char *myname = "acl_msgio_wait";

	if (mio_client->type == ACL_MSGIO_TYPE_ACCEPT)
		acl_msg_fatal("%s(%d): ACL_MSGIO_TYPE_ACCEPT use here",
			myname, __LINE__);
	if (mio_client->aio)
		return (async_wait_msg(mio_client));
	else
		return (sync_wait_msg(mio_client));
}

static void free_mio_onclose(ACL_VSTREAM *stream acl_unused, void *arg)
{
	ACL_MSGIO *mio = (ACL_MSGIO *) arg;
	acl_msgio_unreg_all(mio);
	acl_myfree(mio);
}

static void free_msg_ctx_onclose(ACL_VSTREAM *stream acl_unused, void *arg)
{
	MSGIO_CTX *ctx = (MSGIO_CTX *) arg;
	msg_ctx_free(ctx);
}

static int close_callback(ACL_ASTREAM *astream, void *arg)
{
	const char *myname = "close_callback";
	MSGIO_CTX *ctx = (MSGIO_CTX *) arg;

	acl_msg_error("%s: close it(%d) now(%s)", myname,
		ACL_VSTREAM_SOCK(acl_aio_vstream(astream)), acl_last_serror());

	ctx->info.hdr.type = ACL_MSGIO_EXCEPT;
	ctx->info.hdr.dlen = 0;
	message_dispatch(ctx);
	return (-1);
}

static int io_timeout_callback(ACL_ASTREAM *astream acl_unused, void *arg)
{
	MSGIO_CTX *ctx = (MSGIO_CTX *) arg;

	ctx->info.hdr.type = ACL_MSGIO_TIMEOUT;
	ctx->info.hdr.dlen = 0;
	if (message_dispatch(ctx) < 0)
		return (-1);

	return (0);
}

/* 消息服务器接收客户端连接 */

static ACL_MSGIO *accept_connection(ACL_VSTREAM *sstream, ACL_MSGIO *listener)
{
	const char *myname = "accept_connection";
	ACL_VSTREAM *stream;
	ACL_MSGIO *mio_client;
	MSGIO_CTX *ctx_client;

	stream = acl_vstream_accept(sstream, NULL, 0);
	if (stream == NULL) {
		acl_msg_error("%s(%d): accept error(%s)",
			myname, __LINE__, acl_last_serror());
		return (NULL);
	}

	acl_non_blocking(ACL_VSTREAM_SOCK(stream), ACL_BLOCKING);
	mio_client = msgio_new();
	mio_client->type = ACL_MSGIO_TYPE_SERVER;
	mio_client->rw_timeout = listener->rw_timeout;
	msg_list_clone(listener, mio_client);
	mio_client->aio = listener->aio;

	ctx_client = msgio_ctx_new(mio_client, ACL_VSTREAM_PEER(stream));
	acl_vstream_add_close_handle(stream, free_msg_ctx_onclose, ctx_client);
	acl_vstream_add_close_handle(stream, free_mio_onclose, mio_client);

	if (mio_client->aio) {
		/* 若是异步读消息，则... */
		mio_client->stream.async = acl_aio_open(mio_client->aio, stream);
		acl_aio_ctl(mio_client->stream.async,
			ACL_AIO_CTL_TIMEOUT, mio_client->rw_timeout,
			ACL_AIO_CTL_TIMEO_HOOK_ADD, io_timeout_callback, ctx_client,
			ACL_AIO_CTL_CLOSE_HOOK_ADD, close_callback, ctx_client,
			ACL_AIO_CTL_CTX, ctx_client,
			ACL_AIO_CTL_END);
	} else
		mio_client->stream.sync = stream;

	return (mio_client);
}

/* 监听描述符可读的回调函数 */

static int listen_callback(ACL_ASTREAM *sstream acl_unused, void *arg)
{
	const char *myname = "listen_callback";
	MSGIO_CTX *ctx = (MSGIO_CTX *) arg;
	ACL_MSGIO *mio_client;

	if (sstream != ctx->mio->stream.async)
		acl_msg_fatal("%s(%d): sstream invalid", myname, __LINE__);

	mio_client = accept_connection(acl_aio_vstream(sstream), ctx->mio);
	if (mio_client == NULL)
		acl_msg_warn("%s(%d): accept null", myname, __LINE__);
	else if (acl_msgio_wait(mio_client) < 0)
		acl_msg_warn("%s(%d): acl_msgio_wait error", myname, __LINE__);
	return (0);
}

ACL_MSGIO *acl_msgio_listen(ACL_AIO *aio, const char *addr)
{
	const char *myname = "acl_msgio_listen";
	const char *local = "127.0.0.1:0";
	const char *addr_ptr;
	MSGIO_CTX *ctx;
	ACL_MSGIO *listener;
	ACL_VSTREAM *stream;

	if (aio == NULL)
		acl_msg_fatal("%s: aio null", myname);
	if (addr != NULL)
		addr_ptr = addr;
	else
		addr_ptr = local;

	listener = msgio_new();
	listener->type = ACL_MSGIO_TYPE_ACCEPT;
	stream = acl_vstream_listen_ex(
			addr_ptr, 128, ACL_INET_FLAG_NBLOCK, 1024, 0);

	if (stream == NULL)
		acl_msg_fatal("%s: listen(%s) error(%s)",
			myname, addr_ptr, acl_last_serror());

	if (ACL_VSTREAM_LOCAL(stream) && *ACL_VSTREAM_LOCAL(stream))
		ACL_SAFE_STRNCPY(listener->addr, ACL_VSTREAM_LOCAL(stream),
			sizeof(listener->addr));
	else
		listener->addr[0] = 0;

	ctx = msgio_ctx_new(listener, listener->addr);

	listener->aio = aio;
	listener->stream.async = acl_aio_open(aio, stream);
	acl_aio_ctl(listener->stream.async,
		ACL_AIO_CTL_LISTEN_FN, listen_callback,
		ACL_AIO_CTL_TIMEOUT, 0,
		ACL_AIO_CTL_CTX, ctx,
		ACL_AIO_CTL_END);
	acl_aio_listen(listener->stream.async);
	return (listener);
}

ACL_MSGIO *acl_msgio_accept(ACL_MSGIO *listener)
{
	ACL_MSGIO *mio_client;

	mio_client = accept_connection(
			acl_aio_vstream(listener->stream.async),
			listener);
	return (mio_client);
}

/* 连接超时回调函数 */

static int connect_timeout_callback(ACL_ASTREAM *astream acl_unused, void *arg)
{
	MSGIO_CTX *ctx = (MSGIO_CTX *) arg;

	ctx->info.hdr.type = ACL_MSGIO_CONNECT_TIMEOUT;
	ctx->info.hdr.dlen = 0;
	(void) message_dispatch(ctx);
	return (-1);
}

/* 连接成功回调函数 */

static int connect_callback(ACL_ASTREAM *astream, void *arg)
{
	MSGIO_CTX *ctx = (MSGIO_CTX *) arg;

	/* reset the timeout handler */
	acl_aio_ctl(astream, ACL_AIO_CTL_TIMEO_HOOK_ADD, io_timeout_callback, ctx,
		ACL_AIO_CTL_END);

	ctx->info.hdr.type = ACL_MSGIO_CONNECT;
	ctx->info.hdr.dlen = 0;

	if (message_dispatch(ctx) < 0)
		return (-1);

	/* 异步等待消息 */
	return (acl_msgio_wait(ctx->mio));
}

/* 开始异步连接消息服务器 */

static ACL_ASTREAM *async_connect(ACL_AIO *aio, const char *addr,
	int rw_timeout, MSGIO_CTX *ctx)
{
	ACL_ASTREAM *astream;

	astream = acl_aio_connect(aio, addr, rw_timeout);
	if (astream == NULL)
		return (NULL);

	acl_aio_ctl(astream,
		ACL_AIO_CTL_CONNECT_HOOK_ADD, connect_callback, ctx,
		ACL_AIO_CTL_CLOSE_HOOK_ADD, close_callback, ctx,
		ACL_AIO_CTL_TIMEO_HOOK_ADD, connect_timeout_callback, ctx,
		ACL_AIO_CTL_TIMEOUT, rw_timeout,
		ACL_AIO_CTL_END);
	return (astream);
}

void acl_msgio_set_noblock(ACL_AIO *aio, ACL_MSGIO *mio)
{
	mio->aio = aio;
	mio->stream.async = acl_aio_open(aio, mio->stream.sync);
	acl_aio_ctl(mio->stream.async,
		ACL_AIO_CTL_CTX, mio->ctx,
		ACL_AIO_CTL_END);

	/* 异步等待消息 */
	(void) acl_msgio_wait(mio);
}

ACL_MSGIO *acl_msgio_connect(ACL_AIO *aio, const char *addr, int rw_timeout)
{
	MSGIO_CTX *ctx;
	ACL_MSGIO *mio;
	ACL_VSTREAM *stream;

	mio = msgio_new();
	mio->keep_alive = 1;
	mio->type = ACL_MSGIO_TYPE_CLIENT;
	mio->rw_timeout = rw_timeout > 0 ? rw_timeout : 0;
	ACL_SAFE_STRNCPY(mio->addr, addr, sizeof(mio->addr));

	ctx = msgio_ctx_new(mio, addr);

	if (aio != NULL) {
		mio->aio = aio;
		mio->stream.async = async_connect(aio, addr, rw_timeout, ctx);
		if (mio->stream.async)
			stream = acl_aio_vstream(mio->stream.async);
		else
			stream = NULL;
	} else {
		stream = acl_vstream_connect(addr, ACL_BLOCKING, rw_timeout,
				rw_timeout, 1024);
		mio->stream.sync = stream;
	}

	if (stream == NULL) {
		msg_ctx_free(ctx);
		acl_msgio_close(mio);
		return (NULL);
	}

	acl_vstream_add_close_handle(stream, free_msg_ctx_onclose, ctx);
	acl_vstream_add_close_handle(stream, free_mio_onclose, mio);
	return (mio);
}

/* 同步发送消息 */

static int send_msg(ACL_MSGIO *mio, int type, void *data, int dlen)
{
	const char *myname = "send_msg";
	ACL_MSGIO_INFO info;
	ACL_VSTREAM *stream;
	struct iovec vector[2];
	int  ret, n = 0;

	if (mio->aio)
		stream = acl_aio_vstream(mio->stream.async);
	else
		stream = mio->stream.sync;

	if (stream == NULL)
		acl_msg_fatal("%s: stream NULL", myname);

	if (data == NULL || dlen < 0)
		dlen = 0;

	memset(&info, 0, sizeof(ACL_MSGIO_INFO));
	info.hdr.type = type;
	info.hdr.dlen = dlen;

#if 1
	vector[0].iov_base = (void*) &info.hdr;
	vector[0].iov_len = sizeof(info.hdr);
	n++;
	if (data && dlen > 0) {
		vector[1].iov_base = data;
		vector[1].iov_len = dlen;
		n++;
	}
	ret = acl_vstream_writevn(stream, vector, n);
	if (ret == ACL_VSTREAM_EOF) {
		acl_msg_error("%s: write msg hdr error(%s)",
			myname, acl_last_serror());
		return (-1);
	}
	return (0);
#elif 1
	/* 同步发送消息头 */
	ret = acl_vstream_buffed_writen(stream, &info.hdr, sizeof(info.hdr));
	if (ret == ACL_VSTREAM_EOF) {
		acl_msg_error("%s: write msg hdr error(%s)",
			myname, acl_last_serror());
		return (-1);
	}

	if (dlen > 0) {
		/* 同步发送消息体 */
		ret = acl_vstream_buffed_writen(stream, data, dlen);
		if (ret == ACL_VSTREAM_EOF) {
			acl_msg_error("%s: write msg body error(%s)",
				myname, acl_last_serror());
			return (-1);
		}
	}

	if (acl_vstream_fflush(stream) == ACL_VSTREAM_EOF) {
		acl_msg_error("%s: fflush to stream(%d) error(%s)",
			myname, ACL_VSTREAM_SOCK(stream), acl_last_serror());
		return (-1);
	}
#else
	/* 同步发送消息头 */
	ret = acl_vstream_writen(stream, &info.hdr, sizeof(info.hdr));
	if (ret == ACL_VSTREAM_EOF) {
		acl_msg_error("%s: write msg hdr error(%s)",
			myname, acl_last_serror());
		return (-1);
	}

	if (dlen > 0) {
		/* 同步发送消息体 */
		ret = acl_vstream_writen(stream, data, dlen);
		if (ret == ACL_VSTREAM_EOF) {
			acl_msg_error("%s: write msg body error(%s)",
				myname, acl_last_serror());
			return (-1);
		}
	}
#endif
	return (0);
}

int acl_msgio_send(ACL_MSGIO *mio, int type, void *data, int dlen)
{
	const char *myname = "acl_msgio_send";

	if (mio == NULL || mio->ctx == NULL)
		acl_msg_fatal("%s: input invalid", myname);

	if (mio->type == ACL_MSGIO_TYPE_ACCEPT) {
		ACL_MSGIO *client;
		char  addr[256];
		int   ret;

		acl_msgio_addr(mio, addr, sizeof(addr));
		client = acl_msgio_connect(NULL, addr, 10);
		if (client == NULL) {
			acl_msg_error("%s: connect server(%s) error(%s)",
				myname, addr, acl_last_serror());
			return (-1);
		}
		ret = send_msg(client, type, data, dlen);
		acl_msgio_close(client);
		return (ret);
	}

	return (send_msg(mio, type, data, dlen));
}

void acl_msgio_addr(const ACL_MSGIO *mio, char *buf, size_t size)
{
	ACL_SAFE_STRNCPY(buf, mio->addr, size);
}

ACL_AIO *acl_msgio_aio(ACL_MSGIO *mio)
{
	return (mio->aio);
}

ACL_VSTREAM *acl_msgio_vstream(ACL_MSGIO *mio)
{
	if (mio->aio)
		return (acl_aio_vstream(mio->stream.async));
	else
		return (mio->stream.sync);
}

ACL_ASTREAM *acl_msgio_astream(ACL_MSGIO *mio)
{
	if (mio->aio)
		return (mio->stream.async);
	else
		return (NULL);
}
