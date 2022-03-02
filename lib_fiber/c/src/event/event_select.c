#include "stdafx.h"
#include "common.h"

#ifdef HAS_SELECT

#include "fiber/libfiber.h"
#include "hook/hook.h"
#include "event.h"
#include "event_select.h"

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
#ifdef	DELAY_CALL
	ARRAY *r_ready;
	ARRAY *w_ready;
#endif
} EVENT_SELECT;

static void select_free(EVENT *ev)
{
	EVENT_SELECT *es = (EVENT_SELECT *) ev;
	mem_free(es->files);
#ifdef	DELAY_CALL
	array_free(es->r_ready, NULL);
	array_free(es->w_ready, NULL);
#endif
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
	FILE_EVENT *fe;
#ifdef	DELAY_CALL
	ITER iter;
#endif
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

	n = (*sys_select)(0, &rset, &wset, &xset, tp);
#else
	if (es->dirty) {
		es->maxfd = -1;
		for (i = 0; i < es->count; i++) {
			fe = es->files[i];
			if (fe->fd > es->maxfd) {
				es->maxfd = fe->fd;
			}
		}
	}
	n = (*sys_select)(es->maxfd + 1, &rset, &wset, &xset, tp);
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
		fe = es->files[i];

		if (FD_ISSET(fe->fd, &xset)) {
			if (FD_ISSET(fe->fd, &es->rset) && fe->r_proc) {
				CLR_READWAIT(fe);
#ifdef	DELAY_CALL
				array_append(es->r_ready, fe);
#else
				fe->r_proc(ev, fe);
#endif
			}
			if (FD_ISSET(fe->fd, &es->wset) && fe->w_proc) {
				CLR_WRITEWAIT(fe);
#ifdef	DELAY_CALL
				array_append(es->w_ready, fe);
#else
				fe->w_proc(ev, fe);
#endif
			}
		} else {
			if (FD_ISSET(fe->fd, &rset) && fe->r_proc) {
				CLR_READWAIT(fe);
#ifdef	DELAY_CALL
				array_append(es->r_ready, fe);
#else
				fe->r_proc(ev, fe);
#endif
			}
			if (FD_ISSET(fe->fd, &wset) && fe->w_proc) {
				CLR_WRITEWAIT(fe);
#ifdef	DELAY_CALL
				array_append(es->w_ready, fe);
#else
				fe->w_proc(ev, fe);
#endif
			}
		}
	}

#ifdef	DELAY_CALL
	foreach(iter, es->r_ready) {
		fe = (FILE_EVENT *) iter.data;
		fe->r_proc(ev, fe);
	}

	foreach(iter, es->w_ready) {
		fe = (FILE_EVENT *) iter.data;
		fe->w_proc(ev, fe);
	}

	array_clean(es->r_ready, NULL);
	array_clean(es->w_ready, NULL);
#endif

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

	if (sys_select == NULL) {
		hook_once();
		if (sys_select == NULL) {
			msg_error("%s: sys_select NULL", __FUNCTION__);
			return NULL;
		}
	}

	// override size with system open limit setting
	size      = open_limit(0);
	es->maxfd = -1;
	es->dirty = 0;
	es->files = (FILE_EVENT**) mem_calloc(size, sizeof(FILE_EVENT*));
	es->size  = size;

#ifdef	DELAY_CALL
	es->r_ready = array_create(100);
	es->w_ready = array_create(100);
#endif

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
