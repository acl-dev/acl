#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#ifdef	ACL_UNIX
#include <unistd.h>
#endif

#include "stdlib/acl_sys_patch.h"
#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_mystring.h"
#include "stdlib/acl_msg.h"
#include "stdlib/acl_ring.h"
#include "stdlib/acl_vstream.h"
#include "stdlib/acl_iostuff.h"
#include "event/acl_events.h"

#endif

#include "events_define.h"

#ifdef	ACL_EVENTS_KERNEL_STYLE

#include "events_epoll.h"
#include "events_devpoll.h"
#include "events_kqueue.h"
#include "events_fdtable.h"
#include "events_dog.h"
#include "events.h"

typedef struct EVENT_KERNEL_THR {
	EVENT_THR event;
	EVENT_BUFFER *event_buf;
	int   event_fdslots;
	int   event_fd;
#ifdef	USE_FDMAP
	ACL_FD_MAP *fdmap;
#endif
} EVENT_KERNEL_THR;

static void event_enable_read(ACL_EVENT *eventp, ACL_VSTREAM *stream,
	int timeout, ACL_EVENT_NOTIFY_RDWR callback, void *context)
{
	const char *myname = "event_enable_read";
	EVENT_KERNEL_THR *event_thr = (EVENT_KERNEL_THR *) eventp;
	ACL_EVENT_FDTABLE *fdp;
	ACL_SOCKET sockfd;
	int   err = 0;

	sockfd = ACL_VSTREAM_SOCK(stream);
	fdp = (ACL_EVENT_FDTABLE*) stream->fdp;
	if (fdp == NULL) {
		fdp = event_fdtable_alloc();
		fdp->listener = 0;
		fdp->stream = stream;
		stream->fdp = (void *) fdp;
	}

	/* 对同一连接的读写操作禁止同时进行监控 */
	else if (fdp->flag & EVENT_FDTABLE_FLAG_WRITE)
		acl_msg_panic("%s(%d), %s: fd %d: multiple I/O request",
			__FILE__, __LINE__, myname, sockfd);
	else {
		fdp->listener = 0;
		fdp->stream = stream;
	}

	if (fdp->r_callback != callback || fdp->r_context != context) {
		fdp->r_callback = callback;
		fdp->r_context = context;
	}

	if (timeout > 0) {
		fdp->r_timeout = ((acl_int64) timeout) * 1000000;
		fdp->r_ttl = eventp->present + fdp->r_timeout;
	} else {
		fdp->r_ttl = 0;
		fdp->r_timeout = 0;
	}

	if ((fdp->flag & EVENT_FDTABLE_FLAG_READ) == 0) {
		stream->nrefer++;
		fdp->flag = EVENT_FDTABLE_FLAG_READ | EVENT_FDTABLE_FLAG_EXPT;

		THREAD_LOCK(&event_thr->event.tb_mutex);
		
		fdp->fdidx = eventp->fdcnt;
		eventp->fdtabs[eventp->fdcnt] = fdp;
		eventp->fdcnt++;

#ifdef	USE_FDMAP
		acl_fdmap_add(event_thr->fdmap, sockfd, fdp);
#endif

		EVENT_REG_ADD_READ(err, event_thr->event_fd, sockfd, fdp);

		THREAD_UNLOCK(&event_thr->event.tb_mutex);

		if (err < 0) {
			acl_msg_fatal("%s: %s: %s, err(%d), fd(%d)",
				myname, EVENT_REG_ADD_TEXT, acl_last_serror(),
				err, sockfd);
		}
	}

	/* 主要是为了减少通知次数 */
	if (event_thr->event.blocked && event_thr->event.evdog
	    && event_dog_client(event_thr->event.evdog) != stream)
		event_dog_notify(event_thr->event.evdog);
}

static void event_enable_listen(ACL_EVENT *eventp, ACL_VSTREAM *stream,
	int timeout, ACL_EVENT_NOTIFY_RDWR callback, void *context)
{
	const char *myname = "event_enable_listen";
	EVENT_KERNEL_THR *event_thr = (EVENT_KERNEL_THR *) eventp;
	ACL_EVENT_FDTABLE *fdp;
	ACL_SOCKET sockfd;
	int   err = 0;

	sockfd = ACL_VSTREAM_SOCK(stream);
	fdp = (ACL_EVENT_FDTABLE*) stream->fdp;
	if (fdp == NULL) {
		fdp = event_fdtable_alloc();
		fdp->stream = stream;
		fdp->listener = 1;
		stream->fdp = (void *) fdp;
	}

	/* 对同一连接的读写操作禁止同时进行监控 */
	else if (fdp->flag & EVENT_FDTABLE_FLAG_WRITE)
		acl_msg_panic("%s(%d)->%s: fd %d: multiple I/O request",
			__FILE__, __LINE__, myname, sockfd);
	else {
		fdp->stream = stream;
		fdp->listener = 1;
	}

	if (fdp->r_callback != callback || fdp->r_context != context) {
		fdp->r_callback = callback;
		fdp->r_context = context;
	}

	if (timeout > 0) {
		fdp->r_timeout = ((acl_int64) timeout) * 1000000;
		fdp->r_ttl = eventp->present + fdp->r_timeout;
	} else {
		fdp->r_ttl = 0;
		fdp->r_timeout = 0;
	}

	if ((fdp->flag & EVENT_FDTABLE_FLAG_READ) == 0) {
		fdp->flag = EVENT_FDTABLE_FLAG_READ | EVENT_FDTABLE_FLAG_EXPT;
		stream->nrefer++;

		THREAD_LOCK(&event_thr->event.tb_mutex);

		fdp->fdidx = eventp->fdcnt;
		eventp->fdtabs[eventp->fdcnt] = fdp;
		eventp->fdcnt++;

#ifdef	USE_FDMAP
		acl_fdmap_add(event_thr->fdmap, sockfd, fdp);
#endif
		EVENT_REG_ADD_READ(err, event_thr->event_fd, sockfd, fdp);

		THREAD_UNLOCK(&event_thr->event.tb_mutex);

		if (err < 0)
			acl_msg_fatal("%s: %s: %s, err(%d), fd(%d)",
				myname, EVENT_REG_ADD_TEXT, acl_last_serror(),
				err, sockfd);
	}
}

static void event_enable_write(ACL_EVENT *eventp, ACL_VSTREAM *stream,
	int timeout, ACL_EVENT_NOTIFY_RDWR callback, void *context)
{
	const char *myname = "event_enable_write";
	EVENT_KERNEL_THR *event_thr = (EVENT_KERNEL_THR *) eventp;
	ACL_EVENT_FDTABLE *fdp;
	ACL_SOCKET sockfd;
	int   err = 0;

	sockfd = ACL_VSTREAM_SOCK(stream);
	fdp = (ACL_EVENT_FDTABLE*) stream->fdp;
	if (fdp == NULL) {
		fdp = event_fdtable_alloc();
		fdp->listener = 0;
		fdp->stream = stream;
		stream->fdp = (void *) fdp;
	}
	/* 对同一连接的读写操作禁止同时进行监控 */
	else if (fdp->flag & EVENT_FDTABLE_FLAG_READ)
		acl_msg_panic("%s(%d)->%s: fd %d: multiple I/O request",
			__FILE__, __LINE__, myname, sockfd);
	else {
		fdp->listener = 0;
		fdp->stream = stream;
	}

	if (fdp->w_callback != callback || fdp->w_context != context) {
		fdp->w_callback = callback;
		fdp->w_context = context;
	}

	if (timeout > 0) {
		fdp->w_timeout = ((acl_int64) timeout) * 1000000;
		fdp->w_ttl = eventp->present + fdp->w_timeout;
	} else {
		fdp->w_ttl = 0;
		fdp->w_timeout = 0;
	}

	if ((fdp->flag & EVENT_FDTABLE_FLAG_WRITE) == 0) {
		fdp->flag = EVENT_FDTABLE_FLAG_WRITE | EVENT_FDTABLE_FLAG_EXPT;
		stream->nrefer++;

		THREAD_LOCK(&event_thr->event.tb_mutex);

		fdp->fdidx = eventp->fdcnt;
		eventp->fdtabs[eventp->fdcnt] = fdp;
		eventp->fdcnt++;

#ifdef	USE_FDMAP
		acl_fdmap_add(event_thr->fdmap, sockfd, fdp);
#endif
		EVENT_REG_ADD_WRITE(err, event_thr->event_fd, sockfd, fdp);

		THREAD_UNLOCK(&event_thr->event.tb_mutex);

		if (err < 0) {
			acl_msg_fatal("%s: %s: %s, err(%d), fd(%d)",
				myname, EVENT_REG_ADD_TEXT, acl_last_serror(),
				err, sockfd);
		}
	}

	if (event_thr->event.blocked && event_thr->event.evdog
	    && event_dog_client(event_thr->event.evdog) != stream)
		event_dog_notify(event_thr->event.evdog);
}

/* event_disable_readwrite - disable request for read or write events */

static void event_disable_readwrite(ACL_EVENT *eventp, ACL_VSTREAM *stream)
{
	const char *myname = "event_disable_readwrite";
	EVENT_KERNEL_THR *event_thr = (EVENT_KERNEL_THR *) eventp;
	ACL_EVENT_FDTABLE *fdp;
	ACL_SOCKET sockfd;
	int   err = 0;

	sockfd = ACL_VSTREAM_SOCK(stream);

	fdp = (ACL_EVENT_FDTABLE *) stream->fdp;
	if (fdp == NULL) {
		acl_msg_error("%s(%d): fdp null", myname, __LINE__);
		return;
	}

	if ((fdp->flag & (EVENT_FDTABLE_FLAG_READ | EVENT_FDTABLE_FLAG_WRITE)) == 0) {
		acl_msg_error("%s(%d): sockfd(%d) not be set",
			myname, __LINE__, sockfd);
		return;
	}
	if (fdp->fdidx == -1)
		acl_msg_fatal("%s(%d): fdidx(%d) invalid",
			myname, __LINE__, fdp->fdidx);

	if (eventp->fdtabs[fdp->fdidx] != fdp)
		acl_msg_fatal("%s(%d): fdidx(%d)'s fdp invalid",
			myname, __LINE__, fdp->fdidx);

	THREAD_LOCK(&event_thr->event.tb_mutex);

	if (fdp->fdidx < --eventp->fdcnt) {
		eventp->fdtabs[fdp->fdidx] = eventp->fdtabs[eventp->fdcnt];
		eventp->fdtabs[fdp->fdidx]->fdidx = fdp->fdidx;
	}

#ifdef	USE_FDMAP
	acl_fdmap_del(event_thr->fdmap, sockfd);
#endif

#ifdef	EVENT_REG_DEL_BOTH
	EVENT_REG_DEL_BOTH(err, event_thr->event_fd, sockfd);
	if (fdp->flag & EVENT_FDTABLE_FLAG_READ)
		stream->nrefer--;
	if (fdp->flag & EVENT_FDTABLE_FLAG_WRITE)
		stream->nrefer--;
#else
	if (fdp->flag & EVENT_FDTABLE_FLAG_READ) {
		EVENT_REG_DEL_READ(err, event_thr->event_fd, sockfd);
		stream->nrefer--;
	}
	if (fdp->flag & EVENT_FDTABLE_FLAG_WRITE) {
		EVENT_REG_DEL_WRITE(err, event_thr->event_fd, sockfd);
		stream->nrefer--;
	}
#endif

	THREAD_UNLOCK(&event_thr->event.tb_mutex);

	if (err < 0) {
		acl_msg_fatal("%s: %s: %s", myname, EVENT_REG_DEL_TEXT,
			acl_last_serror());
	}
	event_fdtable_reset(fdp);
}

static int event_isrset(ACL_EVENT *eventp acl_unused, ACL_VSTREAM *stream)
{
	ACL_EVENT_FDTABLE *fdp;

	fdp = (ACL_EVENT_FDTABLE *) stream->fdp;
	if (fdp == NULL)
		return (0);

	return ((fdp->flag & EVENT_FDTABLE_FLAG_READ));
}

static int event_iswset(ACL_EVENT *eventp acl_unused, ACL_VSTREAM *stream)
{
	ACL_EVENT_FDTABLE *fdp;

	fdp = (ACL_EVENT_FDTABLE *) stream->fdp;
	if (fdp == NULL)
		return (0);

	return ((fdp->flag & EVENT_FDTABLE_FLAG_WRITE));
}

static int event_isxset(ACL_EVENT *eventp acl_unused, ACL_VSTREAM *stream)
{
	ACL_EVENT_FDTABLE *fdp;

	fdp = (ACL_EVENT_FDTABLE *) stream->fdp;
	if (fdp == NULL)
		return (0);

	return ((fdp->flag & EVENT_FDTABLE_FLAG_EXPT));
}

static void event_loop(ACL_EVENT *eventp)
{
	const char *myname = "event_loop";
	EVENT_KERNEL_THR *event_thr = (EVENT_KERNEL_THR *) eventp;
	ACL_EVENT_TIMER *timer;
	int   nready;
	acl_int64 delay;
	ACL_EVENT_FDTABLE *fdp;
	EVENT_BUFFER *bp;

	delay = eventp->delay_sec * 1000000 + eventp->delay_usec;
	if (delay <= DELAY_MIN)
		delay = DELAY_MIN;

	SET_TIME(eventp->present);

	THREAD_LOCK(&event_thr->event.tm_mutex);

	/*
	 * Find out when the next timer would go off. Timer requests are sorted.
	 * If any timer is scheduled, adjust the delay appropriately.
	 */
	if ((timer = ACL_FIRST_TIMER(&eventp->timer_head)) != 0) {
		acl_int64 n = timer->when - eventp->present;
		if (n <= 0)
			delay = 0;
		else if (n < delay)
			delay = n;
	}

	THREAD_UNLOCK(&event_thr->event.tm_mutex);

	eventp->ready_cnt = 0;

	if (eventp->present - eventp->last_check >= eventp->check_inter) {
		eventp->last_check = eventp->present;

		THREAD_LOCK(&event_thr->event.tb_mutex);

		if (event_thr_prepare(eventp) == 0) {
			THREAD_UNLOCK(&event_thr->event.tb_mutex);

			if (eventp->ready_cnt == 0)
				acl_doze(delay > DELAY_MIN ? (int) delay / 1000 : 1);

			nready = 0;
			goto TAG_DONE;
		}

		THREAD_UNLOCK(&event_thr->event.tb_mutex);

		if (eventp->ready_cnt > 0)
			delay = 0;
	}

	event_thr->event.blocked = 1;
	EVENT_BUFFER_READ(nready, event_thr->event_fd, event_thr->event_buf,
		event_thr->event_fdslots, (int) (delay / 1000));
	event_thr->event.blocked = 0;

	if (nready < 0) {
		if (acl_last_error() != ACL_EINTR) {
			acl_msg_fatal("%s(%d), %s: event_loop: select: %s",
				__FILE__, __LINE__, myname, acl_last_serror());
		}
		goto TAG_DONE;
	} else if (nready == 0)
		goto TAG_DONE;

	bp = event_thr->event_buf;

#ifdef	USE_FDMAP
	THREAD_LOCK(&event_thr->event.tb_mutex);
#endif

	for (; bp < event_thr->event_buf + nready; bp++) {
#ifdef	USE_FDMAP
		ACL_SOCKET sockfd;

		sockfd = EVENT_GET_FD(bp);
		fdp = acl_fdmap_ctx(event_thr->fdmap, sockfd);
		if (fdp == NULL || fdp->stream == NULL)
			continue;

		if (sockfd != ACL_VSTREAM_SOCK(fdp->stream))
			acl_msg_fatal("%s(%d): sockfd(%d) != %d", myname,
				__LINE__, sockfd, ACL_VSTREAM_SOCK(fdp->stream));
#else
		fdp = (ACL_EVENT_FDTABLE *) EVENT_GET_CTX(bp);
#endif
		if ((fdp->event_type & (ACL_EVENT_XCPT | ACL_EVENT_RW_TIMEOUT)))
			continue;

		if ((fdp->flag & EVENT_FDTABLE_FLAG_READ)
			&& EVENT_TEST_READ(bp))
		{
			if ((fdp->event_type & ACL_EVENT_READ) == 0) {
				fdp->event_type |= ACL_EVENT_READ;
				fdp->fdidx_ready = eventp->ready_cnt;
				eventp->ready[eventp->ready_cnt++] = fdp;
			}

			if (fdp->listener)
				fdp->event_type |= ACL_EVENT_ACCEPT;
			else
				fdp->stream->read_ready = 1;
		} else if ((fdp->flag & EVENT_FDTABLE_FLAG_WRITE)
			&& EVENT_TEST_WRITE(bp))
		{
			fdp->event_type |= ACL_EVENT_WRITE;
			fdp->fdidx_ready = eventp->ready_cnt;
			eventp->ready[eventp->ready_cnt++] = fdp;
		}
	}

#ifdef	USE_FDMAP
	THREAD_UNLOCK(&event_thr->event.tb_mutex);
#endif

TAG_DONE:

	/* Deliver timer events */
	event_timer_trigger_thr(&event_thr->event);

	if (eventp->ready_cnt > 0)
		event_thr_fire(eventp);
}

static void event_add_dog(ACL_EVENT *eventp)
{
	EVENT_KERNEL_THR *event_thr = (EVENT_KERNEL_THR*) eventp;

	event_thr->event.evdog = event_dog_create((ACL_EVENT*) event_thr, 1);
}

static void event_free(ACL_EVENT *eventp)
{
	const char *myname = "event_free";
	EVENT_KERNEL_THR *event_thr = (EVENT_KERNEL_THR *) eventp;

	if (eventp == NULL)
		acl_msg_fatal("%s, %s(%d): eventp null",
			__FILE__, myname, __LINE__);

	LOCK_DESTROY(&event_thr->event.tm_mutex);
	LOCK_DESTROY(&event_thr->event.tb_mutex);

#ifdef	USE_FDMAP
	acl_fdmap_free(event_thr->fdmap);
#endif
	acl_myfree(event_thr->event_buf);
	close(event_thr->event_fd);
	acl_myfree(eventp);
}

ACL_EVENT *event_new_kernel_thr(int fdsize acl_unused)
{
	EVENT_KERNEL_THR *event_thr;
	static int __default_max_events = 1024;

	event_thr = (EVENT_KERNEL_THR*) event_alloc(sizeof(EVENT_KERNEL_THR));

	snprintf(event_thr->event.event.name, sizeof(event_thr->event.event.name),
		"thread events - %s", EVENT_NAME);
	event_thr->event.event.event_mode           = ACL_EVENT_KERNEL;
	event_thr->event.event.use_thread           = 1;
	event_thr->event.event.loop_fn              = event_loop;
	event_thr->event.event.free_fn              = event_free;
	event_thr->event.event.add_dog_fn           = event_add_dog;
	event_thr->event.event.enable_read_fn       = event_enable_read;
	event_thr->event.event.enable_write_fn      = event_enable_write;
	event_thr->event.event.enable_listen_fn     = event_enable_listen;
	event_thr->event.event.disable_readwrite_fn = event_disable_readwrite;
	event_thr->event.event.isrset_fn            = event_isrset;
	event_thr->event.event.iswset_fn            = event_iswset;
	event_thr->event.event.isxset_fn            = event_isxset;
	event_thr->event.event.timer_request        = event_timer_request_thr;
	event_thr->event.event.timer_cancel         = event_timer_cancel_thr;
	event_thr->event.event.timer_keep           = event_timer_keep_thr;
	event_thr->event.event.timer_ifkeep         = event_timer_ifkeep_thr;

	LOCK_INIT(&event_thr->event.tm_mutex);
	LOCK_INIT(&event_thr->event.tb_mutex);

	EVENT_REG_INIT_HANDLE(event_thr->event_fd, fdsize);
	event_thr->event_fdslots = __default_max_events;
	event_thr->event_buf = (EVENT_BUFFER *)
		acl_mycalloc(event_thr->event_fdslots + 1, sizeof(EVENT_BUFFER));
#ifdef	USE_FDMAP
	event_thr->fdmap = acl_fdmap_create(fdsize);
#endif
	return ((ACL_EVENT *) event_thr);
}

#endif	/* ACL_EVENTS_KERNEL_STYLE */
