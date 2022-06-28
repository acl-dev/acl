#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_msg.h"

#endif

#include "events.h"

void event_check_fds(ACL_EVENT *ev)
{
	ACL_EVENT_FDTABLE *fdp;
	int   i;

	if (ev->fdpos >= ev->fdcnt) {
		ev->fdpos = 0;
	}

	for (i = 0; i < 5000 && ev->fdpos < ev->fdcnt; i++, ev->fdpos++) {
		fdp = ev->fdtabs[ev->fdpos];
		if ((fdp->stream->flag & ACL_VSTREAM_FLAG_BAD) != 0) {
			fdp->stream->flag &= ~ACL_VSTREAM_FLAG_BAD;
			fdp->event_type |= ACL_EVENT_XCPT;
			fdp->fdidx_ready = ev->ready_cnt;
			ev->ready[ev->ready_cnt++] = fdp;
		} else if ((fdp->flag & EVENT_FDTABLE_FLAG_READ)) {
			if (ACL_VSTREAM_BFRD_CNT(fdp->stream) > 0) {
				fdp->stream->read_ready = 0;
				fdp->event_type |= ACL_EVENT_READ;
				fdp->fdidx_ready = ev->ready_cnt;
				ev->ready[ev->ready_cnt++] = fdp;
			} else if (fdp->stream->read_ready && !fdp->listener) {
				fdp->event_type |= ACL_EVENT_READ;
				fdp->fdidx_ready = ev->ready_cnt;
				ev->ready[ev->ready_cnt++] = fdp;
			} else if (fdp->r_ttl > 0 && ev->present > fdp->r_ttl) {
				fdp->event_type |= ACL_EVENT_RW_TIMEOUT;
				fdp->fdidx_ready = ev->ready_cnt;
				ev->ready[ev->ready_cnt++] = fdp;
			}
		} else if ((fdp->flag & EVENT_FDTABLE_FLAG_WRITE)) {
			if (fdp->w_ttl > 0 && ev->present > fdp->w_ttl) {
				fdp->event_type |= ACL_EVENT_RW_TIMEOUT;
				fdp->fdidx_ready = ev->ready_cnt;
				ev->ready[ev->ready_cnt++] = fdp;
			}
		}
	}
}

int event_prepare(ACL_EVENT *ev)
{
	ACL_EVENT_FDTABLE *fdp;
	ACL_SOCKET sockfd;
	int   i, nwait = 0;

	ev->ready_cnt = 0;

	for (i = 0; i < ev->fdcnt; i++) {
		fdp = ev->fdtabs[i];
		sockfd = ACL_VSTREAM_SOCK(fdp->stream);
		fdp->event_type = 0;
		if (ev->maxfd < sockfd)
			ev->maxfd = sockfd;
		if ((fdp->stream->flag & ACL_VSTREAM_FLAG_BAD) != 0) {
			fdp->stream->flag &= ~ACL_VSTREAM_FLAG_BAD;
			fdp->event_type |= ACL_EVENT_XCPT;
			fdp->fdidx_ready = ev->ready_cnt;
			ev->ready[ev->ready_cnt++] = fdp;
		} else if ((fdp->flag & EVENT_FDTABLE_FLAG_READ)) {
			if (fdp->stream->read_ready && !fdp->listener) {
				fdp->event_type |= ACL_EVENT_READ;
				fdp->fdidx_ready = ev->ready_cnt;
				ev->ready[ev->ready_cnt++] = fdp;
			} else if (ACL_VSTREAM_BFRD_CNT(fdp->stream) > 0) {
				fdp->stream->read_ready = 0;
				fdp->event_type |= ACL_EVENT_READ;
				fdp->fdidx_ready = ev->ready_cnt;
				ev->ready[ev->ready_cnt++] = fdp;
			} else if (fdp->r_ttl > 0 && ev->present > fdp->r_ttl) {
				fdp->event_type |= ACL_EVENT_RW_TIMEOUT;
				fdp->fdidx_ready = ev->ready_cnt;
				ev->ready[ev->ready_cnt++] = fdp;
			} else
				nwait++;
		} else if ((fdp->flag & EVENT_FDTABLE_FLAG_WRITE)) {
			if (fdp->w_ttl > 0 && ev->present > fdp->w_ttl) {
				fdp->event_type |= ACL_EVENT_RW_TIMEOUT;
				fdp->fdidx_ready = ev->ready_cnt;
				ev->ready[ev->ready_cnt++] = fdp;
			} else
				nwait++;
		} else
			nwait++;
	}

	return nwait;
}

void event_fire(ACL_EVENT *ev)
{
	ACL_EVENT_FDTABLE *fdp;
	int   i, type;
	acl_int64   r_timeout, w_timeout;
	ACL_EVENT_NOTIFY_RDWR r_callback, w_callback;
	ACL_EVENT_FDTABLE **ready = ev->ready;

	if (ev->fire_begin)
		ev->fire_begin(ev, ev->fire_ctx);

	for (i = 0; i < ev->ready_cnt; i++) {
		fdp = ready[i];

		/* ready[i] maybe been set NULL in timer callback */
		if (fdp == NULL || fdp->stream == NULL) 
			continue;

		type = fdp->event_type;

		if ((type & ACL_EVENT_XCPT) != 0) {
			fdp->event_type &= ~ACL_EVENT_XCPT;
			r_callback = fdp->r_callback;
			w_callback = fdp->w_callback;

			if (r_callback)
				r_callback(ACL_EVENT_XCPT, ev,
					fdp->stream, fdp->r_context);

			/* ready[i] maybe been set NULL in r_callback */
			if (w_callback && ready[i])
				w_callback(ACL_EVENT_XCPT, ev,
					fdp->stream, fdp->w_context);
			continue;
		}

		if ((type & ACL_EVENT_RW_TIMEOUT) != 0) {
			fdp->event_type &= ~ACL_EVENT_RW_TIMEOUT;
			r_timeout = fdp->r_timeout;
			w_timeout = fdp->w_timeout;
			r_callback = fdp->r_callback;
			w_callback = fdp->w_callback;

			if (r_timeout > 0 && r_callback) {
				fdp->r_ttl = ev->present + fdp->r_timeout;
				fdp->r_callback(ACL_EVENT_RW_TIMEOUT, ev,
					fdp->stream, fdp->r_context);
			}

			/* ready[i] maybe been set NULL in r_callback */
			if (w_timeout > 0 && w_callback && ready[i]) {
				fdp->w_ttl = ev->present + fdp->w_timeout;
				fdp->w_callback(ACL_EVENT_RW_TIMEOUT, ev,
					fdp->stream, fdp->w_context);
			}
			continue;
		}

		if ((type & (ACL_EVENT_READ | ACL_EVENT_ACCEPT))) {
			fdp->event_type &= ~(ACL_EVENT_READ | ACL_EVENT_ACCEPT);
			if (fdp->r_timeout > 0)
				fdp->r_ttl = ev->present + fdp->r_timeout;
			fdp->r_callback(type, ev, fdp->stream, fdp->r_context);

			/* If there's some data lefting in stream's buf, then
			 * increasing the eventp->read_ready to trigger the
			 * event_check_fds proccess for next event loop
			 * more quickly.
			 */
			if (ready[i] && ready[i]->stream
				&& (ready[i]->stream->read_ready ||
				  ACL_VSTREAM_BFRD_CNT(ready[i]->stream) > 0))
			{
				ev->read_ready++;
			}
		}

		/* ready[i] maybe been set NULL in fdp->r_callback() */
		if (ready[i] == NULL)
			continue;

		if ((type & (ACL_EVENT_WRITE | ACL_EVENT_CONNECT))) {
			if (fdp->w_timeout > 0)
				fdp->w_ttl = ev->present + fdp->w_timeout;
			fdp->event_type &= ~(ACL_EVENT_WRITE | ACL_EVENT_CONNECT);
			fdp->w_callback(type, ev, fdp->stream, fdp->w_context);
		}
	}

	if (ev->fire_end)
		ev->fire_end(ev, ev->fire_ctx);
}

int event_thr_prepare(ACL_EVENT *ev)
{
	ACL_SOCKET sockfd;
	ACL_EVENT_FDTABLE *fdp;
	int   i, nwait = 0;

	for (i = 0; i < ev->fdcnt; i++) {
		fdp = ev->fdtabs[i];
		sockfd = ACL_VSTREAM_SOCK(fdp->stream);
		fdp->event_type = 0;
		if (ev->maxfd < sockfd)
			ev->maxfd = sockfd;

		if (fdp->listener) {
			nwait++;
			continue;
		}

		if (fdp->stream->flag & ACL_VSTREAM_FLAG_BAD) {
			fdp->stream->flag &= ~ACL_VSTREAM_FLAG_BAD;
			fdp->event_type |= ACL_EVENT_XCPT;
			fdp->fdidx_ready = ev->ready_cnt;
			ev->ready[ev->ready_cnt++] = fdp;
		} else if ((fdp->flag & EVENT_FDTABLE_FLAG_READ)) {
			if (fdp->stream->read_ready && !fdp->listener) {
				fdp->event_type = ACL_EVENT_READ;
				fdp->fdidx_ready = ev->ready_cnt;
				ev->ready[ev->ready_cnt++] = fdp;
			} else if (ACL_VSTREAM_BFRD_CNT(fdp->stream) > 0) {
				fdp->stream->read_ready = 0;
				fdp->event_type = ACL_EVENT_READ;
				fdp->fdidx_ready = ev->ready_cnt;
				ev->ready[ev->ready_cnt++] = fdp;
			} else if (fdp->r_ttl > 0 && ev->present > fdp->r_ttl) {
				fdp->event_type = ACL_EVENT_RW_TIMEOUT;
				fdp->fdidx_ready = ev->ready_cnt;
				ev->ready[ev->ready_cnt++] = fdp;
			} else
				nwait++;
		} else if ((fdp->flag & EVENT_FDTABLE_FLAG_WRITE)) {
			if (fdp->w_ttl > 0 && ev->present > fdp->w_ttl) {
				fdp->event_type = ACL_EVENT_RW_TIMEOUT;
				fdp->fdidx_ready = ev->ready_cnt;
				ev->ready[ev->ready_cnt++] = fdp;
			} else
				nwait++;
		} else
			nwait++;
	}

	return nwait;
}

void event_thr_fire(ACL_EVENT *ev)
{
	ACL_EVENT_FDTABLE *fdp;
	ACL_EVENT_NOTIFY_RDWR callback;
	ACL_VSTREAM *stream;
	void *context;
	int   type;
	int   i;

	if (ev->fire_begin)
		ev->fire_begin(ev, ev->fire_ctx);

	for (i = 0; i < ev->ready_cnt; i++) {
		fdp = ev->ready[i];

		/* ev->ready[i] maybe be set NULL by timer callback */
		if (fdp == NULL || fdp->stream == NULL)
			continue;

		stream = fdp->stream;
		type = fdp->event_type;

		if ((type & (ACL_EVENT_READ | ACL_EVENT_ACCEPT))) {
			fdp->event_type &= ~(ACL_EVENT_READ | ACL_EVENT_ACCEPT);
			callback = fdp->r_callback;
			context = fdp->r_context;
			if (!fdp->listener)
				ev->disable_readwrite_fn(ev, stream);
			callback(ACL_EVENT_READ, ev, stream, context);
		} else if ((type & (ACL_EVENT_WRITE | ACL_EVENT_CONNECT))) {
			fdp->event_type &= ~(ACL_EVENT_WRITE | ACL_EVENT_CONNECT);
			callback = fdp->w_callback;
			context = fdp->w_context;
			ev->disable_readwrite_fn(ev, stream);
			callback(ACL_EVENT_WRITE, ev, stream, context);
		} else if ((type & ACL_EVENT_RW_TIMEOUT)) {
			fdp->event_type &= ~ACL_EVENT_RW_TIMEOUT;
			if (fdp->r_callback) {
				callback = fdp->r_callback;
				context = fdp->r_context;
			} else if (fdp->w_callback) {
				callback = fdp->w_callback;
				context = fdp->w_context;
			} else {
				callback = NULL;
				context = NULL;
			}
			if (!fdp->listener)
				ev->disable_readwrite_fn(ev, stream);
			if (callback)
				callback(ACL_EVENT_RW_TIMEOUT, ev,
					stream, context);
		} else if ((type & ACL_EVENT_XCPT)) {
			fdp->event_type &= ~ACL_EVENT_XCPT;
			if (fdp->r_callback) {
				callback = fdp->r_callback;
				context = fdp->r_context;
			} else if (fdp->w_callback) {
				callback = fdp->w_callback;
				context = fdp->w_context;
			} else {
				callback = NULL;
				context = NULL;
			}
			if (!fdp->listener)
				ev->disable_readwrite_fn(ev, stream);
			if (callback)
				callback(ACL_EVENT_XCPT, ev, stream, context);
		}
	}

	if (ev->fire_end)
		ev->fire_end(ev, ev->fire_ctx);
}
