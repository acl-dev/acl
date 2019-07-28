#ifndef ACL_INIT_INCLUDE_H
#define ACL_INIT_INCLUDE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../stdlib/acl_define.h"

/**
 * 初始化整个ACL库
 */
ACL_API void acl_lib_init(void);

/**
 * 结束整个ACL库
 */
ACL_API void acl_lib_end(void);

/**
 * 是否优先使用 poll 而非 select
 * @param yesno {int} 非 0 时表示优先使用 poll
 */
ACL_API void acl_poll_prefered(int yesno);

/**
 * 获得当前 acl 库的版本信息
 * @return {const char*} 当前 acl 库版本信息
 */
ACL_API const char *acl_version(void);


/**
 * 获得主线程的线程号
 * @return {unsigned int}
 */
ACL_API unsigned long acl_main_thread_self(void);

#ifdef __cplusplus
}
#endif
#endif
