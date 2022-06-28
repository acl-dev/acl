#ifndef FIBER_EVENT_INCLUDE_H
#define FIBER_EVENT_INCLUDE_H

#include "fiber_define.h"

#ifdef __cplusplus
extern "C" {
#endif

/* fiber_event.c */

/**
 * Fiber event mutex object based on IO event, which is thread safety. That's
 * to say one event object can used in different threads
 */
typedef struct ACL_FIBER_EVENT ACL_FIBER_EVENT;

/**
 * When the fiber_event is used in multiple threads for sync, if there're
 * many threads, the flag FIBER_FLAG_USE_MUTEX should be set to avoid internal
 * thundering herd which maybe happen by using atomic; if the threads' number
 * is less than one hundred, the flag FIBER_FLAG_USE_MUTEX needn't be set
 */
#define	FIBER_FLAG_USE_MUTEX	(1 << 0)

/**
 * If this flag is set, msg_fatal will be used other msg_error when error
 * happened, this flag is optional for users
 */
#define FIBER_FLAG_USE_FATAL	(1 << 1)

/**
 * Create fiber event mutex which can be used in fibers mode or threads mode
 * @param flag {unsigned} define as FIBER_FLAG_XXX above
 * @return {ACL_FIBER_EVENT *}
 */
FIBER_API ACL_FIBER_EVENT *acl_fiber_event_create(unsigned flag);

/**
 * Free event mutex returned by acl_fiber_event_create
 * @param {ACL_FIBER_EVENT *}
 */
FIBER_API void acl_fiber_event_free(ACL_FIBER_EVENT *event);

/**
 * Wait for event can be available
 * @param {ACL_FIBER_EVENT *}
 * @return {int} 0 returned if successful, or -1 if error happened
 */
FIBER_API int acl_fiber_event_wait(ACL_FIBER_EVENT *event);

/**
 * Try to wait for event can be available
 * @param {ACL_FIBER_EVENT *}
 * @return {int} 0 returned if successful, or -1 if the event been locked
 */
FIBER_API int acl_fiber_event_trywait(ACL_FIBER_EVENT *event);

/**
 * The event's owner notify the waiters that the event mutex can be available,
 * and the waiter will get the event mutex
 * @param {ACL_FIBER_EVENT *}
 * @return {int} 0 returned if successful, or -1 if error happened
 */
FIBER_API int acl_fiber_event_notify(ACL_FIBER_EVENT *event);

#ifdef __cplusplus
}
#endif

#endif
