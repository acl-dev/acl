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
#include "thread/acl_pthread.h"
#include "net/acl_net.h"
#include "aio/acl_aio.h"

#endif
#include "stdlib/acl_array.h"
#include "../event/events.h"
#include "aio.h"

static int accept_callback(ACL_ASTREAM *astream acl_unused,
	void *context acl_unused)
{
	const char *myname = "accept_callback";

	acl_msg_fatal("%s: default accept_callback be called", myname);
	return (-1);
}

static int listen_callback(ACL_ASTREAM *astream acl_unused,
	void *context acl_unused)
{
	const char *myname = "listen_callback";

	acl_msg_fatal("%s: default listen_callback be called", myname);
	return (-1);
}

ACL_AIO *acl_aio_handle(ACL_ASTREAM *stream)
{
	if (stream == NULL)
		return (NULL);
	return (stream->aio);
}

void acl_aio_set_ctx(ACL_ASTREAM *stream, void *ctx)
{
	if (stream)
		stream->context = ctx;
}

void *acl_aio_get_ctx(ACL_ASTREAM *stream)
{
	return (stream ? stream->context : NULL);
}

ACL_ASTREAM *acl_aio_open(ACL_AIO *aio, ACL_VSTREAM *stream)
{
	ACL_ASTREAM *astream;
	
	acl_non_blocking(ACL_VSTREAM_SOCK(stream), ACL_NON_BLOCKING);

	astream = acl_mymalloc(sizeof(ACL_ASTREAM));
	
	astream->aio = aio;
	astream->stream = stream;
	stream->rw_timeout = 0; /* 防止在读时阻塞 */

	acl_vstring_init(&astream->strbuf, __default_line_length);
	astream->timeout = 0;
	astream->nrefer = 0;
	astream->flag = 0;

	/* for read */
	astream->read_nested = 0;
	astream->read_nested_limit = __AIO_NESTED_MAX;
	astream->read_ready_fn = NULL;
	astream->count = 0;
	astream->line_length = 0;
	astream->keep_read = aio->keep_read;  /* 继承异步句柄的持续读标志 */

	/* just for listen fd */
	if ((stream->type & ACL_VSTREAM_TYPE_LISTEN)) {
		if (stream->read_buf == NULL)
			stream->read_buf_len = aio->rbuf_size;
		astream->accept_nloop = 1;
	}

	/* for write */
	acl_fifo_init(&astream->write_fifo);
	astream->write_left = 0;
	astream->write_offset = 0;
	astream->write_nested = 0;
	astream->write_nested_limit = __AIO_NESTED_MAX;

	/* set default callback functions */
	astream->accept_fn = accept_callback;
	astream->listen_fn = listen_callback;
	astream->context = NULL;

	/* create callback link */
	astream->read_handles  = acl_array_create(10);
	astream->write_handles = acl_array_create(10);
	astream->close_handles = acl_array_create(10);
	astream->timeo_handles = acl_array_create(10);
	astream->connect_handles = acl_array_create(10);

	acl_fifo_init(&astream->reader_fifo);
	acl_fifo_init(&astream->writer_fifo);

	return (astream);
}

void acl_aio_set_accept_nloop(ACL_ASTREAM *astream, int accept_nloop)
{
	astream->accept_nloop = accept_nloop;
}

ACL_VSTREAM *acl_aio_cancel(ACL_ASTREAM *astream)
{
	ACL_VSTREAM *stream;

	stream = astream->stream;
	stream->flag = 0;
	acl_aio_clean_hooks(astream);
	acl_myfree(astream);
	return (stream);
}

int acl_aio_refer_value(ACL_ASTREAM * astream)
{
	const char *myname = "acl_aio_refer_value";

	if (astream == NULL) {
		acl_msg_error("%s(%d): astream null", myname, __LINE__);
		return (-1);
	}
	return (astream->nrefer);
}

void acl_aio_refer(ACL_ASTREAM *astream)
{
	if (astream)
		astream->nrefer++;
}

void acl_aio_unrefer(ACL_ASTREAM *astream)
{
	if (astream)
		astream->nrefer--;
}

static void aio_disable_readwrite(ACL_AIO *aio, ACL_ASTREAM *astream)
{
	if (astream->stream == NULL)
		return;

	if ((astream->flag & ACL_AIO_FLAG_ISRD) != 0) {
		astream->flag &= ~ACL_AIO_FLAG_ISRD; 
		acl_event_disable_read(aio->event, astream->stream);
	}
	if ((astream->flag & ACL_AIO_FLAG_ISWR) != 0) {
		astream->flag &= ~ACL_AIO_FLAG_ISWR;
		acl_event_disable_write(aio->event, astream->stream);
	}
}

static void close_astream(ACL_ASTREAM *astream)
{
	if (astream->stream) {
		if ((astream->flag & ACL_AIO_FLAG_ISRD) != 0) {
			astream->flag &= ~ACL_AIO_FLAG_ISRD; 
			acl_event_disable_read(astream->aio->event,
				astream->stream);
		}
		if ((astream->flag & ACL_AIO_FLAG_ISWR) != 0) {
			astream->flag &= ~ACL_AIO_FLAG_ISWR;
			acl_event_disable_write(astream->aio->event,
				astream->stream);
		}
		acl_vstream_close(astream->stream);
	}
	acl_aio_clean_hooks(astream);

	/* bugfix: 在 acl_aio_clean_hooks 中并不会释放数组对象 --zsx, 2012.7.2 */
	acl_array_destroy(astream->read_handles, NULL);
	acl_array_destroy(astream->write_handles, NULL);
	acl_array_destroy(astream->close_handles, NULL);
	acl_array_destroy(astream->timeo_handles, NULL);
	acl_array_destroy(astream->connect_handles, NULL);

	acl_myfree(astream);
}

static void aio_delay_close(ACL_ASTREAM *astream)
{
	if (astream->nrefer > 0) {
		/* 如果该异步流对象的引用计数大于0，则只需要置标志位 */
		astream->flag |= ACL_AIO_FLAG_IOCP_CLOSE;
		/* 放在延迟关闭队列中 */
		if ((astream->flag & ACL_AIO_FLAG_DELAY_CLOSE) == 0) {
			astream->aio->dead_streams->push_back(
				astream->aio->dead_streams, astream);
			astream->flag |= ACL_AIO_FLAG_DELAY_CLOSE;
		}
		return;
	} else if (astream->stream == NULL) {
		/* 需要关闭该异步流对象 */
		aio_close_callback(astream);
		close_astream(astream);
		return;
	}

	astream->flag &= ~ACL_AIO_FLAG_IOCP_CLOSE;
	astream->nrefer++;
	acl_assert(astream->nrefer > 0);
	aio_close_callback(astream);
	astream->nrefer--;

	/* 真正关闭异步流 */
	close_astream(astream);
}

void aio_delay_check(ACL_AIO *aio)
{
	ACL_ASTREAM *astream;

	while (1) {
		astream = (ACL_ASTREAM*)
			aio->dead_streams->pop_back(aio->dead_streams);
		if (astream == NULL)
			break;

		aio_delay_close(astream);
	}
}

#define ACL_USE_EVENT_TIMER

#ifdef ACL_WINDOWS

# ifdef ACL_USE_TLS_POOL

static acl_pthread_key_t __aio_tls_key = ACL_TLS_OUT_OF_INDEXES;

static VOID CALLBACK CloseTimer(HWND hwnd, UINT uMsg,
	UINT_PTR idEvent, DWORD dwTime)
{
	const char *myname = "CloseTimer";
	ACL_AIO *tls_aio;

	KillTimer(hwnd, idEvent);

	if (__aio_tls_key == ACL_TLS_OUT_OF_INDEXES)
		acl_msg_fatal("%s(%d): __aio_tls_key invalid",
			myname, __LINE__);

	tls_aio = acl_pthread_tls_get(&__aio_tls_key);
	if (tls_aio == NULL)
		acl_msg_fatal("%s(%d): get tls aio error(%s), tls_key: %d",
			myname, __LINE__, acl_last_serror(),
			(int) __aio_tls_key);
	aio_delay_check(tls_aio);
}

# elif defined(ACL_USE_EVENT_TIMER)

static void CloseTimer(int event_type acl_unused, ACL_EVENT *event, void *ctx)
{
	ACL_AIO *aio = (ACL_AIO*) ctx;

	aio_delay_check(aio);
	aio->timer_active = 0;
}

# else

static acl_pthread_key_t  __aio_tls_key = ACL_TLS_OUT_OF_INDEXES;
static acl_pthread_once_t __aio_once_control = ACL_PTHREAD_ONCE_INIT;

static VOID CALLBACK CloseTimer(HWND hwnd, UINT uMsg,
	UINT_PTR idEvent, DWORD dwTime)
{
	const char *myname = "CloseTimer";
	ACL_AIO *tls_aio;

	KillTimer(hwnd, idEvent);

	if (__aio_tls_key == ACL_TLS_OUT_OF_INDEXES)
		acl_msg_fatal("%s(%d): __aio_tls_key invalid",
			myname, __LINE__);

	tls_aio = acl_pthread_getspecific(__aio_tls_key);
	if (tls_aio == NULL)
		acl_msg_fatal("%s(%d): get tls aio error(%s), tls_key: %d",
		myname, __LINE__, acl_last_serror(), (int) __aio_tls_key);
	aio_delay_check(tls_aio);
	tls_aio->timer_active = 0;
}

static void finish_thread_aio(void *arg acl_unused)
{

}

static void init_thread_aio(void)
{
	acl_pthread_key_create(&__aio_tls_key, finish_thread_aio);
}

# endif  /* ACL_USE_TLS_POOL */

#endif  /* ACL_WINDOWS */

/* 该函数非常关键，采用的IO完成时才关闭的策略，防止重复关闭 */
void acl_aio_iocp_close(ACL_ASTREAM *astream)
{
	const char *myname = "acl_aio_iocp_close";
	ACL_ITER iter;
	ACL_AIO *aio = astream->aio;

	if ((astream->flag & ACL_AIO_FLAG_DELAY_CLOSE))
		return;
	if (!(astream->flag & ACL_AIO_FLAG_DEAD)
		&& (astream->flag & ACL_AIO_FLAG_ISWR))
	{
		astream->flag |= ACL_AIO_FLAG_IOCP_CLOSE;
		return;
	}

	acl_foreach(iter, aio->dead_streams) {
		ACL_ASTREAM *s = (ACL_ASTREAM*) iter.data;
		if (s == astream)
			acl_msg_fatal("%s(%d): flag: %d, size: %d",
				myname, __LINE__, astream->flag, iter.size);
	}
	/* 放在延迟关闭队列中 */
	aio->dead_streams->push_back(aio->dead_streams, astream);
	astream->flag |= ACL_AIO_FLAG_DELAY_CLOSE;
	aio_disable_readwrite(aio, astream);
#ifdef ACL_WINDOWS

# ifdef ACL_USE_TLS_POOL

	if (aio->event_mode == ACL_EVENT_WMSG && !aio->timer_active) {
		HWND hWnd = acl_event_wmsg_hwnd(aio->event);
		ACL_AIO *tls_aio = acl_pthread_tls_get(&__aio_tls_key);

		acl_assert(hWnd != NULL);
		if (__aio_tls_key == ACL_TLS_OUT_OF_INDEXES)
			acl_msg_fatal("%s(%d): __tls_key invalid",
				myname, __LINE__);
		if (tls_aio == NULL)
			acl_pthread_tls_set(__aio_tls_key, aio, NULL);
		else if (tls_aio != aio)
			acl_msg_fatal("%s(%d): tls_aio != aio",
				myname, __LINE__);
		aio->timer_active = 1;
		SetTimer(hWnd, aio->tid, 1000, CloseTimer);
	}
# elif defined(ACL_USE_EVENT_TIMER)

	if (aio->event_mode == ACL_EVENT_WMSG && !aio->timer_active) {
		acl_event_request_timer(aio->event, CloseTimer, aio, 1000, 0);
		aio->timer_active = 1;
	}

# else

	if (aio->event_mode == ACL_EVENT_WMSG && !aio->timer_active) {
		HWND hWnd = acl_event_wmsg_hwnd(aio->event);
		ACL_AIO *tls_aio;

		acl_assert(hWnd != NULL);

		(void) acl_pthread_once(&__aio_once_control, init_thread_aio);

		if (__aio_tls_key == ACL_TLS_OUT_OF_INDEXES)
			acl_msg_fatal("%s(%d): __tls_key invalid",
				myname, __LINE__);

		tls_aio = acl_pthread_getspecific(__aio_tls_key);
		if (tls_aio == NULL)
			acl_pthread_setspecific(__aio_tls_key, aio);
		else if (tls_aio != aio)
			acl_msg_fatal("%s(%d): tls_aio != aio",
				myname, __LINE__);

		aio->timer_active = 1;
		SetTimer(hWnd, aio->tid, 1, CloseTimer);
	}

# endif  /* ACL_USE_TLS_POOL */

#endif  /* ACL_WINDOWS */
}

void acl_aio_add_read_hook(ACL_ASTREAM *astream,
	ACL_AIO_READ_FN callback, void *ctx)
{
	const char *myname = "acl_aio_add_read_hook";
	AIO_READ_HOOK *handle;
	ACL_ITER iter;

	acl_assert(callback);

	acl_foreach(iter, &astream->reader_fifo) {
		handle = (AIO_READ_HOOK *) iter.data;
		if (handle->callback == callback) {
			handle->disable = 0;
			handle->ctx = ctx;
			return;
		}
	}

	handle = acl_mymalloc(sizeof(AIO_READ_HOOK));
	handle->callback = callback;
	handle->ctx = ctx;
	handle->disable = 0;
	if (acl_array_append(astream->read_handles, handle) < 0)
		acl_msg_fatal("%s(%d), %s: add to array error",
			__FILE__, __LINE__, myname);
}

void acl_aio_add_write_hook(ACL_ASTREAM *astream,
	ACL_AIO_WRITE_FN callback, void *ctx)
{
	const char *myname = "acl_aio_add_write_hook";
	AIO_WRITE_HOOK *handle;
	ACL_ITER iter;

	acl_assert(callback);

	acl_foreach(iter, &astream->writer_fifo) {
		handle = (AIO_WRITE_HOOK *) iter.data;
		if (handle->callback == callback) {
			handle->disable = 0;
			handle->ctx = ctx;
			return;
		}
	}

	handle = acl_mymalloc(sizeof(AIO_WRITE_HOOK));
	handle->callback = callback;
	handle->ctx = ctx;
	handle->disable = 0;
	if (acl_array_append(astream->write_handles, handle) < 0)
		acl_msg_fatal("%s(%d), %s: add to array error",
			__FILE__, __LINE__, myname);
}

void acl_aio_add_close_hook(ACL_ASTREAM *astream,
	ACL_AIO_CLOSE_FN callback, void *ctx)
{
	const char *myname = "acl_aio_add_close_hook";
	AIO_CLOSE_HOOK *handle;
	ACL_ITER iter;

	acl_assert(callback);

	acl_foreach(iter, astream->close_handles) {
		handle = (AIO_CLOSE_HOOK *) iter.data;
		if (handle->callback == callback) {
			handle->disable = 0;
			handle->ctx = ctx;
			return;
		}
	}

	handle = acl_mymalloc(sizeof(AIO_CLOSE_HOOK));
	handle->callback = callback;
	handle->ctx = ctx;
	handle->disable = 0;
	if (acl_array_append(astream->close_handles, handle) < 0)
		acl_msg_fatal("%s(%d), %s: add to array error",
			__FILE__, __LINE__, myname);
}

void acl_aio_add_timeo_hook(ACL_ASTREAM *astream,
	ACL_AIO_TIMEO_FN callback, void *ctx)
{
	const char *myname = "acl_aio_add_timeo_hook";
	AIO_TIMEO_HOOK *handle;
	ACL_ITER iter;

	acl_assert(callback);

	acl_foreach(iter, astream->timeo_handles) {
		handle = (AIO_TIMEO_HOOK *) iter.data;
		if (handle->callback == callback) {
			handle->disable = 0;
			handle->ctx = ctx;
			return;
		}
	}

	handle = acl_mymalloc(sizeof(AIO_TIMEO_HOOK));
	handle->callback = callback;
	handle->ctx = ctx;
	handle->disable = 0;
	if (acl_array_append(astream->timeo_handles, handle) < 0)
		acl_msg_fatal("%s(%d), %s: add to array error",
			__FILE__, __LINE__, myname);
}

void acl_aio_add_connect_hook(ACL_ASTREAM *astream,
	ACL_AIO_CONNECT_FN callback, void *ctx)
{
	const char *myname = "acl_aio_add_connect_hook";
	AIO_CONNECT_HOOK *handle;
	ACL_ITER iter;

	acl_assert(callback);

	acl_foreach(iter, astream->connect_handles) {
		handle = (AIO_CONNECT_HOOK *) iter.data;
		if (handle->callback == callback) {
			handle->disable = 0;
			handle->ctx = ctx;
			return;
		}
	}

	handle = acl_mymalloc(sizeof(AIO_TIMEO_HOOK));
	handle->callback = callback;
	handle->ctx = ctx;
	handle->disable = 0;
	if (acl_array_append(astream->connect_handles, handle) < 0)
		acl_msg_fatal("%s(%d), %s: add to array error",
			__FILE__, __LINE__, myname);
}

void acl_aio_del_read_hook(ACL_ASTREAM *astream,
	ACL_AIO_READ_FN callback, void *ctx)
{
	ACL_ITER iter;

	acl_foreach(iter, astream->read_handles) {
		AIO_READ_HOOK *handle = (AIO_READ_HOOK *) iter.data;
		if (handle->callback == callback && handle->ctx == ctx) {
			handle->disable = 1;
			handle->ctx = NULL;
			return;
		}
	}
	acl_foreach(iter, &astream->reader_fifo) {
		AIO_READ_HOOK *handle = (AIO_READ_HOOK*) iter.data;
		if (handle->callback == callback && handle->ctx == ctx) {
			handle->disable = 1;
			handle->ctx = NULL;
			break;
		}
	}
}

void acl_aio_del_write_hook(ACL_ASTREAM *astream,
	ACL_AIO_WRITE_FN callback, void *ctx)
{
	ACL_ITER iter;

	acl_foreach(iter, astream->write_handles) {
		AIO_WRITE_HOOK *handle = (AIO_WRITE_HOOK *) iter.data;
		if (handle->callback == callback && handle->ctx == ctx) {
			handle->disable = 1;
			handle->ctx = NULL;
			return;
		}
	}
	acl_foreach(iter, &astream->writer_fifo) {
		AIO_WRITE_HOOK *handle = (AIO_WRITE_HOOK*) iter.data;
		if (handle->callback == callback && handle->ctx == ctx) {
			handle->disable = 1;
			handle->ctx = NULL;
			break;
		}
	}
}

void acl_aio_del_close_hook(ACL_ASTREAM *astream,
	ACL_AIO_CLOSE_FN callback, void *ctx)
{
	ACL_ITER iter;

	acl_foreach(iter, astream->close_handles) {
		AIO_CLOSE_HOOK *handle = (AIO_CLOSE_HOOK *) iter.data;
		if (handle->callback == callback && handle->ctx == ctx) {
			handle->disable = 1;
			handle->ctx = NULL;
			break;
		}
	}
}

void acl_aio_del_timeo_hook(ACL_ASTREAM *astream,
	ACL_AIO_TIMEO_FN callback, void *ctx)
{
	ACL_ITER iter;

	acl_foreach(iter, astream->timeo_handles) {
		AIO_TIMEO_HOOK *handle = (AIO_TIMEO_HOOK *) iter.data;
		if (handle->callback == callback && handle->ctx == ctx) {
			handle->disable = 1;
			handle->ctx = NULL;
			break;
		}
	}
}

void acl_aio_del_connect_hook(ACL_ASTREAM *astream,
	ACL_AIO_CONNECT_FN callback, void *ctx)
{
	ACL_ITER iter;

	acl_foreach(iter, astream->connect_handles) {
		AIO_CONNECT_HOOK *handle = (AIO_CONNECT_HOOK *) iter.data;
		if (handle->callback == callback && handle->ctx == ctx) {
			handle->disable = 1;
			handle->ctx = NULL;
			break;
		}
	}
}

static void free_handle(void *handle)
{
	acl_myfree(handle);
}

void acl_aio_clean_read_hooks(ACL_ASTREAM *astream)
{
	acl_array_clean(astream->read_handles, free_handle);
	while (1) {
		AIO_READ_HOOK *handle = astream->reader_fifo.pop_back(
			&astream->reader_fifo);
		if (handle == NULL)
			break;
		free_handle(handle);
	}
	acl_vstring_free_buf(&astream->strbuf);
}

void acl_aio_clean_write_hooks(ACL_ASTREAM *astream)
{
	acl_array_clean(astream->write_handles, free_handle);
	while (1) {
		AIO_WRITE_HOOK *handle = astream->writer_fifo.pop_back(
			&astream->writer_fifo);
		if (handle == NULL)
			break;
		free_handle(handle);
	}
	while (1) {
		ACL_VSTRING *str = (ACL_VSTRING*) acl_fifo_pop(
			&astream->write_fifo);
		if (str == NULL)
			break;
		acl_vstring_free(str);
	}
}

void acl_aio_clean_close_hooks(ACL_ASTREAM *astream)
{
	acl_array_clean(astream->close_handles, free_handle);
}

void acl_aio_clean_timeo_hooks(ACL_ASTREAM *astream)
{
	acl_array_clean(astream->timeo_handles, free_handle);
}

void acl_aio_clean_connect_hooks(ACL_ASTREAM *astream)
{
	acl_array_clean(astream->connect_handles, free_handle);
}

void acl_aio_clean_hooks(ACL_ASTREAM *astream)
{
	acl_aio_clean_read_hooks(astream);
	acl_aio_clean_write_hooks(astream);
	acl_aio_clean_close_hooks(astream);
	acl_aio_clean_timeo_hooks(astream);
	acl_aio_clean_connect_hooks(astream);
}

void acl_aio_stream_set_keep_read(ACL_ASTREAM *astream, int onoff)
{
	astream->keep_read = onoff;
}

int acl_aio_stream_get_keep_read(ACL_ASTREAM *astream)
{
	return (astream->keep_read);
}

void acl_aio_ctl(ACL_ASTREAM *astream, int name, ...)
{
	const char *myname = "acl_aio_ctl";
	va_list ap;
	ACL_AIO_READ_FN    read_fn;
	ACL_AIO_WRITE_FN   write_fn;
	ACL_AIO_ACCEPT_FN  accept_fn;
	ACL_AIO_LISTEN_FN  listen_fn;
	ACL_AIO_CONNECT_FN connect_fn;
	ACL_AIO_TIMEO_FN   timeo_fn;
	ACL_AIO_CLOSE_FN   close_fn;
	ACL_VSTREAM *stream;
	void *ctx;

	if (astream == NULL) {
		acl_msg_error("%s: astream null", myname);
		return;
	}

	va_start(ap, name);

	for (; name != ACL_AIO_CTL_END; name = va_arg(ap, int)) {
		switch (name) {
		case ACL_AIO_CTL_ACCEPT_FN:
			accept_fn = va_arg(ap, ACL_AIO_ACCEPT_FN);
			if (accept_fn)
				astream->accept_fn = accept_fn;
			else
				astream->accept_fn = accept_callback;
			break;
		case ACL_AIO_CTL_LISTEN_FN:
			listen_fn = va_arg(ap, ACL_AIO_LISTEN_FN);
			if (listen_fn)
				astream->listen_fn = listen_fn;
			else
				astream->listen_fn = listen_callback;
			break;
		case ACL_AIO_CTL_CTX:
			astream->context = va_arg(ap, void *);
			break;
		case ACL_AIO_CTL_TIMEOUT:
			astream->timeout = va_arg(ap, int);
			break;
		case ACL_AIO_CTL_LINE_LENGTH:
			astream->line_length = va_arg(ap, int);
			break;
		case ACL_AIO_CTL_STREAM:
			stream = va_arg(ap, ACL_VSTREAM*);
			astream->stream = stream;
			break;
		case ACL_AIO_CTL_READ_NESTED:
			astream->read_nested_limit = va_arg(ap, int);
			if (astream->read_nested_limit < 0)
				astream->read_nested_limit = 0;
			break;
		case ACL_AIO_CTL_WRITE_NESTED:
			astream->write_nested_limit = va_arg(ap, int);
			if (astream->write_nested_limit < 0)
				astream->write_nested_limit = 0;
			break;
		case ACL_AIO_CTL_KEEP_READ:
			astream->keep_read = va_arg(ap, int);
			break;
		case ACL_AIO_CTL_READ_HOOK_ADD:
			read_fn = va_arg(ap, ACL_AIO_READ_FN);
			ctx = va_arg(ap, void *);
			acl_aio_add_read_hook(astream, read_fn, ctx);
			break;
		case ACL_AIO_CTL_READ_HOOK_DEL:
			read_fn = va_arg(ap, ACL_AIO_READ_FN);
			ctx = va_arg(ap, void *);
			acl_aio_del_read_hook(astream, read_fn, ctx);
			break;
		case ACL_AIO_CTL_WRITE_HOOK_ADD:
			write_fn = va_arg(ap, ACL_AIO_WRITE_FN);
			ctx = va_arg(ap, void *);
			acl_aio_add_write_hook(astream, write_fn, ctx);
			break;
		case ACL_AIO_CTL_WRITE_HOOK_DEL:
			write_fn = va_arg(ap, ACL_AIO_WRITE_FN);
			ctx = va_arg(ap, void *);
			acl_aio_del_write_hook(astream, write_fn, ctx);
			break;
		case ACL_AIO_CTL_CLOSE_HOOK_ADD:
			close_fn = va_arg(ap, ACL_AIO_CLOSE_FN);
			ctx = va_arg(ap, void *);
			acl_aio_add_close_hook(astream, close_fn, ctx);
			break;
		case ACL_AIO_CTL_CLOSE_HOOK_DEL:
			close_fn = va_arg(ap, ACL_AIO_CLOSE_FN);
			ctx = va_arg(ap, void *);
			acl_aio_del_close_hook(astream, close_fn, ctx);
			break;
		case ACL_AIO_CTL_TIMEO_HOOK_ADD:
			timeo_fn = va_arg(ap, ACL_AIO_TIMEO_FN);
			ctx = va_arg(ap, void *);
			acl_aio_add_timeo_hook(astream, timeo_fn, ctx);
			break;
		case ACL_AIO_CTL_TIMEO_HOOK_DEL:
			timeo_fn = va_arg(ap, ACL_AIO_TIMEO_FN);
			ctx = va_arg(ap, void *);
			acl_aio_del_timeo_hook(astream, timeo_fn, ctx);
			break;
		case ACL_AIO_CTL_CONNECT_HOOK_ADD:
			connect_fn = va_arg(ap, ACL_AIO_CONNECT_FN);
			ctx = va_arg(ap, void *);
			acl_aio_add_connect_hook(astream, connect_fn, ctx);
			break;
		case ACL_AIO_CTL_CONNECT_HOOK_DEL:
			connect_fn = va_arg(ap, ACL_AIO_CONNECT_FN);
			ctx = va_arg(ap, void *);
			acl_aio_del_connect_hook(astream, connect_fn, ctx);
			break;
		default:
			acl_msg_fatal("%s(%d): unknown name flag(%d)",
				myname, __LINE__, name);
			break;
		}
	}

	va_end(ap);
}

ACL_VSTREAM *acl_aio_vstream(ACL_ASTREAM *astream)
{
	if (astream && astream->stream)
		return (astream->stream);

	return (NULL);
}

void acl_aio_disable_readwrite(ACL_ASTREAM *astream)
{
	acl_aio_disable_read(astream);
	acl_aio_disable_write(astream);
}

int acl_aio_isset(ACL_ASTREAM *astream)
{
	const char *myname = "acl_aio_isset";

	if (astream == NULL)
		acl_msg_fatal("%s: input invalid", myname);
	else if (astream->stream == NULL)
		return (0);

	return (acl_event_isset(astream->aio->event, astream->stream));
}
