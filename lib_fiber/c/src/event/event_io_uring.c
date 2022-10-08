#include "stdafx.h"
#include "common.h"

#ifdef HAS_IO_URING

#include <dlfcn.h>
#include <liburing.h>
#include "event.h"
#include "event_io_uring.h"

typedef int (*que_init_fn)(unsigned, struct io_uring*, struct io_uring_params*);
typedef void (*que_exit_fn)(struct io_uring*);
typedef struct io_uring_sqe (*get_sqe_fn)(struct io_uring*);
typedef void (*sqe_set_flags_fn)(struct io_uring_sqe*, unsigned);
typedef void (*sqe_set_data_fn)(struct io_uring_sqe*, void*);
typedef void (*cqe_get_data_fn)(struct io_uring_cqe*);
typedef void (*prep_accept_fn)(struct io_uring_sqe*, int, struct sockaddr*, socklen_t*, int);
typedef void (*prep_read_fn)(struct io_uring_sqe*, int, void*, unsigned, __u64);
typedef void (*prep_write_fn)(struct io_uring_sqe*, int, const void*, unsigned, __u64);
typedef int (*submit_and_wait_fn)(struct io_uring*, struct io_uring_cqe**,
		unsigned, struct __kernel_timespec, sigset_t*);

static que_init_fn __sys_que_init = NULL;
static que_exit_fn __sys_que_exit = NULL;
static get_sqe_fn __sys_get_sqe = NULL;
static sqe_set_flags_fn __sys_sqe_set_flags = NULL;
static sqe_set_data __sys_sqe_set_data = NULL;
static *cqe_get_data_fn __sys_cqe_get_data = NULL;
static prep_accept_fn __sys_prep_accept = NULL;
static prep_read_fn __sys_prep_read = NULL;
static prep_write_fn __sys_prep_write = NULL;
static submit_and_wait_fn __sys_submit_and_wait = NULL;

static void hook_api(void)
{
	__sys_que_init = (que_init_fn) dlsym(RTLD_NEXT, "io_uring_queue_init_params");
	assert(__sys_que_init);

	__sys_que_exit = (que_exit_fn) dlsym(RTLD_NEXT, "io_uring_queue_exit");
	assert(__sys_que_exit);

	__sys_get_sqe = (get_sqe_fn) dlsym(RTLD_NEXT, "io_uring_get_sqe");
	assert(__sys_get_sqe);

	__sys_sqe_set_flags = (sqe_set_flags_fn) dlsym(RTLD_NEXT, "io_uring_sqe_set_flags");
	assert(__sys_sqe_set_flags);

	__sys_sqe_set_data = (sqe_set_data_fn) dlsym(RTLD_NEXT, "io_uring_sqe_set_data");
	assert(__sys_sqe_set_data);

	__sys_cqe_get_data = (cqe_get_data_fn) dlsym(RTLD_NEXT, "io_uring_cqe_get_data");
	assert(__sys_cqe_get_data);

	__sys_prep_accept = (prep_accept_fn) dlsym(RTLD_NEXT, "io_uring_prep_accept");
	assert(__sys_prep_accept);

	__sys_prep_read = (prep_read_fn) dlsym(RTLD_NEXT, "io_uring_prep_read");
	assert(__sys_prep_read);

	__sys_prep_write = (prep_write_fn) dlsym(RTLD_NEXT, "io_uring_prep_write");
	assert(__sys_prep_write);

	__sys_submit_and_wait = (submit_and_wait_fn) dlsym(RTLD_NEXT, "io_uring_submit_and_wait_timeout");
	assert(__sys_submit_and_wait);
}

static pthread_once_t __once_control = PTHREAD_ONCE_INIT;

static void hook_init(void)
{
	if (pthread_once(&__once_control, hook_api) != 0) {
		abort();
	}
}

/****************************************************************************/

typedef struct EVENT_URING {
	EVENT event;
	struct io_uring ring;
} EVENT_URING;

static void event_uring_free(EVENT *ev)
{
	EVENT_URING *eu = (EVENT_URING*) ev;

	__sys_que_exit(&eu->ring);
	mem_free(eu);
}

static int event_uring_add_read(EVENT_URING *eu, FILE_EVENT *fe)
{
	struct io_uring_sqe *sqe = __sys_get_sqe(&eu->ring);
	assert(sqe);

	__sys_prep_read(sqe, fe->fd, fe->rbuf, fe->rsize, 0);
	__sys_sqe_set_data(sqe, fe);
	return 0;
}

static int event_uring_add_write(EVENT_URING *ep, FILE_EVENT *fe)
{
	struct io_uring_sqe *sqe = __sys_get_sqe(&eu->ring);
	assert(sqe);

	__sys_prep_write(sqe, fe->fd, fe->wbuf, fe->wsize, 0);
	__sys_sqe_set_data(sqe, fe);
	return 0;
}

static int event_uring_del_read(EVENT_URING *ep, FILE_EVENT *fe)
{
	if (!(fe->mask & EVENT_READ)) {
		return 0;
	}

	fe->mask &= ~EVENT_READ;
	return 0;
}

static int event_uring_del_write(EVENT_URING *ep, FILE_EVENT *fe)
{
	if (!(fe->mask & EVENT_WRITE)) {
		return 0;
	}

	fe->mask &= ~EVENT_WRITE;
	return 0;
}

static int event_uring_wait(EVENT *ev, int timeout)
{
	EVENT_URING *eu = (EVENT_URING*) ev;
	struct __kernel_timespec ts, *tp;
	struct io_uring_cqe *cqes;
	unsigned count = 0;
	FILE_EVENT *fe;
	int n, i;

	if (timeout >= 0) {
		ts.tv_sec = timeout / 1000;
		ts.tv_nsec = (((long long) timeout) % 1000) * 1000000;
	} else {
		tp = NULL;
	}

	n = __sys_submit_and_wait(&eu->ring, &cqe, 1, &tp, NULL);
	if (n == 0) {
		return 0;
	} else if (n < 0) {
		if (n == -ETIME) {
			return 0;
		}
		return -1;
	}

	for (i = 0; i < n; i++) {
		count++;
		fe = (FILE_EVENT*) io_uring_cqe_get_data();
	}
}

static int event_uring_checkfd(EVENT *ev UNUSED, FILE_EVENT *fe UNUSED)
{
	return 0;
}

static long event_uring_handle(EVENT *ev)
{
	return (long) &ev->ring;
}

static const char *event_uring_name(void)
{
	return "io_uring";
}

EVENT *event_uring_create(int size)
{
	EVENT_URING *eu = (EVENT_URING *) mem_calloc(1, sizeof(EVENT_URING));
	struct io_uring_params params;

	if (__sys_init_params == NULL) {
		hook_init();
	}

	if (size <= 0 || size > 100) {
		size = 100;
	}

	memset(&params, 0, sizeof(params));
	if (__sys_init_params(size, &eu->ring, &params) < 0) {
		abort();
	}

	eu->event.name   = event_uring_name;
	eu->event.handle = (acl_handle_t (*)(EVENT *)) event_uring_handle;
	eu->event.free   = event_uring_free;

	eu->event.event_wait = event_uring_wait;
	eu->event.checkfd    = (event_oper *) event_uring_checkfd;
	eu->event.add_read   = (event_oper *) event_uring_add_read;
	eu->event.add_write  = (event_oper *) event_uring_add_write;
	eu->event.del_read   = (event_oper *) event_uring_del_read;
	eu->event.del_write  = (event_oper *) event_uring_del_write;

	return (EVENT*) eu;
}

#endif /* HAS_IO_URING */
