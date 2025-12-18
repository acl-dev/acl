#ifndef	ACL_READLINE_INCLUDE_H
#define	ACL_READLINE_INCLUDE_H

#include "acl_vstream.h"
#include "acl_vstring.h"

#ifdef	__cplusplus
extern "C" {
#endif

/**
 * Read a logical line from a stream. Lines have continuation properties: if the last
 * non-whitespace character of a line is "\", the line is also a continuation line.
 * A logical line starts with a non-whitespace, non-"#" character. All subsequent
 * lines starting with whitespace or TAB are part of this logical line.
 * @param buf {ACL_VSTRING*} Storage buffer, must not be NULL
 * @param fp {ACL_VSTREAM*} Stream object, must not be NULL
 * @param lineno {int} If not NULL, records the actual line number in the logical line
 * @return {ACL_VSTRING*} If no logical line is read, returns NULL; otherwise returns
 *  the same address as buf
 */
ACL_API ACL_VSTRING *acl_readlline(ACL_VSTRING *buf, ACL_VSTREAM *fp, int *lineno);

#ifdef	__cplusplus
}
#endif

#endif
