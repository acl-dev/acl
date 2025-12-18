#ifndef ACL_STRINGOPS_INCLUDE_H
#define ACL_STRINGOPS_INCLUDE_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "acl_define.h"

/**
 * Check whether the given string is all digits.
 * @param str {const char*} String
 * @return {int} 0: no; 1: yes
 */
ACL_API int acl_alldig(const char *str);

/**
 * Check whether the given string is a floating point number.
 * @param s {const char*} String
 * @return {int} 0: no; 1: yes
 */
ACL_API int acl_is_double(const char *s);

/**
 * Concatenate multiple strings into one string.
 * @param arg0 {const char*} First non-empty string
 * @param ... Variable number of strings, must end with NULL
 * @return {char*} Concatenated string, must not be NULL, string needs
 *  to be freed with acl_myfree
 */
ACL_API char *acl_concatenate(const char *arg0,...);

/**
 * Extract file name from a full path file name, e.g.:
 * Extract "test.txt" from "/tmp/test.txt" or "\\tmp\\test.txt"
 * @param path {const char*} Full path file name, e.g., "/tmp/test.txt"
 *  or "\\tmp\\test.txt"
 * @return {const char*} File name, return value does not need to be freed,
 *  if returned address is empty (first byte is '\0'), it means path does not
 *  contain file name
 */
ACL_API const char *acl_safe_basename(const char *path);

/**
 * Parse a string with separator, extract name and value addresses. String format
 * should be {sp}{name}{sp}={sp}{value}{sp}. If parsing succeeds, results are
 * stored directly, where {sp} characters can be: space, "\t", "\r", "\n"
 * @param buf {char*} Source string, must not be NULL
 * @param name {char**} Storage address pointer for name, must not be NULL
 * @param value {char**} Storage address pointer for value, must not be NULL
 * @param sep {char} Separator
 * @return {const char*} If return value is NULL, indicates parsing succeeded;
 *  if non-NULL, indicates parsing failed and return value is failure reason
 */
ACL_API const char *acl_split_nameval2(char *buf, char **name, char **value, char sep);

/**
 * Same as acl_split_nameval2, only separator is fixed as '='
 * @param buf {char*}
 * @param name {char**}
 * @param value {char**}
 * @return {const char*}
 */
ACL_API const char *acl_split_nameval(char *buf, char **name, char **value);

#ifdef  __cplusplus
}
#endif

#endif
