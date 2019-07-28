#ifndef FIBER_LOCK_INCLUDE_H
#define FIBER_LOCK_INCLUDE_H

#include "fiber_define.h"

#ifdef __cplusplus
extern "C" {
#endif

/* fiber locking */

/**
 * fiber mutex, thread unsafety, one fiber mutex can only be used in the
 * same thread, otherwise the result is unpredictable
 */
typedef struct ACL_FIBER_MUTEX ACL_FIBER_MUTEX;

/**
 * fiber read/write mutex, thread unsafety, can only be used in the sampe thread
 */
typedef struct ACL_FIBER_RWLOCK ACL_FIBER_RWLOCK;

/**
 * create one fiber mutex, can only be used in the same thread
 * @return {ACL_FIBER_MUTEX*} fiber mutex returned
 */
FIBER_API ACL_FIBER_MUTEX* acl_fiber_mutex_create(void);

/**
 * free fiber mutex created by acl_fiber_mutex_create
 * @param l {ACL_FIBER_MUTEX*} created by acl_fiber_mutex_create
 */
FIBER_API void acl_fiber_mutex_free(ACL_FIBER_MUTEX* l);

/**
 * lock the specified fiber mutex, return immediately when locked, or will
 * wait until the mutex can be used
 * @param l {ACL_FIBER_MUTEX*} created by acl_fiber_mutex_create
 */
FIBER_API void acl_fiber_mutex_lock(ACL_FIBER_MUTEX* l);

/**
 * try lock the specified fiber mutex, return immediately no matter the mutex
 * can be locked.
 * @param l {ACL_FIBER_MUTEX*} created by acl_fiber_mutex_create
 * @return {int} 0 returned when locking successfully, -1 when locking failed
 */
FIBER_API int acl_fiber_mutex_trylock(ACL_FIBER_MUTEX* l);

/**
 * the fiber mutex be unlock by its owner fiber, fatal will happen when others
 * release the fiber mutex
 * @param l {ACL_FIBER_MUTEX*} created by acl_fiber_mutex_create
 */
FIBER_API void acl_fiber_mutex_unlock(ACL_FIBER_MUTEX* l);

/****************************************************************************/

/**
 * create one fiber rwlock, can only be operated in the sampe thread
 * @return {ACL_FIBER_RWLOCK*}
 */
FIBER_API ACL_FIBER_RWLOCK* acl_fiber_rwlock_create(void);

/**
 * free rw mutex created by acl_fiber_rwlock_create
 * @param l {ACL_FIBER_RWLOCK*} created by acl_fiber_rwlock_create
 */
FIBER_API void acl_fiber_rwlock_free(ACL_FIBER_RWLOCK* l);

/**
 * lock the rwlock, if there is no any write locking on it, the
 * function will return immediately; otherwise, the caller will wait for all
 * write locking be released. Read lock on it will successful when returning
 * @param l {ACL_FIBER_RWLOCK*} created by acl_fiber_rwlock_create
 */
FIBER_API void acl_fiber_rwlock_rlock(ACL_FIBER_RWLOCK* l);

/**
 * try to locking the Readonly lock, return immediately no matter locking
 * is successful.
 * @param l {ACL_FIBER_RWLOCK*} crated by acl_fiber_rwlock_create
 * @retur {int} 1 returned when successfully locked, or 0 returned if locking
 *  operation is failed.
 */
FIBER_API int acl_fiber_rwlock_tryrlock(ACL_FIBER_RWLOCK* l);

/**
 * lock the rwlock in Write Lock mode, return until no any one locking it
 * @param l {ACL_FIBER_RWLOCK*} created by acl_fiber_rwlock_create
 */
FIBER_API void acl_fiber_rwlock_wlock(ACL_FIBER_RWLOCK* l);

/**
 * try to lock the rwlock in Write Lock mode. return immediately no matter
 * locking is successful.
 * @param l {ACL_FIBER_RWLOCK*} created by acl_fiber_rwlock_create
 * @return {int} 1 returned when locking successfully, or 0 returned when
 *  locking failed
 */
FIBER_API int acl_fiber_rwlock_trywlock(ACL_FIBER_RWLOCK* l);

/**
 * the rwlock's Read-Lock owner unlock the rwlock
 * @param l {ACL_FIBER_RWLOCK*} crated by acl_fiber_rwlock_create
 */
FIBER_API void acl_fiber_rwlock_runlock(ACL_FIBER_RWLOCK* l);

/**
 * the rwlock's Write-Lock owner unlock the rwlock
 * @param l {ACL_FIBER_RWLOCK*} created by acl_fiber_rwlock_create
 */
FIBER_API void acl_fiber_rwlock_wunlock(ACL_FIBER_RWLOCK* l);

#ifdef __cplusplus
}
#endif

#endif
