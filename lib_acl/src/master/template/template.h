#ifndef	__SERVER_TEMPLATE_INCLUDE_H__
#define	__SERVER_TEMPLATE_INCLUDE_H__

#ifdef	__cplusplus
extern "C" {
#endif

#include "stdlib/acl_define.h"

#ifdef	ACL_UNIX

void master_log_close(void);
void set_core_limit(void);

#endif

#ifdef	__cplusplus
}
#endif

#endif
