#ifndef ACL_WATCHDOG_INCLUDE_H
#define ACL_WATCHDOG_INCLUDE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "../acl_define.h"

#ifdef ACL_UNIX

 /*
  * External interface.
  */
typedef struct ACL_WATCHDOG ACL_WATCHDOG;
typedef void (*ACL_WATCHDOG_FN) (ACL_WATCHDOG *, char *);
extern ACL_WATCHDOG *acl_watchdog_create(unsigned, ACL_WATCHDOG_FN, char *);
extern void acl_watchdog_start(ACL_WATCHDOG *);
extern void acl_watchdog_stop(ACL_WATCHDOG *);
extern void acl_watchdog_destroy(ACL_WATCHDOG *);
extern void acl_watchdog_pat(void);

#endif /* ACL_UNIX*/

#ifdef	__cplusplus
}
#endif

#endif

