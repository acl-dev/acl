#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <stdio.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_ring.h"
#include "stdlib/acl_msg.h"
#include "event/acl_events.h"

#endif

#include "events.h"
#include "events_fdtable.h"

ACL_EVENT_FDTABLE *event_fdtable_alloc()
{
	ACL_EVENT_FDTABLE *fdp = acl_mycalloc(1, sizeof(ACL_EVENT_FDTABLE));
	fdp->fdidx = -1;
	fdp->fdidx_ready = -1;
	return (fdp);
}

void event_fdtable_free(ACL_EVENT_FDTABLE *fdp)
{
	acl_myfree(fdp);
}

void event_fdtable_reset(ACL_EVENT_FDTABLE *fdp)
{
	memset(fdp, 0, sizeof(ACL_EVENT_FDTABLE));
	fdp->fdidx = -1;
	fdp->fdidx_ready = -1;
}
