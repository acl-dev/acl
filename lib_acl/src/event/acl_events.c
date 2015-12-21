#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <time.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_msg.h"
#include "stdlib/acl_ring.h"
#include "stdlib/acl_vstream.h"
#include "stdlib/acl_iostuff.h"
#include "event/acl_events.h"

#endif

#include "events_define.h"
#include "events_epoll.h"
#include "events_devpoll.h"
#include "events_iocp.h"
#include "events_wmsg.h"
#include "events.h"

static void event_init(ACL_EVENT *eventp, int fdsize,
	int delay_sec, int delay_usec)
{
	eventp->fdsize = fdsize;
	/* eventp->fdtab_free_cnt = 0; */
	eventp->fdcnt  = 0;
	eventp->ready_cnt  = 0;
	eventp->fdtabs = (ACL_EVENT_FDTABLE **)
		acl_mycalloc(fdsize,sizeof(ACL_EVENT_FDTABLE *));
	eventp->ready = (ACL_EVENT_FDTABLE **)
		acl_mycalloc(fdsize, sizeof(ACL_EVENT_FDTABLE *));

	eventp->maxfd  = 0;
	eventp->nested = 0;

	eventp->delay_sec  = delay_sec + delay_usec / 1000000;
	eventp->delay_usec = delay_usec % 1000000;

	acl_ring_init(&eventp->timer_head);
	eventp->timer_keep = 0;
	SET_TIME(eventp->present);
	SET_TIME(eventp->last_debug);

	eventp->check_inter = 100000;  /* default: 100 ms */
	eventp->read_ready = 0;

	if (eventp->init_fn)
		eventp->init_fn(eventp);
}

static int event_limit(int fdsize)
{
	const char *myname = "event_limit";

#ifdef ACL_UNIX
	if ((fdsize = acl_open_limit(fdsize)) < 0) {
		acl_msg_fatal("%s: unable to determine open file limit, err=%s",
			myname, acl_last_serror());
	}
#else
	if (fdsize == 0)
		fdsize = 1024;
#endif
	if ((unsigned) (fdsize) < FD_SETSIZE / 2 && fdsize < 256)
		acl_msg_warn("%s: could allocate space for only %d open files",
			myname, fdsize);

	acl_msg_info("%s: max fdsize: %d", myname, fdsize);

	return fdsize;
}

ACL_EVENT *acl_event_new_select(int delay_sec, int delay_usec)
{
	ACL_EVENT *eventp;
	int   fdsize;

	fdsize = event_limit(FD_SETSIZE);
	eventp = event_new_select();
	event_init(eventp, fdsize, delay_sec, delay_usec);
	return eventp;
}

ACL_EVENT *acl_event_new_select_thr(int delay_sec, int delay_usec)
{
	ACL_EVENT *eventp;
	int   fdsize;

	fdsize = event_limit(FD_SETSIZE);
	eventp = event_new_select_thr();
	event_init(eventp, fdsize, delay_sec, delay_usec);
	return eventp;
}

ACL_EVENT *acl_event_new_poll(int delay_sec, int delay_usec)
{
#ifdef	ACL_EVENTS_POLL_STYLE
	ACL_EVENT *eventp;
	int   fdsize;

	fdsize = event_limit(0);
	eventp = event_new_poll(fdsize);
	event_init(eventp, fdsize, delay_sec, delay_usec);
	return eventp;
#else
	const char *myname = "acl_event_new_poll";

	(void) delay_sec;
	(void) delay_usec;
	acl_msg_fatal("%s(%d): not support!", myname, __LINE__);
	return NULL;
#endif
}

ACL_EVENT *acl_event_new_poll_thr(int delay_sec, int delay_usec)
{
#ifdef	ACL_EVENTS_POLL_STYLE
	ACL_EVENT *eventp;
	int   fdsize;

	fdsize = event_limit(0);
	eventp = event_poll_alloc_thr(fdsize);
	event_init(eventp, fdsize, delay_sec, delay_usec);
	return eventp;
#else
	const char *myname = "acl_event_new_poll_thr";

	(void) delay_sec;
	(void) delay_usec;
	acl_msg_fatal("%s(%d): not support!", myname, __LINE__);
	return NULL;
#endif
}

ACL_EVENT *acl_event_new_kernel(int delay_sec, int delay_usec)
{
#ifdef	ACL_EVENTS_KERNEL_STYLE
	ACL_EVENT *eventp;
	int   fdsize;

	fdsize = event_limit(0);
	eventp = event_new_kernel(fdsize);
	event_init(eventp, fdsize, delay_sec, delay_usec);
	return eventp;
#elif defined(ACL_EVENTS_STYLE_IOCP)
	ACL_EVENT *eventp;
	int   fdsize;

	/* 在 ACL_WINDOWS 下的 iocp 可以支撑更大的连接，默认设为 102400 个连接
	 */
	fdsize = 102400;
	eventp = event_new_iocp(fdsize);
	event_init(eventp, fdsize, delay_sec, delay_usec);
	return eventp;
#else
	const char *myname = "acl_event_new_kernel";

	(void) delay_sec;
	(void) delay_usec;
	acl_msg_fatal("%s(%d): not support!", myname, __LINE__);
	return NULL;
#endif
}

ACL_EVENT *acl_event_new_kernel_thr(int delay_sec, int delay_usec)
{
#ifdef	ACL_EVENTS_KERNEL_STYLE
	ACL_EVENT *eventp;
	int   fdsize;

	fdsize = event_limit(0);
#if (ACL_EVENTS_KERNEL_STYLE == ACL_EVENTS_STYLE_EPOLL)
	eventp = event_epoll_alloc_thr(fdsize);
#else
	eventp = event_new_kernel_thr(fdsize);
#endif
	event_init(eventp, fdsize, delay_sec, delay_usec);
	return eventp;
#else
	const char *myname = "acl_event_new_kernel_thr";

	(void) delay_sec;
	(void) delay_usec;
	acl_msg_fatal("%s(%d): not support!", myname, __LINE__);
	return NULL;
#endif
}

ACL_EVENT *acl_event_new_wmsg(unsigned int nMsg)
{
#ifdef	ACL_EVENTS_STYLE_WMSG
	ACL_EVENT *eventp;
	int   fdsize;

	fdsize = event_limit(0);
	eventp = event_new_wmsg(nMsg);
	event_init(eventp, fdsize, 0, 0);
	return eventp;
#else
	const char *myname = "acl_event_new_kernel";

	(void) nMsg;
	acl_msg_fatal("%s(%d): not support!", myname, __LINE__);
	return NULL;
#endif
}

ACL_EVENT *acl_event_new(int event_mode, int use_thr,
	int delay_sec, int delay_usec)
{
	const char *myname = "acl_event_new";
	ACL_EVENT *eventp = NULL;

	if (use_thr) {
		switch (event_mode) {
		case ACL_EVENT_SELECT:
			eventp = acl_event_new_select_thr(delay_sec,
					delay_usec);
			break;
		case ACL_EVENT_KERNEL:
			eventp = acl_event_new_kernel_thr(delay_sec,
					delay_usec);
			break;
		case ACL_EVENT_POLL:
			eventp = acl_event_new_poll_thr(delay_sec, delay_usec);
			break;
		default:
			acl_msg_fatal("%s(%d): unsupport %d event",
				myname, __LINE__, event_mode);
			break;
		}
	} else {
		switch (event_mode) {
		case ACL_EVENT_SELECT:
			eventp = acl_event_new_select(delay_sec, delay_usec);
			break;
		case ACL_EVENT_KERNEL:
			eventp = acl_event_new_kernel(delay_sec, delay_usec);
			break;
		case ACL_EVENT_POLL:
			eventp = acl_event_new_poll(delay_sec, delay_usec);
			break;
		case ACL_EVENT_WMSG:
			/* 使用该值作为消息号 */
			eventp = acl_event_new_wmsg((unsigned int) delay_sec);
			break;
		default:
			acl_msg_fatal("%s(%d): unsupport %d event",
				myname, __LINE__, event_mode);
			break;
		}
	}

	return eventp;
}

void acl_event_set_check_inter(ACL_EVENT *eventp, int n)
{
	if (n >= 0)
		eventp->check_inter = ((acl_int64) n) * 1000;
}

void acl_event_set_fire_hook(ACL_EVENT *eventp,
	void (*fire_begin)(ACL_EVENT*, void*),
	void (*fire_end)(ACL_EVENT*, void*), void* ctx)
{
	eventp->fire_begin = fire_begin;
	eventp->fire_end = fire_end;
	eventp->fire_ctx = ctx;
}

void acl_event_add_dog(ACL_EVENT *eventp)
{
	eventp->add_dog_fn(eventp);
}

void acl_event_free(ACL_EVENT *eventp)
{
	void (*free_fn)(ACL_EVENT *) = eventp->free_fn;
	ACL_EVENT_TIMER *timer;

	while ((timer = ACL_FIRST_TIMER(&eventp->timer_head)) != 0) {
		acl_ring_detach(&timer->ring);
		acl_myfree(timer);
	}

	acl_myfree(eventp->fdtabs);
	acl_myfree(eventp->ready);
	free_fn(eventp);
}

acl_int64 acl_event_time(ACL_EVENT *eventp)
{
	return eventp->present;
}

void acl_event_enable_read(ACL_EVENT *eventp, ACL_VSTREAM *stream,
	int read_timeout, ACL_EVENT_NOTIFY_RDWR callback, void *context)
{
	const char *myname = "acl_event_enable_read";
	ACL_SOCKET sockfd = ACL_VSTREAM_SOCK(stream);
	if (sockfd == ACL_SOCKET_INVALID)
		acl_msg_fatal("%s(%d): sockfd(%d) invalid",
			myname, __LINE__, sockfd);
	eventp->enable_read_fn(eventp, stream, read_timeout,
			callback, context);
}

void acl_event_enable_write(ACL_EVENT *eventp, ACL_VSTREAM *stream,
	int write_timeout, ACL_EVENT_NOTIFY_RDWR callback, void *context)
{
	const char *myname = "acl_event_enable_write";
	ACL_SOCKET sockfd = ACL_VSTREAM_SOCK(stream);
	if (sockfd == ACL_SOCKET_INVALID)
		acl_msg_fatal("%s(%d): sockfd(%d) invalid",
			myname, __LINE__, sockfd);
	eventp->enable_write_fn(eventp, stream, write_timeout,
			callback, context);
}

void acl_event_enable_listen(ACL_EVENT *eventp, ACL_VSTREAM *stream,
	int read_timeout, ACL_EVENT_NOTIFY_RDWR callback, void *context)
{
	eventp->enable_listen_fn(eventp, stream, read_timeout,
			callback, context);
}

void acl_event_disable_read(ACL_EVENT *eventp, ACL_VSTREAM *stream)
{
	const char *myname = "acl_event_disable_read";
	ACL_SOCKET sockfd = ACL_VSTREAM_SOCK(stream);
	if (sockfd == ACL_SOCKET_INVALID)
		acl_msg_fatal("%s(%d): sockfd(%d) invalid",
			myname, __LINE__, sockfd);
	eventp->disable_read_fn(eventp, stream);
}

void acl_event_disable_write(ACL_EVENT *eventp, ACL_VSTREAM *stream)
{
	const char *myname = "acl_event_disable_write";
	ACL_SOCKET sockfd = ACL_VSTREAM_SOCK(stream);
	if (sockfd == ACL_SOCKET_INVALID)
		acl_msg_fatal("%s(%d): sockfd(%d) invalid",
			myname, __LINE__, sockfd);
	eventp->disable_write_fn(eventp, stream);
}

void acl_event_disable_readwrite(ACL_EVENT *eventp, ACL_VSTREAM *stream)
{
	eventp->disable_readwrite_fn(eventp, stream);
}

int acl_event_isset(ACL_EVENT *eventp, ACL_VSTREAM *stream)
{
	return acl_event_isrset(eventp, stream)
		|| acl_event_iswset(eventp, stream);
}

int acl_event_isrset(ACL_EVENT *eventp, ACL_VSTREAM *stream)
{
	return eventp->isrset_fn(eventp, stream);
}

int acl_event_iswset(ACL_EVENT *eventp, ACL_VSTREAM *stream)
{
	return eventp->iswset_fn(eventp, stream);
}

int acl_event_isxset(ACL_EVENT *eventp, ACL_VSTREAM *stream)
{
	return eventp->isxset_fn(eventp, stream);
}

acl_int64 acl_event_request_timer(ACL_EVENT *eventp,
	ACL_EVENT_NOTIFY_TIME callback, void *context,
	acl_int64 delay, int keep)
{
	const char *myname = "acl_event_request_timer";

	if (delay < 0)
		acl_msg_panic("%s: invalid delay: %lld", myname, delay);

	return eventp->timer_request(eventp, callback, context, delay, keep);
}

acl_int64 acl_event_cancel_timer(ACL_EVENT *eventp,
	ACL_EVENT_NOTIFY_TIME callback, void *context)
{
	return eventp->timer_cancel(eventp, callback, context);
}

void acl_event_keep_timer(ACL_EVENT *eventp,
	ACL_EVENT_NOTIFY_TIME callback, void *context, int onoff)
{
	eventp->timer_keep(eventp, callback, context, onoff);
}

int acl_event_timer_ifkeep(ACL_EVENT *eventp,
	ACL_EVENT_NOTIFY_TIME callback, void *context)
{
	return eventp->timer_ifkeep(eventp, callback, context);
}

void acl_event_loop(ACL_EVENT *eventp)
{
	eventp->loop_fn(eventp);
}

void acl_event_set_delay_sec(ACL_EVENT *eventp, int sec)
{
	eventp->delay_sec = sec;
}

void acl_event_set_delay_usec(ACL_EVENT *eventp, int usec)
{
	eventp->delay_usec = usec;
}

int acl_event_use_thread(ACL_EVENT *eventp)
{
	return eventp->use_thread;
}

int acl_event_mode(ACL_EVENT *eventp)
{
	return eventp->event_mode;
}
