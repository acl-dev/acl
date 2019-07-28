
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
#include <assert.h>

/* Utility library. */
#include "mysql.h"

/* Global library. */

/* Application-specific. */

static void test_batch_insert(MYSQL *mysql)
{
#if 0
	const char *sql[] = {
		"INSERT IGNORE INTO address_book_group_00(owner, name) "
			"values('xxx@ikmail.com' , 'xxx1');",
		"INSERT IGNORE INTO address_book_group_00 "
			"SET  owner='xxx@ikmail.com' , name='xxx2';",
		"INSERT IGNORE INTO address_book_group_00 "
			"SET  owner='xxx@ikmail.com' , name='xxx3'",
		"INSERT IGNORE INTO address_book_group_00 "
			"SET  owner='xxx@ikmail.com' , name='xxx4';" };

	printf("sql0: %s\n", sql[0]);
	printf("sql1: %s\n", sql[1]);
	printf("sql2: %s\n", sql[2]);
	printf("sql3: %s\n", sql[3]);
	assert(mysql_query(mysql, sql[0]) == 0);
	assert(mysql_query(mysql, sql[1]) == 0);
	assert(mysql_query(mysql, sql[2]) == 0);
	assert(mysql_query(mysql, sql[3]) == 0);
#else
	const char *sql =
		"INSERT IGNORE INTO address_book_group_00(owner, name) "
			"values('xxx@ikmail.com' , 'xxx1'); "
		"INSERT IGNORE INTO address_book_group_00 "
			"SET  owner='xxx@ikmail.com' , name='xxx2'; "
		"INSERT IGNORE INTO address_book_group_00 "
			"SET  owner='xxx@ikmail.com' , name='xxx3'; "
		"INSERT IGNORE INTO address_book_group_00 "
			"SET  owner='xxx@ikmail.com' , name='xxx4';";

	printf("sql: %s\n", sql);
	assert(mysql_query(mysql, sql) == 0);

#endif
}


int main(void)
{
	MYSQL *mysql;
	int   reconnect = 1;

	mysql_library_init(0, NULL, NULL);

	mysql = mysql_init(NULL);
	assert(mysql);
	mysql_options(mysql, MYSQL_OPT_RECONNECT, (const void*) &reconnect);
	assert(mysql_real_connect(mysql, "127.0.0.1", "root", "", "ikmail", 3306, 0, CLIENT_MULTI_STATEMENTS));
	mysql_autocommit(mysql, 1);

	test_batch_insert(mysql);

	mysql_library_end();
	return 0;
}
