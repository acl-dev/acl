#ifndef ACL_SPLIT_AT_INCLUDE_H
#define ACL_SPLIT_AT_INCLUDE_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "acl_define.h"

/**
 * 从字符串左边开始将包含给定分隔符在内的右边截断
 * @param string {char*} 源字符串
 * @param delimiter {int} 分隔符
 * @return {char*} 分隔符以右的字符串，当为NULL时表明未找到指定分隔符
 */
ACL_API char *acl_split_at(char *string, int delimiter);

/**
 * 从字符串右边开始将包含给定分隔符在内的右边截断
 * @param string {char*} 源字符串
 * @param delimiter {int} 分隔符
 * @return {char*} 分隔符以右的字符串，当为NULL时表明未找到指定分隔符
 */
ACL_API char *acl_split_at_right(char *string, int delimiter);

#ifdef  __cplusplus
}
#endif

#endif

