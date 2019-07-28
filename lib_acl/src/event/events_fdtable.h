#ifndef	__EVENTS_FDTABLE_INCLUDE_H__
#define	__EVENTS_FDTABLE_INCLUDE_H__

#ifdef	__cplusplus
extern "C" {
#endif

#include "event/acl_events.h"

extern ACL_EVENT_FDTABLE *event_fdtable_alloc(void);
extern void event_fdtable_free(ACL_EVENT_FDTABLE *fdp);
extern void event_fdtable_reset(ACL_EVENT_FDTABLE *fdp);

#ifdef	__cplusplus
}
#endif

#endif

