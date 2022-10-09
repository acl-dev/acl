#include "stdafx.h"
#include "common.h"

#ifdef HAS_IO_URING

#include <dlfcn.h>
#include <liburing.h>
#include "event.h"
#include "event_io_uring.h"

typedef struct EVENT_URING {
	EVENT event;
	struct io_uring ring;
	size_t sqe_size;
	size_t appending;
} EVENT_URING;

static void event_uring_free(EVENT *ev)
{
	EVENT_URING *ep = (EVENT_URING*) ev;

	io_uring_queue_exit(&ep->ring);
	mem_free(ep);
}

static void add_read_wait(EVENT_URING *ep, FILE_EVENT *fe, int tmo_ms)
{
	struct io_uring_sqe *sqe;

	sqe = io_uring_get_sqe(&ep->ring);
	io_uring_prep_poll_add(sqe, fe->fd, POLLIN | POLLHUP | POLLERR);
	io_uring_sqe_set_data(sqe, fe);
	sqe->flags = IOSQE_IO_LINK;
	if (++ep->appending >= ep->sqe_size) {
		ep->appending = 0;
		io_uring_submit(&ep->ring);
	}

	file_event_refer(fe);

	fe->rts.tv_sec  = tmo_ms / 1000;
	fe->rts.tv_nsec = (((long long) tmo_ms) % 1000) * 1000000;

	sqe = io_uring_get_sqe(&ep->ring);
	io_uring_prep_link_timeout(sqe, &fe->rts, 0);
	io_uring_sqe_set_data(sqe, fe);

	file_event_refer(fe);

	if (++ep->appending >= ep->sqe_size) {
		ep->appending = 0;
		io_uring_submit(&ep->ring);
	}
}

static int event_uring_add_read(EVENT_URING *ep, FILE_EVENT *fe)
{
	if (fe->mask & EVENT_READ) {
		return 0;
	}

	fe->mask |= EVENT_READ;

	if (fe->mask & EVENT_POLLIN) {
		add_read_wait(ep, fe, fe->r_timeout);
	} else if (fe->mask & EVENT_ACCEPT) {
		struct io_uring_sqe *sqe = io_uring_get_sqe(&ep->ring);
		assert(sqe);

		fe->addr_len = (socklen_t) sizeof(fe->peer_addr);
		io_uring_prep_accept(sqe, fe->fd,
			(struct sockaddr*) &fe->peer_addr,
			(socklen_t*) &fe->addr_len, 0);
		io_uring_sqe_set_data(sqe, fe);
		if (++ep->appending >= ep->sqe_size) {
			ep->appending = 0;
			io_uring_submit(&ep->ring);
		}
	} else {
		struct io_uring_sqe *sqe = io_uring_get_sqe(&ep->ring);
		io_uring_prep_read(sqe, fe->fd, fe->rbuf, fe->rsize, 0);
		io_uring_sqe_set_data(sqe, fe);
		if (++ep->appending >= ep->sqe_size) {
			ep->appending = 0;
			io_uring_submit(&ep->ring);
		}
	}

	return 0;
}

static int event_uring_add_write(EVENT_URING *ep, FILE_EVENT *fe)
{
	struct io_uring_sqe *sqe;

	if (fe->mask & EVENT_WRITE) {
		return 0;
	}

	fe->mask |= EVENT_WRITE;
	sqe = io_uring_get_sqe(&ep->ring);
	assert(sqe);
	io_uring_sqe_set_data(sqe, fe);

	if (fe->mask & EVENT_CONNECT) {
		io_uring_prep_connect(sqe, fe->fd,
			(struct sockaddr*) &fe->peer_addr,
			(socklen_t) fe->addr_len);
	} else {
		io_uring_prep_write(sqe, fe->fd, fe->wbuf, fe->wsize, 0);
	}

	if (++ep->appending >= ep->sqe_size) {
		ep->appending = 0;
		io_uring_submit(&ep->ring);
	}
	return 0;
}

static int event_uring_del_read(EVENT_URING *ep UNUSED, FILE_EVENT *fe)
{
	if (!(fe->mask & EVENT_READ)) {
		return 0;
	}

	fe->mask &= ~EVENT_READ;
	return 0;
}

static int event_uring_del_write(EVENT_URING *ep UNUSED, FILE_EVENT *fe)
{
	if (!(fe->mask & EVENT_WRITE)) {
		return 0;
	}

	fe->mask &= ~EVENT_WRITE;
	return 0;
}

static int event_uring_wait(EVENT *ev, int timeout)
{
	EVENT_URING *ep = (EVENT_URING*) ev;
	struct __kernel_timespec ts, *tp;
	struct io_uring_cqe *cqe;
	FILE_EVENT *fe;
	int ret, count = 0;

	if (timeout >= 0) {
		ts.tv_sec  = timeout / 1000;
		ts.tv_nsec = (((long long) timeout) % 1000) * 1000000;
		tp         = &ts;
	} else {
		ts.tv_sec  = 0;
		ts.tv_nsec = 0;
		tp         = NULL;
		assert(0);
	}

	if (ep->appending > 0) {
		printf("\r\n>>>>fid=%d, submit append=%d\r\n",
			acl_fiber_self(), (int) ep->appending);
		ep->appending = 0;
		io_uring_submit(&ep->ring);
	}

	while (1) {
		cqe = NULL;

		if (count > 0) {
			ret = io_uring_peek_cqe(&ep->ring, &cqe);
		} else {
			//ret = io_uring_wait_cqe(&ep->ring, &cqe);
			ret = io_uring_wait_cqes(&ep->ring, &cqe, 1, tp, NULL);
		}

		if (ret) {
			if (ret == -ETIME) {
				return 0;
			} else if (ret == -EAGAIN) {
				break;
			}

			msg_error("io_uring_wait_cqe error=%s", strerror(-ret));
			return -1;
		}

		assert(cqe);
		count++;

		int res = cqe->res;
		fe = (FILE_EVENT*) io_uring_cqe_get_data(cqe);

		io_uring_cqe_seen(&ep->ring, cqe);

		if (res == -ENOBUFS) {
			msg_error("%s(%d): ENOBUFS error", __FUNCTION__, __LINE__);
			return -1;
		}

		usleep(1000000);

		if ((fe->mask & EVENT_READ) && fe->r_proc) {
			fe->mask &= ~EVENT_READ;
			if (fe->mask & EVENT_ACCEPT) {
				fe->iocp_sock = res;
			} else if (fe->mask & EVENT_POLLIN) {
				if (res == -ETIME) {
					printf("fd read timeout=%d\r\n", fe->fd);
					fe->rlen = -1;
					file_event_unrefer(fe);
				} else if (res == -ECANCELED) {
					printf("fd=%d canceled\n", fe->fd);
					file_event_unrefer(fe);
					continue;
				} else if (res & POLLIN) {
				}
			} else {
				fe->rlen = res;
			}

			fe->r_proc(ev, fe);
		}

		if ((fe->mask & EVENT_WRITE) && fe->w_proc) {
			fe->mask &= ~EVENT_WRITE;
			if (fe->mask & EVENT_CONNECT) {
				fe->iocp_sock = res;
			} else {
				fe->wlen = res;
			}

			fe->w_proc(ev, fe);
		}
	}

	printf(">>>fid=%d, handle count=%d<<<<\n\n", acl_fiber_self(), count);
	return count;
}

static int event_uring_checkfd(EVENT *ev UNUSED, FILE_EVENT *fe UNUSED)
{
	return 0;
}

static long event_uring_handle(EVENT *ev)
{
	EVENT_URING *ep = (EVENT_URING *) ev;
	return (long) &ep->ring;
}

static const char *event_uring_name(void)
{
	return "io_uring";
}

EVENT *event_io_uring_create(int size)
{
	EVENT_URING *eu = (EVENT_URING *) mem_calloc(1, sizeof(EVENT_URING));
	struct io_uring_params params;
	int ret;

	if (size <= 0 || size >= 4096) {
		eu->sqe_size = 2048;
	} else {
		eu->sqe_size = size;
	}

	memset(&params, 0, sizeof(params));
	ret = io_uring_queue_init_params(eu->sqe_size, &eu->ring, &params);
	if (ret < 0) {
		msg_fatal("%s(%d): init io_uring error=%s, size=%zd",
			__FUNCTION__, __LINE__, strerror(-ret), eu->sqe_size);
	} else {
		msg_info("%s(%d): init io_uring ok, size=%zd",
			__FUNCTION__, __LINE__, eu->sqe_size);
	}

	eu->appending    = 0;

	eu->event.name   = event_uring_name;
	eu->event.handle = (acl_handle_t (*)(EVENT *)) event_uring_handle;
	eu->event.free   = event_uring_free;
	eu->event.flag   = EVENT_F_IO_URING;

	eu->event.event_wait = event_uring_wait;
	eu->event.checkfd    = (event_oper *) event_uring_checkfd;
	eu->event.add_read   = (event_oper *) event_uring_add_read;
	eu->event.add_write  = (event_oper *) event_uring_add_write;
	eu->event.del_read   = (event_oper *) event_uring_del_read;
	eu->event.del_write  = (event_oper *) event_uring_del_write;

	return (EVENT*) eu;
}

#endif /* HAS_IO_URING */
