#include "stdafx.h"
#include "common.h"

#ifdef	HAS_KQUEUE

#include <dlfcn.h>
#include <sys/event.h>
#include "event.h"
#include "event_kqueue.h"

typedef int (*kqueue_fn)(void);
typedef int (*kevent_fn)(int, const struct kevent *, int, struct kevent *,
	int, const struct timespec *);

static kqueue_fn __sys_kqueue = NULL;
static kevent_fn __sys_kevent = NULL;

static void hook_api(void)
{
	__sys_kqueue = (kqueue_fn) dlsym(RTLD_NEXT, "kqueue");
	assert(__sys_kqueue);

	__sys_kevent = (kevent_fn) dlsym(RTLD_NEXT, "kevent");
	assert(__sys_kevent);
}

static pthread_once_t __once_control = PTHREAD_ONCE_INIT;

static void hook_init(void)
{
	if (pthread_once(&__once_control, hook_api) != 0) {
		abort();
	}
}

/****************************************************************************/

typedef struct EVENT_KQUEUE {
	EVENT  event;
	int    kqfd;
	struct kevent *changes;
	int    setsize;
	int    nchanges;
	struct kevent *events;
	int    nevents;
} EVENT_KQUEUE;

static void kqueue_free(EVENT *ev)
{
	EVENT_KQUEUE *ek = (EVENT_KQUEUE *) ev;

	close(ek->kqfd);
	mem_free(ek->changes);
	mem_free(ek->events);
	mem_free(ek);
}

static int kqueue_fflush(EVENT_KQUEUE *ek)
{
	struct timespec ts;
	int nchanges;

	if (ek->changes == 0) {
		return 0;
	}

	ts.tv_sec  = 0;
	ts.tv_nsec = 0;
	if (__sys_kevent(ek->kqfd, ek->changes, ek->nchanges, NULL, 0, &ts) == -1) {
		msg_error("%s(%d): kevent error %s, kqfd=%d",
			__FUNCTION__, __LINE__, last_serror(), ek->kqfd);
		return -1;
	}

	nchanges = ek->nchanges;
	ek->nchanges = 0;
	return nchanges;
}

static int kqueue_add_read(EVENT_KQUEUE *ek, FILE_EVENT *fe)
{
	struct kevent *kev;

	if (ek->nchanges >= ek->setsize) {
		if (kqueue_fflush(ek) == -1) {
			return -1;
		}
	}

	kev = &ek->changes[ek->nchanges++];
	EV_SET(kev, fe->fd, EVFILT_READ, EV_ADD, 0, 0, fe);
	if (!(fe->mask & EVENT_WRITE)) {
		ek->event.fdcount++;
	}
	fe->mask |= EVENT_READ;
	return 0;
}

static int kqueue_add_write(EVENT_KQUEUE *ek, FILE_EVENT *fe)
{
	struct kevent *kev;

	if (ek->nchanges >= ek->setsize) {
		if (kqueue_fflush(ek) == -1) {
			return -1;
		}
	}

	kev = &ek->changes[ek->nchanges++];
	EV_SET(kev, fe->fd, EVFILT_WRITE, EV_ADD, 0, 0, fe);
	if (!(fe->mask & EVENT_READ)) {
		ek->event.fdcount++;
	}
	fe->mask |= EVENT_WRITE;
	return 0;
}

static int kqueue_del_read(EVENT_KQUEUE *ek, FILE_EVENT *fe)
{
	struct kevent *kev;

	if (ek->nchanges >= ek->setsize) {
		if (kqueue_fflush(ek) == -1) {
			return -1;
		}
	}

	kev = &ek->changes[ek->nchanges++];
	EV_SET(kev, fe->fd, EVFILT_READ, EV_DELETE, 0, 0, fe);
	fe->mask &= ~EVENT_READ;
	if (!(fe->mask & EVENT_WRITE)) {
		ek->event.fdcount--;
	}
	return 0;
}

static int kqueue_del_write(EVENT_KQUEUE *ek, FILE_EVENT *fe)
{
	struct kevent *kev;

	if (ek->nchanges >= ek->setsize) {
		if (kqueue_fflush(ek) == -1) {
			return -1;
		}
	}

	kev = &ek->changes[ek->nchanges++];
	EV_SET(kev, fe->fd, EVFILT_WRITE, EV_DELETE, 0, 0, fe);
	fe->mask &= ~EVENT_WRITE;
	if (!(fe->mask & EVENT_READ)) {
		ek->event.fdcount--;
	}
	return 0;
}

static int kqueue_wait(EVENT *ev, int timeout)
{
	EVENT_KQUEUE *ek = (EVENT_KQUEUE *) ev;
	struct timespec ts;
	struct kevent *kev;
	FILE_EVENT *fe;
	int n, i;

	ts.tv_sec = timeout / 1000;
	ts.tv_nsec = (timeout % 1000) * 1000000;

	n = __sys_kevent(ek->kqfd, ek->changes, ek->nchanges, ek->events,
		ek->nevents, &ts);
	ek->nchanges = 0;

	if (n == -1) {
		if (acl_fiber_last_error() == FIBER_EINTR) {
			return 0;
		}
		msg_fatal("%s: kqueue error %s", __FUNCTION__, last_serror());
	} else if (n == 0) {
		return 0;
	}

	for (i = 0; i < n; i++) {
		kev = &ek->events[i];
		fe  = (FILE_EVENT *) kev->udata;

		if (kev && kev->filter == EVFILT_READ && fe && fe->r_proc) {
			fe->r_proc(ev, fe);
		}

		if (kev && kev->filter == EVFILT_WRITE && fe && fe->w_proc) {
			fe->w_proc(ev, fe);
		}
	}

	return n;
}

static int kqueue_checkfd(EVENT *ev UNUSED, FILE_EVENT *fe UNUSED)
{
	return -1;
}

static long kqueue_handle(EVENT *ev)
{
	EVENT_KQUEUE *ek = (EVENT_KQUEUE *) ev;

	return ek->kqfd;
}

static const char *kqueue_name(void)
{
	return "kqueue";
}

EVENT *event_kqueue_create(int size)
{
	EVENT_KQUEUE *ek = (EVENT_KQUEUE *) mem_calloc(1, sizeof(EVENT_KQUEUE));

	if (__sys_kqueue == NULL) {
		hook_init();
	}

	if (size <= 0 || size > 1024) {
		size = 1024;
	}
	ek->changes  = (struct kevent *) mem_malloc(sizeof(struct kevent) * size);
	ek->setsize  = size;
	ek->nchanges = 0;

	ek->nevents  = 100;
	ek->events   = (struct kevent *) mem_malloc(sizeof(struct kevent) * ek->nevents);

	ek->kqfd     = __sys_kqueue();
	assert(ek->kqfd >= 0);

	ek->event.name   = kqueue_name;
	ek->event.handle = (acl_handle_t (*)(EVENT *)) kqueue_handle;
	ek->event.free   = kqueue_free;

	ek->event.event_fflush = (int (*)(EVENT*)) kqueue_fflush;
	ek->event.event_wait   = kqueue_wait;
	ek->event.checkfd      = (event_oper *) kqueue_checkfd;
	ek->event.add_read     = (event_oper *) kqueue_add_read;
	ek->event.add_write    = (event_oper *) kqueue_add_write;
	ek->event.del_read     = (event_oper *) kqueue_del_read;
	ek->event.del_write    = (event_oper *) kqueue_del_write;

	return (EVENT *) ek;
}

#endif
