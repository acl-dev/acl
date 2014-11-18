// sqlite.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stdlib/util.hpp"
#include "acl_cpp/db/db_sqlite.hpp"

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

int main(void)
{
	acl::string dbfile("测试.db");
	acl::db_sqlite db(dbfile);
	int   max = 100;

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

	acl::meter_time(__FILE__, __LINE__, "---begin insert---");
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
	printf(">>insert total affect: %d\n", db.affect_total_count());

	acl::meter_time(__FILE__, __LINE__, "---end insert---");
	acl::meter_time(__FILE__, __LINE__, "---begin select---");

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
	acl::meter_time(__FILE__, __LINE__, "---end select---");

	acl::meter_time(__FILE__, __LINE__, "---begin delete---");
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
	printf(">>delete total affected: %d\n", db.affect_total_count());
	acl::meter_time(__FILE__, __LINE__, "---end delete---");

	printf("Enter any key to exit.\r\n");
	getchar();
	return 0;
}
