#include "lib_acl.h" // just for ACL_METER_TIME
#include "acl_cpp/lib_acl.hpp"
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static ACL_DB_POOL *__db_pool = NULL;
static bool debug_on = false;

static void usage(const char* procname)
{
	printf("usage: %s -h[help] -s addr -m -a[add] -q[find] -g[find2] -Q[pool find] -u[update] -U[pool update] -n num\n", procname);
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

	//db_info.db_before_connect = db_before_connect;
	//db_info.db_after_connect = db_after_connect;

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
	if (db_handle == NULL)
		acl_msg_fatal("%s(%d): get db conn error", myname, __LINE__);

	return (db_handle);
}

static void dbconn_put(ACL_DB_HANDLE *db_handle)
{
	const char *myname = "dbconn_put";

	if (db_handle == NULL)
		acl_msg_fatal("%s(%d): input null", myname, __LINE__);

	acl_dbpool_release(db_handle);
}

static void my_find(const char* db_addr, int num)
{
	const char* db_name = "test", *db_user = "root", *db_pass = "";
	int   db_max = 10, db_ping = 100, db_timeout = 1000;

	dbconn_init(db_addr, db_name, db_user, db_pass, db_max, db_ping, db_timeout);

	ACL_DB_HANDLE* db_handle = dbconn_get();
	ACL_SQL_RES *sql_res;
	ACL_ITER iter;
	char  sql[1024];
	int   error;

	for (int i = 0; i < num; i++)
	{
		snprintf(sql, sizeof(sql), "select user_id, user_name, user_email from user where user_id=%d", i);
		sql_res = acl_dbsql_select(db_handle, sql, &error);
		if (sql_res == NULL)
		{
			printf("over now, i: %d, num: %d\n", i, num);
			break;
		}
		if (i > 10)
		{
			acl_dbsql_free_result(db_handle, sql_res);
			if (i % 10000 == 0)
			{
				snprintf(sql, sizeof(sql), "find, num: %d\n", i);
				ACL_METER_TIME(sql);
			}
			continue;
		}
		printf(">>results:\n");
		acl_foreach(iter, sql_res)
		{
			const char **my_row = (const char**) iter.data;
			printf("\t%s\t%s\t%s\n", my_row[0], my_row[1], my_row[2]);
		}
		acl_dbsql_free_result(db_handle, sql_res);
	}

	dbconn_put(db_handle);
}

static void hs_insert(const char* addr, int num, bool enable_cache)
{
	const char* dbn = "test", *tbl = "user", *idx = "PRIMARY";
	const char* flds = "user_id,user_name,user_email";
	const char* bufs[3];
	char  buf1[256], buf2[256], buf3[256];
	acl::hsclient client(addr, enable_cache);

	if (client.open_tbl(dbn, tbl, idx, flds) == false)
	{
		printf("connect %s error\n", addr);
		return;
	}

	for (int i = 0; i < num; i++)
	{
		snprintf(buf1, sizeof(buf1), "%d", i);
		snprintf(buf2, sizeof(buf2), "zsxxsz%d", i);
		snprintf(buf3, sizeof(buf3), "zsxxsz%d@test.com", i);
		bufs[0] = buf1;
		bufs[1] = buf2;
		bufs[2] = buf3;
		if (client.add(bufs, 3) == false)
		{
			printf(">>insert error: %d, %s\n", client.get_error(),
				client.get_last_serror());
			break;
		}
		if (i < 10)
			printf(">>insert %s,%s,%s ok\n", buf1, buf2, buf3);
		if (i % 10000 == 0)
		{
			snprintf(buf1, sizeof(buf1), "insert num: %d\n", i);
			ACL_METER_TIME(buf1);
		}
	}
}

static void hs_dump(const std::vector<acl::hsrow*>& rows)
{
	std::vector<acl::hsrow*>::const_iterator cit_row;
	std::vector<const char*>::const_iterator cit_colum;

	cit_row = rows.begin();

	for (; cit_row != rows.end(); cit_row++)
	{
		cit_colum = (*cit_row)->get_row().begin();

		printf(">>results:\n");

		for (; cit_colum != (*cit_row)->get_row().end();
				cit_colum++)
		{
			printf("\t|%s|", *cit_colum);
		}

		printf("\n");
	}
}

static void hs_find(const char* addr, int num, bool enable_cache)
{
	const char* dbn = "test", *tbl = "user", *idx = "user_name";
	const char* flds = "user_id,user_name,user_email";
	const char* bufs[2];
	char  buf[256];
	acl::hsclient client(addr, enable_cache);

	if (client.open_tbl(dbn, tbl, idx, flds) == false)
	{
		printf("connect %s error\n", addr);
		return;
	}

	printf(">>dbn: %s, tbl: %s, idx: %s, flds: %s\n", dbn, tbl, idx, flds);

	for (int i = 0; i < num; i++)
	{
		snprintf(buf, sizeof(buf), "zsxxsz%d", i);
		bufs[0] = buf;

		const std::vector<acl::hsrow*>& rows =
			client.get(bufs, 1, "=", 1, 0);
		if (rows.empty())
		{
			printf("find(%s) error:%s(%d)\n",
				buf, client.get_last_serror(), client.get_error());
			break;
		}

		if (i > 0 && i % 10000 == 0)
		{
			snprintf(buf, sizeof(buf), "find, num: %d\n", i);
			ACL_METER_TIME(buf);
		}
		if (i > 1000)
			continue;
		hs_dump(rows);
	}
}

static void hs_find2(const char* addr, int num, bool enable_cache)
{
	const char* dbn = "test", *tbl = "user", *idx = "PRIMARY";
	const char* flds = "user_id,user_name,user_email";
	char  buf[256];
	acl::hsclient client(addr, enable_cache);

	if (client.open_tbl(dbn, tbl, idx, flds) == false)
	{
		printf("open %s error: %s\n", addr, client.get_last_serror());
		return;
	}

	for (int i = 0; i < num; i++)
	{
		snprintf(buf, sizeof(buf), "%d", i);

		const std::vector<acl::hsrow*>& rows =
			client.get(buf, NULL);
		if (rows.empty())
		{
			printf("find(%s) error\n", buf);
			break;
		}


		if (i % 10000 == 0)
		{
			snprintf(buf, sizeof(buf), "find, num: %d\n", i);
			ACL_METER_TIME(buf);
		}
		if (i > 1000)
			continue;

		hs_dump(rows);
	}
}

static void hs_update(const char* addr, int num, bool enable_cache)
{
	const char* dbn = "test", *tbl = "user", *idx = "PRIMARY";
	const char* flds = "user_name,user_email";
	const char* bufs[2], *bufs_to[2];
	char  buf[256], buf_to1[256], buf_to2[256];
	acl::hsclient client(addr, enable_cache);

	if (client.open_tbl(dbn, tbl, idx, flds) == false)
	{
		printf("connect %s error: %s(%d)\n",
			addr, client.get_last_serror(), client.get_error());
		return;
	}

	printf(">>dbn: %s, tbl: %s, idx: %s, flds: %s\n", dbn, tbl, idx, flds);

	for (int i = 0; i < num; i++)
	{
		snprintf(buf, sizeof(buf), "%d", i);
		bufs[0] = buf;

		snprintf(buf_to1, sizeof(buf_to1), "zsxxsz_%d", i);
		snprintf(buf_to2, sizeof(buf_to2), "zsxxsz_%d@test.com", i);
		bufs_to[0] = buf_to1;
		bufs_to[1] = buf_to2;

		if (client.mod(bufs, 1, bufs_to, 2) == false)
		{
			printf("update error: %s(%d)\n",
				client.get_last_serror(), client.get_error());
			break;
		}

		if (i > 0 && i % 10000 == 0)
		{
			snprintf(buf, sizeof(buf), "update, num: %d\n", i);
			ACL_METER_TIME(buf);
		}
		if (i > 100)
			continue;
		printf(">> update num: %d ok\n", i);
	}
}

static void hspool_find(const char* addr, int num, bool enable_cache)
{
	const char* dbn = "test", *tbl = "user", *idx;
	const char* flds = "user_id,user_name,user_email";
	char  buf[256];
	const char* addr_rd = "192.168.1.232:9998";
	acl::hspool pool(addr, addr_rd, enable_cache);
	acl::hsclient* client;

	idx = "PRIMARY";
	for (int i = 0; i < 10; i++)
	{
		client = pool.peek(dbn, tbl, idx, flds);
		if (client == NULL)
		{
			printf("peek error\n");
			break;
		}
		printf(">>read addr: %s\n", client->get_addr());
		pool.put(client);
	}

	printf("========================================================\n");

	for (int i = 0; i < 10; i++)
	{
		client = pool.peek(dbn, tbl, idx, flds);
		if (client == NULL)
		{
			printf("peek error\n");
			break;
		}
		printf(">>read/write addr: %s\n", client->get_addr());
		pool.put(client);
	}

	//////////////////////////////////////////////////////////////////////

	printf("========================================================\n");
	idx = "PRIMARY";
	for (int i = 0; i < num; i++)
	{
		snprintf(buf, sizeof(buf), "%d", i);

		client = pool.peek(dbn, tbl, idx, flds);
		if (client == NULL)
		{
			printf("peek error\n");
			break;
		}

		const std::vector<acl::hsrow*>& rows =
			client->get(buf, NULL);
		if (rows.empty())
		{
			printf("find(%s) error\n", buf);
			pool.put(client);
			break;
		}


		if (i > 0 && i % 10000 == 0)
		{
			snprintf(buf, sizeof(buf), "find, num: %d\n", i);
			ACL_METER_TIME(buf);
		}
		if (i <= 10)
		{
			printf(">>hs_id: %d, key: %s ", client->get_id(), buf);
			hs_dump(rows);
		}
		pool.put(client);
	}

	//////////////////////////////////////////////////////////////////////

	printf("========================================================\n");
	idx = "user_name";
	for (int i = 0; i < num; i++)
	{
		snprintf(buf, sizeof(buf), "zsxxsz%d", i);

		client = pool.peek(dbn, tbl, idx, flds);
		if (client == NULL)
		{
			printf("peek error\n");
			break;
		}

		const std::vector<acl::hsrow*>& rows =
			client->get(buf, NULL);
		if (rows.empty())
		{
			printf("find(%s) error\n", buf);
			pool.put(client);
			break;
		}


		if (i > 0 && i % 10000 == 0)
		{
			snprintf(buf, sizeof(buf), "find, num: %d\n", i);
			ACL_METER_TIME(buf);
		}
		if (i <= 10)
		{
			printf(">>hs_id: %d, key: %s ", client->get_id(), buf);
			hs_dump(rows);
		}
		pool.put(client);
	}

	//////////////////////////////////////////////////////////////////////

	printf("========================================================\n");
	idx = "PRIMARY";
	for (int i = 0; i < num; i++)
	{
		snprintf(buf, sizeof(buf), "%d", i);

		client = pool.peek(dbn, tbl, idx, flds);
		if (client == NULL)
		{
			printf("peek error\n");
			break;
		}

		const std::vector<acl::hsrow*>& rows =
			client->get(buf, NULL);
		if (rows.empty())
		{
			printf("find(%s) error\n", buf);
			pool.put(client);
			break;
		}


		if (i > 0 && i % 10000 == 0)
		{
			snprintf(buf, sizeof(buf), "find, num: %d\n", i);
			ACL_METER_TIME(buf);
		}
		if (i <= 10)
		{
			printf(">>hs_id: %d, key: %s ", client->get_id(), buf);
			hs_dump(rows);
		}
		pool.put(client);
	}
}

static void hspool_update(const char* addr, int num, bool enable_cache)
{
	const char* dbn = "test", *tbl = "user", *idx;
	const char* flds = "user_name,user_email";
	const char* bufs[2], *bufs_to[2];
	char  buf[256], buf_to1[256], buf_to2[256];
	const char* addr_rd = "192.168.1.232:9998";
	acl::hspool pool(addr, addr_rd, enable_cache);
	acl::hsclient* client;

	idx = "PRIMARY";
	for (int i = 0; i < num; i++)
	{
		snprintf(buf, sizeof(buf), "%d", i);

		client = pool.peek(dbn, tbl, idx, flds);
		if (client == NULL)
		{
			printf("peek error\n");
			break;
		}

		snprintf(buf, sizeof(buf), "%d", i);
		bufs[0] = buf;

		snprintf(buf_to1, sizeof(buf_to1), "zsxxsz_%d", i);
		snprintf(buf_to2, sizeof(buf_to2), "zsxxsz_%d@test.com", i);
		bufs_to[0] = buf_to1;
		bufs_to[1] = buf_to2;

		if (client->mod(bufs, 1, bufs_to, 2) == false)
		{
			printf("update error: %s(%d)\n",
				client->get_last_serror(), client->get_error());
			break;
		}

		if (i > 0 && i % 10000 == 0)
		{
			snprintf(buf, sizeof(buf), "update, num: %d\n", i);
			ACL_METER_TIME(buf);
		}
		if (i <= 10)
			printf(">>update, num: %d, hs_id: %d, key: %s\n",
				i, client->get_id(), buf);
		pool.put(client);
	}
}

int main(int argc, char *argv[])
{
	int   ch, num = 10;
	char* addr = strdup("127.0.0.1:19999");
	void (*func)(const char* addr, int num, bool enable_cache) = NULL;
	bool use_mysql = false, enable_cache = false;

	while ((ch = getopt(argc, argv, "hs:aqgQuUn:mdc")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return (0);
		case 's':
			free(addr);
			addr = strdup(optarg);
			break;
		case 'a':
			func = hs_insert;
			break;
		case 'q':
			func = hs_find;
			break;
		case 'g':
			func = hs_find2;
			break;
		case 'Q':
			func = hspool_find;
			break;
		case 'u':
			func = hs_update;
			break;
		case 'U':
			func = hspool_update;
			break;
		case 'n':
			num = atoi(optarg);
			break;
		case 'm':
			use_mysql = true;
			break;
		case 'd':
			debug_on = true;
			break;
		case 'c':
			enable_cache = true;
			break;
		default:
			break;
		}
	}

	if (use_mysql)
	{
		printf("mysql\n");
		my_find(addr, num);
		return (0);
	}

	logger_open("test.log", "test", "all:2");

	if (func != NULL)
		func(addr, num, enable_cache);
	else
		usage(argv[0]);

	free(addr);
	return (0);
}
