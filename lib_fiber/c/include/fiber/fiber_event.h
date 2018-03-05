#ifndef FIBER_EVENT_INCLUDE_H
#define FIBER_EVENT_INCLUDE_H

#include "fiber_define.h"

#ifdef __cplusplus
extern "C" {
#endif

/* fiber_event.c */

/* fiber event mutex object based on IO event, which is thread safety. That's
 * to say one event object can used in different threads
 */
typedef struct ACL_FIBER_EVENT ACL_FIBER_EVENT;

/**
 * create fiber event mutex which can be used in fibers mode or threads mode
 * @return {ACL_FIBER_EVENT *}
 */
FIBER_API ACL_FIBER_EVENT *acl_fiber_event_create(void);

/**
 * free event mutex returned by acl_fiber_event_create
 * @param {ACL_FIBER_EVENT *}
 */
FIBER_API void acl_fiber_event_free(ACL_FIBER_EVENT *event);

/**
 * wait for event can be available
 * @param {ACL_FIBER_EVENT *}
 * @return {int} 0 returned if successful, or -1 if error happened
 */
FIBER_API int acl_fiber_event_wait(ACL_FIBER_EVENT *event);

/**
 * try to wait for event can be available
 * @param {ACL_FIBER_EVENT *}
 * @return {int} 0 returned if successful, or -1 if the event been locked
 */
FIBER_API int acl_fiber_event_trywait(ACL_FIBER_EVENT *event);

/**
 * the event's owner notify the waiters that the event mutex can be available,
 * and the waiter will get the event mutex
 * @param {ACL_FIBER_EVENT *}
 * @return {int} 0 returned if successful, or -1 if error happened
 */
FIBER_API int acl_fiber_event_notify(ACL_FIBER_EVENT *event);

#ifdef __cplusplus
}
#endif

#endif
