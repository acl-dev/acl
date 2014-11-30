
/* 
 * Copyright (C) 2010 51IKER
 * All rights reserved.
 *
 * AUTHOR(S)
 *	Xiaohua Jia
 *	Beijing, RPC 100006
 *	E-mail: jiaxiaohua@51iker.com
 */

/* System library. */
#include <stdio.h>
#include <stdlib.h>

/* Utility library. */
#include "mysql.h"
#include "lib_acl.h"

/* Global library. */

/* Application-specific. */


void test_select(ACL_DB_HANDLE *db_handle);
void test_select(ACL_DB_HANDLE *db_handle)
{
	int error = 0;
	unsigned int i = 0;
	unsigned int num_fields = 0;

	ACL_ITER iter;
	ACL_SQL_RES *res = NULL;	

	MYSQL *mysql = NULL;
	MYSQL_RES *my_res= NULL;
	MYSQL_FIELD *fields = NULL;

	const char **row = NULL;
	ACL_VSTRING *xml_datas = NULL;

	res = acl_dbsql_select(db_handle, 
		"SELECT * FROM filter_rules WHERE frid=\"4\"", &error);
	mysql = (MYSQL *)acl_dbpool_export(db_handle);
	printf("error=%d; mysql errno=%d; "
			"error=%s;", error,
			mysql_errno(mysql), mysql_error(mysql));

	xml_datas = acl_vstring_alloc(64);
	my_res = (MYSQL_RES*)res->res;


	num_fields = mysql_num_fields(my_res);
	fields = mysql_fetch_fields(my_res);

	printf("res=%p; num=%d; fel=%p; ss=%lu; sf=%lu\n", 
	(void *)my_res, num_fields, (void *)fields, sizeof(*my_res), sizeof(*fields));

	acl_foreach(iter, res) {
		row = (const char**) iter.data;

		acl_vstring_sprintf_append(xml_datas, "<data ");


		for(i = 0; i < num_fields; i++)	{
			printf("Field %u is %s\n", i, fields[i].name);
			acl_vstring_sprintf_append(xml_datas, 
				" %s=\"%s\"", fields[i].name, row[i]);
		}
	}

	printf("%s\n", acl_vstring_str(xml_datas));
}


static void test_batch_insert(ACL_DB_HANDLE *db_handle)
{
	int ret = 0;
	int error = 0;

	const char *sql = "START TRANSACTION; "
		"INSERT IGNORE INTO address_book_group_00 "
		"SET  owner=\'xxx@ikmail.com\' , name=\'xxx1\'; "
		"INSERT IGNORE INTO address_book_group_00 "
		"SET  owner=\'xxx@ikmail.com\' , name=\'xxx2\'; "
		"INSERT IGNORE INTO address_book_group_00 "
		"SET  owner=\'xxx@ikmail.com\' , name=\'xxx3\'; "
		"INSERT IGNORE INTO address_book_group_00 "
		"SET  owner=\'xxx@ikmail.com\' , name=\'xxx4\';   COMMIT;";

	ret = acl_dbsql_update(db_handle, sql, &error);	

	printf("ret=%d; error=%d; sql=%s\n", ret, error, sql);
}


int main(int argc acl_unused, char *argv[] acl_unused)
{

	ACL_DB_INFO dbi;
	ACL_DB_HANDLE *db_handle = NULL;
	ACL_DB_POOL *db_pool = NULL;
	

	mysql_library_init(0, NULL, NULL);

	memset(&dbi, 0, sizeof(dbi));

	dbi.auto_commit = 0;
	dbi.buf_size = 0;
	dbi.conn_timeout = 6;
	dbi.db_max = 20;
	dbi.debug_flag = 0;
	dbi.ping_inter = 3;
	dbi.rw_timeout = 3;
	dbi.timeout_inter = 3;
	dbi.db_flags = CLIENT_MULTI_STATEMENTS ;

	strncpy(dbi.db_addr, "127.0.0.1:3306", sizeof(dbi.db_addr) - 1);
	strncpy(dbi.db_name, "ikmail", sizeof(dbi.db_name) - 1);
	strncpy(dbi.db_user, "root", sizeof(dbi.db_user) - 1);
	strncpy(dbi.db_pass, "", sizeof(dbi.db_pass) - 1);

	db_pool = acl_dbpool_create("mysql", &dbi);

	db_handle = acl_dbpool_peek(db_pool);

	test_batch_insert(db_handle);

	acl_dbpool_destroy(db_pool);
	mysql_library_end();
	return 0;
}
