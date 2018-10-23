#ifndef	ACL_CORE_LIMIT_INCLUDE_H
#define	ACL_CORE_LIMIT_INCLUDE_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "../acl_define.h"
#ifdef ACL_UNIX

/**
 * 调用此函数设置程序崩溃时产生的 core 文件的最大值
 * @param max {long long int} 根据 max 的值范围不同，生成 core 规则有所不同：
 *  1)   0：禁止生成 core 文件
 *  2) < 0：生成 core 文件，且不限制 core 文件生成大小
 *  3) > 0：生成 core 文件，且 core 文件大小由 max 决定
 */
void acl_set_core_limit(long long int max);

#endif  /* ACL_UNIX */

#ifdef  __cplusplus
}
#endif

#endif
