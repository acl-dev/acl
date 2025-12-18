#ifndef __ACL_MASTER_FLOW_INCLUDED_H__
#define __ACL_MASTER_FLOW_INCLUDED_H__

#include "../stdlib/acl_define.h"

#ifndef ACL_CLIENT_ONLY

#ifdef	__cplusplus
extern "C" {
#endif

#ifdef ACL_UNIX

 /*
  * Functional interface.
  */
extern int acl_var_master_flow_pipe[2];

extern void acl_master_flow_init(void);
extern int acl_master_flow_get(int);
extern int acl_master_flow_put(int);
extern int acl_master_flow_count(void);

#endif /* ACL_UNIX */

#ifdef	__cplusplus
}

#endif /* ACL_CLIENT_ONLY */
#endif

#endif
