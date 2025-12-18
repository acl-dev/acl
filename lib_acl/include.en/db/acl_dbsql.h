#ifndef ACL_DBSQL_INCLUDE_H
#define ACL_DBSQL_INCLUDE_H

#ifndef ACL_CLIENT_ONLY

#ifdef __cplusplus
extern "C" {
#endif

#include "../stdlib/acl_define.h"
#include "acl_dbpool.h"

#ifndef	ACL_DB_ATOU
#define ACL_DB_ATOU(_str_) (_str_ ? strtoul(_str_, (char **) NULL, 10) : 0)
#endif

/**
 * Database query function, used to execute select SQL
 * statements and query the database.
 * @param handle {ACL_DB_HANDLE*} database connection handle, must not be NULL
 * @param sql {const char*} select query statement, must not be NULL
 * @param error {int*} error code, if NULL and this parameter
 *  is a non-NULL pointer, the address pointed to will store
 *  the error code, error codes refer to acl_dberr.h
 * @return {ACL_SQL_RES*} query result set, if query fails or
 *  query result is empty, returns NULL, otherwise returns
 *  ACL_SQL_RES object (after using the result, you need to
 *  call acl_dbsql_free_result to free) to indicate success.
 *
 *  ACL_DB_HANDLE* handle = ...;
 *  ACL_SQL_RES* res = acl_dbsql_select(...);
 *  ACL_ITER iter;
 *  if (res)
 *  {
 *    acl_foreach(iter, res)
 *    {
 *      const char **my_row = (const char**) iter.data;
 *      printf("first item: %s\n", my_row[0]);
 *      ...
 *    }
 *    acl_dbsql_free_result(handle, res);
 *  }
 *
 */
ACL_API ACL_SQL_RES *acl_dbsql_select(ACL_DB_HANDLE *handle,
	const char *sql, int *error);

/**
 * Free the result set returned by acl_dbsql_select.
 * @param handle {ACL_DB_HANDLE*} database connection handle, must not be NULL
 * @param res {ACL_SQL_RES*} result set returned by
 *  acl_dbsql_select, must not be NULL
 */
ACL_API void acl_dbsql_free_result(ACL_DB_HANDLE *handle, ACL_SQL_RES *res);

/**
 * Query the database in a callback manner, and return all
 * matching query results through the user-defined callback
 * function.
 * @param handle {ACL_DB_HANDLE*} database connection handle, must not be NULL
 * @param sql {const char*} select query statement, must not be NULL
 * @param error {int*} error code, if -1 and this parameter
 *  is a non-NULL pointer, the address pointed to will store
 *  the error code, error codes refer to acl_dberr.h
 * @param walk_fn {int (*)(const void**, void*)} user-defined
 *  query result callback function, must not be NULL, each
 *  time a matching result row is found, this callback
 *  function will be called. If the query result is multiple
 *  rows, it will automatically call this callback function
 *  multiple times. The result_row in the callback is an
 *  array pointer, users can access their own callback
 *  function through result_row[i] to get the data they
 *  need (corresponding to the columns in the select
 *  statement in order)
 * @param arg {void*} user-defined parameter, this parameter
 *  will be automatically passed to the walk_fn callback
 *  function as the second parameter of walk_fn.
 * @return {int} query result count, -1 indicates query
 *  operation failed, 0 indicates no matching query result,
 *  > 0 indicates the number of query results.
 */
ACL_API int acl_dbsql_results(ACL_DB_HANDLE *handle, const char *sql, int *error,
	int (*walk_fn)(const void** result_row, void *arg), void *arg);

/**
 * Query a record from the database in a callback manner, and
 * return the query result through the user-defined callback
 * function.
 * @param handle {ACL_DB_HANDLE*} database connection handle, must not be NULL
 * @param sql {const char*} select query statement, must not be NULL
 * @param error {int*} error code, if -1 and this parameter
 *  is a non-NULL pointer, the address pointed to will store
 *  the error code, error codes refer to acl_dberr.h
 * @param walk_fn {int (*)(const void**, void*)} user-defined
 *  query result callback function, must not be NULL, when a
 *  matching result is found, this callback function will be
 *  called. Unlike acl_dbsql_results, this callback function
 *  will only be called once. The result_row in the callback
 *  is an array pointer, users can access their own callback
 *  function through result_row[i] to get the data they need
 *  (corresponding to the columns in the select statement in
 *  order)
 * @param arg {void*} user-defined parameter, this parameter
 *  will be automatically passed to the walk_fn callback
 *  function as the second parameter of walk_fn.
 * @return {int} return value only indicates status: -1
 *  indicates query operation failed, 0 indicates query
 *  result is empty, 1 indicates a matching result was
 *  found. If -1, *error records the failure reason, refer
 *  to acl_dberr.h
 */
ACL_API int acl_dbsql_result(ACL_DB_HANDLE *handle, const char *sql, int *error,
	int (*walk_fn)(const void** result_row, void *arg), void *arg);

/**
 * Modify database data, update, insert, delete and other operations that modify
 * the database use this function.
 * @param handle {ACL_DB_HANDLE*} database connection handle, must not be NULL
 * @param sql {const char*} database modification statement, must not be NULL
 * @param error {int*} error code, if -1 and this parameter
 *  is a non-NULL pointer, the address pointed to will store
 *  the error code, error codes refer to acl_dberr.h
 * @return {int} return value only indicates status: -1 indicates failure
 *  (if error pointer is non-NULL, it will record the failure reason, error codes
 *  refer to: acl_dberr.h), 0 indicates modification succeeded but did not affect
 *  the database original information (the reason is that the modified information
 *  is the same as the database original information), > 0 indicates the number
 *  of rows affected by the database modification
 */
ACL_API int acl_dbsql_update(ACL_DB_HANDLE *handle, const char *sql, int *error);

#ifdef __cplusplus
}
#endif

#endif /* ACL_CLIENT_ONLY */
#endif

