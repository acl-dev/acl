/*
 * Name: misc.h
 * Author: zsx
 * Date: 2003-12-16
 * Version: 1.0
 */

#ifndef	ACL_MISC_INCLUDE_H
#define	ACL_MISC_INCLUDE_H

#ifdef  __cplusplus
extern "C" {
#endif
#include "acl_define.h"
#include <time.h>

/* acl_str2time.c */
/**
 * 将时间字符串转换为 time_t 类型
 * @param str 时间字符串格式为: year-month-mday(如: 2004-1-1)
 * @return time_t 类型的值
 */
ACL_API time_t acl_str2time_t(const char *str);

#ifdef  __cplusplus
}
#endif

#endif

