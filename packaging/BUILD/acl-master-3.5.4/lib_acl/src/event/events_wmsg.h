#ifndef __EVENTS_WMSG_INCLUDE_H__
#define __EVENTS_WMSG_INCLUDE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "events_define.h"

#ifdef	ACL_EVENTS_STYLE_WMSG
# define	EVENT_NAME_WMSG		"windows message"
ACL_API ACL_EVENT *event_new_wmsg(UINT nMsg);
#endif 

#ifdef __cplusplus
}
#endif

#endif /* __EVENTS_WMSG_INCLUDE_H__ */
