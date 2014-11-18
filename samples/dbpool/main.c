#include "lib_acl.h"
#include <getopt.h>

static void usage(const char *procname)
{
	printf("usage: %s -s db_addr -d db_name -u db_user -p db_pass\n", procname);
}

int main(int argc, char *argv[])
{
	ACL_DB_POOL *db_pool;
	ACL_DB_HANDLE *db_handle;
	ACL_DB_INFO  db_info;
	int   ch, i;

	memset(&db_info, 0, sizeof(db_info));
	db_info.db_max = 10;
	db_info.ping_inter = 30;
	db_info.timeout_inter = 30;

	while ((ch = getopt(argc, argv, "hs:d:u:p:")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return (0);
		case 's':
			snprintf(db_info.db_addr, sizeof(db_info.db_addr),
					"%s", optarg);
			break;
		case 'd':
			snprintf(db_info.db_name, sizeof(db_info.db_name),
					"%s", optarg);
			break;
		case 'u':
			snprintf(db_info.db_user, sizeof(db_info.db_user),
					"%s", optarg);
			break;
		case 'p':
			snprintf(db_info.db_pass, sizeof(db_info.db_pass),
					"%s", optarg);
			break;
		default:
			break;
		}
	}

	if (db_info.db_addr[0] == 0 || db_info.db_name[0] == 0
		|| db_info.db_user[0] == 0 || db_info.db_pass[0] == 0)
	{
		usage(argv[0]);
		return (1);
	}

	db_pool = acl_dbpool_create("mysql", &db_info);
	if (db_pool == NULL)
	{
		printf("acl_dbpool_create error\n");
		return (1);
	}
	printf("acl_dbpool_create ok\n");

	for (i = 0; i < 100; i++)
	{
		db_handle = acl_dbpool_peek(db_pool);
		if (db_handle == NULL)
		{
			printf("acl_dbpool_peek error\n");
			return (1);
		}
		printf("acl_dbpool_peek ok\n");

		acl_dbpool_release(db_handle);
		printf("acl_dbpool_release ok\n");
	}

	acl_dbpool_destroy(db_pool);
	printf("acl_dbpool_destroy ok\n");

	return (0);
}
