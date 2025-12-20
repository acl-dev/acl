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
 * Convert a time string to time_t type.
 * @param str Time string format: year-month-mday (e.g., 2004-1-1)
 * @return time_t type value
 */
ACL_API time_t acl_str2time_t(const char *str);

/* acl_localtime.c */
/**
 * Convert time_t type time to the specified time structure object, similar to
 * localtime_r() in glibc. The localtime_r() implementation in glibc uses internal
 * locks, which affects performance, and may also cause issues with fork() in
 * multi-threaded environments. Therefore, Redis's implementation is referenced here.
 * @param t {const time_t*} Time value since 1970.1.1 (in seconds)
 * @param result {struct tm*} Structure object to store the conversion result
 * @return {struct tm *} Return value: NULL indicates conversion success, non-NULL indicates failure
 */
ACL_API struct tm *acl_localtime_r(const time_t *t, struct tm *result);

#ifdef  __cplusplus
}
#endif

#endif
