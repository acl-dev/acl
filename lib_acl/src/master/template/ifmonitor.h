#ifndef __IFMONITOR_INCLUDE_H__
#define __IFMONITOR_INCLUDE_H__

#ifdef	__cplusplus
extern "C" {
#endif

#include "event/acl_events.h"

#ifdef ACL_LINUX

typedef void (*monitor_callback)(void *);

void netlink_monitor(ACL_EVENT *event, monitor_callback callback, void *ctx);
#endif

#ifdef	__cplusplus
}
#endif

#endif
