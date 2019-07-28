#ifndef __EVENTS_IOCP_INCLUDE_H__
#define __EVENTS_IOCP_INCLUDE_H__

#ifdef	__cplusplus
extern "C" {
#endif

#include "events_define.h"

#ifdef	ACL_EVENTS_STYLE_IOCP

#define	EVENT_NAME			"iocp"
#define MAX_WAIT_OBJECTS	64

ACL_API ACL_EVENT *event_new_iocp(int fdsize);

#endif

#ifdef	__cplusplus
}
#endif

#endif
