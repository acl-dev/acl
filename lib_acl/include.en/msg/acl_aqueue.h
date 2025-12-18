#ifndef ACL_AQUEUE_INCLUDE_H
#define	ACL_AQUEUE_INCLUDE_H

#include "../stdlib/acl_define.h"
#include "../thread/acl_thread.h"

#ifdef __cplusplus
extern "C" {
#endif

#define	ACL_AQUEUE_ERR_UNKNOWN      -1
#define	ACL_AQUEUE_OK               0
#define	ACL_AQUEUE_ERR_LOCK         1
#define	ACL_AQUEUE_ERR_UNLOCK       2
#define	ACL_AQUEUE_ERR_TIMEOUT      3
#define	ACL_AQUEUE_ERR_COND_WAIT    4
#define	ACL_AQUEUE_ERR_COND_SIGNALE 5

typedef struct ACL_AQUEUE_ITEM ACL_AQUEUE_ITEM;
typedef struct ACL_AQUEUE ACL_AQUEUE;

typedef void (*ACL_AQUEUE_FREE_FN)(void *);
/**
 * Create a new queue object.
 * @return ACL_AQUEUE structure pointer
 */
ACL_API ACL_AQUEUE *acl_aqueue_new(void);

/**
 * Set whether to strictly check queue owner, default is not to
 * check. This function must be called before acl_aqueue_free
 * @param queue ACL_AQUEUE structure pointer
 * @param flag Flag value
 */
ACL_API void acl_aqueue_check_owner(ACL_AQUEUE *queue, char flag);

/**
 * Set queue owner, only owner has permission to free queue,
 * otherwise acl_aqueue_free() will fail
 * @param queue ACL_AQUEUE structure pointer
 * @param owner Thread ID identifier, owner's ID
 */
ACL_API void acl_aqueue_set_owner(ACL_AQUEUE *queue, unsigned int owner);

/**
 * Free queue object.
 * @param queue ACL_AQUEUE structure pointer
 * @param free_fn When freeing queue, if this function is not
 *  NULL, internally uses this function to free user-registered
 *  data in queue entries
 */
ACL_API void acl_aqueue_free(ACL_AQUEUE *queue, ACL_AQUEUE_FREE_FN free_fn);

/**
 * Pop an element from queue, if queue is empty, wait until
 * element becomes available.
 * @param queue ACL_AQUEUE structure pointer
 * @return Element pointer passed by user via acl_aqueue_push
 */
ACL_API void *acl_aqueue_pop(ACL_AQUEUE *queue);

/**
 * Pop an element from queue, if queue is empty, wait until
 * element becomes available or timeout.
 * @param queue ACL_AQUEUE structure pointer
 * @param tmo_sec Timeout time for popping element from queue, unit is seconds
 * @param tmo_usec Timeout time for popping element from queue,
 *  unit is microseconds
 * @return Element pointer passed by user via acl_aqueue_push
 */
ACL_API void *acl_aqueue_pop_timedwait(ACL_AQUEUE *queue, int tmo_sec, int tmo_usec);

/**
 * Push an element to queue.
 * @param queue ACL_AQUEUE structure pointer
 * @param data User-provided pointer
 * @return {int} Whether adding queue element succeeded, 0: ok; < 0: error
 */
ACL_API int acl_aqueue_push(ACL_AQUEUE *queue, void *data);

/**
 * Get error code of last queue operation, define as: ACL_AQUEUE_XXX
 * @param queue ACL_AQUEUE structure pointer
 * @return Error code
 */
ACL_API int acl_aqueue_last_error(const ACL_AQUEUE *queue);

/**
 * Set queue to quit state.
 * @param queue ACL_AQUEUE structure pointer
 */
ACL_API void acl_aqueue_set_quit(ACL_AQUEUE *queue);

/**
 * Get current number of elements in queue.
 * @param queue {ACL_AQUEUE*}
 * @return {int} Number of queue elements; < 0 indicates error
 */
ACL_API int acl_aqueue_qlen(ACL_AQUEUE* queue);

#ifdef __cplusplus
}
#endif
#endif
