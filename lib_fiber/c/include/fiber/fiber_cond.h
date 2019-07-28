#ifndef FIBER_COND_INCLUDE_H
#define FIBER_COND_INCLUDE_H

#include "fiber_define.h"
#include "fiber_event.h"

#if !defined(_WIN32) && !defined(_WIN64)

#ifdef __cplusplus
extern "C" {
#endif

/* fiber_cond.h */

/**
 * fiber_cond object look like pthread_cond_t which is used between threads
 * and fibers
 */
typedef struct ACL_FIBER_COND ACL_FIBER_COND;

/**
 * create fiber cond which can be used in fibers more or threads mode
 * @param flag {unsigned} current not used, just for the future extend
 * @return {ACL_FIBER_COND *}
 */
FIBER_API ACL_FIBER_COND *acl_fiber_cond_create(unsigned flag);

/**
 * free cond created by acl_fiber_cond_create
 * @param cond {ACL_FIBER_COND *}
 */
FIBER_API void acl_fiber_cond_free(ACL_FIBER_COND *cond);

/**
 * wait for cond event to be signaled
 * @param cond {ACL_FIBER_COND *}
 * @param event {ACL_FIBER_EVENT *} must be owned by the current caller
 * @return {int} return 0 if ok or return error value
 */
FIBER_API int acl_fiber_cond_wait(ACL_FIBER_COND *cond, ACL_FIBER_EVENT *event);

/**
 * wait for cond event to be signaled with the specified timeout
 * @param cond {ACL_FIBER_COND *}
 * @return {int} return 0 if ok or return error value, when timedout ETIMEDOUT
 *  will be returned
 */
FIBER_API int acl_fiber_cond_timedwait(ACL_FIBER_COND *cond,
	ACL_FIBER_EVENT *event, int delay_ms);

/**
 * signle the cond which will wakeup one waiter for the cond to be signaled
 * @param cond {ACL_FIBER_COND *}
 * @return {int} return 0 if ok or return error value
 */
FIBER_API int acl_fiber_cond_signal(ACL_FIBER_COND *cond);

#ifdef __cplusplus
}
#endif

#endif // !defined(_WIN32) && !defined(_WIN64)

#endif
