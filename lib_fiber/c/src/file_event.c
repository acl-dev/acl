#include "stdafx.h"
#include "common.h"

#include "fiber/libfiber.h"
#include "event.h"

void file_event_init(FILE_EVENT *fe, socket_t fd)
{
	ring_init(&fe->me);
	fe->fiber_r  = acl_fiber_running();
	fe->fiber_w  = acl_fiber_running();
	fe->fd     = fd;
	fe->id     = -1;
	fe->status = STATUS_NONE;
	fe->type   = TYPE_NONE;
	fe->oper   = 0;
	fe->mask   = 0;
	fe->r_proc = NULL;
	fe->w_proc = NULL;

#ifdef HAS_POLL
	fe->pfd    = NULL;
#endif

#ifdef	HAS_IO_URING
	memset(&fe->in, 0, sizeof(fe->in));
	memset(&fe->out, 0, sizeof(fe->out));
	memset(&fe->var, 0, sizeof(fe->var));
	memset(&fe->reader_ctx, 0, sizeof(fe->reader_ctx));
	memset(&fe->writer_ctx, 0, sizeof(fe->writer_ctx));
	fe->r_timeout = -1;
	fe->w_timeout = -1;
#endif

#ifdef HAS_IOCP
	fe->rbuf    = NULL;
	fe->rsize   = 0;
	fe->res     = 0;
	fe->h_iocp  = NULL;
	fe->reader  = NULL;
	fe->writer  = NULL;
	fe->poller_read  = NULL;
	fe->poller_write = NULL;
	fe->iocp_sock    = INVALID_SOCKET;
	fe->sock_type    = getsocktype(fd);
	memset(&fe->var, 0, sizeof(fe->var));
#endif

	fe->refer = 1;
	fe->busy  = 0;
}

FILE_EVENT *file_event_alloc(socket_t fd)
{
	FILE_EVENT *fe = (FILE_EVENT *) mem_calloc(1, sizeof(FILE_EVENT));
	file_event_init(fe, fd);
	return fe;
}

static void file_event_free(FILE_EVENT *fe)
{
	if (fe->mbox_wsem) {
		acl_fiber_sem_free(fe->mbox_wsem);
	}

	memset(fe, 0, sizeof(*fe));
	mem_free(fe);
}

int file_event_refer(FILE_EVENT *fe)
{
	return ++fe->refer;
}

int file_event_unrefer(FILE_EVENT *fe)
{
	if (--fe->refer <= 0) {
		file_event_free(fe);
		return 0;
	} else {
		return fe->refer;
	}
}
