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

#ifdef WIN32
static void __connect_notify_callback(int event_type, ACL_EVENT *event,
	ACL_VSTREAM *stream, void *context);

static void ConnectTimer(int event_type acl_unused,
	ACL_EVENT *event acl_unused, void *ctx)
{
	ACL_ASTREAM *astream = (ACL_ASTREAM*) ctx;

	if (astream->aio->event_mode != ACL_EVENT_WMSG)
		acl_msg_fatal("event_mode(%d) != ACL_EVENT_WMSG(%d)",
			astream->aio->event_mode, ACL_EVENT_WMSG);
	__connect_notify_callback(ACL_EVENT_RW_TIMEOUT, NULL, NULL, ctx);
}
#endif

static void __connect_notify_callback(int event_type, ACL_EVENT *event,
	ACL_VSTREAM *stream acl_unused, void *context)
{
	const char *myname = "__connect_notify_callback";
	ACL_ASTREAM *astream = (ACL_ASTREAM *) context;
	int   ret, err;
	socklen_t errlen;

	WRITE_SAFE_DIABLE(astream);

	/* 先判断是否是超时导致返回 */
	if ((event_type & ACL_EVENT_RW_TIMEOUT) != 0) {
		if (aio_timeout_callback(astream) < 0)
			acl_aio_iocp_close(astream);
		else if (astream->flag & ACL_AIO_FLAG_IOCP_CLOSE)
			/* 该流正处于IO延迟关闭状态，因为本次写IO已经成功完成，
			 * 所以需要完成流的IO延迟关闭过程
			 */
			acl_aio_iocp_close(astream);
		else
			acl_event_enable_write(event, astream->stream,
				astream->timeout, __connect_notify_callback,
				astream);
		return;
	}

#ifdef WIN32
	/* 如果是基于 win32 窗口消息的事件引擎则需要取消之前设置的超时定时器 */
	if (astream->aio->event_mode == ACL_EVENT_WMSG)
		acl_aio_cancel_timer(astream->aio, ConnectTimer, astream);
#endif

	if ((event_type & ACL_EVENT_XCPT) != 0) {
		acl_aio_iocp_close(astream);
		return;
	}

	errlen = sizeof(err);
	ret = getsockopt(ACL_VSTREAM_SOCK(acl_aio_vstream(astream)),
			SOL_SOCKET, SO_ERROR, (char *) &err, &errlen);
	if (ret >= 0)
		acl_set_error(err);

#if defined(ACL_SUNOS5)
	/*
	 * Solaris 2.4's socket emulation doesn't allow you
	 * to determine the error from a failed non-blocking
	 * connect and just returns EPIPE.  Create a fake
	 * error message for connect.   -- fenner@parc.xerox.com
	 */
	if (ret < 0 && errno == EPIPE)
		acl_set_error(ACL_ENOTCONN);
#endif

	if (errno == 0 || errno == ACL_EISCONN)
		event_type = ACL_EVENT_CONNECT;
	else if ((event_type & ACL_EVENT_CONNECT) == 0)
		event_type |= ACL_EVENT_XCPT;

	if ((event_type & ACL_EVENT_XCPT) != 0) {
		astream->flag |= ACL_AIO_FLAG_DEAD;
		acl_aio_iocp_close(astream);
		return;
	}

	if ((event_type & ACL_EVENT_CONNECT) == 0)
		acl_msg_fatal("%s: unknown event: %d", myname, event_type);

	/* 将引用计数加1以防止在 connect_fn 内部调用了关闭过程，connect_fn
	 * 可通过返回-1，在回调返回后真正关闭
	 */
	astream->nrefer++;

	if (astream->connect_handles) {
		ACL_ITER iter;
		ACL_FIFO connect_handles;

		/* 必须将各个回调句柄从回调队列中一一提出置入一个单独队列中,
		 * 因为 ACL_AIO 在回调过程中有可能发生嵌套，防止被重复调用
		 */

		acl_fifo_init(&connect_handles);
		acl_foreach_reverse(iter, astream->connect_handles) {
			AIO_CONNECT_HOOK *handle = (AIO_CONNECT_HOOK*) iter.data;
			if (handle->disable)
				continue;
			acl_fifo_push(&connect_handles, handle);
		}

		while (1) {
			AIO_CONNECT_HOOK *handle = acl_fifo_pop(&connect_handles);
			if (handle == NULL)
				break;
			ret = handle->callback(astream, handle->ctx);
			if (ret == 0)
				continue;

			astream->nrefer--;
			if (ret < 0 || astream->flag  & ACL_AIO_FLAG_IOCP_CLOSE)
				acl_aio_iocp_close(astream);
			return;
		}
	}

	astream->nrefer--;

	if (ret < 0)
		acl_aio_iocp_close(astream);
	else if ((astream->flag  & ACL_AIO_FLAG_IOCP_CLOSE))
		/* 之前该流已经被设置了IO完成延迟关闭标志位，
		 * 则再次启动IO完成延迟关闭过程
		 */
		acl_aio_iocp_close(astream);
}

ACL_ASTREAM *acl_aio_connect(ACL_AIO *aio, const char *addr, int timeout)
{
	const char *myname = "acl_aio_connect";
	ACL_ASTREAM *astream;
	ACL_VSTREAM *cstream;

	if (aio == NULL || addr == NULL || *addr == 0)
		acl_msg_fatal("%s: input invalid", myname);

#ifdef ACL_EVENTS_STYLE_IOCP
	if (aio->event_mode == ACL_EVENT_KERNEL) {
		ACL_SOCKET connfd = WSASocket(AF_INET, SOCK_STREAM,
			IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);

		cstream = acl_vstream_fdopen(connfd, ACL_VSTREAM_FLAG_RW,
				aio->rbuf_size, timeout, ACL_VSTREAM_TYPE_SOCK);
		acl_assert(cstream);
		acl_vstream_set_peer(cstream, addr);
	} else
#endif
		cstream = acl_vstream_connect(addr, ACL_NON_BLOCKING,
			0, 0, aio->rbuf_size);

	if (cstream == NULL) {
		acl_msg_error("%s: connect addr(%s) error", myname, addr);
		return (NULL);
	}

	cstream->flag |= ACL_VSTREAM_FLAG_CONNECTING;

	astream = acl_aio_open(aio, cstream);
	if (astream == NULL)
		acl_msg_fatal("%s: open astream error", myname);

#ifdef WIN32
	if (timeout > 0 && aio->event_mode == ACL_EVENT_WMSG)
		acl_aio_request_timer(aio, ConnectTimer, astream,
			timeout * 1000000, 0);
#endif
	astream->error = acl_last_error();
	acl_aio_ctl(astream, ACL_AIO_CTL_TIMEOUT, timeout, ACL_AIO_CTL_END);

	WRITE_SAFE_ENABLE(astream, __connect_notify_callback);
	return astream;
}
