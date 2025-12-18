#ifndef ACL_SPLIT_AT_INCLUDE_H
#define ACL_SPLIT_AT_INCLUDE_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "acl_define.h"

/**
 * Search from the left of the string, find the specified delimiter,
 * and truncate to the right of the delimiter.
 * @param string {char*} Source string
 * @param delimiter {int} Delimiter
 * @return {char*} String to the right of the delimiter found, if NULL,
 *  indicates delimiter not found
 */
ACL_API char *acl_split_at(char *string, int delimiter);

/**
 * Search from the right of the string, find the specified delimiter,
 * and truncate to the right of the delimiter.
 * @param string {char*} Source string
 * @param delimiter {int} Delimiter
 * @return {char*} String to the right of the delimiter found, if NULL,
 *  indicates delimiter not found
 */
ACL_API char *acl_split_at_right(char *string, int delimiter);

#ifdef  __cplusplus
}
#endif

#endif
