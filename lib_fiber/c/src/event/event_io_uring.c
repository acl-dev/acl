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
} EVENT_URING;

static void event_uring_free(EVENT *ev)
{
	EVENT_URING *ep = (EVENT_URING*) ev;

	io_uring_queue_exit(&ep->ring);
	mem_free(ep);
}

static int event_uring_add_read(EVENT_URING *ep, FILE_EVENT *fe)
{
	struct io_uring_sqe *sqe;

	if (fe->mask & EVENT_READ) {
		return 0;
	}

	fe->mask |= EVENT_READ;
	sqe = io_uring_get_sqe(&ep->ring);
	assert(sqe);

	io_uring_sqe_set_data(sqe, fe);
	io_uring_prep_read(sqe, fe->fd, fe->rbuf, fe->rsize, 0);
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
	io_uring_prep_write(sqe, fe->fd, fe->wbuf, fe->wlen, 0);
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
	unsigned count = 0, head;
	FILE_EVENT *fe;
	int n;

	if (timeout >= 0) {
		ts.tv_sec = timeout / 1000;
		ts.tv_nsec = (((long long) timeout) % 1000) * 1000000;
		tp = &ts;
	} else {
		tp = NULL;
	}

	n = io_uring_submit_and_wait_timeout(&ep->ring, &cqe, 1, tp, NULL);
	if (n == 0) {
		return 0;
	} else if (n < 0) {
		if (n == -ETIME) {
			return 0;
		}
		printf("%s(%d): wait error=%d\r\n", __FUNCTION__, __LINE__, n);
		return -1;
	}

	io_uring_for_each_cqe(&ep->ring, head, cqe) {
		if (cqe->res == -ENOBUFS) {
			return -1;
		}

		count++;
		fe = (FILE_EVENT*) io_uring_cqe_get_data(cqe);
		assert(fe);

		io_uring_cqe_seen(&ep->ring, cqe);

		if (fe && (fe->mask & EVENT_READ) && fe->r_proc) {
			fe->mask &= ~EVENT_READ;
			fe->rlen = cqe->res;
			fe->r_proc(ev, fe);
		}

		if (fe && (fe->mask & EVENT_WRITE) && fe->w_proc) {
			fe->mask &= ~EVENT_WRITE;
			fe->w_proc(ev, fe);
		}
	}

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

	if (size <= 0 || size > 100) {
		size = 100;
	}

	memset(&params, 0, sizeof(params));
	if (io_uring_queue_init_params(size, &eu->ring, &params) < 0) {
		abort();
	}

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
