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

/* acl_localtime.c */
/**
 * 将整型的时间转换为指定的时间结构对象,类似于glibc中的 localtime_r(), 而 glibc
 * 实现的 localtime_r() 内部有锁, 会影响效率, 同时可能会造成fork()后的死锁问题,
 * 所以将 Redis 的代码中的实现放在此处.
 * @param t {const time_t*} 1970.1.1以来的时间值(秒级)
 * @param result {struct tm*} 存放转换结果对象
 * @return {struct tm *} 返回值非NULL表示转换成功,否则表示失败
 */
ACL_API struct tm *acl_localtime_r(const time_t *t, struct tm *result);

#ifdef  __cplusplus
}
#endif

#endif

