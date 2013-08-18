#ifndef _SAFE_OPEN_H_INCLUDED_
#define _SAFE_OPEN_H_INCLUDED_

#ifdef  __cplusplus
extern "C" {
#endif

#include "../acl_define.h"

#ifdef ACL_UNIX

 /*
  * System library.
  */
#include <sys/stat.h>
#include <fcntl.h>

 /*
  * Utility library.
  */
#include "../acl_vstream.h"
#include "../acl_vstring.h"

 /*
  * External interface.
  */
extern ACL_VSTREAM *acl_safe_open(const char *path, int flags, int mode,
	           	 struct stat * st, uid_t user, gid_t group, ACL_VSTRING *why);

#endif /* ACL_UNIX */

#ifdef  __cplusplus
}
#endif

#endif

