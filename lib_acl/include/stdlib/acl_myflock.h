#ifndef ACL_FLOCK_INCLUDE_H
#define ACL_FLOCK_INCLUDE_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "acl_define.h"

/**
 * Lock an opened file descriptor.
 * @param fd {ACL_FILE_HANDLE} File descriptor
 * @param lock_style {int} System-provided API lock type, only effective on UNIX:
 *  ACL_FLOCK_STYLE_FLOCK or ACL_FLOCK_STYLE_FCNTL
 * @param operation {int} Lock request type, ACL_FLOCK_OP_XXX
 * @return {int} 0: lock successful; -1: lock failed
 */
ACL_API int acl_myflock(ACL_FILE_HANDLE fd, int lock_style, int operation);

/*
 * Lock styles.
 */
#define ACL_FLOCK_STYLE_FLOCK     1  /**< Use flock function (unix) */
#define ACL_FLOCK_STYLE_FCNTL     2  /**< Use fcntl function (unix) */

/*
 * Lock request types.
 */
#define ACL_FLOCK_OP_NONE         0  /**< No lock */
#define ACL_FLOCK_OP_SHARED       1  /**< Shared lock */
#define ACL_FLOCK_OP_EXCLUSIVE    2  /**< Exclusive lock */

/**
 * No-wait flag, can be combined with ACL_FLOCK_OP_SHARED, or ACL_FLOCK_OP_EXCLUSIVE,
 * cannot be combined with ACL_FLOCK_OP_SHARED alone.
 */
#define ACL_FLOCK_OP_NOWAIT       4

/**
 * Lock type bit mask.
 */
#define ACL_FLOCK_OP_BITS \
	(ACL_FLOCK_OP_SHARED | ACL_FLOCK_OP_EXCLUSIVE | ACL_FLOCK_OP_NOWAIT)

#ifdef  __cplusplus
}
#endif

#endif
