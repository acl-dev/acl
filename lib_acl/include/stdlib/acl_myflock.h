#ifndef ACL_FLOCK_INCLUDE_H
#define ACL_FLOCK_INCLUDE_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "acl_define.h"

/**
 * 对打开的文件句柄进行加锁
 * @param fd {ACL_FILE_HANDLE} 文件句柄
 * @param lock_style {int} 系统提供的API加锁类型（仅对UNIX有效）
 *  ACL_FLOCK_STYLE_FLOCK or ACL_FLOCK_STYLE_FCNTL
 * @param operation {int} 加锁操作方式, ACL_FLOCK_OP_XXX
 * @return {int} 0: 加锁成功; -1: 加锁失败
 */
ACL_API int acl_myflock(ACL_FILE_HANDLE fd, int lock_style, int operation);

/*
 * Lock styles.
 */
#define ACL_FLOCK_STYLE_FLOCK     1  /**< 调用 flock 函数加锁(unix) */
#define ACL_FLOCK_STYLE_FCNTL     2  /**< 调用 fcntl 函数加锁(unix) */

/*
 * Lock request types.
 */
#define ACL_FLOCK_OP_NONE         0  /**< 解锁 */
#define ACL_FLOCK_OP_SHARED       1  /**< 共享锁 */
#define ACL_FLOCK_OP_EXCLUSIVE    2  /**< 排它独享锁 */

/**
 * 无等待加锁, 可以与 ACL_FLOCK_OP_SHARED, 或 ACL_FLOCK_OP_EXCLUSIVE 相与,
 * 可以与 ACL_FLOCK_OP_SHARED 相与
 */
#define ACL_FLOCK_OP_NOWAIT       4

/**
 * 加锁方式的位集合
 */
#define ACL_FLOCK_OP_BITS \
	(ACL_FLOCK_OP_SHARED | ACL_FLOCK_OP_EXCLUSIVE | ACL_FLOCK_OP_NOWAIT)

#ifdef  __cplusplus
}
#endif

#endif
