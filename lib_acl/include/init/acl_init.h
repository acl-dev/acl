#ifndef ACL_INIT_INCLUDE_H
#define ACL_INIT_INCLUDE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../stdlib/acl_define.h"

/**
 * Initialize ACL library.
 */
ACL_API void acl_lib_init(void);

/**
 * Cleanup ACL library.
 */
ACL_API void acl_lib_end(void);

/**
 * Whether to prefer using poll over select.
 * @param yesno {int} Non-zero indicates prefer using poll
 */
ACL_API void acl_poll_prefered(int yesno);

/**
 * Get current ACL library version information.
 * @return {const char*} Current ACL library version information
 */
ACL_API const char *acl_version(void);

/**
 * Get current ACL library compilation configuration information.
 * @return {const char*} Returns non-empty string, this function is thread-safe
 */
ACL_API const char *acl_verbose(void);

/**
 * Get main thread's thread ID.
 * @return {unsigned int}
 */
ACL_API unsigned long acl_main_thread_self(void);

#ifdef __cplusplus
}
#endif
#endif
