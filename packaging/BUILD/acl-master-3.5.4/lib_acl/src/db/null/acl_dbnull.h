#ifndef __ACL_DB_NULL_INCLUDE_H__
#define __ACL_DB_NULL_INCLUDE_H__
#include "stdlib/acl_define.h"
#include "db/acl_dbpool.h"

#ifndef ACL_CLIENT_ONLY

#ifdef __cplusplus
extern "C" {
#endif

int acl_dbnull_results(ACL_DB_HANDLE *handle, const char *sql, int  *error,
	int (*walk_fn)(const void** my_row, void *arg), void *arg);
int acl_dbnull_result(ACL_DB_HANDLE *handle, const char *sql, int  *error,
	int (*callback)(const void** my_row, void *arg), void *arg);
int acl_dbnull_update(ACL_DB_HANDLE *handle, const char *sql, int  *error);

#ifdef __cplusplus
}
#endif

#endif /* ACL_CLIENT_ONLY */
#endif

