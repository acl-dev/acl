#include "stdafx.h"
#include "common.h"

#ifdef HAS_SELECT

#include "fiber/libfiber.h"
#include "event.h"
#include "event_select.h"

#ifdef SYS_WIN
typedef int(WINAPI *select_fn)(int, fd_set *, fd_set *, fd_set *, struct timeval *);
#else
typedef int(*select_fn)(int, fd_set *, fd_set *, fd_set *, struct timeval *);
#endif

static select_fn __sys_select = NULL;

static void hook_api(void)
{
#ifdef SYS_UNIX
	__sys_select = (select_fn) dlsym(RTLD_NEXT, "select");
	assert(__sys_select);
#elif defined(SYS_WIN)
	__sys_select = (select_fn) select;
#endif
}

static pthread_once_t __once_control = PTHREAD_ONCE_INIT;

static void hook_init(void)
{
	if (pthread_once(&__once_control, hook_api) != 0) {
		abort();
	}
}

/****************************************************************************/

typedef struct EVENT_SELECT {
	EVENT  event;
	fd_set rset;
	fd_set wset;
	fd_set xset;
	FILE_EVENT **files;
	int    size;
	int    count;
	socket_t maxfd;
	int    dirty;
	ARRAY *ready;
} EVENT_SELECT;

static void select_free(EVENT *ev)
{
	EVENT_SELECT *es = (EVENT_SELECT *) ev;
	mem_free(es->files);
	array_free(es->ready, NULL);
	mem_free(es);
}

static int select_add_read(EVENT_SELECT *es, FILE_EVENT *fe)
{
	if (FD_ISSET(fe->fd, &es->wset) || FD_ISSET(fe->fd, &es->rset)) {
		assert(fe->id >= 0 && fe->id < es->count);
		assert(es->files[fe->id] == fe);
	} else {
		assert(es->count < es->size);
		es->files[es->count] = fe;
		fe->id = es->count++;
		FD_SET(fe->fd, &es->xset);
		if (fe->fd > es->maxfd) {
			es->maxfd = fe->fd;
		}
		es->event.fdcount++;
	}

	fe->mask |= EVENT_READ;
	FD_SET(fe->fd, &es->rset);
	return 0;
}

static int select_add_write(EVENT_SELECT *es, FILE_EVENT *fe)
{
	if (FD_ISSET(fe->fd, &es->rset) || FD_ISSET(fe->fd, &es->wset)) {
		assert(fe->id >= 0 && fe->id < es->count);
		assert(es->files[fe->id] == fe);
	} else {
		assert(es->count < es->size);
		es->files[es->count] = fe;
		fe->id = es->count++;
		FD_SET(fe->fd, &es->xset);
		if (fe->fd > es->maxfd) {
			es->maxfd = fe->fd;
		}
		es->event.fdcount++;
	}

	fe->mask |= EVENT_WRITE;
	FD_SET(fe->fd, &es->wset);
	return 0;
}

static int select_del_read(EVENT_SELECT *es, FILE_EVENT *fe)
{
	assert(fe->id >= 0 && fe->id < es->count);
	if (FD_ISSET(fe->fd, &es->rset)) {
		FD_CLR(fe->fd, &es->rset);
	}
	if (!FD_ISSET(fe->fd, &es->wset)) {
		FD_CLR(fe->fd, &es->xset);
		if (fe->id < --es->count) {
			es->files[fe->id] = es->files[es->count];
			es->files[fe->id]->id = fe->id;
		}
		fe->id = -1;
		if (fe->fd == es->maxfd) {
			es->dirty = 1;
		}
		es->event.fdcount--;
	}
	fe->mask &= ~EVENT_READ;
	return 0;
}

static int select_del_write(EVENT_SELECT *es, FILE_EVENT *fe)
{
	assert(fe->id >= 0 && fe->id < es->count);
	if (FD_ISSET(fe->fd, &es->wset)) {
		FD_CLR(fe->fd, &es->wset);
	}
	if (!FD_ISSET(fe->fd, &es->rset)) {
		FD_CLR(fe->fd, &es->xset);
		if (fe->id < --es->count) {
			es->files[fe->id] = es->files[es->count];
			es->files[fe->id]->id = fe->id;
		}
		fe->id = -1;
		if (fe->fd == es->maxfd) {
			es->dirty = 1;
		}
		es->event.fdcount--;
	}
	fe->mask &= ~EVENT_WRITE;
	return 0;
}

static int select_event_wait(EVENT *ev, int timeout)
{
	EVENT_SELECT *es = (EVENT_SELECT *) ev;
	fd_set rset = es->rset, wset = es->wset, xset = es->xset;
	struct timeval tv, *tp;
	ITER   iter;
	int n, i;

	if (timeout >= 0) {
		tv.tv_sec = timeout / 1000;
		tv.tv_usec = (timeout % 1000) * 1000;
		tp = &tv;
	} else {
		tp = NULL;
	}

#ifdef SYS_WIN
	if (ev->fdcount == 0) {
		Sleep(timeout);
		return 0;
	}

	n = __sys_select(0, &rset, &wset, &xset, tp);
#else
	if (es->dirty) {
		es->maxfd = -1;
		for (i = 0; i < es->count; i++) {
			FILE_EVENT *fe = es->files[i];
			if (fe->fd > es->maxfd) {
				es->maxfd = fe->fd;
			}
		}
	}
	n = __sys_select(es->maxfd + 1, &rset, 0, &xset, tp);
#endif
	if (n < 0) {
		if (acl_fiber_last_error() == FIBER_EINTR) {
			return 0;
		}
		msg_fatal("%s: select error %s", __FUNCTION__, last_serror());
	} else if (n == 0) {
		return 0;
	}

	for (i = 0; i < es->count; i++) {
		FILE_EVENT *fe = es->files[i];
		array_append(es->ready, fe);
	}

	foreach(iter, es->ready) {
		FILE_EVENT *fe = (FILE_EVENT *) iter.data;

		if (FD_ISSET(fe->fd, &xset)) {
			if (FD_ISSET(fe->fd, &es->rset) && fe->r_proc) {
				fe->r_proc(ev, fe);
			}
			if (FD_ISSET(fe->fd, &es->wset) && fe->w_proc) {
				fe->w_proc(ev, fe);
			}
		} else {
			if (FD_ISSET(fe->fd, &rset) && fe->r_proc) {
				fe->r_proc(ev, fe);
			}
			if (FD_ISSET(fe->fd, &wset) && fe->w_proc) {
				fe->w_proc(ev, fe);
			}
		}
	}

	array_clean(es->ready, NULL);

	return n;
}

static int select_checkfd(EVENT *ev UNUSED, FILE_EVENT *fe UNUSED)
{
	return -1;
}

static acl_handle_t select_handle(EVENT *ev)
{
	(void) ev;
	return (acl_handle_t)-1;
}

static const char *select_name(void)
{
	return "select";
}

EVENT *event_select_create(int size)
{
	EVENT_SELECT *es = (EVENT_SELECT *) mem_calloc(1, sizeof(EVENT_SELECT));

	if (__sys_select == NULL) {
		hook_init();
	}

	// override size with system open limit setting
	size      = open_limit(0);
	es->maxfd = -1;
	es->dirty = 0;
	es->files = (FILE_EVENT**) mem_calloc(size, sizeof(FILE_EVENT*));
	es->size  = size;
	es->ready = array_create(100);
	es->count = 0;
	FD_ZERO(&es->rset);
	FD_ZERO(&es->wset);
	FD_ZERO(&es->xset);

	es->event.name   = select_name;
	es->event.handle = select_handle;
	es->event.free   = select_free;

	es->event.event_wait = select_event_wait;
	es->event.checkfd    = (event_oper *) select_checkfd;
	es->event.add_read   = (event_oper *) select_add_read;
	es->event.add_write  = (event_oper *) select_add_write;
	es->event.del_read   = (event_oper *) select_del_read;
	es->event.del_write  = (event_oper *) select_del_write;

	return (EVENT*) es;
}

#endif
