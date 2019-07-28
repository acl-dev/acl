#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "db/acl_dbsql.h"
#include "stdlib/acl_stdlib.h"
#include "db/acl_dbsql.h"

#endif

#ifndef ACL_CLIENT_ONLY

ACL_SQL_RES *acl_dbsql_select(ACL_DB_HANDLE *handle, const char *sql, int *error)
{
	const char *myname = "acl_dbsql_select";

	if (handle == NULL || sql == NULL || *sql == 0)
		acl_msg_fatal("%s(%d): input invalid", myname, __LINE__);

	if (handle->sql_select == NULL)
		acl_msg_fatal("%s(%d): sql_select null", myname, __LINE__);

	return (handle->sql_select(handle, sql, error));
}

void acl_dbsql_free_result(ACL_DB_HANDLE *handle, ACL_SQL_RES *res)
{
	const char *myname = "acl_dbsql_free_result";

	if (handle == NULL || res == NULL) {
		acl_msg_error("%s(%d): input invalid", myname, __LINE__);
		return;
	}
	if (handle->free_result == NULL)
		acl_msg_fatal("%s(%d): free_result null", myname, __LINE__);
	handle->free_result(res);
}

/* 用于有返回结果集的查询 */

int acl_dbsql_results(ACL_DB_HANDLE *handle, const char *sql, int  *error,
	int (*walk_fn)(const void** result_row, void *arg), void *arg)
{       
	const char *myname = "acl_dbsql_results";
	int   n, err;

	if (handle == NULL || sql == NULL || *sql == 0 || walk_fn == NULL)
		acl_msg_fatal("%s(%d): input invalid", myname, __LINE__);

	if (handle->sql_results == NULL)
		acl_msg_fatal("%s(%d): sql_results null", myname, __LINE__);

	n = handle->sql_results(handle, sql, &err, walk_fn, arg);
	if (error)
		*error = err;

	return (n);
}

/* 用于仅查询一个结果的情况 */

int acl_dbsql_result(ACL_DB_HANDLE *handle, const char *sql, int  *error,
	int (*walk_fn)(const void** result_row, void *arg), void *arg)
{
	const char *myname = "acl_dbsql_result";
	int   n, err;

	if (handle == NULL || sql == NULL || *sql == 0 || walk_fn == NULL)
		acl_msg_fatal("%s(%d): input invalid", myname, __LINE__);

	if (handle->sql_results == NULL)
		acl_msg_fatal("%s(%d): sql_result null", myname, __LINE__);

	n = handle->sql_result(handle, sql, &err, walk_fn, arg);
	if (error)
		*error = err;

	return (n);
}

int acl_dbsql_update(ACL_DB_HANDLE *handle, const char *sql, int  *error)
{
	const char *myname = "acl_dbsql_update";
	int   n, err;

	if (handle == NULL || sql == NULL || *sql == 0)
		acl_msg_fatal("%s(%d): input invalid", myname, __LINE__);

	if (handle->sql_update == NULL)
		acl_msg_fatal("%s(%d): sql_update null", myname, __LINE__);

	n = handle->sql_update(handle, sql, &err);
	if (error)
		*error = err;

	return (n);
}

#endif /* ACL_CLIENT_ONLY */
