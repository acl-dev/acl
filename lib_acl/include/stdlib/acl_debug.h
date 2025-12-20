#ifndef ACL_DEBUG_INCLUDE_H
#define	ACL_DEBUG_INCLUDE_H

#include "acl_define.h"
#include "acl_msg.h"

#ifdef __cplusplus
extern "C" {
#endif

#define	ACL_DEBUG_INTER_BASE    0  /**< Minimum section tag value */
#define	ACL_DEBUG_WQ            (ACL_DEBUG_INTER_BASE + 1)  /**< ACL_WORKQ
							     *  section tag */
#define ACL_DEBUG_PROCTL        (ACL_DEBUG_INTER_BASE + 2)  /**< ACL_PROCTL
							     *  section tag */
#define ACL_DEBUG_THR_POOL      (ACL_DEBUG_INTER_BASE + 3)  /**<
							     *  ACL_PTHREAD_POOL
							     *  section tag */
#define	ACL_DEBUG_EVENT		(ACL_DEBUG_INTER_BASE + 4)  /**< ACL_EVENT
							     *  section tag */

/**
 * Debug logging interface.
 * @param SECTION {int} Section tag value
 * @param LEVEL {int} Level value corresponding to SECTION section tag
 */
#define acl_debug(SECTION, LEVEL) \
	!acl_do_debug((SECTION), (LEVEL)) ? (void) 0 : acl_msg_info

/**
 * Free some internal memory resources.
 */
ACL_API void acl_debug_end(void);

/**
 * Initialize debug logging interface.
 * @param ptr {const char*} Section tag configuration string, format:
 *  Format: 1:1, 2:10, 3:8...  or 1:1; 2:10; 3:8...
 */
ACL_API void acl_debug_init(const char *ptr);

/**
 * Initialize debug logging interface.
 * @param ptr {const char*} Section tag configuration string, format:
 *  Format: 1:1, 2:10, 3:8...  or 1:1; 2:10; 3:8...
 * @param max_debug_level {int} Maximum section tag value
 */
ACL_API void acl_debug_init2(const char *ptr, int max_debug_level);

/**
 * Check whether the given section tag's level is within debug logging range.
 * @param section {int} Tag value
 * @param level {int} Level value
 */
ACL_API int acl_do_debug(int section, int level);

#ifdef __cplusplus
}
#endif

#endif
