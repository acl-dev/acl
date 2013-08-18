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
#include "stdlib/acl_debug.h"
#include "stdlib/acl_vstream.h"
#include "event/acl_events.h"

#endif

#include "events_define.h"

#ifdef	ACL_EVENTS_KERNEL_STYLE

#include "events_epoll.h"
#include "events_devpoll.h"
#include "events_kqueue.h"
#include "events_fdtable.h"
#include "events.h"

typedef struct EVENT_KERNEL {
	ACL_EVENT event;
	EVENT_BUFFER *event_buf;
	int   event_fdslots;
	int   event_fd;
#ifdef	USE_FDMAP
	ACL_FD_MAP *fdmap;
#endif
} EVENT_KERNEL;

#include "stdlib/acl_meter_time.h"

static void event_enable_read(ACL_EVENT *eventp, ACL_VSTREAM *stream,
	int timeout, ACL_EVENT_NOTIFY_RDWR callback, void *context)
{
	const char *myname = "event_enable_read";
	EVENT_KERNEL *ev = (EVENT_KERNEL *) eventp;
	ACL_EVENT_FDTABLE *fdp;
	ACL_SOCKET sockfd;
	char  ebuf[256];
	int   err = 0;

	sockfd = ACL_VSTREAM_SOCK(stream);
	fdp = (ACL_EVENT_FDTABLE *) stream->fdp;
	if (fdp == NULL) {
		fdp = event_fdtable_alloc();

		fdp->flag = EVENT_FDTABLE_FLAG_READ | EVENT_FDTABLE_FLAG_EXPT;
		fdp->stream = stream;
		fdp->fdidx = eventp->fdcnt;
		eventp->fdtabs[eventp->fdcnt] = fdp;
		eventp->fdcnt++;

		stream->fdp = (void *) fdp;
		stream->nrefer++;
#ifdef	USE_FDMAP
		acl_fdmap_add(ev->fdmap, sockfd, fdp);
#endif
		EVENT_REG_ADD_READ(err, ev->event_fd, sockfd, fdp);
	} else if (fdp->stream != stream) {
		acl_msg_fatal("%s(%d), %s: corrupt stream",
			__FILE__, __LINE__, myname);
	} else if (fdp->fdidx < 0 || fdp->fdidx >= eventp->fdcnt || eventp->fdtabs[fdp->fdidx] != fdp) {
		/* xxx: can this will happen ? */

		fdp->flag = EVENT_FDTABLE_FLAG_READ | EVENT_FDTABLE_FLAG_EXPT;
		fdp->fdidx = eventp->fdcnt;
		eventp->fdtabs[eventp->fdcnt] = fdp;
		eventp->fdcnt++;

		stream->nrefer++;
#ifdef	USE_FDMAP
		acl_fdmap_add(ev->fdmap, sockfd, fdp);
#endif
		EVENT_REG_ADD_READ(err, ev->event_fd, sockfd, fdp);
	} else if (fdp->flag & EVENT_FDTABLE_FLAG_WRITE) {
		fdp->flag |= EVENT_FDTABLE_FLAG_READ;

		stream->nrefer++;
#if (ACL_EVENTS_KERNEL_STYLE == ACL_EVENTS_STYLE_KQUEUE)
		EVENT_REG_ADD_READ(err, ev->event_fd, sockfd, fdp);
#else
		EVENT_REG_MOD_RDWR(err, ev->event_fd, sockfd, fdp);
#endif
	}

	if (err < 0) {
		acl_msg_fatal("%s: %s(%s): %s, err(%d), fd(%d)", myname,
			EVENT_REG_ADD_TEXT,
			fdp->flag & EVENT_FDTABLE_FLAG_WRITE ? "write also" : "read only",
			acl_last_strerror(ebuf, sizeof(ebuf)), err, sockfd);
	}

	if (eventp->maxfd != ACL_SOCKET_INVALID && eventp->maxfd < sockfd)
		eventp->maxfd = sockfd;

	if (fdp->r_callback != callback || fdp->r_context != context) {
		fdp->r_callback = callback;
		fdp->r_context = context;
	}

	if (timeout > 0) {
		fdp->r_timeout = timeout * 1000000;
		fdp->r_ttl = eventp->event_present + fdp->r_timeout;
	} else {
		fdp->r_ttl = 0;
		fdp->r_timeout = 0;
	}
}

static void event_enable_write(ACL_EVENT *eventp, ACL_VSTREAM *stream,
	int timeout, ACL_EVENT_NOTIFY_RDWR callback, void *context)
{
	const char *myname = "event_enable_write";
	EVENT_KERNEL *ev = (EVENT_KERNEL *) eventp;
	ACL_EVENT_FDTABLE *fdp;
	ACL_SOCKET sockfd;
	char  ebuf[256];
	int   err = 0;

	sockfd = ACL_VSTREAM_SOCK(stream);
	fdp = (ACL_EVENT_FDTABLE *) stream->fdp;
	if (fdp == NULL) {
		fdp = event_fdtable_alloc();

		fdp->flag = EVENT_FDTABLE_FLAG_WRITE | EVENT_FDTABLE_FLAG_EXPT;
		fdp->stream = stream;
		fdp->fdidx = eventp->fdcnt;
		eventp->fdtabs[eventp->fdcnt] = fdp;
		eventp->fdcnt++;

		stream->fdp = (void *) fdp;
		stream->nrefer++;
#ifdef	USE_FDMAP
		acl_fdmap_add(ev->fdmap, sockfd, fdp);
#endif
		EVENT_REG_ADD_WRITE(err, ev->event_fd, sockfd, fdp);
	} else if (fdp->stream != stream) {
		acl_msg_fatal("%s(%d), %s: corrupt stream",
			__FILE__, __LINE__, myname);
	} else if (fdp->fdidx < 0 || fdp->fdidx >= eventp->fdcnt || eventp->fdtabs[fdp->fdidx] != fdp) {
		/* xxx: can this will happen ? */
		
		fdp->flag = EVENT_FDTABLE_FLAG_WRITE | EVENT_FDTABLE_FLAG_EXPT;
		fdp->fdidx = eventp->fdcnt;
		eventp->fdtabs[eventp->fdcnt] = fdp;
		eventp->fdcnt++;

		stream->nrefer++;
#ifdef	USE_FDMAP
		acl_fdmap_add(ev->fdmap, sockfd, fdp);
#endif
		EVENT_REG_ADD_WRITE(err, ev->event_fd, sockfd, fdp);
	} else if (fdp->flag & EVENT_FDTABLE_FLAG_READ) {
		fdp->flag |= EVENT_FDTABLE_FLAG_WRITE;

		stream->nrefer++;
#if (ACL_EVENTS_KERNEL_STYLE == ACL_EVENTS_STYLE_KQUEUE)
		EVENT_REG_ADD_WRITE(err, ev->event_fd, sockfd, fdp);
#else
		EVENT_REG_MOD_RDWR(err, ev->event_fd, sockfd, fdp);
#endif
	}

	if (err < 0) {
		acl_msg_fatal("%s: %s(%s): %s, err(%d), fd(%d)", myname,
			EVENT_REG_ADD_TEXT,
			fdp->flag & EVENT_FDTABLE_FLAG_READ ? "read also" : "write only",
			acl_last_strerror(ebuf, sizeof(ebuf)), err, sockfd);
	}


	if (eventp->maxfd != ACL_SOCKET_INVALID && eventp->maxfd < sockfd)
		eventp->maxfd = sockfd;

	if (fdp->w_callback != callback || fdp->w_context != context) {
		fdp->w_callback = callback;
		fdp->w_context = context;
	}

	if (timeout > 0) {
		fdp->w_timeout = timeout * 1000000;
		fdp->w_ttl = eventp->event_present + fdp->w_timeout;
	} else {
		fdp->w_ttl = 0;
		fdp->w_timeout = 0;
	}
}

/* event_disable_read - disable request for read events */

static void event_disable_read(ACL_EVENT *eventp, ACL_VSTREAM *stream)
{
	const char *myname = "event_disable_read";
	EVENT_KERNEL *ev = (EVENT_KERNEL *) eventp;
	ACL_EVENT_FDTABLE *fdp;
	ACL_SOCKET sockfd;
	int   flag;
	char  ebuf[256];
	int   err = 0;

	sockfd = ACL_VSTREAM_SOCK(stream);
	fdp = (ACL_EVENT_FDTABLE *) stream->fdp;
	if (fdp == NULL) {
		acl_msg_warn("%s(%d): fdp null", myname, __LINE__);
		return;
	}

	if (fdp->fdidx < 0 || fdp->fdidx >= eventp->fdcnt) {
		acl_msg_warn("%s(%d): sockfd(%d)'s fdidx invalid",
			myname, __LINE__, sockfd);
		return;
	}

	if (!(fdp->flag & EVENT_FDTABLE_FLAG_READ)) {
		acl_msg_warn("%s(%d): sockfd(%d) not be set",
			myname, __LINE__, sockfd);
		return;
	}

	flag = fdp->flag;  /* save the fdp's flag */

	if (!(fdp->flag & EVENT_FDTABLE_FLAG_WRITE)) {
		if (eventp->maxfd == sockfd)
			eventp->maxfd = ACL_SOCKET_INVALID;
		if (fdp->fdidx < --eventp->fdcnt) {
			eventp->fdtabs[fdp->fdidx] = eventp->fdtabs[eventp->fdcnt];
			eventp->fdtabs[fdp->fdidx]->fdidx = fdp->fdidx;
		}
		if (fdp->fdidx_ready > 0
		    && fdp->fdidx_ready < eventp->fdcnt_ready
		    && eventp->fdtabs_ready[fdp->fdidx_ready] == fdp)
		{
			eventp->fdtabs_ready[fdp->fdidx_ready] = NULL;
		}
#ifdef	USE_FDMAP
		acl_fdmap_del(ev->fdmap, sockfd);
#endif
		event_fdtable_free(fdp);
		stream->fdp = NULL;
#ifdef	EVENT_REG_DEL_BOTH
		EVENT_REG_DEL_BOTH(err, ev->event_fd, sockfd);
#else
		EVENT_REG_DEL_READ(err, ev->event_fd, sockfd);
#endif
	} else {
		fdp->flag &= ~EVENT_FDTABLE_FLAG_READ;
		fdp->event_type &= ~ACL_EVENT_READ;
		fdp->r_ttl = 0;
		fdp->r_timeout = 0;
		fdp->r_callback = NULL;
#if (ACL_EVENTS_KERNEL_STYLE == ACL_EVENTS_STYLE_KQUEUE)
		EVENT_REG_DEL_READ(err, ev->event_fd, sockfd);
#else
		EVENT_REG_MOD_WRITE(err, ev->event_fd, sockfd, fdp);
#endif
	}

	if (err < 0) {
		acl_msg_fatal("%s: %s: %s, %s, err(%d), fd(%d)", myname,
			EVENT_REG_DEL_TEXT,
			flag & EVENT_FDTABLE_FLAG_WRITE ? "write only" : "no read write",
			acl_last_strerror(ebuf, (int) sizeof(ebuf)), err, sockfd);
	}

	stream->nrefer--;
}

/* event_disable_write - disable request for write events */

static void event_disable_write(ACL_EVENT *eventp, ACL_VSTREAM *stream)
{
	const char *myname = "event_disable_write";
	EVENT_KERNEL *ev = (EVENT_KERNEL *) eventp;
	ACL_EVENT_FDTABLE *fdp;
	ACL_SOCKET sockfd;
	int   flag;
	char  ebuf[256];
	int   err = 0;

	sockfd = ACL_VSTREAM_SOCK(stream);
	fdp = (ACL_EVENT_FDTABLE *) stream->fdp;
	if (fdp == NULL) {
		 acl_msg_warn("%s(%d): fdp null", myname, __LINE__);
		return;
	}

	if (fdp->fdidx < 0 || fdp->fdidx >= eventp->fdcnt) {
		acl_msg_warn("%s(%d): sockfd(%d)'s fdidx(%d) invalid",
			myname, __LINE__, sockfd, fdp->fdidx);
		return;
	}

	if (!(fdp->flag & EVENT_FDTABLE_FLAG_WRITE)) {
		acl_msg_warn("%s(%d): sockfd(%d) not be set",
			myname, __LINE__, sockfd);
		return;
	}

	flag = fdp->flag;  /* save the fdp's flag */

	if (!(fdp->flag & EVENT_FDTABLE_FLAG_READ)) {
		if (eventp->maxfd == sockfd)
			eventp->maxfd = ACL_SOCKET_INVALID;
		if (fdp->fdidx < --eventp->fdcnt) {
			eventp->fdtabs[fdp->fdidx] = eventp->fdtabs[eventp->fdcnt];
			eventp->fdtabs[fdp->fdidx]->fdidx = fdp->fdidx;
		}
		if (fdp->fdidx_ready > 0
		    && fdp->fdidx_ready < eventp->fdcnt_ready
		    && eventp->fdtabs_ready[fdp->fdidx_ready] == fdp)
		{
			eventp->fdtabs_ready[fdp->fdidx_ready] = NULL;
		}
#ifdef	USE_FDMAP
		acl_fdmap_del(ev->fdmap, sockfd);
#endif
		event_fdtable_free(fdp);
		stream->fdp = NULL;
#ifdef	EVENT_REG_DEL_BOTH
		EVENT_REG_DEL_BOTH(err, ev->event_fd, sockfd);
#else
		EVENT_REG_DEL_WRITE(err, ev->event_fd, sockfd);
#endif
	} else {
		fdp->flag &= ~EVENT_FDTABLE_FLAG_WRITE;
		fdp->event_type &= ~ACL_EVENT_WRITE;
		fdp->w_ttl = 0;
		fdp->w_timeout = 0;
		fdp->w_callback = NULL;
#if (ACL_EVENTS_KERNEL_STYLE == ACL_EVENTS_STYLE_KQUEUE)
		EVENT_REG_DEL_WRITE(err, ev->event_fd, sockfd);
#else
		EVENT_REG_MOD_READ(err, ev->event_fd, sockfd, fdp);
#endif
	}

	if (err < 0) {
		acl_msg_fatal("%s: %s(%s): %s, err(%d), fd(%d)", myname,
			EVENT_REG_DEL_TEXT,
			flag & EVENT_FDTABLE_FLAG_READ ? "read only" : "no read write",
			acl_last_strerror(ebuf, sizeof(ebuf)), err, sockfd);
	}

	stream->nrefer--;
}

/* event_disable_readwrite - disable request for read or write events */

static void event_disable_readwrite(ACL_EVENT *eventp, ACL_VSTREAM *stream)
{
	const char *myname = "event_disable_readwrite";
	EVENT_KERNEL *ev = (EVENT_KERNEL *) eventp;
	ACL_EVENT_FDTABLE *fdp;
	ACL_SOCKET sockfd;
	char  ebuf[256];
	int   err = 0;

	sockfd = ACL_VSTREAM_SOCK(stream);
	fdp = (ACL_EVENT_FDTABLE *) stream->fdp;
	if (fdp == NULL) {
		/*
		 * acl_msg_warn("%s(%d): fdp null", myname, __LINE__);
		 */
		return;
	}

	if (fdp->flag == 0 || fdp->fdidx < 0 || fdp->fdidx >= eventp->fdcnt) {
		acl_msg_warn("%s(%d): sockfd(%d) no set, fdp no null",
			myname, __LINE__, sockfd);
		event_fdtable_free(fdp);
		stream->fdp = NULL;
		return;
	}

	if (eventp->maxfd == sockfd)
		eventp->maxfd = ACL_SOCKET_INVALID;
	if (fdp->fdidx < --eventp->fdcnt) {
		eventp->fdtabs[fdp->fdidx] = eventp->fdtabs[eventp->fdcnt];
		eventp->fdtabs[fdp->fdidx]->fdidx = fdp->fdidx;
	}

#ifdef	EVENT_REG_DEL_BOTH
	EVENT_REG_DEL_BOTH(err, ev->event_fd, sockfd);
#else
	if (fdp->flag & EVENT_FDTABLE_FLAG_READ) {
		EVENT_REG_DEL_READ(err, ev->event_fd, sockfd);
	}
	if (fdp->flag & EVENT_FDTABLE_FLAG_WRITE) {
		EVENT_REG_DEL_WRITE(err, ev->event_fd, sockfd);
	}
#endif

	if (fdp->flag & EVENT_FDTABLE_FLAG_READ) {
		stream->nrefer--;
	}
	if (fdp->flag & EVENT_FDTABLE_FLAG_WRITE) {
		stream->nrefer--;
	}

	if (err < 0) {
		acl_msg_fatal("%s: %s: %s", myname, EVENT_REG_DEL_TEXT,
			acl_last_strerror(ebuf, sizeof(ebuf)));
	}

#ifdef	USE_FDMAP
	acl_fdmap_del(ev->fdmap, sockfd);
#endif
	if (fdp->fdidx_ready > 0
	    && fdp->fdidx_ready < eventp->fdcnt_ready
	    && eventp->fdtabs_ready[fdp->fdidx_ready] == fdp)
	{
		eventp->fdtabs_ready[fdp->fdidx_ready] = NULL;
	}
	event_fdtable_free(fdp);
	stream->fdp = NULL;
}

static int event_isrset(ACL_EVENT *eventp acl_unused, ACL_VSTREAM *stream)
{
	ACL_EVENT_FDTABLE *fdp;

	fdp = (ACL_EVENT_FDTABLE *) stream->fdp;
	if (fdp == NULL) {
		return (0);
	}

	return ((fdp->flag & EVENT_FDTABLE_FLAG_READ));
}

static int event_iswset(ACL_EVENT *eventp acl_unused, ACL_VSTREAM *stream)
{
	ACL_EVENT_FDTABLE *fdp;

	fdp = (ACL_EVENT_FDTABLE *) stream->fdp;
	if (fdp == NULL) {
		return (0);
	}

	return ((fdp->flag & EVENT_FDTABLE_FLAG_WRITE));

}

static int event_isxset(ACL_EVENT *eventp acl_unused, ACL_VSTREAM *stream)
{
	ACL_EVENT_FDTABLE *fdp;

	fdp = (ACL_EVENT_FDTABLE *) stream->fdp;
	if (fdp == NULL) {
		return (0);
	}

	return ((fdp->flag & EVENT_FDTABLE_FLAG_EXPT));
}

static void event_loop(ACL_EVENT *eventp)
{
	const char *myname = "event_loop";
	EVENT_KERNEL *ev = (EVENT_KERNEL *) eventp;
	ACL_EVENT_NOTIFY_FN worker_fn;
	void    *worker_arg;
	ACL_EVENT_TIMER *timer;
	int   delay, n, nready;
	ACL_EVENT_FDTABLE *fdp;
	EVENT_BUFFER *bp;

	delay = eventp->delay_sec * 1000 + eventp->delay_usec / 1000;
	if (delay <= 0)
		delay = 100; /* 100 milliseconds at least */

	SET_TIME(eventp->event_present);

	/*
	 * Find out when the next timer would go off. Timer requests are sorted.
	 * If any timer is scheduled, adjust the delay appropriately.
	 */
	if ((timer = ACL_FIRST_TIMER(&eventp->timer_head)) != 0) {
		n = (int) (timer->when - eventp->event_present + 1000000 - 1) / 1000000;
		if (n <= 0) {
			delay = 0;
		} else if (delay > eventp->delay_sec) {
			delay = n * 1000 + eventp->delay_usec / 1000;
		}
	}

	if (event_prepare(eventp) == 0) {
		if (eventp->fdcnt_ready == 0) {
			sleep(1);
		}
		goto TAG_DONE;
	}

	if (eventp->fdcnt_ready > 0)
		delay = 0;

	EVENT_BUFFER_READ(nready,
			ev->event_fd,
			ev->event_buf,
			ev->event_fdslots,
			delay);

	if (eventp->nested++ > 0)
		acl_msg_fatal("%s(%d): recursive call", myname, __LINE__);
	if (nready < 0) {
		if (acl_last_error() != ACL_EINTR) {
			char  ebuf[256];
			acl_msg_fatal("%s(%d), %s: select: %s",
				__FILE__, __LINE__, myname,
				acl_last_strerror(ebuf, sizeof(ebuf)));
		}
		goto TAG_DONE;
	} else if (nready == 0)
		goto TAG_DONE;

	for (bp = ev->event_buf; bp < ev->event_buf + nready; bp++) {
#ifdef	USE_FDMAP
		ACL_SOCKET sockfd;

		sockfd = EVENT_GET_FD(bp);
		fdp = acl_fdmap_ctx(ev->fdmap, sockfd);
		if (fdp == NULL || fdp->stream == NULL)
			continue;
		if (sockfd != ACL_VSTREAM_SOCK(fdp->stream))
			acl_msg_fatal("%s(%d): sockfd(%d) != %d",
				myname, __LINE__, sockfd, ACL_VSTREAM_SOCK(fdp->stream));
#else
		fdp = (ACL_EVENT_FDTABLE *) EVENT_GET_CTX(bp);
		if (fdp == NULL || fdp->stream == NULL)
			continue;
#endif

		if ((fdp->event_type & (ACL_EVENT_XCPT | ACL_EVENT_RW_TIMEOUT)))
			continue;

		if ((fdp->flag & EVENT_FDTABLE_FLAG_READ) && EVENT_TEST_READ(bp)) {
			fdp->stream->sys_read_ready = 1;
			if ((fdp->event_type & (ACL_EVENT_READ | ACL_EVENT_WRITE)) == 0)
			{
				fdp->event_type |= ACL_EVENT_READ;
				fdp->fdidx_ready = eventp->fdcnt_ready;
				eventp->fdtabs_ready[eventp->fdcnt_ready++] = fdp;
			}
		}

		if ((fdp->flag & EVENT_FDTABLE_FLAG_WRITE) && EVENT_TEST_WRITE(bp)) {
			if ((fdp->event_type & (ACL_EVENT_READ | ACL_EVENT_WRITE)) == 0)
			{
				fdp->event_type |= ACL_EVENT_WRITE;
				fdp->fdidx_ready = eventp->fdcnt_ready;
				eventp->fdtabs_ready[eventp->fdcnt_ready++] = fdp;
			}
		}
#ifdef	EVENT_TEST_ERROR
		if (EVENT_TEST_ERROR(bp)) {
			if ((fdp->event_type & (ACL_EVENT_READ | ACL_EVENT_WRITE)) == 0)
			{
				fdp->event_type |= ACL_EVENT_XCPT;
				fdp->fdidx_ready = eventp->fdcnt_ready;
				eventp->fdtabs_ready[eventp->fdcnt_ready++] = fdp;
			}
		}
#endif
	}

TAG_DONE:

	/*
	 * Deliver timer events. Requests are sorted: we can stop when we reach
	 * the future or the list end. Allow the application to update the timer
	 * queue while it is being called back. To this end, we repeatedly pop
	 * the first request off the timer queue before delivering the event to
	 * the application.
	 */
	SET_TIME(eventp->event_present);
	while ((timer = ACL_FIRST_TIMER(&eventp->timer_head)) != 0) {
		if (timer->when > eventp->event_present)
			break;
		worker_fn  = timer->callback;
		worker_arg = timer->context;

		/* 如果定时器的时间间隔 > 0 且允许定时器被循环调用，则再重设定时器 */
		if (timer->delay > 0 && timer->keep) {
			timer->ncount++;
			eventp->timer_request(eventp, timer->callback,
				timer->context, timer->delay, timer->keep);
		} else {
			acl_ring_detach(&timer->ring);		/* first this */
			timer->nrefer--;
			if (timer->nrefer != 0)
				acl_msg_fatal("%s(%d): nrefer(%d) != 0",
					myname, __LINE__, timer->nrefer);
			acl_myfree(timer);
		}
		worker_fn(ACL_EVENT_TIME, worker_arg);
	}

	event_fire(eventp);
	eventp->nested--;
}

static void event_free(ACL_EVENT *eventp)
{
	const char *myname = "event_free";
	EVENT_KERNEL *ev = (EVENT_KERNEL *) eventp;

	if (eventp == NULL)
		acl_msg_fatal("%s, %s(%d): eventp null", __FILE__, myname, __LINE__);

#ifdef	USE_FDMAP
	acl_fdmap_free(ev->fdmap);
#endif
	acl_myfree(ev->event_buf);
	close(ev->event_fd);
	acl_myfree(ev);
}

ACL_EVENT *event_new_kernel2(int fdsize acl_unused)
{
	ACL_EVENT *eventp;
	EVENT_KERNEL *ev;
	static int __default_max_events = 1000;

	eventp = event_alloc(sizeof(EVENT_KERNEL));

	snprintf(eventp->name, sizeof(eventp->name), "events - %s", EVENT_NAME);
	eventp->event_mode           = ACL_EVENT_KERNEL2;
	eventp->use_thread           = 0;
	eventp->loop_fn              = event_loop;
	eventp->free_fn              = event_free;
	eventp->enable_read_fn       = event_enable_read;
	eventp->enable_write_fn      = event_enable_write;
	eventp->enable_listen_fn     = event_enable_read;
	eventp->disable_read_fn      = event_disable_read;
	eventp->disable_write_fn     = event_disable_write;
	eventp->disable_readwrite_fn = event_disable_readwrite;
	eventp->isrset_fn            = event_isrset;
	eventp->iswset_fn            = event_iswset;
	eventp->isxset_fn            = event_isxset;
	eventp->timer_request        = event_timer_request;
	eventp->timer_cancel         = event_timer_cancel;
	eventp->timer_keep           = event_timer_keep;
	eventp->timer_ifkeep         = event_timer_ifkeep;

	ev = (EVENT_KERNEL*) eventp;
	EVENT_REG_INIT_HANDLE(ev->event_fd, fdsize);
	ev->event_fdslots = __default_max_events;
	ev->event_buf = (EVENT_BUFFER *) acl_mycalloc(ev->event_fdslots + 1, sizeof(EVENT_BUFFER));
#ifdef	USE_FDMAP
	ev->fdmap = acl_fdmap_create(fdsize);
#endif
	return (eventp);
}
#endif	/* ACL_EVENTS_KERNEL_STYLE */
