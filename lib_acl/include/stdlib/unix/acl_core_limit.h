#ifndef	ACL_CORE_LIMIT_INCLUDE_H
#define	ACL_CORE_LIMIT_INCLUDE_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "../acl_define.h"
#ifdef ACL_UNIX

/**
 * 调用此函数设置程序崩溃时产生的 core 文件的最大值
 * @param max {unsigned long long int} 当该值 == 0 时，
 *  则使用系统规定的最大值，否则将使用该值
 */
void acl_set_core_limit(unsigned long long int max);

#endif  /* ACL_UNIX */

#ifdef  __cplusplus
}
#endif

#endif
