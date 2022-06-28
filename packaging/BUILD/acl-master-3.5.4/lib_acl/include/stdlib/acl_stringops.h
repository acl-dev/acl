#ifndef ACL_STRINGOPS_INCLUDE_H
#define ACL_STRINGOPS_INCLUDE_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "acl_define.h"

/**
 * 判断给定字符串是否全为数字
 * @param str {const char*} 字符串
 * @return {int} 0: 否; 1: 是
 */
ACL_API int acl_alldig(const char *str);

/**
 * 判断给定字符串是否为浮点数
 * @param s {const char*} 字符串
 * @return {int} 0: 否; 1: 是
 */
ACL_API int acl_is_double(const char *s);

/**
 * 将多个字符串拼接成一个字符串
 * @param arg0 {const char*} 第一个非空字符串
 * @param ... 后续的字符串集合，结束符是 NULL
 * @return {char*} 结果字符串，不为空, 该字符串需要调用 acl_myfree 释放
 */
ACL_API char *acl_concatenate(const char *arg0,...);

/**
 * 从一个全路径的文件名中取得文件名部分，如：
 * 从 "/tmp/test.txt" 或 "\\tmp\\test.txt" 中取得 test.txt
 * @param path {const char*} 带有路径的文件名，如："/tmp/test.txt"
 *  或 "\\tmp\\test.txt"
 * @return {const char*} 文件名，该返回值不需要释放，如果返回的地址
 *  为空串(即第一个字节为 '\0') 则说明所给路径不含文件名
 */
ACL_API const char *acl_safe_basename(const char *path);

/**
 * 将所给的字符串进行分隔，分别取出 name, value 地址, 输入字符串可以为
 * {sp}{name}{sp}={sp}{value}{sp}, 如果分析成功，则将结果分别进行存储,
 * 其中 {sp} 字符可以为: 空格, "\t", "\r", "\n"
 * @param buf {char*} 输入的字符串, 不能为空
 * @param name {char**} 存储结果的地址指针，不能为空
 * @param value {char**} 存储结果的地址指针，不能为空
 * @return {const char*} 出错原因，如果为空则表示解析成功，否则表示解析失败并返回
 *  失败原因
 */
ACL_API const char *acl_split_nameval(char *buf, char **name, char **value);

#ifdef  __cplusplus
}
#endif

#endif

