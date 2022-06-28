#ifndef	__EVENTS_DOG_INCLUDE_H__
#define	__EVENTS_DOG_INCLUDE_H__

#ifdef	__cplusplus
extern "C" {
#endif

#include "stdlib/acl_define.h"
#include "event/acl_events.h"
	
typedef struct EVENT_DOG EVENT_DOG;

EVENT_DOG *event_dog_create(ACL_EVENT *eventp, int thread_mode);
void event_dog_notify(EVENT_DOG *evdog);
void event_dog_wait(EVENT_DOG *evdog);
ACL_VSTREAM *event_dog_client(EVENT_DOG *evdog);
void event_dog_free(EVENT_DOG *evdog);

#ifdef	__cplusplus
}
#endif

#endif
