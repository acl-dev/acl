#include "stdafx.h"
#include "common.h"

#include "fiber/libfiber.h"
#include "event.h"

void file_event_init(FILE_EVENT *fe, socket_t fd)
{
	ring_init(&fe->me);
	fe->fiber  = acl_fiber_running();
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

#ifdef HAS_IOCP
	fe->h_iocp = NULL;
	fe->reader = NULL;
	fe->writer = NULL;
	fe->iocp_sock = INVALID_SOCKET;
	memset(&fe->peer_addr, 0, sizeof(fe->peer_addr));
#endif
}

FILE_EVENT *file_event_alloc(socket_t fd)
{
	FILE_EVENT *fe = (FILE_EVENT *) mem_calloc(1, sizeof(FILE_EVENT));
	file_event_init(fe, fd);
	return fe;
}

void file_event_free(FILE_EVENT *fe)
{
	mem_free(fe);
}
