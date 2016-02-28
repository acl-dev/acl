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
#include "event/acl_events.h"

#endif

#include "events_dog.h"
#include "events.h"
#include "events_fdtable.h"

typedef struct EVENT_SELECT_THR {
	EVENT_THR event;
	fd_set rmask;
	fd_set wmask;
	fd_set xmask;
} EVENT_SELECT_THR;

static void event_enable_read(ACL_EVENT *eventp, ACL_VSTREAM *stream,
	int timeout, ACL_EVENT_NOTIFY_RDWR callback, void *context)
{
	const char *myname = "event_enable_read";
	EVENT_SELECT_THR *event_thr = (EVENT_SELECT_THR *) eventp;
	ACL_EVENT_FDTABLE *fdp;
	ACL_SOCKET sockfd;

	sockfd = ACL_VSTREAM_SOCK(stream);

	THREAD_LOCK(&event_thr->event.tb_mutex);

	/*
	* Disallow multiple requests on the same file descriptor.
	* Allow duplicates of the same request.
	*/
	if (FD_ISSET(sockfd, &event_thr->wmask))
		acl_msg_panic("%s(%d), %s: fd %d: multiple I/O request",
			__FILE__, __LINE__, myname, sockfd);

	fdp = stream->fdp;
	if (fdp == NULL) {
		fdp = event_fdtable_alloc();
		fdp->listener = 0;
	} else
		fdp->listener = 0;

	if (fdp->flag & EVENT_FDTABLE_FLAG_WRITE)
		acl_msg_panic("%s(%d)->%s: fd %d: multiple I/O request",
			__FILE__, __LINE__, myname, sockfd);

	if ((fdp->flag & EVENT_FDTABLE_FLAG_READ) == 0) {
		fdp->flag = EVENT_FDTABLE_FLAG_READ | EVENT_FDTABLE_FLAG_EXPT;

		if (FD_ISSET(sockfd, &event_thr->rmask))
			acl_msg_fatal("%s, %s(%d): sockfd(%d) has been in rmask",
				myname, __FILE__, __LINE__, sockfd);

		FD_SET(sockfd, &event_thr->xmask);
		FD_SET(sockfd, &event_thr->rmask);

		stream->fdp = (void *) fdp;
		stream->nrefer++;
		fdp->stream = stream;
		fdp->fdidx = eventp->fdcnt;
		eventp->fdtabs[eventp->fdcnt] = fdp;
		eventp->fdcnt++;

		if (eventp->maxfd != ACL_SOCKET_INVALID && eventp->maxfd < sockfd)
			eventp->maxfd = sockfd;
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

	THREAD_UNLOCK(&event_thr->event.tb_mutex);

	/* 主要是为了减少通知次数 */
	if (event_thr->event.blocked && event_thr->event.evdog
	    && event_dog_client(event_thr->event.evdog) != stream)
		event_dog_notify(event_thr->event.evdog);
}

static void event_enable_listen(ACL_EVENT *eventp, ACL_VSTREAM *stream,
	int timeout, ACL_EVENT_NOTIFY_RDWR callback, void *context)
{
	const char *myname = "event_enable_listen";
	EVENT_SELECT_THR *event_thr = (EVENT_SELECT_THR *) eventp;
	ACL_EVENT_FDTABLE *fdp;
	ACL_SOCKET sockfd;

	sockfd = ACL_VSTREAM_SOCK(stream);

	THREAD_LOCK(&event_thr->event.tb_mutex);

	/*
	* Disallow multiple requests on the same file descriptor.
	* Allow duplicates of the same request.
	*/
	if (FD_ISSET(sockfd, &event_thr->wmask))
		acl_msg_panic("%s(%d), %s: fd %d: multiple I/O request",
			__FILE__, __LINE__, myname, sockfd);

	fdp = stream->fdp;
	if (fdp == NULL) {
		fdp = event_fdtable_alloc();
		fdp->listener = 1;
	} else
		fdp->listener = 1;

	if (fdp->flag & EVENT_FDTABLE_FLAG_WRITE)
		acl_msg_panic("%s(%d)->%s: fd %d: multiple I/O request",
			__FILE__, __LINE__, myname, sockfd);

	if ((fdp->flag & EVENT_FDTABLE_FLAG_READ) == 0) {
		fdp->flag = EVENT_FDTABLE_FLAG_READ | EVENT_FDTABLE_FLAG_EXPT;

		if (FD_ISSET(sockfd, &event_thr->rmask))
			acl_msg_fatal("%s, %s(%d): sockfd(%d) has been in rmask",
				myname, __FILE__, __LINE__, sockfd);

		FD_SET(sockfd, &event_thr->xmask);
		FD_SET(sockfd, &event_thr->rmask);

		stream->fdp = (void *) fdp;
		stream->nrefer++;
		fdp->stream = stream;
		fdp->fdidx = eventp->fdcnt;
		eventp->fdtabs[eventp->fdcnt] = fdp;
		eventp->fdcnt++;

		if (eventp->maxfd != ACL_SOCKET_INVALID && eventp->maxfd < sockfd)
			eventp->maxfd = sockfd;
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

	THREAD_UNLOCK(&event_thr->event.tb_mutex);
}

static void event_enable_write(ACL_EVENT *eventp, ACL_VSTREAM *stream,
	int timeout, ACL_EVENT_NOTIFY_RDWR callback, void *context)
{
	const char *myname = "event_enable_write";
	EVENT_SELECT_THR *event_thr = (EVENT_SELECT_THR *) eventp;
	ACL_EVENT_FDTABLE *fdp;
	ACL_SOCKET sockfd;

	sockfd = ACL_VSTREAM_SOCK(stream);

	THREAD_LOCK(&event_thr->event.tb_mutex);

	/*
	* Disallow multiple requests on the same file descriptor.
	* Allow duplicates of the same request.
	*/
	if (FD_ISSET(sockfd, &event_thr->rmask))
		acl_msg_panic("%s(%d), %s: fd %d: multiple I/O request",
			__FILE__, __LINE__, myname, sockfd);

	fdp = stream->fdp;
	if (fdp == NULL)
		fdp = event_fdtable_alloc();

	if (fdp->flag & EVENT_FDTABLE_FLAG_READ)
		acl_msg_panic("%s(%d)->%s: fd %d: multiple I/O request",
			__FILE__, __LINE__, myname, sockfd);


	if ((fdp->flag & EVENT_FDTABLE_FLAG_WRITE) == 0) {
		fdp->flag = EVENT_FDTABLE_FLAG_WRITE | EVENT_FDTABLE_FLAG_EXPT;

		if (FD_ISSET(sockfd, &event_thr->wmask))
			acl_msg_fatal("%s, %s(%d): sockfd(%d) has been in wmask",
				myname, __FILE__, __LINE__, sockfd);

		FD_SET(sockfd, &event_thr->xmask);
		FD_SET(sockfd, &event_thr->wmask);

		stream->fdp = (void *) fdp;
		stream->nrefer++;
		fdp->stream = stream;
		fdp->listener = 0;
		fdp->fdidx = eventp->fdcnt;
		eventp->fdtabs[eventp->fdcnt] = fdp;
		eventp->fdcnt++;

		if (eventp->maxfd != ACL_SOCKET_INVALID && eventp->maxfd < sockfd)
			eventp->maxfd = sockfd;
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

	THREAD_UNLOCK(&event_thr->event.tb_mutex);

	if (event_thr->event.blocked && event_thr->event.evdog
	    && event_dog_client(event_thr->event.evdog) != stream)
		event_dog_notify(event_thr->event.evdog);
}

/* event_disable_readwrite - disable request for read or write events */

static void event_disable_readwrite(ACL_EVENT *eventp, ACL_VSTREAM *stream)
{
	const char *myname = "event_disable_readwrite";
	EVENT_SELECT_THR *event_thr = (EVENT_SELECT_THR *) eventp;
	ACL_EVENT_FDTABLE *fdp;
	ACL_SOCKET sockfd;

	sockfd = ACL_VSTREAM_SOCK(stream);

	THREAD_LOCK(&event_thr->event.tb_mutex);
	if (!FD_ISSET(sockfd, &event_thr->xmask)) {
		THREAD_UNLOCK(&event_thr->event.tb_mutex);
		return;
	}
	fdp = (ACL_EVENT_FDTABLE *) stream->fdp;
	if (fdp == NULL) {
		acl_msg_error("%s(%d): fdp null", myname, __LINE__);
		THREAD_UNLOCK(&event_thr->event.tb_mutex);
		return;
	}

	if (fdp->fdidx == -1)
		acl_msg_fatal("%s(%d): fdidx(%d) invalid",
			myname, __LINE__, fdp->fdidx);

	FD_CLR(sockfd, &event_thr->xmask);
	FD_CLR(sockfd, &event_thr->rmask);
	FD_CLR(sockfd, &event_thr->wmask);

	fdp->flag = 0;

	if (eventp->maxfd == sockfd)
		eventp->maxfd = ACL_SOCKET_INVALID;
	if (eventp->fdtabs[fdp->fdidx] == fdp) {
		if (fdp->fdidx < --eventp->fdcnt) {
			eventp->fdtabs[fdp->fdidx] = eventp->fdtabs[eventp->fdcnt];
			eventp->fdtabs[fdp->fdidx]->fdidx = fdp->fdidx;
		}
	} else
		acl_msg_fatal("%s(%d): fdidx(%d)'s fdp invalid",
			myname, __LINE__, fdp->fdidx);

	if (fdp->fdidx_ready > 0
	    && fdp->fdidx_ready < eventp->ready_cnt
	    && eventp->ready[fdp->fdidx_ready] == fdp)
	{
		eventp->ready[fdp->fdidx_ready] = NULL;
	}
	event_fdtable_free(fdp);
	stream->fdp = NULL;
	stream->nrefer--;
	THREAD_UNLOCK(&event_thr->event.tb_mutex);
}

static int event_isrset(ACL_EVENT *eventp, ACL_VSTREAM *stream)
{
	EVENT_SELECT_THR *event_thr = (EVENT_SELECT_THR *) eventp;

	return FD_ISSET(ACL_VSTREAM_SOCK(stream), &event_thr->rmask);
}

static int event_iswset(ACL_EVENT *eventp, ACL_VSTREAM *stream)
{
	EVENT_SELECT_THR *event_thr = (EVENT_SELECT_THR *) eventp;

	return FD_ISSET(ACL_VSTREAM_SOCK(stream), &event_thr->wmask);
}

static int event_isxset(ACL_EVENT *eventp, ACL_VSTREAM *stream)
{
	EVENT_SELECT_THR *event_thr = (EVENT_SELECT_THR *) eventp;

	return FD_ISSET(ACL_VSTREAM_SOCK(stream), &event_thr->xmask);
}

static void event_loop(ACL_EVENT *eventp)
{
	const char *myname = "event_loop";
	EVENT_SELECT_THR *event_thr = (EVENT_SELECT_THR *) eventp;
	ACL_EVENT_NOTIFY_TIME timer_fn;
	void    *timer_arg;
	ACL_SOCKET sockfd;
	ACL_EVENT_TIMER *timer;
	int   select_delay, nready, i;
	ACL_EVENT_FDTABLE *fdp;
	ACL_RING timer_ring, *entry_ptr;
	struct timeval tv, *tvp;
	fd_set rmask;  /* enabled read events */
	fd_set wmask;  /* enabled write events */
	fd_set xmask;  /* for bad news mostly */

	acl_ring_init(&timer_ring);

	SET_TIME(eventp->present);
	THREAD_LOCK(&event_thr->event.tm_mutex);

	/*
	 * Find out when the next timer would go off. Timer requests are sorted.
	 * If any timer is scheduled, adjust the delay appropriately.
	 */
	if ((timer = ACL_FIRST_TIMER(&eventp->timer_head)) != 0) {
		select_delay = (int) ((timer->when - eventp->present + 1000000 - 1)
			/ 1000000);
		if (select_delay < 0)
			select_delay = 0;
		else if (eventp->delay_sec >= 0 && select_delay > eventp->delay_sec)
			select_delay = eventp->delay_sec;
	} else
		select_delay = eventp->delay_sec;

	THREAD_UNLOCK(&event_thr->event.tm_mutex);

	THREAD_LOCK(&event_thr->event.tb_mutex);

	eventp->ready_cnt = 0;

	if (event_thr_prepare(eventp) == 0) {
		THREAD_UNLOCK(&event_thr->event.tb_mutex);

		if (eventp->ready_cnt == 0) {
			select_delay /= 1000000;
			if (select_delay <= 0)
				select_delay = 1;
			sleep((int) select_delay);
		}

		nready = 0;
		goto TAG_DONE;
	}

	if (eventp->ready_cnt > 0) {
		tv.tv_sec  = 0;
		tv.tv_usec = 0;
		tvp = &tv;
	} else if (select_delay < 0) {
		tvp = NULL;
	} else {
		tv.tv_sec  = select_delay;
		tv.tv_usec = eventp->delay_usec;
		tvp = &tv;
	}

	rmask = event_thr->rmask;
	wmask = event_thr->wmask;
	xmask = event_thr->xmask;

	THREAD_UNLOCK(&event_thr->event.tb_mutex);

	event_thr->event.blocked = 1;
	nready = select((int) eventp->maxfd + 1, &rmask, &wmask, &xmask, tvp);
	event_thr->event.blocked = 0;
	if (nready < 0) {
		if (acl_last_error() != ACL_EINTR)
			acl_msg_fatal("%s(%d), %s: event_loop: select: %s",
				__FILE__, __LINE__, myname, acl_last_serror());
		goto TAG_DONE;
	} else if (nready == 0)
		goto TAG_DONE;

	THREAD_LOCK(&event_thr->event.tb_mutex);

	for (i = 0; i < eventp->fdcnt; i++) {
		fdp = eventp->fdtabs[i];

		/* if fdp has been set in eventp->ready ? */
		if ((fdp->event_type & (ACL_EVENT_XCPT | ACL_EVENT_RW_TIMEOUT)))
			continue;

		sockfd = ACL_VSTREAM_SOCK(fdp->stream);

		if (FD_ISSET(sockfd, &xmask)) {
			fdp->event_type |= ACL_EVENT_XCPT;
			fdp->fdidx_ready = eventp->ready_cnt;
			eventp->ready[eventp->ready_cnt++] = fdp;
			continue;
		}

		if (FD_ISSET(sockfd, &rmask)) {
			/* has been set in ready ? */
			if ((fdp->event_type & ACL_EVENT_READ) == 0) {
				fdp->event_type |= ACL_EVENT_READ;
				fdp->fdidx_ready = eventp->ready_cnt;
				eventp->ready[eventp->ready_cnt++] = fdp;
			}

			if (fdp->listener)
				fdp->event_type |= ACL_EVENT_ACCEPT;
			else
				fdp->stream->read_ready = 1;
		} else if (fdp->w_callback && FD_ISSET(sockfd, &wmask)) {
			fdp->event_type |= ACL_EVENT_WRITE;
			fdp->fdidx_ready = eventp->ready_cnt;
			eventp->ready[eventp->ready_cnt++] = fdp;
		}
	}

	THREAD_UNLOCK(&event_thr->event.tb_mutex);

TAG_DONE:

	/*
	 * Deliver timer events. Requests are sorted: we can stop when we reach
	 * the future or the list end. Allow the application to update the timer
	 * queue while it is being called back. To this end, we repeatedly pop
	 * the first request off the timer queue before delivering the event to
	 * the application.
	 */

	SET_TIME(eventp->present);

	THREAD_LOCK(&event_thr->event.tm_mutex);

	while ((timer = ACL_FIRST_TIMER(&eventp->timer_head)) != 0) {
		if (timer->when > eventp->present)
			break;

		acl_ring_detach(&timer->ring);          /* first this */
		acl_ring_prepend(&timer_ring, &timer->ring);
	}

	THREAD_UNLOCK(&event_thr->event.tm_mutex);

	while (1) {
		entry_ptr = acl_ring_pop_head(&timer_ring);
		if (entry_ptr == NULL)
			break;

		timer     = ACL_RING_TO_TIMER(entry_ptr);
		timer_fn  = timer->callback;
		timer_arg = timer->context;

		timer_fn(ACL_EVENT_TIME, eventp, timer_arg);

		acl_myfree(timer);
	}

	if (eventp->ready_cnt > 0)
		event_thr_fire(eventp);
}

static void event_add_dog(ACL_EVENT *eventp)
{
	EVENT_SELECT_THR *event_thr = (EVENT_SELECT_THR*) eventp;

	event_thr->event.evdog = event_dog_create((ACL_EVENT*) event_thr, 1);
}

static void event_free(ACL_EVENT *eventp)
{
	const char *myname = "event_free";
	EVENT_SELECT_THR *event_thr = (EVENT_SELECT_THR *) eventp;

	if (eventp == NULL)
		acl_msg_fatal("%s, %s(%d): eventp null",
				__FILE__, myname, __LINE__);

	LOCK_DESTROY(&event_thr->event.tm_mutex);
	LOCK_DESTROY(&event_thr->event.tb_mutex);

	acl_myfree(eventp);
}

ACL_EVENT *event_new_select_thr(void)
{
	EVENT_SELECT_THR *event_thr;

	event_thr = (EVENT_SELECT_THR*) event_alloc(sizeof(EVENT_SELECT_THR));

	snprintf(event_thr->event.event.name, sizeof(event_thr->event.event.name),
		 "thread events - select");
	event_thr->event.event.event_mode           = ACL_EVENT_SELECT;
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

        FD_ZERO(&event_thr->rmask);
        FD_ZERO(&event_thr->wmask);
        FD_ZERO(&event_thr->xmask);

	LOCK_INIT(&event_thr->event.tm_mutex);
	LOCK_INIT(&event_thr->event.tb_mutex);

	return (ACL_EVENT *) event_thr;
}

