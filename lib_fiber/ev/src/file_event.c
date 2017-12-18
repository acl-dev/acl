#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include "fiber/lib_fiber.h"
#include "event.h"

void file_event_init(FILE_EVENT *fe, int fd)
{
	acl_ring_init(&fe->me);
	fe->fiber  = acl_fiber_running();
	fe->fd     = fd;
	fe->type   = 0;
	fe->oper   = 0;
	fe->mask   = 0;
	fe->r_proc = NULL;
	fe->w_proc = NULL;
	fe->pfd    = NULL;
}

FILE_EVENT *file_event_alloc(int fd)
{
	FILE_EVENT *fe = (FILE_EVENT *) acl_mymalloc(sizeof(FILE_EVENT));
	file_event_init(fe, fd);
	return fe;
}

void file_event_free(FILE_EVENT *fe)
{
	acl_myfree(fe);
}
