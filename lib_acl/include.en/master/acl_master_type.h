#ifndef	ACL_MASTER_TYPE_INCLUDE_H
#define	ACL_MASTER_TYPE_INCLUDE_H

#include "../stdlib/acl_xinetd_cfg.h"

#ifndef ACL_CLIENT_ONLY

#ifdef	__cplusplus
extern "C" {
#endif

#define	ACL_CONFIG_INT_TABLE	ACL_CFG_INT_TABLE
#define	ACL_CONFIG_INT64_TABLE	ACL_CFG_INT64_TABLE
#define	ACL_CONFIG_STR_TABLE	ACL_CFG_STR_TABLE
#define	ACL_CONFIG_BOOL_TABLE	ACL_CFG_BOOL_TABLE

#ifdef	__cplusplus
}
#endif

#endif /* ACL_CLIENT_ONLY */
#endif
