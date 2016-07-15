#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <time.h>

#if 0
#define	NDEBUG
#endif

#include <errno.h>

#ifdef	ACL_UNIX
#include <unistd.h>
#endif

#include "stdlib/acl_sys_patch.h"
#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_mystring.h"
#include "stdlib/acl_msg.h"
#include "stdlib/acl_debug.h"
#include "stdlib/acl_vstream.h"
#include "stdlib/acl_fifo.h"
#include "net/acl_sane_socket.h"
#include "stdlib/acl_meter_time.h"	/* just for performance test */
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
	ACL_RING fdp_delay_list;
	int   event_fdslots;
	int   event_fd;
#ifdef	USE_FDMAP
	ACL_FD_MAP *fdmap;
#endif
} EVENT_KERNEL;

/**
 * 对于 epoll/kqueue/devpoll 这些内核级操作，当描述符关闭时该描述符将
 * 会自动从内核监控中去除
 */
#undef	EVENT_AUTO_DEL

#include "stdlib/acl_meter_time.h"

static void stream_on_close(ACL_VSTREAM *stream, void *arg)
{
	const char *myname = "stream_on_close";
	EVENT_KERNEL *ev = (EVENT_KERNEL*) arg;
	ACL_EVENT_FDTABLE *fdp = (ACL_EVENT_FDTABLE*) stream->fdp;
	ACL_SOCKET sockfd = ACL_VSTREAM_SOCK(stream);
	int   err = 0, ret = 0;

	if (fdp == NULL)
		return;

#ifdef EVENT_REG_DEL_BOTH
	if ((fdp->flag & EVENT_FDTABLE_FLAG_READ)
		|| (fdp->flag & EVENT_FDTABLE_FLAG_WRITE))
	{
# ifndef EVENT_AUTO_DEL
		EVENT_REG_DEL_BOTH(err, ev->event_fd, sockfd);
		ret = 1;
# else
		ret = 2;
# endif
	}
#else
	if ((fdp->flag & EVENT_FDTABLE_FLAG_READ)
		&& (fdp->flag & EVENT_FDTABLE_FLAG_WRITE))
	{
# ifndef EVENT_AUTO_DEL
		EVENT_REG_DEL_READ(err, ev->event_fd, sockfd);
		EVENT_REG_DEL_WRITE(err, ev->event_fd, sockfd);
		ret = 3;
# else
		ret = 4;
# endif
	} else if ((fdp->flag & EVENT_FDTABLE_FLAG_READ)) {
# ifndef EVENT_AUTO_DEL
		EVENT_REG_DEL_READ(err, ev->event_fd, sockfd);
		ret = 5;
# else
		ret = 6;
# endif
	} else if ((fdp->flag & EVENT_FDTABLE_FLAG_WRITE)) {
# ifndef EVENT_AUTO_DEL
		EVENT_REG_DEL_WRITE(err, ev->event_fd, sockfd);
# else
		ret = 7;
# endif
		ret = 8;
	}
#endif

	if (err < 0) {
		acl_msg_fatal("%s: %s: %s, err(%d), fd(%d), ret(%d)",
			myname, EVENT_REG_DEL_TEXT, acl_last_serror(),
			err, sockfd, ret);
	}

	if ((fdp->flag & EVENT_FDTABLE_FLAG_DELAY_OPER)) {
		fdp->flag &= ~EVENT_FDTABLE_FLAG_DELAY_OPER;
		acl_ring_detach(&fdp->delay_entry);
	}

	if (ret) {
#ifdef	USE_FDMAP
		acl_fdmap_del(ev->fdmap, sockfd);
#endif
	}

	if (ev->event.maxfd == ACL_VSTREAM_SOCK(fdp->stream))
		ev->event.maxfd = ACL_SOCKET_INVALID;
	if (fdp->fdidx >= 0 && fdp->fdidx < --ev->event.fdcnt) {
		ev->event.fdtabs[fdp->fdidx] = ev->event.fdtabs[ev->event.fdcnt];
		ev->event.fdtabs[fdp->fdidx]->fdidx = fdp->fdidx;
	}
	fdp->fdidx = -1;

	if (fdp->fdidx_ready >= 0
		&& fdp->fdidx_ready < ev->event.ready_cnt
		&& ev->event.ready[fdp->fdidx_ready] == fdp)
	{
		ev->event.ready[fdp->fdidx_ready] = NULL;
	}

	fdp->fdidx_ready = -1;
	event_fdtable_free(fdp);
	stream->fdp = NULL;
}

static ACL_EVENT_FDTABLE *read_enable(ACL_EVENT *eventp, ACL_VSTREAM *stream,
	int timeout, ACL_EVENT_NOTIFY_RDWR callback, void *context)
{
	EVENT_KERNEL *ev = (EVENT_KERNEL *) eventp;
	ACL_EVENT_FDTABLE *fdp = (ACL_EVENT_FDTABLE *) stream->fdp;
	ACL_SOCKET sockfd = ACL_VSTREAM_SOCK(stream);

	if (fdp == NULL) {
		fdp = event_fdtable_alloc();
		fdp->flag = EVENT_FDTABLE_FLAG_ADD_READ | EVENT_FDTABLE_FLAG_EXPT;
		fdp->stream = stream;
		acl_ring_append(&ev->fdp_delay_list, &fdp->delay_entry);
		fdp->flag |= EVENT_FDTABLE_FLAG_DELAY_OPER;

		stream->fdp = (void *) fdp;
		/* 添加流关闭时的回调函数 */
		acl_vstream_add_close_handle(stream, stream_on_close, eventp);
#ifdef	USE_FDMAP
		acl_fdmap_add(ev->fdmap, sockfd, fdp);
#endif
	} else if ((fdp->flag & EVENT_FDTABLE_FLAG_ADD_READ)) {
		goto END;
	} else if ((fdp->flag & EVENT_FDTABLE_FLAG_DEL_READ)) {

		/* 停止禁止读监听过程 */

		acl_assert((fdp->flag & EVENT_FDTABLE_FLAG_READ));

		/* 重新启用读监听过程, 因为之前的过程是正在拆除读监听过程但
		 * 还没有正式拆除，所以只需要清除拆除标志位即可
		 */

		fdp->flag &= ~EVENT_FDTABLE_FLAG_DEL_READ;
	} else if (!(fdp->flag & EVENT_FDTABLE_FLAG_READ)) {
		fdp->flag |= EVENT_FDTABLE_FLAG_ADD_READ;
		if (!(fdp->flag & EVENT_FDTABLE_FLAG_DELAY_OPER)) {
			acl_ring_append(&ev->fdp_delay_list, &fdp->delay_entry);
			fdp->flag |= EVENT_FDTABLE_FLAG_DELAY_OPER;
		}
	}

END:
	if (fdp->fdidx == -1) {
		fdp->fdidx = eventp->fdcnt;
		eventp->fdtabs[eventp->fdcnt++] = fdp;
	}
	if (eventp->maxfd != ACL_SOCKET_INVALID && eventp->maxfd < sockfd)
		eventp->maxfd = sockfd;

	if (fdp->r_callback != callback || fdp->r_context != context) {
		fdp->r_callback = callback;
		fdp->r_context = context;
	}

	if (stream->read_ready || ACL_VSTREAM_BFRD_CNT(stream) > 0)
		eventp->read_ready++;

	if (timeout > 0) {
		fdp->r_timeout = ((acl_int64) timeout) * 1000000;
		fdp->r_ttl = eventp->present + fdp->r_timeout;
	} else {
		fdp->r_ttl = 0;
		fdp->r_timeout = 0;
	}

	return fdp;
}

static void event_enable_listen(ACL_EVENT *eventp, ACL_VSTREAM *stream,
	int timeout, ACL_EVENT_NOTIFY_RDWR callback, void *context)
{
	ACL_EVENT_FDTABLE *fdp = read_enable(eventp, stream, timeout,
			callback, context);
#if defined(ACL_MACOSX)
	fdp->listener = 1;
#else
	fdp->listener = acl_is_listening_socket(ACL_VSTREAM_SOCK(stream));
#endif
}

static void event_enable_read(ACL_EVENT *eventp, ACL_VSTREAM *stream,
	int timeout, ACL_EVENT_NOTIFY_RDWR callback, void *context)
{
	ACL_EVENT_FDTABLE *fdp = read_enable(eventp, stream, timeout,
			callback, context);
	fdp->listener = acl_is_listening_socket(ACL_VSTREAM_SOCK(stream));
}

static void event_enable_write(ACL_EVENT *eventp, ACL_VSTREAM *stream,
	int timeout, ACL_EVENT_NOTIFY_RDWR callback, void *context)
{
	EVENT_KERNEL *ev = (EVENT_KERNEL *) eventp;
	ACL_EVENT_FDTABLE *fdp = (ACL_EVENT_FDTABLE *) stream->fdp;
	ACL_SOCKET sockfd = ACL_VSTREAM_SOCK(stream);

	if (fdp == NULL) {
		fdp = event_fdtable_alloc();

		fdp->flag = EVENT_FDTABLE_FLAG_ADD_WRITE | EVENT_FDTABLE_FLAG_EXPT;
		fdp->stream = stream;
		acl_ring_append(&ev->fdp_delay_list, &fdp->delay_entry);
		fdp->flag |= EVENT_FDTABLE_FLAG_DELAY_OPER;
		stream->fdp = (void *) fdp;
		/* 添加流关闭时的回调函数 */
		acl_vstream_add_close_handle(stream, stream_on_close, eventp);
#ifdef	USE_FDMAP
		acl_fdmap_add(ev->fdmap, sockfd, fdp);
#endif
	} else if ((fdp->flag & EVENT_FDTABLE_FLAG_ADD_WRITE)) {
		goto END;
	} else if ((fdp->flag & EVENT_FDTABLE_FLAG_DEL_WRITE)) {
		acl_assert((fdp->flag & EVENT_FDTABLE_FLAG_WRITE));

		fdp->flag &= ~EVENT_FDTABLE_FLAG_DEL_WRITE;
	} else if (!(fdp->flag & EVENT_FDTABLE_FLAG_WRITE)) {
		fdp->flag |= EVENT_FDTABLE_FLAG_ADD_WRITE;
		if (!(fdp->flag & EVENT_FDTABLE_FLAG_DELAY_OPER)) {
			acl_ring_append(&ev->fdp_delay_list, &fdp->delay_entry);
			fdp->flag |= EVENT_FDTABLE_FLAG_DELAY_OPER;
		}
	}

END:
	if (fdp->fdidx == -1) {
		fdp->fdidx = eventp->fdcnt;
		eventp->fdtabs[eventp->fdcnt++] = fdp;
	}

	if (eventp->maxfd != ACL_SOCKET_INVALID && eventp->maxfd < sockfd)
		eventp->maxfd = sockfd;

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
}

/* event_disable_read - disable request for read events */

static void event_disable_read(ACL_EVENT *eventp, ACL_VSTREAM *stream)
{
	const char *myname = "event_disable_read";
	EVENT_KERNEL *ev = (EVENT_KERNEL *) eventp;
	ACL_EVENT_FDTABLE *fdp = (ACL_EVENT_FDTABLE *) stream->fdp;

	if (fdp == NULL) {
		acl_msg_warn("%s(%d): fdp null", myname, __LINE__);
		return;
	}
	if (fdp->fdidx < 0 || fdp->fdidx >= eventp->fdcnt) {
		acl_msg_warn("%s(%d): sockfd(%d)'s fdidx(%d) invalid, fdcnt: %d",
			myname, __LINE__, ACL_VSTREAM_SOCK(stream),
			fdp->fdidx, eventp->fdcnt);
		return;
	}
	if ((fdp->flag & EVENT_FDTABLE_FLAG_DEL_READ)) {
		return;
	}
	if ((fdp->flag & EVENT_FDTABLE_FLAG_ADD_READ)) {
		fdp->flag &= ~EVENT_FDTABLE_FLAG_ADD_READ;
		goto DEL_READ_TAG;
	}

	if (!(fdp->flag & EVENT_FDTABLE_FLAG_READ)) {
		acl_msg_warn("%s(%d): sockfd(%d) not be set",
			myname, __LINE__, ACL_VSTREAM_SOCK(stream));
		return;
	}
	fdp->flag |= EVENT_FDTABLE_FLAG_DEL_READ;
	if (!(fdp->flag & EVENT_FDTABLE_FLAG_DELAY_OPER)) {
		acl_ring_append(&ev->fdp_delay_list, &fdp->delay_entry);
		fdp->flag |= EVENT_FDTABLE_FLAG_DELAY_OPER;
	}

DEL_READ_TAG:

	fdp->r_ttl = 0;
	fdp->r_timeout = 0;
	fdp->r_callback = NULL;
	fdp->event_type &= ~(ACL_EVENT_READ | ACL_EVENT_ACCEPT);

	if ((fdp->flag & EVENT_FDTABLE_FLAG_WRITE)
		|| (fdp->flag & EVENT_FDTABLE_FLAG_ADD_WRITE))
	{
		return;
	}

	if (eventp->maxfd == ACL_VSTREAM_SOCK(fdp->stream))
		eventp->maxfd = ACL_SOCKET_INVALID;

	if (fdp->fdidx < --eventp->fdcnt) {
		eventp->fdtabs[fdp->fdidx] = eventp->fdtabs[eventp->fdcnt];
		eventp->fdtabs[fdp->fdidx]->fdidx = fdp->fdidx;
	}
	fdp->fdidx = -1;

	if (fdp->fdidx_ready >= 0
		&& fdp->fdidx_ready < eventp->ready_cnt
		&& eventp->ready[fdp->fdidx_ready] == fdp)
	{
		eventp->ready[fdp->fdidx_ready] = NULL;
	}
	fdp->fdidx_ready = -1;
}

/* event_disable_write - disable request for write events */

static void event_disable_write(ACL_EVENT *eventp, ACL_VSTREAM *stream)
{
	const char *myname = "event_disable_write";
	EVENT_KERNEL *ev = (EVENT_KERNEL *) eventp;
	ACL_EVENT_FDTABLE *fdp = (ACL_EVENT_FDTABLE *) stream->fdp;

	if (fdp == NULL) {
		acl_msg_warn("%s(%d): fdp null", myname, __LINE__);
		return;
	}
	if (fdp->fdidx < 0 || fdp->fdidx >= eventp->fdcnt) {
		acl_msg_warn("%s(%d): sockfd(%d)'s fdidx(%d) invalid",
			myname, __LINE__, ACL_VSTREAM_SOCK(stream), fdp->fdidx);
		return;
	}
	if ((fdp->flag & EVENT_FDTABLE_FLAG_DEL_WRITE)) {
		return;
	}
	if ((fdp->flag & EVENT_FDTABLE_FLAG_ADD_WRITE)) {
		fdp->flag &= ~EVENT_FDTABLE_FLAG_ADD_WRITE;
		goto DEL_WRITE_TAG;
	}
	if (!(fdp->flag & EVENT_FDTABLE_FLAG_WRITE)) {
		acl_msg_warn("%s(%d): sockfd(%d) not be set",
			myname, __LINE__, ACL_VSTREAM_SOCK(stream));
		return;
	}

	fdp->flag |= EVENT_FDTABLE_FLAG_DEL_WRITE;
	if (!(fdp->flag & EVENT_FDTABLE_FLAG_DELAY_OPER)) {
		acl_ring_append(&ev->fdp_delay_list, &fdp->delay_entry);
		fdp->flag |= EVENT_FDTABLE_FLAG_DELAY_OPER;
	}

DEL_WRITE_TAG:

	fdp->w_ttl = 0;
	fdp->w_timeout = 0;
	fdp->w_callback = NULL;
	fdp->event_type &= ~(ACL_EVENT_WRITE | ACL_EVENT_CONNECT);

	if ((fdp->flag & EVENT_FDTABLE_FLAG_READ)
		|| (fdp->flag & EVENT_FDTABLE_FLAG_ADD_READ))
	{
		return;
	}

	if (eventp->maxfd == ACL_VSTREAM_SOCK(stream))
		eventp->maxfd = ACL_SOCKET_INVALID;

	if (fdp->fdidx < --eventp->fdcnt) {
		eventp->fdtabs[fdp->fdidx] = eventp->fdtabs[eventp->fdcnt];
		eventp->fdtabs[fdp->fdidx]->fdidx = fdp->fdidx;
	}
	fdp->fdidx = -1;

	if (fdp->fdidx_ready >= 0
		&& fdp->fdidx_ready < eventp->ready_cnt
		&& eventp->ready[fdp->fdidx_ready] == fdp)
	{
		eventp->ready[fdp->fdidx_ready] = NULL;
	}
	fdp->fdidx_ready = -1;
}

/* event_disable_readwrite - disable request for read or write events */

static void event_disable_readwrite(ACL_EVENT *eventp, ACL_VSTREAM *stream)
{
	const char *myname = "event_disable_readwrite";
	EVENT_KERNEL *ev = (EVENT_KERNEL *) eventp;
	ACL_SOCKET sockfd = ACL_VSTREAM_SOCK(stream);
	ACL_EVENT_FDTABLE *fdp = (ACL_EVENT_FDTABLE *) stream->fdp;
	int   err = 0;

	if (fdp == NULL)
		return;

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
	fdp->fdidx = -1;

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

	if (err < 0) {
		acl_msg_fatal("%s: %s: %s", myname, EVENT_REG_DEL_TEXT,
			acl_last_serror());
	}

#ifdef	USE_FDMAP
	acl_fdmap_del(ev->fdmap, sockfd);
#endif
	if (fdp->fdidx_ready >= 0
	    && fdp->fdidx_ready < eventp->ready_cnt
	    && eventp->ready[fdp->fdidx_ready] == fdp)
	{
		eventp->ready[fdp->fdidx_ready] = NULL;
	}
	fdp->fdidx_ready = -1;
	event_fdtable_free(fdp);
	stream->fdp = NULL;
}

static void enable_read(EVENT_KERNEL *ev, ACL_EVENT_FDTABLE *fdp)
{
	const char *myname = "enable_read";
	ACL_SOCKET sockfd = ACL_VSTREAM_SOCK(fdp->stream);
	int   err;

	fdp->flag &= ~EVENT_FDTABLE_FLAG_ADD_READ;
	fdp->flag |= EVENT_FDTABLE_FLAG_READ;

	if ((fdp->flag & EVENT_FDTABLE_FLAG_WRITE)) {
#if (ACL_EVENTS_KERNEL_STYLE == ACL_EVENTS_STYLE_KQUEUE)
		EVENT_REG_ADD_READ(err, ev->event_fd, sockfd, fdp);
#else
		EVENT_REG_MOD_RDWR(err, ev->event_fd, sockfd, fdp);
#endif
	} else {
		EVENT_REG_ADD_READ(err, ev->event_fd, sockfd, fdp);
	}
	if (err < 0) {
		acl_msg_fatal("%s: %s: %s, err(%d), fd(%d)",
			myname, EVENT_REG_ADD_TEXT,
			acl_last_serror(), err, sockfd);
	}
}

static void enable_write(EVENT_KERNEL *ev, ACL_EVENT_FDTABLE *fdp)
{
	const char *myname = "enable_write";
	ACL_SOCKET sockfd = ACL_VSTREAM_SOCK(fdp->stream);
	int   err;

	fdp->flag &= ~EVENT_FDTABLE_FLAG_ADD_WRITE;
	fdp->flag |= EVENT_FDTABLE_FLAG_WRITE;

	if ((fdp->flag & EVENT_FDTABLE_FLAG_READ)) {
#if (ACL_EVENTS_KERNEL_STYLE == ACL_EVENTS_STYLE_KQUEUE)
		EVENT_REG_ADD_WRITE(err, ev->event_fd, sockfd, fdp);
#else
		EVENT_REG_MOD_RDWR(err, ev->event_fd, sockfd, fdp);
#endif
	} else {
		EVENT_REG_ADD_WRITE(err, ev->event_fd, sockfd, fdp);
	}

	if (err < 0) {
		acl_msg_fatal("%s: %s: %s, err(%d), fd(%d)",
			myname, EVENT_REG_ADD_TEXT,
			acl_last_serror(), err, sockfd);
	}
}

static int disable_read(EVENT_KERNEL *ev, ACL_EVENT_FDTABLE *fdp)
{
	const char *myname = "disable_read";
	ACL_VSTREAM *stream = fdp->stream;
	ACL_SOCKET sockfd = ACL_VSTREAM_SOCK(stream);
	int   err, ret = 0;

	fdp->flag &= ~EVENT_FDTABLE_FLAG_DEL_READ;
	fdp->flag &= ~EVENT_FDTABLE_FLAG_READ;
	fdp->event_type &= ~(ACL_EVENT_READ | ACL_EVENT_ACCEPT);

	if ((fdp->flag & EVENT_FDTABLE_FLAG_WRITE)) {
#if (ACL_EVENTS_KERNEL_STYLE == ACL_EVENTS_STYLE_KQUEUE)
		EVENT_REG_DEL_READ(err, ev->event_fd, sockfd);
#else
		EVENT_REG_MOD_WRITE(err, ev->event_fd, sockfd, fdp);
#endif
	} else {
#ifdef	EVENT_REG_DEL_BOTH
		EVENT_REG_DEL_BOTH(err, ev->event_fd, sockfd);
#else
		EVENT_REG_DEL_READ(err, ev->event_fd, sockfd);
#endif
#ifdef	USE_FDMAP
		acl_fdmap_del(ev->fdmap, sockfd);
#endif
		ret = 1;
	}
	if (err < 0) {
		acl_msg_fatal("%s: %s: %s, err(%d), fd(%d), ret(%d)",
			myname, EVENT_REG_DEL_TEXT, acl_last_serror(),
			err, sockfd, ret);
	}
	return ret;
}

static int disable_write(EVENT_KERNEL *ev, ACL_EVENT_FDTABLE *fdp)
{
	const char *myname = "disable_write";
	ACL_VSTREAM *stream = fdp->stream;
	ACL_SOCKET sockfd = ACL_VSTREAM_SOCK(stream);
	int   err, ret = 0;

	fdp->flag &= ~EVENT_FDTABLE_FLAG_DEL_WRITE;
	fdp->flag &= ~EVENT_FDTABLE_FLAG_WRITE;
	fdp->event_type &= ~(ACL_EVENT_WRITE | ACL_EVENT_CONNECT);

	if ((fdp->flag & EVENT_FDTABLE_FLAG_READ)) {
#if (ACL_EVENTS_KERNEL_STYLE == ACL_EVENTS_STYLE_KQUEUE)
		EVENT_REG_DEL_WRITE(err, ev->event_fd, sockfd);
#else
		EVENT_REG_MOD_READ(err, ev->event_fd, sockfd, fdp);
#endif
	} else {
#ifdef	USE_FDMAP
		acl_fdmap_del(ev->fdmap, sockfd);
#endif
#ifdef	EVENT_REG_DEL_BOTH
		EVENT_REG_DEL_BOTH(err, ev->event_fd, sockfd);
#else
		EVENT_REG_DEL_WRITE(err, ev->event_fd, sockfd);
#endif
		ret = 1;
	}
	if (err < 0) {
		acl_msg_fatal("%s: %s: %s, err(%d), fd(%d), ret(%d)",
			myname, EVENT_REG_DEL_TEXT, acl_last_serror(),
			err, sockfd, ret);
	}
	return (ret);
}

static void event_set_all(ACL_EVENT *eventp)
{
	EVENT_KERNEL *ev = (EVENT_KERNEL *) eventp;
	ACL_EVENT_FDTABLE *fdp;

	/* 优先处理添加读/写监控任务, 这样可以把 ADD 中间态转换成正式状态 */

	eventp->ready_cnt = 0;

	if (eventp->present - eventp->last_check >= eventp->check_inter
		|| eventp->read_ready > 0)
	{
		eventp->read_ready = 0;
		eventp->last_check = eventp->present;
		event_check_fds(eventp);
	}

	/* 处理任务项 */

	while (1) {
		ACL_RING *r = acl_ring_pop_head(&ev->fdp_delay_list);
		if (r == NULL)
			break;
		fdp = acl_ring_to_appl(r, ACL_EVENT_FDTABLE, delay_entry);

		if ((fdp->flag & EVENT_FDTABLE_FLAG_ADD_READ)) {
			enable_read(ev, fdp);
		}
		if ((fdp->flag & EVENT_FDTABLE_FLAG_ADD_WRITE)) {
			enable_write(ev, fdp);
		}
		if ((fdp->flag & EVENT_FDTABLE_FLAG_DEL_READ)) {
			disable_read(ev, fdp);
		}
		if ((fdp->flag & EVENT_FDTABLE_FLAG_DEL_WRITE)) {
			disable_write(ev, fdp);
		}

		fdp->flag &= ~EVENT_FDTABLE_FLAG_DELAY_OPER;
	}
}

static void event_loop(ACL_EVENT *eventp)
{
	const char *myname = "event_loop";
	EVENT_KERNEL *ev = (EVENT_KERNEL *) eventp;
	ACL_EVENT_NOTIFY_TIME timer_fn;
	void    *timer_arg;
	ACL_EVENT_TIMER *timer;
	int   delay, nready;
	ACL_EVENT_FDTABLE *fdp;
	EVENT_BUFFER *bp;

	delay = (int) (eventp->delay_sec * 1000 + eventp->delay_usec / 1000);
	if (delay < 0)
		delay = 0; /* 0 milliseconds at least */

	/* 调整事件引擎的时间截 */

	SET_TIME(eventp->present);

	/* 根据定时器任务的最近任务计算 epoll/kqueue/devpoll 的检测超时上限 */

	if ((timer = ACL_FIRST_TIMER(&eventp->timer_head)) != 0) {
		acl_int64 n = (timer->when - eventp->present) / 1000;

		if (n <= 0)
			delay = 0;
		else if ((int) n < delay) {
			delay = (int) n;
			if (delay <= 0)  /* xxx */
				delay = 100;
		}
	}

	/* 设置描述字对象的状态，添加/删除之前设置的描述字对象 */

	event_set_all(eventp);

	if (eventp->fdcnt == 0) {
		if (eventp->ready_cnt == 0)
			sleep(1);
		goto TAG_DONE;
	}

	/* 如果已经有描述字准备好则检测超时时间置 0 */

	if (eventp->ready_cnt > 0)
		delay = 0;

	/* 调用 epoll/kquque/devpoll 系统调用检测可用描述字 */

	EVENT_BUFFER_READ(nready, ev->event_fd, ev->event_buf,
		ev->event_fdslots, delay);

	if (eventp->nested++ > 0)
		acl_msg_fatal("%s(%d): recursive call, nested: %d",
			myname, __LINE__, eventp->nested);
	if (nready < 0) {
		if (acl_last_error() != ACL_EINTR) {
			acl_msg_fatal("%s(%d), %s: select: %s", __FILE__,
				__LINE__, myname, acl_last_serror());
		}
		goto TAG_DONE;
	} else if (nready == 0)
		goto TAG_DONE;

	/* 检查检测结果 */

	for (bp = ev->event_buf; bp < ev->event_buf + nready; bp++) {
#ifdef	USE_FDMAP
		ACL_SOCKET sockfd;

		sockfd = EVENT_GET_FD(bp);
		fdp = acl_fdmap_ctx(ev->fdmap, sockfd);
		if (fdp == NULL || fdp->stream == NULL)
			continue;
		if (sockfd != ACL_VSTREAM_SOCK(fdp->stream))
			acl_msg_fatal("%s(%d): sockfd(%d) != %d", myname,
				__LINE__, sockfd, ACL_VSTREAM_SOCK(fdp->stream));
#else
		fdp = (ACL_EVENT_FDTABLE *) EVENT_GET_CTX(bp);
		if (fdp == NULL || fdp->stream == NULL)
			continue;
#endif

		/* 如果该描述字对象已经在被设置为异常或超时状态则继续 */

		if ((fdp->event_type & (ACL_EVENT_XCPT | ACL_EVENT_RW_TIMEOUT)))
			continue;

		/* 检查描述字是否可读 */

		if ((fdp->flag & EVENT_FDTABLE_FLAG_READ)
			&& EVENT_TEST_READ(bp))
		{
			/* 给该描述字对象附加可读属性 */
			if ((fdp->event_type & (ACL_EVENT_READ
				| ACL_EVENT_WRITE)) == 0)
			{
				fdp->event_type |= ACL_EVENT_READ;
				fdp->fdidx_ready = eventp->ready_cnt;
				eventp->ready[eventp->ready_cnt++] = fdp;
			}

			if (fdp->listener)
				fdp->event_type |= ACL_EVENT_ACCEPT;

			/* 该描述字可读则设置 ACL_VSTREAM 的系统可读标志从而
			 * 触发 ACL_VSTREAM 流在读时调用系统的 read 函数
			 */
			else
				fdp->stream->read_ready = 1;
		}

		/* 检查描述字是否可写 */

		if ((fdp->flag & EVENT_FDTABLE_FLAG_WRITE)
			&& EVENT_TEST_WRITE(bp))
		{

			/* 给该描述字对象附加可写属性 */

			if ((fdp->event_type & (ACL_EVENT_READ
				| ACL_EVENT_WRITE)) == 0)
			{
				fdp->event_type |= ACL_EVENT_WRITE;
				fdp->fdidx_ready = eventp->ready_cnt;
				eventp->ready[eventp->ready_cnt++] = fdp;
			}
		}

#ifdef	EVENT_TEST_ERROR
		if (EVENT_TEST_ERROR(bp)) {
			/* 如果出现异常则设置异常属性 */

			if ((fdp->event_type & (ACL_EVENT_READ
				| ACL_EVENT_WRITE)) == 0)
			{
				fdp->event_type |= ACL_EVENT_XCPT;
				fdp->fdidx_ready = eventp->ready_cnt;
				eventp->ready[eventp->ready_cnt++] = fdp;
			}
		}
#endif
	}

TAG_DONE:

	/*
	 * Deliver timer events. Requests are sorted: we can stop when we
	 * reach the future or the list end. Allow the application to update
	 * the timer queue while it is being called back. To this end, we
	 * repeatedly pop the first request off the timer queue before
	 * delivering the event to the application.
	 */

	/* 调整事件引擎的时间截 */

	SET_TIME(eventp->present);

	while ((timer = ACL_FIRST_TIMER(&eventp->timer_head)) != 0) {
		if (timer->when > eventp->present)
			break;
		timer_fn  = timer->callback;
		timer_arg = timer->context;

		/* 定时器时间间隔 > 0 且允许定时器被循环调用，则重设定时器 */
		if (timer->delay > 0 && timer->keep) {
			timer->ncount++;
			eventp->timer_request(eventp, timer->callback,
				timer->context, timer->delay, timer->keep);
		} else {
			acl_ring_detach(&timer->ring);	/* first this */
			timer->nrefer--;
			if (timer->nrefer != 0)
				acl_msg_fatal("%s(%d): nrefer(%d) != 0",
					myname, __LINE__, timer->nrefer);
			acl_myfree(timer);
		}
		timer_fn(ACL_EVENT_TIME, eventp, timer_arg);
	}

	/* 处理准备好的描述字事件 */

	if (eventp->ready_cnt > 0)
		event_fire(eventp);

	eventp->nested--;
}

static int event_isrset(ACL_EVENT *eventp acl_unused, ACL_VSTREAM *stream)
{
	ACL_EVENT_FDTABLE *fdp = (ACL_EVENT_FDTABLE *) stream->fdp;

	return fdp == NULL ? 0 : (fdp->flag & EVENT_FDTABLE_FLAG_READ);
}

static int event_iswset(ACL_EVENT *eventp acl_unused, ACL_VSTREAM *stream)
{
	ACL_EVENT_FDTABLE *fdp = (ACL_EVENT_FDTABLE *) stream->fdp;

	return fdp == NULL ? 0 : (fdp->flag & EVENT_FDTABLE_FLAG_WRITE);

}

static int event_isxset(ACL_EVENT *eventp acl_unused, ACL_VSTREAM *stream)
{
	ACL_EVENT_FDTABLE *fdp = (ACL_EVENT_FDTABLE *) stream->fdp;

	return fdp == NULL ? 0 : (fdp->flag & EVENT_FDTABLE_FLAG_EXPT);
}

static void event_free(ACL_EVENT *eventp)
{
	EVENT_KERNEL *ev = (EVENT_KERNEL *) eventp;

#ifdef	USE_FDMAP
	acl_fdmap_free(ev->fdmap);
#endif
	acl_myfree(ev->event_buf);
	close(ev->event_fd);
	acl_myfree(ev);
}

ACL_EVENT *event_new_kernel(int fdsize acl_unused)
{
	ACL_EVENT *eventp;
	EVENT_KERNEL *ev;
	static int __default_max_events = 1000;

	eventp = event_alloc(sizeof(EVENT_KERNEL));

	snprintf(eventp->name, sizeof(eventp->name), "events - %s", EVENT_NAME);
	eventp->event_mode           = ACL_EVENT_KERNEL;
	eventp->use_thread           = 0;
	eventp->loop_fn              = event_loop;
	eventp->free_fn              = event_free;
	eventp->enable_read_fn       = event_enable_read;
	eventp->enable_write_fn      = event_enable_write;
	eventp->enable_listen_fn     = event_enable_listen;
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
	ev->event_buf = (EVENT_BUFFER *)
		acl_mycalloc(ev->event_fdslots + 1, sizeof(EVENT_BUFFER));
	acl_ring_init(&ev->fdp_delay_list);
#ifdef	USE_FDMAP
	ev->fdmap = acl_fdmap_create(fdsize);
#endif
	return eventp;
}
#endif	/* ACL_EVENTS_KERNEL_STYLE */
