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
#include "stdlib/acl_msg.h"
#include "stdlib/acl_ring.h"
#include "stdlib/acl_vstream.h"
#include "stdlib/acl_iostuff.h"
#include "net/acl_sane_socket.h"
#include "event/acl_events.h"

#endif

#include "events_define.h"

#ifdef	ACL_EVENTS_KERNEL_STYLE
#if (ACL_EVENTS_KERNEL_STYLE == ACL_EVENTS_STYLE_EPOLL)

#include <sys/epoll.h>
#include "events_fdtable.h"
#include "events.h"

typedef struct EVENT_EPOLL_THR {
	EVENT_THR event;
	struct epoll_event *ebuf;
	int   fdslots;
	int   handle;
} EVENT_EPOLL_THR;

static void event_enable_read(ACL_EVENT *eventp, ACL_VSTREAM *fp,
	int timeout, ACL_EVENT_NOTIFY_RDWR callback, void *context)
{
	const char *myname = "event_enable_read";
	EVENT_EPOLL_THR *evthr = (EVENT_EPOLL_THR *) eventp;
	ACL_EVENT_FDTABLE *fdp;
	ACL_SOCKET fd;
	struct epoll_event ev;

	fd = ACL_VSTREAM_SOCK(fp);
	fdp = (ACL_EVENT_FDTABLE*) fp->fdp;
	if (fdp == NULL) {
		fdp = event_fdtable_alloc();
		fdp->listener = 0;
		fdp->stream = fp;

		/* fdp will be freed in acl_vstream_close */
		fp->fdp = (void *) fdp;
	} else if (fdp->flag & EVENT_FDTABLE_FLAG_WRITE)
		acl_msg_panic("%s(%d), %s: fd %d: multiple I/O request",
			__FILE__, __LINE__, myname, fd);
	else {
		fdp->listener = 0;
		fdp->stream = fp;
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

	if ((fdp->flag & EVENT_FDTABLE_FLAG_READ) != 0) {
		acl_msg_info("has set read, fd: %d", fd);
		return;
	}

	fp->nrefer++;
	fdp->flag = EVENT_FDTABLE_FLAG_READ | EVENT_FDTABLE_FLAG_EXPT;

	if (ACL_VSTREAM_BFRD_CNT(fp) > 0 || fp->read_ready
		|| (fp->flag & ACL_VSTREAM_FLAG_BAD))
	{
		fdp->flag |= EVENT_FDTABLE_FLAG_FIRE;
	} else
		fdp->flag &= ~EVENT_FDTABLE_FLAG_FIRE;

#if 0
	ev.events = EPOLLIN | EPOLLHUP | EPOLLERR | EPOLLET;
#else
	ev.events = EPOLLIN | EPOLLHUP | EPOLLERR;
#endif
	ev.data.u64 = 0;  /* avoid valgrind warning */
	ev.data.ptr = fdp;

	THREAD_LOCK(&evthr->event.tb_mutex);

	fdp->fdidx = eventp->fdcnt;
	eventp->fdtabs[eventp->fdcnt++] = fdp;

	if (fdp->flag & EVENT_FDTABLE_FLAG_FIRE) {
#if 0
		if (epoll_ctl(evthr->handle, EPOLL_CTL_ADD, fd, &ev) < 0) {
			if (errno == EEXIST)
				acl_msg_warn("%s: epoll_ctl: %s, fd: %d",
					myname, acl_last_serror(), fd);
			else if (errno == EBADF && acl_getsocktype(fd) < 0) {
				acl_msg_error("%s: epool_ctl: %s, fd: %d",
					myname, acl_last_serror(), fd);
				ACL_VSTREAM_SET_SOCK(fp, ACL_SOCKET_INVALID);
				fp->flag |= ACL_VSTREAM_FLAG_ERR;
			} else
				acl_msg_fatal("%s: epoll_ctl: %s, fd: %d, "
					"epfd: %d", myname, acl_last_serror(),
					fd, evthr->handle);
		}
#else
		/* reset the last_check to trigger event_thr_prepare */
		SET_TIME(eventp->last_check);
		eventp->last_check -= eventp->check_inter;
#endif

		THREAD_UNLOCK(&evthr->event.tb_mutex);

		if (evthr->event.blocked && evthr->event.evdog
			&& event_dog_client(evthr->event.evdog) != fp)
		{
			event_dog_notify(evthr->event.evdog);
		}

		return;
	}

	if (epoll_ctl(evthr->handle, EPOLL_CTL_ADD, fd, &ev) < 0) {
		if (errno == EEXIST)
			acl_msg_warn("%s: epoll_ctl: %s, fd: %d",
				myname, acl_last_serror(), fd);
		else if (errno == EBADF && acl_getsocktype(fd) < 0) {
			acl_msg_error("%s: epool_ctl: %s, fd: %d",
				myname, acl_last_serror(), fd);
			ACL_VSTREAM_SET_SOCK(fp, ACL_SOCKET_INVALID);
			fp->flag |= ACL_VSTREAM_FLAG_ERR;
		} else
			acl_msg_fatal("%s: epoll_ctl: %s, fd: %d, epfd: %d",
				myname, acl_last_serror(), fd, evthr->handle);
	}

	THREAD_UNLOCK(&evthr->event.tb_mutex);
}

static void event_enable_listen(ACL_EVENT *eventp, ACL_VSTREAM *fp,
	int timeout, ACL_EVENT_NOTIFY_RDWR callback, void *context)
{
	const char *myname = "event_enable_listen";
	EVENT_EPOLL_THR *evthr = (EVENT_EPOLL_THR *) eventp;
	ACL_EVENT_FDTABLE *fdp;
	ACL_SOCKET fd;
	struct epoll_event ev;

	fd = ACL_VSTREAM_SOCK(fp);
	fdp = (ACL_EVENT_FDTABLE*) fp->fdp;
	if (fdp == NULL) {
		fdp = event_fdtable_alloc();
		fdp->stream = fp;
		fdp->listener = 1;

		/* fdp will be freed in acl_vstream_close */
		fp->fdp = (void *) fdp;
	} else if (fdp->flag & EVENT_FDTABLE_FLAG_WRITE)
		acl_msg_panic("%s(%d)->%s: fd %d: multiple I/O request",
			__FILE__, __LINE__, myname, fd);
	else {
		fdp->stream = fp;
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

	if ((fdp->flag & EVENT_FDTABLE_FLAG_READ) != 0)
		return;

	fdp->flag = EVENT_FDTABLE_FLAG_READ | EVENT_FDTABLE_FLAG_EXPT;
	fp->nrefer++;

	ev.events = EPOLLIN | EPOLLHUP | EPOLLERR;
	ev.data.u64 = 0;  /* avoid valgrind warning */
	ev.data.ptr = fdp;

	THREAD_LOCK(&evthr->event.tb_mutex);

	fdp->fdidx = eventp->fdcnt;
	eventp->fdtabs[eventp->fdcnt++] = fdp;

	if (epoll_ctl(evthr->handle, EPOLL_CTL_ADD, fd, &ev) < 0) {
		if (errno == EEXIST)
			acl_msg_warn("%s: epool_ctl: %s, fd: %d",
				myname, acl_last_serror(), fd);
		else
			acl_msg_fatal("%s: epool_ctl: %s, fd: %d",
				myname, acl_last_serror(), fd);
	}

	THREAD_UNLOCK(&evthr->event.tb_mutex);
}

static void event_enable_write(ACL_EVENT *eventp, ACL_VSTREAM *fp,
	int timeout, ACL_EVENT_NOTIFY_RDWR callback, void *context)
{
	const char *myname = "event_enable_write";
	EVENT_EPOLL_THR *evthr = (EVENT_EPOLL_THR *) eventp;
	ACL_EVENT_FDTABLE *fdp;
	ACL_SOCKET fd;
	struct epoll_event ev;
	int   fd_ready;

	if ((fp->flag & ACL_VSTREAM_FLAG_BAD))
		fd_ready = 1;
	else
		fd_ready = 0;

	fd = ACL_VSTREAM_SOCK(fp);
	fdp = (ACL_EVENT_FDTABLE*) fp->fdp;
	if (fdp == NULL) {
		fdp = event_fdtable_alloc();
		fdp->listener = 0;
		fdp->stream = fp;

		/* fdp will be freed in acl_vstream_close */
		fp->fdp = (void *) fdp;
	} else if (fdp->flag & EVENT_FDTABLE_FLAG_READ)
		acl_msg_panic("%s(%d)->%s: fd %d: multiple I/O request",
			__FILE__, __LINE__, myname, fd);
	else {
		fdp->listener = 0;
		fdp->stream = fp;
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

	if ((fdp->flag & EVENT_FDTABLE_FLAG_WRITE) != 0)
		return;

	fdp->flag = EVENT_FDTABLE_FLAG_WRITE | EVENT_FDTABLE_FLAG_EXPT;
	fp->nrefer++;

	ev.events = EPOLLOUT | EPOLLHUP | EPOLLERR;
	ev.data.u64 = 0;  /* avoid valgrind warning */
	ev.data.ptr = fdp;

	THREAD_LOCK(&evthr->event.tb_mutex);

	fdp->fdidx = eventp->fdcnt;
	eventp->fdtabs[eventp->fdcnt++] = fdp;

	if (fd_ready) {
		if (epoll_ctl(evthr->handle, EPOLL_CTL_ADD, fd, &ev) < 0) {
			if (errno == EEXIST)
				acl_msg_warn("%s: epoll_ctl: %s, fd: %d",
					myname, acl_last_serror(), fd);
			else if (errno == EBADF && acl_getsocktype(fd) < 0) {
				acl_msg_error("%s: epool_ctl: %s, fd: %d",
					myname, acl_last_serror(), fd);
				ACL_VSTREAM_SET_SOCK(fp, ACL_SOCKET_INVALID);
				fp->flag |= ACL_VSTREAM_FLAG_ERR;
			} else
				acl_msg_fatal("%s: epoll_ctl: %s, fd: %d, "
					"epfd: %d", myname, acl_last_serror(),
					fd, evthr->handle);
		}

		THREAD_UNLOCK(&evthr->event.tb_mutex);
		return;
	}
	
	if (epoll_ctl(evthr->handle, EPOLL_CTL_ADD, fd, &ev) < 0) {
		if (errno == EEXIST)
			acl_msg_warn("%s: epoll_ctl: %s, fd: %d",
				myname, acl_last_serror(), fd);
		else if (errno == EBADF && acl_getsocktype(fd) < 0) {
			acl_msg_error("%s: epool_ctl: %s, fd: %d",
				myname, acl_last_serror(), fd);
			ACL_VSTREAM_SET_SOCK(fp, ACL_SOCKET_INVALID);
			fp->flag |= ACL_VSTREAM_FLAG_ERR;
		} else
			acl_msg_fatal("%s: epoll_ctl: %s, fd: %d, epfd: %d",
				myname, acl_last_serror(), fd, evthr->handle);
	}

	THREAD_UNLOCK(&evthr->event.tb_mutex);
}

/* event_disable_readwrite - disable request for read or write events */

static void event_disable_readwrite(ACL_EVENT *eventp, ACL_VSTREAM *stream)
{
	const char *myname = "event_disable_readwrite";
	EVENT_EPOLL_THR *event_thr = (EVENT_EPOLL_THR *) eventp;
	ACL_EVENT_FDTABLE *fdp;
	ACL_SOCKET sockfd;
	struct epoll_event dummy;

	dummy.events = EPOLLHUP | EPOLLERR;
	dummy.data.u64 = 0;  /* avoid valgrind warning */
	dummy.data.ptr = NULL;

	sockfd = ACL_VSTREAM_SOCK(stream);

	fdp = (ACL_EVENT_FDTABLE *) stream->fdp;
	if (fdp == NULL) {
		acl_msg_error("%s(%d): fdp null", myname, __LINE__);
		return;
	}

	if ((fdp->flag & (EVENT_FDTABLE_FLAG_READ
		| EVENT_FDTABLE_FLAG_WRITE)) == 0)
	{
		acl_msg_warn("%s(%d): sockfd(%d) not be set",
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

	if (fdp->flag & EVENT_FDTABLE_FLAG_READ) {
		stream->nrefer--;
		dummy.events |= EPOLLIN;
	}

	if (fdp->flag & EVENT_FDTABLE_FLAG_WRITE) {
		stream->nrefer--;
		dummy.events |= EPOLLOUT;
	}

	THREAD_UNLOCK(&event_thr->event.tb_mutex);

	if ((fdp->flag & EVENT_FDTABLE_FLAG_FIRE) != 0)
		fdp->flag &= ~EVENT_FDTABLE_FLAG_FIRE;
	else if (epoll_ctl(event_thr->handle, EPOLL_CTL_DEL,
			sockfd, &dummy) < 0)
	{
		if (errno == ENOENT)
			acl_msg_warn("%s: epoll_ctl: %s, fd: %d",
				myname, acl_last_serror(), sockfd);
		else
			acl_msg_error("%s: epoll_ctl: %s, fd: %d",
				myname, acl_last_serror(), sockfd);
	}

	/* fdp will be freed in acl_vstream_close */
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
	EVENT_EPOLL_THR *event_thr = (EVENT_EPOLL_THR *) eventp;
	int   nready;
	acl_int64 delay;
	ACL_EVENT_TIMER *timer;
	ACL_EVENT_FDTABLE *fdp;
	EVENT_BUFFER *bp;

	delay = eventp->delay_sec * 1000000 + eventp->delay_usec;
	if (delay <= DELAY_MIN)
		delay = DELAY_MIN;

	SET_TIME(eventp->present);

	THREAD_LOCK(&event_thr->event.tm_mutex);

	/*
	 * Find out when the next timer would go off. Timer requests are
	 * sorted. If any timer is scheduled, adjust the delay appropriately.
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

	THREAD_LOCK(&event_thr->event.tb_mutex);

	if (eventp->present - eventp->last_check >= eventp->check_inter) {

		/* reset the last_check for next call event_thr_prepare */
		eventp->last_check = eventp->present;

		/* check all fds' read/write status */
		if (event_thr_prepare(eventp) == 0) {

			THREAD_UNLOCK(&event_thr->event.tb_mutex);

			if (eventp->ready_cnt == 0)
				acl_doze(delay > DELAY_MIN ? delay / 1000 : 1);

			nready = 0;
			goto TAG_DONE;
		}

		THREAD_UNLOCK(&event_thr->event.tb_mutex);

		if (eventp->ready_cnt > 0)
			delay = 0;
	} else
		THREAD_UNLOCK(&event_thr->event.tb_mutex);

	event_thr->event.blocked = 1;
	nready = epoll_wait(event_thr->handle, event_thr->ebuf,
			event_thr->fdslots, (int) (delay / 1000));
	event_thr->event.blocked = 0;

	if (nready < 0) {
		if (acl_last_error() != ACL_EINTR)
			acl_msg_fatal("%s(%d), %s: event_loop: epoll: %s",
				__FILE__, __LINE__, myname, acl_last_serror());
		goto TAG_DONE;
	} else if (nready == 0)
		goto TAG_DONE;

	for (bp = event_thr->ebuf; bp < event_thr->ebuf + nready; bp++) {
		fdp = (ACL_EVENT_FDTABLE *) bp->data.ptr;
		if ((fdp->event_type & (ACL_EVENT_XCPT | ACL_EVENT_RW_TIMEOUT)))
			continue;

		if ((bp->events & EPOLLIN) != 0) {
			if ((fdp->event_type & ACL_EVENT_READ) == 0) {
				fdp->event_type |= ACL_EVENT_READ;
				fdp->fdidx_ready = eventp->ready_cnt;
				eventp->ready[eventp->ready_cnt] = fdp;
				eventp->ready_cnt++;
			}
			if (fdp->listener)
				fdp->event_type |= ACL_EVENT_ACCEPT;
			fdp->stream->read_ready = 1;
		} else if ((bp->events & EPOLLOUT) != 0) {
			fdp->event_type |= ACL_EVENT_WRITE;
			fdp->fdidx_ready = eventp->ready_cnt;
			eventp->ready[eventp->ready_cnt++] = fdp;
		} else if ((bp->events & (EPOLLERR | EPOLLHUP)) != 0) {
			fdp->event_type |= ACL_EVENT_XCPT;
			fdp->fdidx_ready = eventp->ready_cnt;
			eventp->ready[eventp->ready_cnt++] = fdp;
		}
	}

TAG_DONE:

	/* Deliver timer events */
	event_timer_trigger_thr(&event_thr->event);

	if (eventp->ready_cnt > 0)
		event_thr_fire(eventp);
}

static void event_add_dog(ACL_EVENT *eventp)
{
	EVENT_EPOLL_THR *event_thr = (EVENT_EPOLL_THR*) eventp;

	event_thr->event.evdog = event_dog_create((ACL_EVENT*) event_thr, 1);
}

static void event_free(ACL_EVENT *eventp)
{
	const char *myname = "event_free";
	EVENT_EPOLL_THR *event_thr = (EVENT_EPOLL_THR *) eventp;

	if (eventp == NULL)
		acl_msg_fatal("%s, %s(%d): eventp null",
			__FILE__, myname, __LINE__);

	LOCK_DESTROY(&event_thr->event.tm_mutex);
	LOCK_DESTROY(&event_thr->event.tb_mutex);

	acl_myfree(event_thr->ebuf);
	close(event_thr->handle);
	acl_myfree(eventp);
}

ACL_EVENT *event_epoll_alloc_thr(int fdsize acl_unused)
{
	EVENT_EPOLL_THR *event_thr;
	static int __default_max_events = 100;

	event_thr = (EVENT_EPOLL_THR*) event_alloc(sizeof(EVENT_EPOLL_THR));

	snprintf(event_thr->event.event.name,
		sizeof(event_thr->event.event.name), "thread events - epoll");
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

	event_thr->handle = epoll_create(fdsize);
	event_thr->fdslots = __default_max_events;
	event_thr->ebuf = (EVENT_BUFFER *) acl_mycalloc(event_thr->fdslots + 1,
			sizeof(EVENT_BUFFER));

	acl_msg_info("%s(%d), %s, use %s", __FILE__, __LINE__, __FUNCTION__,
		event_thr->event.event.name);

	return ((ACL_EVENT *) event_thr);
}

#endif	/* ACL_EVENTS_KERNEL_STYLE == ACL_EVENTS_STYLE_EPOLL */
#endif  /* ACL_EVENTS_KERNEL_STYLE */
