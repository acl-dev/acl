#ifndef	ACL_MASTER_CONF_INCLUDE_H
#define	ACL_MASTER_CONF_INCLUDE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "stdlib/acl_define.h"
#include "master/acl_master_type.h"

ACL_API void acl_app_conf_load(const char *pathname);
ACL_API void acl_app_conf_unload(void);
ACL_API void acl_get_app_conf_int_table(ACL_CONFIG_INT_TABLE *table);
ACL_API void acl_get_app_conf_int64_table(ACL_CONFIG_INT64_TABLE *table);
ACL_API void acl_get_app_conf_str_table(ACL_CONFIG_STR_TABLE *table);
ACL_API void acl_get_app_conf_bool_table(ACL_CONFIG_BOOL_TABLE *table);
ACL_API void acl_free_app_conf_str_table(ACL_CONFIG_STR_TABLE *table);

#ifdef	__cplusplus
}
#endif

#endif
