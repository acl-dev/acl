#ifndef	ACL_SANE_BASENAME_INCLUDE_H
#define	ACL_SANE_BASENAME_INCLUDE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "acl_vstring.h"

/**
 * Extract file name from full file path.
 * @parm bp {ACL_VSTRING*} Storage buffer, if NULL, uses internal thread-local storage
 * @return {char*} Must not be NULL
 */
ACL_API char *acl_sane_basename(ACL_VSTRING *bp, const char *path);

/**
 * Extract file directory from full file path.
 * @parm bp {ACL_VSTRING*} Storage buffer, if NULL, uses internal thread-local storage
 * @return {char*} Must not be NULL
 */
ACL_API char *acl_sane_dirname(ACL_VSTRING *bp, const char *path);

#ifdef	__cplusplus
}
#endif

#endif
