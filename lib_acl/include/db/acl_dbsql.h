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
 * 数据库查询语句，根据用户输入的 select SQL 语句从数据库里查询结果
 * @param handle {ACL_DB_HANDLE*} 数据库连接句柄，不能为空
 * @param sql {const char*} select 查询语句，不能为空
 * @param error {int*} 如果返回值 NULL 且该变量非空指针则该指针的地址被
 *  赋予出错的错误号，错误号参见 acl_dberr.h
 * @return {ACL_SQL_RES*} 查询结果集，如果查询失败或查询结果为空，则返回
 *  NULL，否则返回 ACL_SQL_RES 对象(用完后该结果需要调用 acl_dbsql_free_result
 *  释放)，示例：
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
 * 释放由 acl_dbsql_select 返回的结果集对象
 * @param handle {ACL_DB_HANDLE*} 数据库连接句柄，不能为空
 * @param res {ACL_SQL_RES*} acl_dbsql_select 返回的结果对象，不能为空
 */
ACL_API void acl_dbsql_free_result(ACL_DB_HANDLE *handle, ACL_SQL_RES *res);

/**
 * 以回调的方式查询数据库中所有符合条件的结果集，查询结果集合通过用户设置
 * 的回调函数返回给用户
 * @param handle {ACL_DB_HANDLE*} 数据库连接句柄，不能为空
 * @param sql {const char*} select 查询语句，不能为空
 * @param error {int*} 如果返回值 -1 且该变量非空指针则该指针的地址被
 *  赋予出错的错误号，错误号参见 acl_dberr.h
 * @param walk_fn {int (*)(const void**, void*)}，用户设置的查询结果回调函数，
 *  非空，每查一条符合条件的结果都回调用该回调函数，如果查询结果为多条，则会
 *  自动回调多次该回调函数，其中的 result_row 是一个数组指针，用户可以在自己
 *  的回调函数里用 result_row[i] 来取得自己所要求的数据列(必须与 select 语句
 *  中的相匹配)
 * @param arg {void*} 用户自定义的参数，该参数会自动传递给 walk_fn 回调函数，
 *  作为 walk_fn 的最后一个参数返回
 * @return {int} 查询结果总数，如果返回 -1 则表示查询语句失败，0 表示没有符合
 *  查询条件的结果，> 0 表示查询的结果总数
 */
ACL_API int acl_dbsql_results(ACL_DB_HANDLE *handle, const char *sql, int *error,
	int (*walk_fn)(const void** result_row, void *arg), void *arg);

/**
 * 以回调的方式从数据库中查询一条记录，查询结果通过用户设置的回调函数返回给用户
 * @param handle {ACL_DB_HANDLE*} 数据库连接句柄，不能为空
 * @param sql {const char*} select 查询语句，不能为空
 * @param error {int*} 如果返回值 -1 且该变量非空指针则该指针的地址被
 *  赋予出错的错误号，错误号参见 acl_dberr.h
 * @param walk_fn {int (*)(const void**, void*)}，用户设置的查询结果回调函数，
 *  非空，当查到一条符合条件的结果时便回调用该回调函数，与 acl_dbsql_results 不
 *  同，该回调函数最多只会被调用一次，其中的 result_row 是一个数组指针，用户可
 *  以在自己的回调函数里用 result_row[i] 来取得自己所要求的数据列(必须与 select
 *  语句中的相匹配)
 * @param arg {void*} 用户自定义的参数，该参数会自动传递给 walk_fn 回调函数，
 *  作为 walk_fn 的最后一个参数返回
 * @return {int} 返回值只有三种状态，-1 表示查询语句失败，0 表示查询结果为空，
 *  1 表示查到一个结果；如果返回 -1 则 *error 记录着失败原因，参见 acl_dberr.h
 */
ACL_API int acl_dbsql_result(ACL_DB_HANDLE *handle, const char *sql, int *error,
	int (*walk_fn)(const void** result_row, void *arg), void *arg);

/**
 * 更新数据库数据，update, insert, delete 等修改数据库的操作可以使用该函数
 * @param handle {ACL_DB_HANDLE*} 数据库连接句柄，不能为空
 * @param sql {const char*} 数据库修改语句，不能为空
 * @param error {int*} 如果返回值 -1 且该变量非空指针则该指针的地址被
 *  赋予出错的错误号，错误号参见 acl_dberr.h
 * @return {int} 返回值只有三种状态，-1 表示失败(如果 error 指针非空则其中记录着
 *  出错原因，错误号参见: acl_dberr.h)，0 表示更新成功，但并未影响数据库原始信息
 *  (原因是本信息与数据库原信息相同)，> 0 表示更新了数据库中数据存储条数
 */
ACL_API int acl_dbsql_update(ACL_DB_HANDLE *handle, const char *sql, int *error);

#ifdef __cplusplus
}
#endif

#endif /* ACL_CLIENT_ONLY */
#endif

