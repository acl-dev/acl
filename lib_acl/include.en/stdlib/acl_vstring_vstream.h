#ifndef	ACL_VSTRING_VSTREAM_INCLUDE_H
#define	ACL_VSTRING_VSTREAM_INCLUDE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "acl_define.h"
#include "acl_vstring.h"
#include "acl_vstream.h"

/**
 * Read data from stream until encountering a newline or end of stream.
 * @param vp {ACL_VSTRING*} Storage buffer
 * @param fp {ACL_VSTREAM*} Stream object
 * @return {int} Last character's ASCII code or ACL_VSTREAM_EOF
 */
ACL_API int acl_vstring_gets(ACL_VSTRING *vp, ACL_VSTREAM *fp);

/**
 * Read data from stream until encountering a newline or end of stream,
 * read data does not include "\r\n" or "\n"
 * @param vp {ACL_VSTRING*} Storage buffer
 * @param fp {ACL_VSTREAM*} Stream object
 * @return {int} Last character's ASCII code or ACL_VSTREAM_EOF
 */
ACL_API int acl_vstring_gets_nonl(ACL_VSTRING *vp, ACL_VSTREAM *fp);

/**
 * Read data from stream until encountering "\0" terminated data or end of stream,
 * read data does not include "\0"
 * @param vp {ACL_VSTRING*} Storage buffer
 * @param fp {ACL_VSTREAM*} Stream object
 * @return {int} Last character's ASCII code or ACL_VSTREAM_EOF
 */
ACL_API int acl_vstring_gets_null(ACL_VSTRING *vp, ACL_VSTREAM *fp);

/**
 * Read data from stream, read data length cannot exceed specified limit.
 * @param vp {ACL_VSTRING*} Storage buffer
 * @param fp {ACL_VSTREAM*} Stream object
 * @param bound {ssize_t} Maximum length limit for read data
 * @return {int} Last character's ASCII code or ACL_VSTREAM_EOF
 */
ACL_API int acl_vstring_gets_bound(ACL_VSTRING *vp, ACL_VSTREAM *fp, ssize_t bound);

/**
 * Read data from stream, read data length cannot exceed specified limit,
 * and read data does not include "\n" or "\r\n"
 * @param vp {ACL_VSTRING*} Storage buffer
 * @param fp {ACL_VSTREAM*} Stream object
 * @param bound {ssize_t} Maximum length limit for read data
 * @return {int} Last character's ASCII code or ACL_VSTREAM_EOF
 */
ACL_API int acl_vstring_gets_nonl_bound(ACL_VSTRING *vp, ACL_VSTREAM *fp, ssize_t bound);

/**
 * Read data from stream until encountering "\0" terminated data or exceeding
 * maximum length limit, read data does not include "\0"
 * @param vp {ACL_VSTRING*} Storage buffer
 * @param fp {ACL_VSTREAM*} Stream object
 * @param bound {ssize_t} Maximum length limit for read data
 * @return {int} Last character's ASCII code or ACL_VSTREAM_EOF
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
