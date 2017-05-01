// pgsql.cpp : 定义控制台应用程序的入口点。
//

#include "acl_cpp/lib_acl.hpp"

const char* CREATE_TBL =
	"create table group_tbl\r\n"
	"(\r\n"
	"group_name varchar(128) not null,\r\n"
	"uvip_tbl varchar(32) not null default 'uvip_tbl',\r\n"
	"access_tbl varchar(32) not null default 'access_tbl',\r\n"
	"access_week_tbl varchar(32) not null default 'access_week_tbl',\r\n"
	"access_month_tbl varchar(32) not null default 'access_month_tbl',\r\n"
	"update_date date not null default '1970-01-01',\r\n"
	"disable integer not null default 0,\r\n"
	"add_by_hand integer not null default 0,\r\n"
	"class_level integer not null default 0,\r\n"
	"primary key(group_name, class_level)\r\n"
	")";

static bool tbl_create(const char* dbaddr, const char* dbname,
	const char* dbuser, const char* dbpass)
{
	acl::pgsql_conf conf(dbaddr, dbname);
	conf.set_dbuser(dbuser).set_dbpass(dbpass);
	acl::db_pgsql db(conf);
	if (db.open() == false)
	{
		printf("open %s@pgsql error, dbuser: %s, dbpass: %s\r\n",
			dbaddr, dbuser, dbpass);
		return false;
	}

	if (db.tbl_exists("group_tbl"))
	{
		printf("table exist\r\n");
		return (true);
	}
	else if (db.sql_update(CREATE_TBL) == false)
	{
		printf("sql error\r\n");
		return (false);
	}
	else
	{
		printf("create table ok\r\n");
		return (true);
	}
}

// 添加表数据
static bool tbl_insert(acl::db_handle& db, int n)
{
	acl::query query;
	query.create_sql("insert into group_tbl(group_name, uvip_tbl,"
		" update_date) values(:group, :test, :date)")
		.set_format("group", "group:%d", n)
		.set_parameter("test", "test")
		.set_date("date", time(NULL), "%Y-%m-%d");

	if (db.exec_update(query) == false)
		return (false);

	const acl::db_rows* result = db.get_result();
	if (result)
	{
		const std::vector<acl::db_row*>& rows = result->get_rows();
		for (size_t i = 0; i < rows.size(); i++)
		{
			const acl::db_row* row = rows[i];
			for (size_t j = 0; j < row->length(); j++)
				printf("%s, ", (*row)[j]);
			printf("\r\n");
		}
	}

	db.free_result();
	return (true);
}

// 查询表数据
static int tbl_select(acl::db_handle& db, int n)
{
	acl::query query;
	query.create_sql("select * from group_tbl where group_name=:group"
		" and uvip_tbl=:test")
		.set_format("group", "group:%d", n)
		.set_format("test", "test");

	if (db.exec_select(query) == false)
	{
		printf("select sql error\r\n");
		return (-1);
	}

	printf("\r\n---------------------------------------------------\r\n");

	// 列出查询结果方法一
	const acl::db_rows* result = db.get_result();
	if (result)
	{
		const std::vector<acl::db_row*>& rows = result->get_rows();
		for (size_t i = 0; i < rows.size(); i++)
		{
			if (n > 100)
				continue;
			const acl::db_row* row = rows[i];
			for (size_t j = 0; j < row->length(); j++)
				printf("%s, ", (*row)[j]);
			printf("\r\n");
		}
	}

	// 列出查询结果方法二
	for (size_t i = 0; i < db.length(); i++)
	{
		if (n > 100)
			continue;
		const acl::db_row* row = db[i];

		// 取出该行记录中某个字段的值
		const char* ptr = (*row)["group_name"];
		if (ptr == NULL)
		{
			printf("error, no group name\r\n");
			continue;
		}
		printf("group_name=%s: ", ptr);
		for (size_t j = 0; j < row->length(); j++)
			printf("%s, ", (*row)[j]);
		printf("\r\n");
	}

	// 列出查询结果方法三
	const std::vector<acl::db_row*>* rows = db.get_rows();
	if (rows)
	{
		std::vector<acl::db_row*>::const_iterator cit = rows->begin();
		for (; cit != rows->end(); cit++)
		{
			if (n > 100)
				continue;
			const acl::db_row* row = *cit;
			for (size_t j = 0; j < row->length(); j++)
				printf("%s, ", (*row)[j]);
			printf("\r\n");
		}

	}
	int  ret = (int) db.length();

	// 释放查询结果
	db.free_result();
	return (ret);
}

// 删除表数据
static bool tbl_delete(acl::db_handle& db, int n)
{
	acl::query query;
	query.create_sql("delete from group_tbl where group_name=:group")
		.set_format("group", "group:%d", n);

	if (db.exec_update(query) == false)
	{
		printf("delete sql error\r\n");
		return (false);
	}

	printf(">>%s<<\r\n", query.to_string().c_str());

	for (size_t i = 0; i < db.length(); i++)
	{
		const acl::db_row* row = db[i];
		for (size_t j = 0; j < row->length(); j++)
			printf("%s, ", (*row)[j]);
		printf("\r\n");
	}
	// 释放查询结果
	db.free_result();

	return (true);
}

//////////////////////////////////////////////////////////////////////////////

class db_thread : public acl::thread
{
public:
	db_thread(acl::db_pool& dp, int max) : pool_(dp), max_(max) {}
	~db_thread() {}

protected:
	void* run()
	{
		int n = 0;

		for (int i = 0; i < max_; i++)
		{
			acl::db_handle* db = pool_.peek_open();
			if (db == NULL)
			{
				printf("peek db connection error\r\n");
				break;
			}

			bool ret = tbl_insert(*db, i);
			if (ret)
			{
				printf(">>insert ok: i=%d, affected: %d\r",
					i, db->affect_count());
				n++;
			}
			else
				printf(">>insert error: i = %d\r\n", i);

			pool_.put(db);
		}
		printf("\r\n");
		printf(">>insert total: %d\r\n", n);
		getchar();

		n = 0;
		// 批量查询数据
		for (int i = 0; i < max_; i++)
		{
			acl::db_handle* db = pool_.peek_open();
			if (db == NULL)
			{
				printf("peek db connection error\r\n");
				break;
			}

			int  ret = tbl_select(*db, i);
			if (ret >= 0)
			{
				n += ret;
				printf(">>select ok: i=%d, ret=%d\r", i, ret);
			}
			else
				printf(">>select error: i = %d\r\n", i);

			pool_.put(db);
		}
		printf("\r\n");
		printf(">>select total: %d\r\n", n);

		// 批量删除数据
		for (int i = 0; i < max_; i++)
		{
			acl::db_handle* db = pool_.peek_open();
			if (db == NULL)
			{
				printf("peek db connection error\r\n");
				break;
			}

			bool ret = tbl_delete(*db, i);
			if (ret)
				printf(">>delete ok: %d, affected: %d\r",
					i, (int) db->affect_count());
			else
				printf(">>delete error: i = %d\r\n", i);

			pool_.put(db);
		}
		printf("\r\n");

		return NULL;
	}

private:
	acl::db_pool& pool_;
	int   max_;
};

//////////////////////////////////////////////////////////////////////////////

int main(void)
{
	// WIN32 下需要调用此函数进行有关 SOCKET 的初始化
	acl::acl_cpp_init();

	// 允许将错误日志输出至屏幕
	acl::log::stdout_open(true);

	acl::string line;
	acl::stdin_stream in;
	acl::stdout_stream out;

#if	defined(_WIN32) || defined(_WIN64)
	const char* libname = "libpq.dll";
#else
	const char* libname = "libpq.so";
#endif

	acl::string path;

	// 因为采用动态加载的方式，所以需要应用给出 pgsql 客户端库所在的路径
	out.format("Enter %s load path: ", libname);
	if (in.gets(line) && !line.empty())
#if	defined(_WIN32) || defined(_WIN64)
		path.format("%s\\%s", line.c_str(), libname);
#else
		path.format("%s/%s", line.c_str(), libname);
#endif
	else
		path = libname;

	out.format("%s path: %s\r\n", libname, path.c_str());
	// 设置动态库加载的全路径
	acl::db_handle::set_loadpath(path);

	acl::string dbaddr("127.0.0.1:5432");
	acl::string dbname("acl_db"), dbuser, dbpass;

	out.format("Enter dbaddr [default: %s]: ", dbaddr.c_str());
	if (in.gets(line) && !line.empty())
		dbaddr = line;

	out.format("Enter dbname [default: %s]: ", dbname.c_str());
	if (in.gets(line) && !line.empty())
		dbname = line;

	out.format("Enter dbuser [default: %s]: ", dbuser.c_str());
	if (in.gets(line) && !line.empty())
		dbuser = line;

	out.format("Enter dbpass [default: %s]: ", dbpass.c_str());
	if (in.gets(line) && !line.empty())
		dbpass = line;

	out.format("dbname: %s, dbuser: %s, dbpass: %s\r\n",
		dbname.c_str(), dbuser.c_str(), dbpass.c_str());

	// 当数据表不存在时创建表
	if (tbl_create(dbaddr, dbname, dbuser, dbpass) == false)
	{
		printf("create table error\r\n");
		getchar();
		return 1;
	}

	//////////////////////////////////////////////////////////////////////

	out.puts("Enter any key to continue ...");
	(void) in.gets(line);

	// 连接池中最大连接数量限制，该值应和线程池中的最大线程数相等，
	// 以保证每个线程都可以获得一个连接
	int  dblimit = 1;

	// 创建数据库连接池对象
	acl::pgsql_conf dbconf(dbaddr, dbname);
	dbconf.set_dbuser(dbuser).set_dbpass(dbpass).set_dblimit(dblimit);
	acl::db_pool* dp = new acl::pgsql_pool(dbconf);

	// 设置连接池中每个连接的空闲时间(秒)
	dp->set_idle(120);

	// 每个处理线程内部针对每个操作执行的次数
	int  max = 10;

	// 创建多个线程
	std::vector<db_thread*> threads;
	for (int i = 0; i < dblimit; i++)
	{
		db_thread* thread = new db_thread(*dp, max);
		threads.push_back(thread);
		thread->set_detachable(false);
		thread->start();
	}

	// 等待所有工作线程退出
	std::vector<db_thread*>::iterator it;
	for (it = threads.begin(); it != threads.end(); ++it)
	{
		(*it)->wait(NULL);
		delete *it;
	}

	delete dp;
	printf("Enter any key to exit.\r\n");
	getchar();
	return 0;
}
