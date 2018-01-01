#include "stdafx.h"
#include "common.h"

#ifdef HAS_SELECT

#include "event.h"
#include "event_select.h"

typedef int(*select_fn)(int, fd_set *, fd_set *, fd_set *, struct timeval *);

static select_fn __sys_select = NULL;

static void hook_init(void)
{
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
} EVENT_SELECT;

static void select_free(EVENT *ev)
{
	EVENT_SELECT *es = (EVENT_SELECT *) ev;
	free(es->files);
	free(es);
}

static int select_add_read(EVENT_SELECT *es, FILE_EVENT *fe)
{
	if (FD_ISSET(fe->fd, &es->wset)) {
		assert(fe->id >= 0 && fe->id < es->count);
		assert(es->files[fe->id] == fe);
	} else {
		assert(es->count < es->size);
		es->files[es->count] = fe;
		fe->id = es->count++;
		FD_SET(fe->fd, &es->xset);
	}

	FD_SET(fe->fd, &es->rset);
	return 0;
}

static int select_add_write(EVENT_SELECT *es, FILE_EVENT *fe)
{
	if (FD_ISSET(fe->fd, &es->rset)) {
		assert(fe->id >= 0 && fe->id < es->count);
		assert(es->files[fe->id] == fe);
	} else {
		assert(es->count < es->size);
		es->files[es->count] = fe;
		fe->id = es->count++;
		FD_SET(fe->fd, &es->xset);
	}

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
	}
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
	}
	return 0;
}

static int select_event_wait(EVENT *ev, int timeout)
{
	EVENT_SELECT *es = (EVENT_SELECT *) ev;
	fd_set rset = es->rset, wset = es->wset, xset = es->xset;
	struct timeval tv, *tp;
	int n, i;

	if (timeout >= 0) {
		tv.tv_sec = timeout / 1000;
		tv.tv_usec = (timeout % 1000) * 1000;
		tp = &tv;
	} else {
		tp = NULL;
	}

#ifdef SYS_WIN
	n = select(0, &rset, &wset, &xset, tp);
#else
	n = select(eventp->maxfd + 1, &rset, &wset, &xset, tp);
#endif
	if (n < 0) {
		if (errno == EINTR) {
			return 0;
		}
		msg_fatal("%s: select error %s", __FUNCTION__, last_serror());
	}

	for (i = 0; i < es->count; i++) {
		FILE_EVENT *fe = es->files[i];

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

	return n;
}

static int select_handle(EVENT *ev)
{
	(void) ev;
	return -1;
}

static const char *select_name(void)
{
	return "select";
}

EVENT *event_select_create(int size)
{
	EVENT_SELECT *es = (EVENT_SELECT *) malloc(sizeof(EVENT_SELECT));

	if (__sys_select == NULL) {
		hook_init();
	}

	es->files = (FILE_EVENT**) calloc(size, sizeof(FILE_EVENT*));
	es->size  = size;
	es->count = 0;
	FD_ZERO(&es->rset);
	FD_ZERO(&es->wset);
	FD_ZERO(&es->xset);

	es->event.name   = select_name;
	es->event.handle = select_handle;
	es->event.free   = select_free;

	es->event.event_wait = select_event_wait;
	es->event.add_read   = (event_oper *) select_add_read;
	es->event.add_write  = (event_oper *) select_add_write;
	es->event.del_read   = (event_oper *) select_del_read;
	es->event.del_write  = (event_oper *) select_del_write;

	return (EVENT*) es;
}

#endif
