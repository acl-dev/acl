#ifndef	ACL_OPEN_LOCK_INCLUDE_H
#define	ACL_OPEN_LOCK_INCLUDE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "../acl_define.h"

#ifdef	ACL_UNIX
 /*
  * System library.
  */
#include <fcntl.h>

 /*
  * Utility library.
  */
#include "../acl_vstream.h"
#include "../acl_vstring.h"

 /*
  * External interface.
  */
extern ACL_VSTREAM *acl_open_lock(const char *, int, int, ACL_VSTRING *);

#endif /* ACL_UNIX */

#ifdef	__cplusplus
}
#endif

#endif

