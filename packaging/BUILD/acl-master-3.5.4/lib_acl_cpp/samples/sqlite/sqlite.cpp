// sqlite.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "acl_cpp/lib_acl.hpp"

const char* CREATE_TBL =
	"create table group_tbl\r\n"
	"(\r\n"
	"group_name varchar(128) not null,\r\n"
	"uvip_tbl varchar(32) not null default 'uvip_tbl',\r\n"
	"access_tbl varchar(32) not null default 'access_tbl',\r\n"
	"access_week_tbl varchar(32) not null default 'access_week_tbl',\r\n"
	"access_month_tbl varchar(32) not null default 'access_month_tbl',\r\n"
	"update_date date not null default '1970-1-1',\r\n"
	"disable integer not null default 0,\r\n"
	"add_by_hand integer not null default 0,\r\n"
	"class_level integer not null default 0,\r\n"
	"primary key(group_name, class_level)\r\n"
	")";

static bool tbl_create(acl::db_handle& db)
{
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
	const char* sql_fmt = "insert into group_tbl('group_name', 'uvip_tbl')"
		" values('中国人-%d', 'test')";

	acl::string sql;
	sql.format(sql_fmt, n);
	if (db.sql_update(sql.c_str()) == false)
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
	const char* sql_fmt = "select * from group_tbl where"
		" group_name='中国人-%d' and uvip_tbl='test'";

	acl::string sql;
	sql.format(sql_fmt, n);

	if (db.sql_select(sql.c_str()) == false)
	{
		printf("select sql error\r\n");
		return (-1);
	}

	// 列出查询结果方法一
	const acl::db_rows* result = db.get_result();
	if (result)
	{
		const std::vector<acl::db_row*>& rows = result->get_rows();
		for (size_t i = 0; i < rows.size(); i++)
		{
			if (n >= 5)
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
		if (n >= 5)
			continue;
		const acl::db_row* row = db[i];

		// 取出该行记录中某个字段的值
		const char* ptr = (*row)["group_name"];
		if (ptr == NULL)
		{
			printf(("error, no group name\r\n"));
			continue;
		}
		printf("group_name=%s: ", ptr);
		for (size_t j = 0; j < row->length(); j++)
			printf("%s, ", (*row)[j]);
		printf("\r\n");
	}

	int  ret = (int) db.length();

	// 释放查询结果
	db.free_result();
	return (ret);
}

// 删除表数据
static bool tbl_delete(acl::db_handle& db, int n)
{
	const char* sql_fmt = "delete from group_tbl where group_name='中国人-%d'";

	acl::string sql;
	sql.format(sql_fmt, n);

	if (db.sql_update(sql.c_str()) == false)
	{
		printf("delete sql error\r\n");
		return (false);
	}

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

class db_thread : public acl::thread
{
public:
	db_thread(acl::db_sqlite& db, acl::locker& lk, int min, int max)
		: db_(db), lk_(lk), min_(min), max_(max) {}
	~db_thread(void) {}

protected:
	// @override
	void* run(void)
	{
		for (int i = min_; i < max_; i++)
		{
			lk_.lock();
			if (tbl_insert(db_, i))
				printf(">>insert ok: i=%d, affected: %d\r",
					i, db_.affect_count());
			else
			{
				printf(">>insert error: i = %d\r\n", i);
				abort();
			}
			lk_.unlock();
		}
		printf("\r\n");
		printf(">>insert total affect: %d\n", db_.affect_total_count());

		int  n = 0;
		for (int i = min_; i < max_; i++)
		{
			lk_.lock();
			int  ret = tbl_select(db_, i);
			lk_.unlock();

			if (ret >= 0)
			{
				n += ret;
				printf(">>select ok: i=%d, ret=%d\r", i, ret);
			}
			else
			{
				printf(">>select error: i = %d\r\n", i);
				abort();
			}
		}
		printf("\r\n");
		printf(">>select total: %d\r\n", n);

		for (int i = min_; i < max_; i++)
		{
			lk_.lock();
			if (tbl_delete(db_, i))
				printf(">>delete ok: %d, affected: %d\r",
					i, (int) db_.affect_count());
			else
			{
				printf(">>delete error: i = %d\r\n", i);
				abort();
			}
			lk_.unlock();
		}
		printf("\r\n");
		printf(">>delete total affected: %d\n", db_.affect_total_count());

		return NULL;
	}

private:
	acl::db_sqlite& db_;
	acl::locker& lk_;
	int min_;
	int max_;
};

int main(void)
{
	acl::acl_cpp_init();
	acl::log::stdout_open(true);

	acl::stdin_stream in;
	acl::stdout_stream out;
	acl::string line;

#if	defined(_WIN32) || defined(_WIN64)
	const char* libname = "sqlite3.dll";
#else
	const char* libname = "libsqlite3.so";
#endif

	acl::string path;
	out.format("Enter %s load path: ", libname);
	if (in.gets(line) && !line.empty())
#if     defined(_WIN32) || defined(_WIN64)
		path.format("%s\\%s", line.c_str(), libname);
#else
		path.format("%s/%s", line.c_str(), libname);
#endif
	else
		path = libname;

	out.format("%s path: %s\r\n", libname, path.c_str());
	// 设置动态库加载的全路径
	acl::db_handle::set_loadpath(path);

	//acl::string dbfile("测试数据库.db");
	acl::string dbfile("./path1/path2/mydb.db");

	// db_sqlite 类对象的声明需在 set_loadpath 之后，因为在 db_sqlite 的
	// 构造函数中需要运行加载 libsqlite3.so
	acl::db_sqlite db(dbfile, "gbk");

	if (db.open() == false)
	{
		printf("open dbfile: %s error\r\n", dbfile.c_str());
		getchar();
		return 1;
	}

	printf("open dbfile %s ok\r\n", dbfile.c_str());

	if (tbl_create(db) == false)
	{
		printf("create table error\r\n");
		getchar();
		return 1;
	}

	// 配置数据库引擎
	db.set_conf("PRAGMA synchronous = off");
	db.set_conf("PRAGMA encoding = \"UTF-8\"");
	acl::string buf;
	if ((db.get_conf("PRAGMA encoding", buf)))
		printf(">>PRAGMA encoding: %s\r\n", buf.c_str());
	db.show_conf();

	int   max = 10000, nstep = 100;
	std::vector<db_thread*> threads;

	acl::meter_time(__FILE__, __LINE__, "---begin---");

	acl::locker lk;
	int j;
	for (int i = 0; i < max; i = j)
	{
		j = i + nstep;
		db_thread* thread = new db_thread(db, lk, i, j);
		threads.push_back(thread);
		thread->set_detachable(false);
		thread->start();
	}

	for (std::vector<db_thread*>::iterator it = threads.begin();
		it != threads.end(); ++it)
	{
		(*it)->wait();
		delete *it;
	}

	acl::meter_time(__FILE__, __LINE__, "---end---");

	printf("Enter any key to exit.\r\n");
	getchar();
	return 0;
}
