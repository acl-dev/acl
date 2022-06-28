#include "lib_acl.h"
#include "mysql.h"
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

static ACL_DB_POOL *__db_pool = NULL;

static int db_before_connect(ACL_DB_HANDLE* db_handle, void *ctx acl_unused)
{
	const char *myname = "db_before_connect";
	MYSQL *mysql;

	printf("%p\n", (void*) db_handle);
	mysql = (MYSQL*) acl_dbpool_export(db_handle);
	if (mysql_options(mysql, MYSQL_SET_CHARSET_NAME, "GB2312")) {
		printf("%s: mysql_options error\n", myname);
		return (-1);
	}

	printf("%s: ok\n", myname);
	return (0);
}

static int db_after_connect(ACL_DB_HANDLE* db_handle, void *ctx acl_unused)
{
	const char *myname = "db_after_connect";

	(void) db_handle;
	printf("%s: ok\n", myname);
	return (0);
}

static void dbconn_init(const char *addr, const char *name, const char *usr,
	const char *pass, int dbpool_max, int dbping, int dbtimeout)
{
	const char *myname = "dbconn_init";
	ACL_DB_INFO db_info;

	memset(&db_info, 0, sizeof(ACL_DB_INFO));

	ACL_SAFE_STRNCPY(db_info.db_addr, addr, sizeof(db_info.db_addr));
	ACL_SAFE_STRNCPY(db_info.db_name, name, sizeof(db_info.db_name));
	ACL_SAFE_STRNCPY(db_info.db_user, usr, sizeof(db_info.db_user));
	ACL_SAFE_STRNCPY(db_info.db_pass, pass, sizeof(db_info.db_pass));

	db_info.db_max = dbpool_max;
	db_info.ping_inter = dbping;
	db_info.timeout_inter = dbtimeout;
	db_info.auto_commit = 1;

	db_info.db_before_connect = db_before_connect;
	db_info.db_after_connect = db_after_connect;

	__db_pool = acl_dbpool_create("mysql", &db_info);

	if (__db_pool == NULL)
		acl_msg_fatal("%s(%d): init db pool error", myname, __LINE__);

}

static ACL_DB_HANDLE *dbconn_get(void)
{
	const char *myname = "dbconn_get";
	ACL_DB_HANDLE *db_handle;

	if (__db_pool == NULL)
		acl_msg_fatal("%s(%d): db pool null", myname, __LINE__);

	db_handle = acl_dbpool_peek(__db_pool);
	if (db_handle == NULL) {
		acl_msg_error("%s(%d): get db conn error", myname, __LINE__);
		return (NULL);
	}

	return (db_handle);
}

static void dbconn_put(ACL_DB_HANDLE *db_handle)
{
	const char *myname = "dbconn_put";

	if (db_handle == NULL)
		acl_msg_fatal("%s(%d): input null", myname, __LINE__);

	acl_dbpool_release(db_handle);
}

static void dbconn_free(void)
{
	if (__db_pool) {
		acl_dbpool_destroy(__db_pool);
		__db_pool = NULL;
	}
}

static void usage(const char *procname)
{
	printf("usage: %s -h[help] -H mysql_addr -N dbname -U dbuser -P dbpass\n", procname);
}

int main(int argc, char *argv[])
{
	ACL_DB_HANDLE *db_handle;
	char  db_addr[256], db_name[256], db_user[256], db_pass[256];
	int   db_max = 10, db_ping = 60, db_timeout = 60;
	int   ch;

	db_addr[0] = 0;
	db_name[0] = 0;
	db_user[0] = 0;
	db_pass[0] = 0;

	while ((ch = getopt(argc, argv, "hH:N:U:P:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return (0);
		case 'H':
			snprintf(db_addr, sizeof(db_addr), "%s", optarg);
			break;
		case 'N':
			snprintf(db_name, sizeof(db_name), "%s", optarg);
			break;
		case 'U':
			snprintf(db_user, sizeof(db_user), "%s", optarg);
			break;
		case 'P':
			snprintf(db_pass, sizeof(db_pass), "%s", optarg);
			break;
		default:
			break;
		}
	}

	if (db_addr[0] == 0 || db_name[0] == 0 || db_user[0] == 0 || db_pass[0] == 0) {
		usage(argv[0]);
		return (1);
	}

	dbconn_init(db_addr, db_name, db_user, db_pass, db_max, db_ping, db_timeout);
	printf("dbconn_init ok\n");
	db_handle = dbconn_get();
	if (db_handle) {
		printf("dbconn_get ok\n");
		dbconn_put(db_handle);
		printf("dbconn_put ok\n");
	}
	dbconn_free();
	printf("dbconn_free ok\n");
	mysql_library_end();

	return (0);
}
