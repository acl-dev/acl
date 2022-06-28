// mysql.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "lib_acl.h"
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
	const char* sql_fmt = "insert into group_tbl(group_name, uvip_tbl)"
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
	const char* libname = "libmysql.dll";
#else
	const char* libname = "libmysqlclient_r.so";
#endif

	acl::string path;

	// 因为采用动态加载的方式，所以需要应用给出 mysql 客户端库所在的路径
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

#ifdef WIN32
	const char* dbaddr = "192.168.1.251:3306";
#else
	const char* dbaddr = "127.0.0.1:3306";
#endif
	const char* dbname = "acl_test_db";
	const char* dbuser = "acl_user", *dbpass = "111111";
	acl::db_mysql db(dbaddr, dbname, dbuser, dbpass);
	int   max = 100;

	// 允许将错误日志输出至屏幕
	acl_msg_stdout_enable(1);

	if (db.open() == false)
	{
		printf("open db(%s) error\r\n", dbname);
		getchar();
		return 1;
	}

	printf("open db %s ok\r\n", dbname);

	if (tbl_create(db) == false)
	{
		printf("create table error\r\n");
		getchar();
		return 1;
	}

	ACL_METER_TIME("---begin insert---");
	for (int i = 0; i < max; i++)
	{
		bool ret = tbl_insert(db, i);
		if (ret)
			printf(">>insert ok: i=%d, affected: %d\r",
				i, db.affect_count());
		else
			printf(">>insert error: i = %d\r\n", i);
	}
	printf("\r\n");
	ACL_METER_TIME("---end insert---");

	ACL_METER_TIME("---begin select---");
	int  n = 0;
	for (int i = 0; i < max; i++)
	{
		int  ret = tbl_select(db, i);
		if (ret >= 0)
		{
			n += ret;
			printf(">>select ok: i=%d, ret=%d\r", i, ret);
		}
		else
			printf(">>select error: i = %d\r\n", i);
	}
	printf("\r\n");
	printf(">>select total: %d\r\n", n);
	ACL_METER_TIME("---end select---");

	ACL_METER_TIME("---begin delete---");
	for (int i = 0; i < max; i++)
	{
		bool ret = tbl_delete(db, i);
		if (ret)
			printf(">>delete ok: %d, affected: %d\r",
				i, (int) db.affect_count());
		else
			printf(">>delete error: i = %d\r\n", i);
	}
	printf("\r\n");

#ifndef WIN32
//	mysql_server_end();
//	mysql_thread_end();
#endif

	printf("mysqlclient lib's version: %ld, info: %s\r\n",
		db.mysql_libversion(), db.mysql_client_info());

	ACL_METER_TIME("---end delete---");

	printf("Enter any key to exit.\r\n");
	getchar();
	return 0;
}

