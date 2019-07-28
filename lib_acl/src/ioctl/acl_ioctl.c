#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "stdlib/acl_stdlib.h"
#include "event/acl_events.h"
#include "thread/acl_pthread_pool.h"
#include "net/acl_net.h"
#include "ioctl/acl_ioctl.h"

#endif

#include "ioctl_internal.h"

static int __poller_fn(void *arg)
{       
	ACL_IOCTL *ioc = (ACL_IOCTL *) arg;

	acl_event_loop(ioc->event);
	return 0;
}                               

ACL_IOCTL *acl_ioctl_create_ex(int event_mode, int max_threads,
	int idle_timeout, int delay_sec, int delay_usec)
{
	const char *myname = "acl_ioctl_create_ex";
	ACL_IOCTL *ioc;

	if (max_threads < 0)
		max_threads = 0;
	if (max_threads > 0 && idle_timeout <= 0) {
		idle_timeout = 60;
		acl_msg_error("%s, %s(%d): idle_timeout(%d) invalid",
			__FILE__, myname, __LINE__, idle_timeout);
	}

	ioc = (ACL_IOCTL *) acl_mycalloc(1, sizeof(ACL_IOCTL));

	if (delay_sec <= 0 && delay_usec <= 0) {
		delay_sec = 1;
		delay_usec = 0;
	}
	ioc->event_mode   = event_mode;
	ioc->max_threads  = max_threads;
	ioc->idle_timeout = idle_timeout;
	ioc->delay_sec    = delay_sec;
	ioc->delay_usec   = delay_usec;
	ioc->stacksize    = 0;

	return ioc;
}

ACL_IOCTL *acl_ioctl_create(int max_threads, int idle_timeout)
{
	int   delay_sec = 0, delay_usec = 5000;

	return acl_ioctl_create_ex(ACL_EVENT_SELECT, max_threads,
			idle_timeout, delay_sec, delay_usec);
}

void acl_ioctl_ctl(ACL_IOCTL *ioc, int name, ...)
{
	va_list ap;

	va_start(ap, name);

	for (; name != ACL_IOCTL_CTL_END; name = va_arg(ap, int)) {
		switch (name) {
		case ACL_IOCTL_CTL_THREAD_MAX:
			ioc->max_threads = va_arg(ap, int);
			break;
		case ACL_IOCTL_CTL_THREAD_STACKSIZE:
			ioc->stacksize = va_arg(ap, int);
			break;
		case ACL_IOCTL_CTL_THREAD_IDLE:
			ioc->idle_timeout = va_arg(ap, int);
			break;
		case ACL_IOCTL_CTL_DELAY_SEC:
			ioc->delay_sec = va_arg(ap, int);
			if (ioc->event)
				acl_event_set_delay_sec(ioc->event, ioc->delay_sec);
			break;
		case ACL_IOCTL_CTL_DELAY_USEC:
			ioc->delay_usec = va_arg(ap, int);
			if (ioc->event)
				acl_event_set_delay_usec(ioc->event, ioc->delay_usec);
			break;
		case ACL_IOCTL_CTL_INIT_FN:
			ioc->thread_init_fn = va_arg(ap, ACL_IOCTL_THREAD_INIT_FN);
			break;
		case ACL_IOCTL_CTL_EXIT_FN:
			ioc->thread_exit_fn = va_arg(ap, ACL_IOCTL_THREAD_EXIT_FN);
			break;
		case ACL_IOCTL_CTL_INIT_CTX:
			ioc->thread_init_arg = va_arg(ap, void*);
			break;
		case ACL_IOCTL_CTL_EXIT_CTX:
			ioc->thread_exit_arg = va_arg(ap, void*);
			break;	
		default:
			acl_msg_fatal("%s(%d): unknown arg", __FILE__, __LINE__);
			/* not reached */
			break;
		}
	}

	va_end(ap);
}

void acl_ioctl_free(ACL_IOCTL *ioc)
{
	if (ioc == NULL)
		return;

	if (ioc->tp)
		acl_pthread_pool_destroy(ioc->tp);
	if (ioc->event)
		acl_event_free(ioc->event);

	acl_myfree(ioc);
}

void acl_ioctl_add_dog(ACL_IOCTL *ioc)
{
	if (ioc->max_threads > 0)
		ioc->enable_dog = 1;
	else
		ioc->enable_dog = 0;
}

static int __on_thread_init(void *arg_init)
{
	const char *myname = "__on_thread_init";
	ACL_IOCTL *ioc = (ACL_IOCTL*) arg_init;

	if (ioc->thread_init_fn == NULL)
		acl_msg_fatal("%s, %s(%d): thread_init_fn null",
			__FILE__, myname, __LINE__);

	ioc->thread_init_fn(ioc->thread_init_arg);
	return 0;
}

static void __on_thread_exit(void *arg_free)
{
	const char *myname = "__on_thread_exit";
	ACL_IOCTL *ioc = (ACL_IOCTL*) arg_free;

	if (ioc->thread_exit_fn == NULL)
		acl_msg_fatal("%s, %s(%d): thread_exit_fn null",
			__FILE__, myname, __LINE__);

	ioc->thread_exit_fn(ioc->thread_exit_arg);
}

int acl_ioctl_start(ACL_IOCTL *ioc)
{
	/* 单线程模式 */
	if (ioc->max_threads == 0) {
		ioc->tp = NULL;
		ioc->event = acl_event_new(ioc->event_mode, 0,
				ioc->delay_sec, ioc->delay_usec);
		return 0;
	}

	/* 多线程模式 */
	ioc->tp = acl_thread_pool_create(ioc->max_threads, ioc->idle_timeout);
	acl_pthread_pool_set_poller(ioc->tp, __poller_fn, ioc);

	if (ioc->thread_init_fn)
		acl_pthread_pool_atinit(ioc->tp, __on_thread_init, ioc);
	if (ioc->thread_exit_fn)
		acl_pthread_pool_atfree(ioc->tp, __on_thread_exit, ioc);

	ioc->event = acl_event_new(ioc->event_mode, 1,
			ioc->delay_sec, ioc->delay_usec);
	if (ioc->enable_dog)
		acl_event_add_dog(ioc->event);

	return acl_pthread_pool_start_poller(ioc->tp);
}

void acl_ioctl_loop(ACL_IOCTL *ioc)
{
	if (ioc && ioc->event)
		acl_event_loop(ioc->event);
}

ACL_EVENT *acl_ioctl_event(ACL_IOCTL *ioc)
{
	if (ioc)
		return (ioc->event);
	return NULL;
}

void acl_ioctl_disable_readwrite(ACL_IOCTL *ioc, ACL_VSTREAM *stream)
{
	if (ioc && ioc->event && stream)
		acl_event_disable_readwrite(ioc->event, stream);
}

void acl_ioctl_disable_read(ACL_IOCTL *ioc, ACL_VSTREAM *stream)
{
	if (ioc && ioc->event && stream)
		acl_event_disable_read(ioc->event, stream);
}

void acl_ioctl_disable_write(ACL_IOCTL *ioc, ACL_VSTREAM *stream)
{
	if (ioc && ioc->event && stream)
		acl_event_disable_write(ioc->event, stream);
}

int acl_ioctl_isset(ACL_IOCTL *ioc, ACL_VSTREAM *stream)
{
	return acl_event_isset(ioc->event, stream);
}

int acl_ioctl_isrset(ACL_IOCTL *ioc, ACL_VSTREAM *stream)
{
	return acl_event_isrset(ioc->event, stream);
}

int acl_ioctl_iswset(ACL_IOCTL *ioc, ACL_VSTREAM *stream)
{
	return acl_event_iswset(ioc->event, stream);
}

static void __free_ctx(ACL_VSTREAM *stream acl_unused, void *ctx)
{
	acl_myfree(ctx);
}

void acl_ioctl_enable_read(ACL_IOCTL *ioc, ACL_VSTREAM *stream,
	int timeout, ACL_IOCTL_NOTIFY_FN notify_fn, void *context)
{
	const char *myname = "acl_ioctl_enable_read";
	ACL_IOCTL_CTX *ctx;

	if (ioc == NULL || stream == NULL)
		acl_msg_fatal("%s(%d): input invalid", myname, __LINE__);
	
	if (stream->ioctl_read_ctx == NULL) {
		stream->ioctl_read_ctx = acl_mymalloc(sizeof(ACL_IOCTL_CTX));
		((ACL_IOCTL_CTX *) stream->ioctl_read_ctx)->stream = stream;
		acl_vstream_add_close_handle(stream, __free_ctx,
			stream->ioctl_read_ctx);
	}

	ctx = stream->ioctl_read_ctx;

	ctx->ioc        = ioc;
	ctx->notify_fn  = notify_fn;
	ctx->context    = context;
	ctx->event_type = ACL_EVENT_READ;

	/* 将数据流的状态置入事件监控集合中 */
	if (ioc->max_threads == 0)
		acl_event_enable_read(ioc->event, stream,
			timeout, read_notify_callback, (void *) ctx);
	else
		acl_event_enable_read(ioc->event, stream,
			timeout, read_notify_callback_r, (void *) ctx);
}

void acl_ioctl_enable_write(ACL_IOCTL *ioc, ACL_VSTREAM *stream,
	int timeout, ACL_IOCTL_NOTIFY_FN notify_fn, void *context)
{
	const char *myname = "acl_ioctl_enable_write";
	ACL_IOCTL_CTX *ctx;

	if (ioc == NULL || stream == NULL)
		acl_msg_fatal("%s(%d): input invalid", myname, __LINE__);
	
	if (stream->ioctl_write_ctx == NULL) {
		stream->ioctl_write_ctx = acl_mymalloc(sizeof(ACL_IOCTL_CTX));
		((ACL_IOCTL_CTX *) stream->ioctl_write_ctx)->stream = stream;
		acl_vstream_add_close_handle(stream, __free_ctx,
			stream->ioctl_write_ctx);
	}

	ctx = stream->ioctl_write_ctx;

	ctx->ioc       = ioc;
	ctx->notify_fn = notify_fn;
	ctx->context   = context;

	/* 将客户端数据流的状态置入事件监控集合中 */
	if (ioc->max_threads == 0)
		acl_event_enable_write(ioc->event, stream,
			timeout, write_notify_callback, (void *) ctx);
	else
		acl_event_enable_write(ioc->event, stream,
			timeout, write_notify_callback_r, (void *) ctx);
}

void acl_ioctl_enable_connect(ACL_IOCTL *ioc, ACL_VSTREAM *stream,
	int timeout, ACL_IOCTL_NOTIFY_FN notify_fn, void *context)
{
	const char *myname = "acl_ioctl_enable_connect";

	if (ioc == NULL || stream == NULL)
		acl_msg_fatal("%s(%d): input invalid", myname, __LINE__);

	acl_ioctl_enable_write(ioc, stream, timeout, notify_fn, context);
}

void acl_ioctl_enable_listen(ACL_IOCTL *ioc, ACL_VSTREAM *stream,
	int timeout, ACL_IOCTL_NOTIFY_FN notify_fn, void *context)
{
	const char *myname = "acl_ioctl_enable_listen";
	ACL_IOCTL_CTX *ctx;

	if (ioc == NULL || stream == NULL)
		acl_msg_fatal("%s(%d): input invalid", myname, __LINE__);

	if (stream->ioctl_read_ctx == NULL) {
		stream->ioctl_read_ctx = acl_mymalloc(sizeof(ACL_IOCTL_CTX));
		((ACL_IOCTL_CTX *) stream->ioctl_read_ctx)->stream = stream;
		acl_vstream_add_close_handle(stream, __free_ctx, stream->ioctl_read_ctx);
	}

	ctx = stream->ioctl_read_ctx;

	ctx->ioc       = ioc;
	ctx->notify_fn = notify_fn;
	ctx->context   = context;

	if (ioc->max_threads == 0)
		acl_event_enable_listen(ioc->event, stream,
			timeout, listen_notify_callback, (void *) ctx);
	else
		acl_event_enable_listen(ioc->event, stream,
			timeout, listen_notify_callback_r, (void *) ctx);
}

ACL_VSTREAM *acl_ioctl_connect(const char *addr, int timeout)
{
	ACL_VSTREAM *stream;

	if (timeout == 0)
		stream = acl_vstream_connect(addr, ACL_NON_BLOCKING, 0, 0, 4096);
	else if (timeout < 0)
		stream = acl_vstream_connect(addr, ACL_BLOCKING, 0, 0, 4096);
	else
		stream = acl_vstream_connect(addr, ACL_NON_BLOCKING, timeout, 0, 4096);

	return stream;
}

ACL_VSTREAM *acl_ioctl_listen(const char *addr, int qlen)
{
	return acl_vstream_listen(addr, qlen);
}

ACL_VSTREAM *acl_ioctl_listen_ex(const char *addr, int qlen,
	int block_mode, int io_bufsize, int io_timeout)
{
	return acl_vstream_listen_ex(addr, qlen, block_mode == ACL_BLOCKING
		? 0 : ACL_INET_FLAG_NBLOCK, io_bufsize, io_timeout);
}

ACL_VSTREAM *acl_ioctl_accept(ACL_VSTREAM *sstream, char *ipbuf, int size)
{
	return acl_vstream_accept(sstream, ipbuf, size);
}

acl_int64 acl_ioctl_request_timer(ACL_IOCTL *ioc, ACL_EVENT_NOTIFY_TIME timer_fn,
	void *context, acl_int64 idle_limit)
{
	const char *myname = "acl_ioctl_request_timer";

	if (ioc == NULL || timer_fn == NULL)
		acl_msg_fatal("%s(%d): input invalid", myname, __LINE__);
	if (ioc->event == NULL)
		acl_msg_fatal("%s(%d): ioctl's event null", myname, __LINE__);

	return acl_event_request_timer(ioc->event, timer_fn,
			context, idle_limit, 0);
}

acl_int64 acl_ioctl_cancel_timer(ACL_IOCTL *ioc,
	ACL_EVENT_NOTIFY_TIME timer_fn, void *context)
{
	const char *myname = "acl_ioctl_cancel_timer";

	if (ioc == NULL || ioc->event == NULL || timer_fn == NULL)
		acl_msg_fatal("%s(%d): input invalid", myname, __LINE__);

	return acl_event_cancel_timer(ioc->event, timer_fn, context);
}
