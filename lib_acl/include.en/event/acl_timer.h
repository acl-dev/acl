#ifndef	ACL_TIMER_INCLUDE_H
#define	ACL_TIMER_INCLUDE_H

#include "../stdlib/acl_define.h"
#include <time.h>
#include "../stdlib/acl_iterator.h"
#include "../stdlib/acl_ring.h"

#ifdef	__cplusplus
extern "C" {
#endif

/**
 * Timer information object.
 */
typedef struct ACL_TIMER_INFO {               
	/* public */
	void *obj;              /**< Pointer to user data object */
	acl_int64 when;         /**< Expiration time (microseconds) */

	/* private */
	ACL_RING entry;         /**< Internal timer list entry */
} ACL_TIMER_INFO;

/* Timer container structure */
typedef struct ACL_TIMER ACL_TIMER;

struct ACL_TIMER {
        acl_int64 (*request)(ACL_TIMER *timer, void *obj, acl_int64 delay);
        acl_int64 (*cancel)(ACL_TIMER *timer, void *obj);
        void* (*popup)(ACL_TIMER* timer);

        ACL_RING timer_header;
        acl_int64 present;
        acl_int64 time_left;

	/* for acl_iterator */

	/* Get iterator at the head of the timer list */
	const void *(*iter_head)(ACL_ITER*, struct ACL_TIMER*);
	/* Get next iterator position */
	const void *(*iter_next)(ACL_ITER*, struct ACL_TIMER*);
	/* Get iterator at the tail of the timer list */
	const void *(*iter_tail)(ACL_ITER*, struct ACL_TIMER*);
	/* Get previous iterator position */
	const void *(*iter_prev)(ACL_ITER*, struct ACL_TIMER*);

	/* Get the ACL_TIMER_INFO associated with the current iterator position */
	const ACL_TIMER_INFO *(*iter_info)(ACL_ITER*, struct ACL_TIMER*);
};

/**
 * Add a timer request.
 * @param timer {ACL_TIMER*} Timer container
 * @param obj {void*} User-defined context data
 * @param delay {acl_int64} Delay until expiration (microseconds)
 * @return {acl_int64} Absolute expiration time (microseconds)
 */
ACL_API acl_int64 acl_timer_request(ACL_TIMER* timer, void *obj, acl_int64 delay);

/**
 * Cancel a timer request.
 * @param timer {ACL_TIMER*} Timer container
 * @param obj {void*} User-defined context data
 * @return {acl_int64} Time when the next timer will fire (microseconds)
 */
ACL_API acl_int64 acl_timer_cancel(ACL_TIMER* timer, void *obj);

/**
 * Pop the next expired timer from the container.
 * @param timer {ACL_TIMER*} Timer container
 * @return {void*} User-defined context data for the expired timer
 */
ACL_API void *acl_timer_popup(ACL_TIMER* timer);

/**
 * Get the remaining time until the next timer fires.
 * @param timer {ACL_TIMER*} Timer container
 * @return {acl_int64} Remaining time in microseconds
 */
ACL_API acl_int64 acl_timer_left(ACL_TIMER* timer);

/**
 * Walk through all timers in the container and perform a user-defined action.
 * @param timer {ACL_TIMER*} Timer container
 * @param action {void (*)(ACL_TIMER_INFO*, void*)} User callback invoked for each timer
 * @param arg {void*} Second argument passed to the action callback
 */
ACL_API void acl_timer_walk(ACL_TIMER *timer, void (*action)(ACL_TIMER_INFO *, void *), void *arg);

/**
 * Create a new timer container.
 * @return {ACL_TIMER*} Newly allocated timer container
 */
ACL_API ACL_TIMER *acl_timer_new(void);

/**
 * Free a timer container.
 * @param timer {ACL_TIMER*} Timer container
 * @param free_fn {void (*)(void*)} Callback used to free user data objects
 */
ACL_API void acl_timer_free(ACL_TIMER* timer, void (*free_fn)(void*));

/**
 * Get the number of timers stored in the container.
 * @param timer {ACL_TIMER*}
 * @return {int} Number of timers (>= 0)
 */
ACL_API int acl_timer_size(ACL_TIMER *timer);

#ifdef	__cplusplus
}
#endif

#endif
