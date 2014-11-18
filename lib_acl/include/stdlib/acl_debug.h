#ifndef ACL_DEBUG_INCLUDE_H
#define	ACL_DEBUG_INCLUDE_H

#include "acl_define.h"
#include "acl_msg.h"

#ifdef __cplusplus
extern "C" {
#endif

#define	ACL_DEBUG_INTER_BASE    0  /**< 最小调试标签值 */
#define	ACL_DEBUG_WQ            (ACL_DEBUG_INTER_BASE + 1)  /**< ACL_WORKQ 调试标签 */
#define ACL_DEBUG_PROCTL        (ACL_DEBUG_INTER_BASE + 2)  /**< ACL_PROCTL 调试标签 */
#define ACL_DEBUG_THR_POOL      (ACL_DEBUG_INTER_BASE + 3)  /**< ACL_PTHREAD_POOL 调试标签 */
#define	ACL_DEBUG_EVENT		(ACL_DEBUG_INTER_BASE + 4)  /**< ACL_EVENT 调度标签 */

/**
 * 日志调试宏接口
 * @param SECTION {int} 调试标签值
 * @param LEVEL {int} 对应于SECTION调试标签的级别
 */
#define acl_debug(SECTION, LEVEL) \
	!acl_do_debug((SECTION), (LEVEL)) ? (void) 0 : acl_msg_info

/**
 * 释放内部一些内存等资源
 */
ACL_API void acl_debug_end(void);

/**
 * 初始化日志调试调用接口
 * @param ptr {const char*} 调试标签及级别字符串，
 *  格式: 1:1, 2:10, 3:8...  or 1:1; 2:10; 3:8...
 */
ACL_API void acl_debug_init(const char *ptr);

/**
 * 初始化日志调试调用接口
 * @param ptr {const char*} 调试标签及级别字符串，
 *  格式: 1:1, 2:10, 3:8...  or 1:1; 2:10; 3:8...
 * @param max_debug_level {int} 最大调试标签值
 */
ACL_API void acl_debug_init2(const char *ptr, int max_debug_level);

/**
 * 判断给定标签的级别是否在日志输出条件范围内
 * @param section {int} 标签值
 * @param level {int} 级别值
 */
ACL_API int acl_do_debug(int section, int level);

#ifdef __cplusplus
}
#endif

#endif
