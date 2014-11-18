// db_service.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <iostream>
#include "acl_cpp/acl_cpp_init.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stream/aio_handle.hpp"
#include "acl_cpp/db/db_handle.hpp"
#include "acl_cpp/db/db_sqlite.hpp"
#include "acl_cpp/db/db_service_sqlite.hpp"

class myquery : public acl::db_query
{
public:
	myquery(int id) : id_(id)
	{

	}

	~myquery()
	{

	}

	virtual void on_error(acl::db_status status)
	{
		printf("%s:%d\r\n", __FUNCTION__, __LINE__);

		(void) status;
		printf(">>on error, id: %d\r\n", id_);
	}

	virtual void on_ok(const acl::db_rows* rows, int affected)
	{
		(void) rows;
		(void) affected;
		printf(">>on ok, id: %d\r\n", id_);
	}

	virtual void destroy()
	{
		delete this;
	}
protected:
private:
	int   id_;
};

static acl::string __dbfile("测试.db");

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
		return (true);
	if (db.sql_update(CREATE_TBL) == false)
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

static bool create_db(void)
{
	acl::db_sqlite db(__dbfile);

	if (db.open() == false)
	{
		printf("open dbfile: %s error\r\n", __dbfile.c_str());
		return (false);
	}
	db.show_conf();
	return (tbl_create(db));
}

int main(void)
{
	acl::acl_cpp_init();
#ifdef	WIN32
	acl::open_dos();
#endif

	logger_open("dbservice.log", "dbservice", "all:0");

	if (create_db() == false)
	{
		getchar();
		return (1);
	}

	acl::aio_handle handle;
	acl::db_service* server = new acl::db_service_sqlite("DB_TEST", __dbfile, 100, 2);

	// 使消息服务器监听 127.0.0.1 的地址
	if (server->open(&handle) == false)
	{
		delete server;
		std::cout << "open server error!" << std::endl;
		getchar();
		return (1);
	}

	acl::string sql;
	myquery* query;

	for (int i = 0; i < 100; i++)
	{
		query = new myquery(i);
		sql.format("insert into group_tbl('group_name', 'uvip_tbl')"
			" values('中国人-%d', 'test')", i);
		server->sql_update(sql.c_str(), query);
	}

	while (true)
	{
		if (handle.check() == false)
		{
			std::cout << "stop now!" << std::endl;
			break;
		}
	}

	delete server;
	handle.check();

	std::cout << "server stopped!" << std::endl;

	getchar();
	return (0);
}

