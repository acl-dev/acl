#ifndef	__MASTER_LOG_INCLUDE_H__
#define	__MASTER_LOG_INCLUDE_H__

#include "stdlib/acl_define.h"

#ifndef ACL_CLIENT_ONLY

#ifdef	__cplusplus
extern "C" {
#endif
void master_log_open(const char *procname);
void master_log_close(void);

#ifdef	__cplusplus
}
#endif

#endif /* ACL_CLIENT_ONLY */
#endif
