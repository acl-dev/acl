#ifndef	ACL_VSTRING_VSTREAM_INCLUDE_H
#define	ACL_VSTRING_VSTREAM_INCLUDE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "acl_define.h"
#include "acl_vstring.h"
#include "acl_vstream.h"

/**
 * 从数据流中读一行数据，直至读到一行、出错，或读完为止
 * @param vp {ACL_VSTRING*} 存储结果的缓存区
 * @param fp {ACL_VSTREAM*} 数据流
 * @return {int} 最后一个字符的ASCII，或 ACL_VSTREAM_EOF
 */
ACL_API int acl_vstring_gets(ACL_VSTRING *vp, ACL_VSTREAM *fp);

/**
 * 从数据流中读一行数据，直至读到一行、出错，或读完为止, 读的数据不包含
 * "\r\n" 或 "\n"
 * @param vp {ACL_VSTRING*} 存储结果的缓存区
 * @param fp {ACL_VSTREAM*} 数据流
 * @return {int} 最后一个字符的ASCII，或 ACL_VSTREAM_EOF
 */
ACL_API int acl_vstring_gets_nonl(ACL_VSTRING *vp, ACL_VSTREAM *fp);

/**
 * 从数据流中读到 "\0" 结尾的数据或出错为止, 但不包含 "\0"
 * @param vp {ACL_VSTRING*} 存储结果的缓存区
 * @param fp {ACL_VSTREAM*} 数据流
 * @return {int} 最后一个字符的ASCII，或 ACL_VSTREAM_EOF
 */
ACL_API int acl_vstring_gets_null(ACL_VSTRING *vp, ACL_VSTREAM *fp);

/**
 * 从数据流中读一行数据，但读的数据长度不得超过限定值
 * @param vp {ACL_VSTRING*} 存储结果的缓存区
 * @param fp {ACL_VSTREAM*} 数据流
 * @param bound {ssize_t} 所读数据最大长度限制
 * @return {int} 最后一个字符的ASCII，或 ACL_VSTREAM_EOF
 */
ACL_API int acl_vstring_gets_bound(ACL_VSTRING *vp, ACL_VSTREAM *fp, ssize_t bound);

/**
 * 从数据流中读一行数据，但读的数据长度不得超过限定值, 且结果中不包含 "\n" 或 "\r\n"
 * @param vp {ACL_VSTRING*} 存储结果的缓存区
 * @param fp {ACL_VSTREAM*} 数据流
 * @param bound {ssize_t} 所读数据最大长度限制
 * @return {int} 最后一个字符的ASCII，或 ACL_VSTREAM_EOF
 */
ACL_API int acl_vstring_gets_nonl_bound(ACL_VSTRING *vp, ACL_VSTREAM *fp, ssize_t bound);

/**
 * 从数据流中读到 "\0" 结尾的数据或出错或超过最大长度限制为止，结果中不包含 "\0"
 * @param vp {ACL_VSTRING*} 存储结果的缓存区
 * @param fp {ACL_VSTREAM*} 数据流
 * @param bound {ssize_t} 所读数据最大长度限制
 * @return {int} 最后一个字符的ASCII，或 ACL_VSTREAM_EOF
 */
ACL_API int acl_vstring_gets_null_bound(ACL_VSTRING *vp, ACL_VSTREAM *fp, ssize_t bound);

/**
 * Backwards compatibility for code that still uses the vstring_fgets()
 * interface. Unfortunately we can't change the macro name to upper case.
 */

#define acl_vstring_fgets(s, p) \
	(acl_vstring_gets((s), (p)) == ACL_VSTREAM_EOF ? 0 : (s))
#define acl_vstring_fgets_nonl(s, p) \
	(acl_vstring_gets_nonl((s), (p)) == ACL_VSTREAM_EOF ? 0 : (s))
#define acl_vstring_fgets_null(s, p) \
	(acl_vstring_gets_null((s), (p)) == ACL_VSTREAM_EOF ? 0 : (s))
#define acl_vstring_fgets_bound(s, p, l) \
	(acl_vstring_gets_bound((s), (p), (l)) == ACL_VSTREAM_EOF ? 0 : (s))
#define acl_vstring_fgets_nonl_bound(s, p, l) \
	(acl_vstring_gets_nonl_bound((s), (p), (l)) == ACL_VSTREAM_EOF ? 0 : (s))

#ifdef	__cplusplus
}
#endif

#endif
