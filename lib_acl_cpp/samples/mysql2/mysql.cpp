// mysql.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"

// 查询表数据
static int tbl_select(acl::db_handle& db)
{
	const char* sql = "select value, category, type from black_white_list";

	if (db.sql_select(sql) == false)
	{
		printf("select sql error\r\n");
		return (-1);
	}

	printf("\r\n---------------------------------------------------\r\n");

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

	int  ret = (int) db.length();

	// 释放查询结果
	db.free_result();
	return (ret);
}

int main(void)
{

	const char* dbaddr = "127.0.0.1:16811";
	const char* dbname = "inc365_antispam_db";
	const char* dbuser = "root", *dbpass = "";
	acl::db_mysql db(dbaddr, dbname, dbuser, dbpass);

	// 允许将错误日志输出至屏幕
	acl_msg_stdout_enable(1);

	if (db.open(NULL) == false)
	{
		printf("open db(%s) error\r\n", dbname);
		getchar();
		return 1;
	}

	printf("open db %s ok\r\n", dbname);

	int  ret = tbl_select(db);
	if (ret >= 0)
	{
		printf(">>select ok\r\n");
	}
	else
		printf(">>select error\r\n");

	printf("mysqlclient lib's version: %ld, info: %s\r\n",
		db.mysql_libversion(), db.mysql_client_info());
	return 0;
}
