#ifndef __ACL_TIMED_WAIT_H_INCLUDED__
#define __ACL_TIMED_WAIT_H_INCLUDED__

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

