#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>

static bool __show_results = false;

class mysql_oper
{
public:
	mysql_oper(acl::db_pool& dbp, unsigned long id)
		: dbp_(dbp), id_(id)
	{
	}

	~mysql_oper(void)
	{
	}

	void mysql_add(int count)
	{
		for (int i = 0; i < count; i++)
		{
			acl::db_handle* db = dbp_.peek_open();
			if (db == NULL)
			{
				printf("peek db connection error i: %d\r\n", i);
				break;
			}

			add(*db, i);
			dbp_.put(db);
		}
	}

	void mysql_get(int count)
	{
		for (int i = 0; i < count; i++)
		{
			acl::db_handle* db = dbp_.peek_open();
			if (db == NULL)
			{
				printf("peek db connection error i: %d\r\n", i);
				break;
			}

			get(*db, i);
			dbp_.put(db);
		}
	}

private:
	acl::db_pool& dbp_;
	unsigned long id_;

	bool add(acl::db_handle& db, int n)
	{
		acl::query query;
		query.create_sql("insert into group_tbl(group_name, uvip_tbl,"
			" update_date) values(:group, :test, :date)")
			.set_format("group", "group:%lu:%d", id_, n)
			.set_parameter("test", "test")
			.set_date("date", time(NULL), "%Y-%m-%d");
		if (db.exec_update(query) == false)
		{
			printf("exec_update error: %s\r\n", db.get_error());
			return false;
		}

		return true;
	}

	bool get(acl::db_handle& db, int n)
	{
		acl::query query;
		query.create_sql("select * from group_tbl"
			" where group_name=:group"
			" and uvip_tbl=:test")
			.set_format("group", "group:%lu:%d", id_, n)
			.set_format("test", "test");
		if (db.exec_select(query) == false)
		{
			printf("exec_select error: %s\r\n", db.get_error());
			return false;
		}

		const acl::db_rows* result = db.get_result();
		if (__show_results && result)
		{
			const std::vector<acl::db_row*>& rows =
				result->get_rows();
			for (size_t i = 0; i < rows.size(); i++)
			{
				if (n > 10)
					continue;
				const acl::db_row* row = rows[i];
				for (size_t j = 0; j < row->length(); j++)
					printf("%s, ", (*row)[j]);
				printf("\r\n");
			}
		}

		db.free_result();
		return true;
	}
};

//////////////////////////////////////////////////////////////////////////////
// mysql thread class

class mysql_thread : public acl::thread
{
public:
	mysql_thread(int id, acl::db_pool& dbp, const char* oper, int count)
		: id_(id), dbp_(dbp), oper_(oper), count_(count) {}
	~mysql_thread(void) {}

protected:
	void* run(void)
	{
		mysql_oper dboper(dbp_, id_);

		if (oper_.equal("add", false))
			dboper.mysql_add(count_);
		else if (oper_.equal("get", false))
			dboper.mysql_get(count_);
		else
			printf("unknown command: %s\r\n", oper_.c_str());

		return NULL;
	}

private:
	int id_;
	acl::db_pool& dbp_;
	acl::string oper_;
	int count_;
};

//////////////////////////////////////////////////////////////////////////////
// mysql fiber class

static int __max_fibers = 2;
static int __cur_fibers = 2;

class mysql_fiber : public acl::fiber
{
public:
	mysql_fiber(int id, acl::db_pool& dbp, const char* oper, int count)
		: id_(id), dbp_(dbp), oper_(oper), count_(count) {}
	~mysql_fiber(void) {}

protected:
	// @override
	void run(void)
	{
//		printf("fiber-%d-%d running\r\n", get_id(), acl::fiber::self());
		acl_fiber_delay(10);

		mysql_oper dboper(dbp_, id_);

		if (oper_.equal("add", false))
			dboper.mysql_add(count_);
		else if (oper_.equal("get", false))
			dboper.mysql_get(count_);
		else
			printf("unknown command: %s\r\n", oper_.c_str());

		delete this;

		printf("----__cur_fibers: %d----\r\n", __cur_fibers);

		if (--__cur_fibers == 0)
		{
			printf("All fibers Over\r\n");
			acl::fiber::stop();
		}
	}

private:
	int id_;
	acl::db_pool& dbp_;
	acl::string oper_;
	int count_;
};

//////////////////////////////////////////////////////////////////////////////

static void usage(const char* procname)
{
	printf("usage: %s\r\n"
		" -h [help]\r\n"
		" -c cocurrent\r\n"
		" -t [use threads mode]\r\n"
		" -n oper_count\r\n"
		" -f mysqlclient_path\r\n"
		" -s mysql_addr\r\n"
		" -o db_oper[add|get]\r\n"
		" -d [show results of get]\r\n"
		" -C conn_timeout\r\n"
		" -R rw_timeout\r\n"
		" -u dbuser\r\n"
		" -p dbpass\r\n",
		procname);
}

int main(int argc, char *argv[])
{
	int  ch, count = 10, conn_timeout = 10, rw_timeout = 10, cocurrent = 2;
	acl::string mysql_path("../../lib/libmysqlclient_r.so");
	acl::string dbaddr("127.0.0.1:3306"), dbname("acl_db");
	acl::string dbuser("root"), dbpass(""), oper("get");
	bool use_threads = false;

	acl::acl_cpp_init();
	acl::log::stdout_open(true);

	while ((ch = getopt(argc, argv, "hc:tn:f:s:u:o:p:C:R:d")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return 0;
		case 'c':
			cocurrent = atoi(optarg);
			break;
		case 't':
			use_threads = true;
			break;
		case 'n':
			count = atoi(optarg);
			break;
		case 'f':
			mysql_path = optarg;
			break;
		case 's':
			dbaddr = optarg;
			break;
		case 'u':
			dbuser = optarg;
			break;
		case 'p':
			dbpass = optarg;
			break;
		case 'o':
			oper = optarg;
			break;
		case 'C':
			conn_timeout = atoi(optarg);
			break;
		case 'R':
			rw_timeout = atoi(optarg);
			break;
		case 'd':
			__show_results = true;
			break;
		default:
			break;
		}
	}

	// setup libmysqlclient_r.so path
	acl::db_handle::set_loadpath(mysql_path);

	// init mysql connection configure
	acl::mysql_conf dbconf(dbaddr, dbname);
	dbconf.set_dbuser(dbuser)
		.set_dbpass(dbpass)
		.set_dblimit(cocurrent)
		.set_conn_timeout(conn_timeout)
		.set_rw_timeout(rw_timeout);

	// init mysql connections pool
	acl::mysql_pool dbpool(dbconf);

	if (use_threads)
	{
		std::vector<acl::thread*> threads;

		for (int i = 0; i < cocurrent; i++)
		{
			acl::thread* thread = new
				mysql_thread(i, dbpool, oper, count);

			thread->set_detachable(false);
			threads.push_back(thread);
			thread->start();
		}

		for (std::vector<acl::thread*>::iterator it = threads.begin();
				it != threads.end(); ++it)
		{
			(*it)->wait(NULL);
			delete (*it);
		}
	}
	else
	{
		__max_fibers = cocurrent;
		__cur_fibers = __max_fibers;

		for (int i = 0; i < __max_fibers; i++)
		{
			acl::fiber* f = new mysql_fiber(i, dbpool, oper, count);
			f->start();
		}

		acl::fiber::schedule();
	}

	printf("---- exit now ----\r\n");

	return 0;
}
