#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#endif

#ifndef ACL_CLIENT_ONLY

#ifdef HAS_MYSQL

#include "mysql.h"
#include "errmsg.h"
#include "mysqld_error.h"

#include "stdlib/acl_stdlib.h"
#include "db/acl_dberr.h"
#include "acl_dbpool_mysql.h"
#include "acl_dbmysql.h"

static const void *dbmysql_iter_head(ACL_ITER *iter, struct ACL_SQL_RES *res)
{
	MYSQL_RES *my_res = (MYSQL_RES*) res->res;
	MYSQL_ROW  my_row;

	iter->dlen = -1;
	iter->key = NULL;
	iter->klen = -1;
	iter->i = 0;
	iter->size = res->num;

	my_row = mysql_fetch_row(my_res);
	if (my_row == NULL) {
		iter->data = iter->ptr = NULL;
		return (NULL);
	}

	iter->data = iter->ptr = (void*) my_row;
	return (iter->ptr);
}

static const void *dbmysql_iter_next(ACL_ITER *iter, struct ACL_SQL_RES *res)
{
	MYSQL_RES *my_res = (MYSQL_RES*) res->res;
	MYSQL_ROW  my_row;

	my_row = mysql_fetch_row(my_res);
	if (my_row == NULL) {
		iter->data = iter->ptr = NULL;
		return (NULL);
	}

	iter->i++;
	iter->data = iter->ptr = (void*) my_row;
	return (iter->ptr);
}

static MYSQL *sane_mysql_query(ACL_DB_HANDLE *handle, const char *sql)
{
	const char *myname = "acl_mysql_query";
	MYSQL *myconn;
	int   errnum;

	myconn = acl_dbpool_export(handle);
	if (myconn == NULL)
		acl_msg_fatal("%s(%d): null mysql handle", myname, __LINE__);
	if (mysql_query(myconn, sql) == 0)
		return (myconn);

	/* 重新打开MYSQL连接进行重试 */

	errnum = mysql_errno(myconn);
	if (errnum != CR_SERVER_LOST && errnum != CR_SERVER_GONE_ERROR) {
		acl_msg_error("%s(%d): sql(%s) error(%s)",
			myname, __LINE__, sql, mysql_error(myconn));
		return (NULL);
	}
	if (sane_mysql_reopen(handle) < 0)
		return (NULL);

	myconn = acl_dbpool_export(handle);
	if (myconn == NULL)
		acl_msg_fatal("%s(%d): null mysql handle", myname, __LINE__);
	if (mysql_query(myconn, sql) == 0)
		return (myconn);

	acl_msg_error("%s(%d): sql(%s) error(%s)",
		myname, __LINE__, sql, mysql_error(myconn));
	return (NULL);
}

ACL_SQL_RES *acl_dbmysql_select(ACL_DB_HANDLE *handle, const char *sql, int *error)
{
	const char *myname = "acl_dbmysql_select";
	ACL_SQL_RES *res;
	MYSQL *myconn;
	MYSQL_RES *my_res;
	int    n;

	if (handle == NULL || sql == NULL || *sql == 0)
		acl_msg_fatal("%s(%d): input invalid", myname, __LINE__);

	if (error)
		*error = ACL_DB_OK;

	if ((myconn = sane_mysql_query(handle, sql)) == NULL) {
		if (error)
			*error = ACL_DB_ERR_SELECT;
		acl_msg_error("%s(%d): mysql_query error(%s), sql(%s)",
			myname, __LINE__, mysql_error(myconn), sql);
		return (NULL);
	}

	my_res = mysql_store_result(myconn);

	if (my_res == NULL) {
		if (mysql_errno(myconn) == 0) {
			if (error)
				*error = ACL_DB_ERR_EMPTY;
			return (NULL);
		}
		if (error)
			*error = ACL_DB_ERR_STORE;
		acl_msg_error("%s(%d): mysql_store_result error(%s), sql(%s)",
			myname, __LINE__, mysql_error(myconn), sql);
		return (NULL);
	}

	if ((n = mysql_num_rows(my_res)) <= 0) {
		if (error)
			*error = ACL_DB_ERR_EMPTY;
		mysql_free_result(my_res);
		return (NULL);
	}

	res = (ACL_SQL_RES*) acl_mymalloc(sizeof(ACL_SQL_RES));
	if (res == NULL) {
		if (error)
			*error = ACL_DB_ERR_ALLOC;
		acl_msg_error("%s(%d): malloc error", myname, __LINE__);
		mysql_free_result(my_res);
		return (NULL);
	}

	res->res = (void*) my_res;
	res->num = n;
	res->iter_head = dbmysql_iter_head;
	res->iter_next = dbmysql_iter_next;
	return (res);
}

void acl_dbmysql_free_result(ACL_SQL_RES *res)
{
	MYSQL_RES *my_res = (MYSQL_RES*) res->res;

	if (my_res)
		mysql_free_result(my_res);
	acl_myfree(res);
}

/* 用于有返回结果集的查询 */

int acl_dbmysql_results(ACL_DB_HANDLE *handle, const char *sql, int  *error,
	int (*walk_fn)(const void** my_row, void *arg), void *arg)
{       
	const char *myname = "acl_dbmysql_results";
	MYSQL *myconn;
	MYSQL_RES *my_res;
	MYSQL_ROW  my_row;
	int   n;

	if (handle == NULL || sql == NULL || *sql == 0 || walk_fn == NULL)
		acl_msg_fatal("%s(%d): input invalid", myname, __LINE__);

	if (error)
		*error = ACL_DB_OK;

	if ((myconn = sane_mysql_query(handle, sql)) == NULL) {
		if (error)
			*error = ACL_DB_ERR_SELECT;
		acl_msg_error("%s(%d): mysql_query error(%s), sql(%s)",
			myname, __LINE__, mysql_error(myconn), sql);
		return (-1);
	}

	my_res = mysql_store_result(myconn);

	if (my_res == NULL) {
		if (mysql_errno(myconn) == 0) {
			if (error)
				*error = ACL_DB_ERR_EMPTY;
			return (0);
		}
		if (error)
			*error = ACL_DB_ERR_STORE;
		acl_msg_error("%s(%d): mysql_store_result error(%s), sql(%s)",
			myname, __LINE__, mysql_error(myconn), sql);
		return (-1);
	}

	my_row = mysql_fetch_row(my_res);
	if (my_row == NULL) {
		if (error)
			*error = ACL_DB_ERR_EMPTY;
		mysql_free_result(my_res);
		return (0);
	}

	if (walk_fn((const void**) my_row, arg) < 0) {
		if (error)
			*error = ACL_DB_ERR_CALLBACK;
		mysql_free_result(my_res);
		return (-1);
	}

	n = 1;

	while ((my_row = mysql_fetch_row(my_res)) != NULL) {
		if (walk_fn((const void**) my_row, arg) < 0) {
			if (error)
				*error = ACL_DB_ERR_CALLBACK;
			mysql_free_result(my_res);
			return (-1);
		}
		n++;
	}

	mysql_free_result(my_res);

	return (n);
}

/* 用于仅查询一个结果的情况 */

int acl_dbmysql_result(ACL_DB_HANDLE *handle, const char *sql, int  *error,
	int (*callback)(const void** my_row, void *arg), void *arg)
{
	const char *myname = "acl_dbmysql_result";
	MYSQL *myconn;
	MYSQL_RES *my_res;
	MYSQL_ROW  my_row;

	if (handle == NULL || sql == NULL || *sql == 0 || callback == NULL)
		acl_msg_fatal("%s(%d): input invalid", myname, __LINE__);

	if (error)
		*error = ACL_DB_OK;

	if ((myconn = sane_mysql_query(handle, sql)) == NULL) {
		if (error)
			*error = ACL_DB_ERR_SELECT;
		acl_msg_error("%s(%d): mysql_query error(%s), sql(%s)",
			 myname, __LINE__, mysql_error(myconn), sql);
		return (-1);
	}

	my_res = mysql_store_result(myconn);

	if (my_res == NULL) {
		if (mysql_errno(myconn) == 0) {
			if (error)
				*error = ACL_DB_ERR_EMPTY;
			return (0);
		}
		if (error)
			*error = ACL_DB_ERR_STORE;
		acl_msg_error("%s(%d): mysql_store_result error(%s), sql(%s)",
			myname, __LINE__, mysql_error(myconn), sql);
		return (-1);
	}

	my_row = mysql_fetch_row(my_res);
	if (my_row == NULL) {
		if (error)
			*error = ACL_DB_ERR_EMPTY;
		mysql_free_result(my_res);
		return (0);
	}

	if (callback((const void**) my_row, arg) < 0) {
		if (error)
			*error = ACL_DB_ERR_CALLBACK;
		mysql_free_result(my_res);
		return (-1);
	}

	mysql_free_result(my_res);

	return (1);
}

int acl_dbmysql_update(ACL_DB_HANDLE *handle, const char *sql, int  *error)
{
	const char *myname = "acl_dbmysql_update";
	MYSQL *myconn;
	int   ret;

	if (handle == NULL || sql == NULL || *sql == 0)
		acl_msg_fatal("%s(%d): input invalid", myname, __LINE__);

	if (error)
		*error = ACL_DB_OK;

	if ((myconn = sane_mysql_query(handle, sql)) == NULL) {
		if (error)
			*error = ACL_DB_ERR_UPDATE;
		acl_msg_error("%s(%d): mysql_query error(%s), sql(%s)",
			myname, __LINE__, mysql_error(myconn), sql);
		return (-1);
	}

	ret = (int) mysql_affected_rows(myconn);
	if (ret == (int) (-1)) { /* XXX */
		if (error)
			*error = ACL_DB_ERR_AFFECTED;
		acl_msg_error("%s(%d): mysql_affected_rows error(%s), sql(%s)",
			myname, __LINE__, mysql_error(myconn), sql);
		return (-1);
	}

	return (ret);
}

#endif /* HAS_MYSQL */
#endif /* ACL_CLIENT_ONLY */

