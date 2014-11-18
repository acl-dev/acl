#ifndef ACL_TIMED_WAIT_INCLUDE_H
#define ACL_TIMED_WAIT_INCLUDE_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "../acl_define.h"
#ifdef ACL_UNIX
#include <sys/types.h>
#include <unistd.h>
 /*
  * External interface.
  */
extern int acl_timed_waitpid(pid_t, ACL_WAIT_STATUS_T *, int, int);

#endif /* ACL_UNIX */

#ifdef  __cplusplus
}
#endif

#endif

