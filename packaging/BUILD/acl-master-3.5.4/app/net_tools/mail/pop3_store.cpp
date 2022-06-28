#include "StdAfx.h"
#include "rpc/rpc_manager.h"
#include "global/global.h"
#include "pop3_client.h"
#include "pop3_store.h"

pop3_store::pop3_store(const char* user, const char* pop3_ip,
	const POP3_METER& meter, pop3_callback& callback)
: callback_(callback)
, ok_(false)
{
	user_ = acl_mystrdup(user);
	pop3_ip_ = acl_mystrdup(pop3_ip);

	meter_ = (POP3_METER*) acl_mycalloc(1, sizeof(POP3_METER));
	memcpy(meter_, &meter, sizeof(POP3_METER));
}

pop3_store::~pop3_store()
{
	acl_myfree(user_);
	acl_myfree(pop3_ip_);
	acl_myfree(meter_);
}

//////////////////////////////////////////////////////////////////////////
// 主线程过程

void pop3_store::rpc_onover()
{
	logger("store pop3 test results %s!", ok_ ? "OK" : "Failed");
	callback_.pop3_finish(dbpath_.c_str());
	delete this;
}

//////////////////////////////////////////////////////////////////////////
// 子线程过程

static const char* CREATE_TBL =
"create table pop3_tbl\r\n"
"(\r\n"
"user varchar(128) not null,\r\n"
"pop3_ip varchar(32) not null,\r\n"
"nslookup_elapsed float(10,2) not null default 0.00,\r\n"
"connect_elapsed float(10,2) not null default 0.00,\r\n"
"banner_elapsed float(10,2) not null default 0.00,\r\n"
"auth_elapsed float(10,2) not null default 0.00,\r\n"
"uidl_elapsed float(10,2) not null default 0.00,\r\n"
"list_elapsed float(10,2) not null default 0.00,\r\n"
"recv_elapsed float(10,2) not null default 0.00,\r\n"
"quit_elapsed float(10,2) not null default 0.00,\r\n"
"total_elapsed float(10,2) not null default 0.00,\r\n"
"total_list int not null default 0,\r\n"
"total_uidl int not null default 0,\r\n"
"total_size int not null default 0,\r\n"
"recved_count int not null default 0,\r\n"
"recved_size int not null default 0,\r\n"
"recved_speed int not null default 0\r\n"
");\r\n"
"create index pop3_user_idx on pop3_tbl(user);\r\n";

void pop3_store::rpc_run()
{
	const char* path = global::get_instance().get_path();
	dbpath_.format("%s/pop3_store.db", path);

	acl::db_sqlite db(dbpath_.c_str());
	if (db.open() == false)
		logger_error("open db: %s failed", dbpath_.c_str());
	else if (create_tbl(db) == false)
		logger_error("create table failed for %s", dbpath_.c_str());
	else
	{
		logger("open db(%s) ok", dbpath_.c_str());
		insert_tbl(db);
		ok_ = true;
	}
}

bool pop3_store::create_tbl(acl::db_handle& db)
{
	if (db.tbl_exists("pop3_tbl"))
	{
		logger("pop3_tbl table exist");
		return (true);
	}
	else if (db.sql_update(CREATE_TBL) == false)
	{
		logger_error("sql(%s) error", CREATE_TBL);
		return (false);
	}
	else
	{
		logger("create table pop3_tbl ok");
		return (true);
	}
}

void pop3_store::insert_tbl(acl::db_handle& db)
{
	acl::string sql;

	sql.format("insert into pop3_tbl(user, pop3_ip, nslookup_elapsed, "
		"connect_elapsed, banner_elapsed, "
		"auth_elapsed, uidl_elapsed, "
		"list_elapsed, recv_elapsed, "
		"quit_elapsed, total_elapsed, "
		"total_list, total_uidl, total_size, "
		"recved_count, recved_size, recved_speed)"
		"values('%s', '%s', %0.2f, %0.2f, %0.2f, %0.2f, "
		"%0.2f, %0.2f, %0.2f, %0.2f, %0.2f, %d, %d, %d, "
		"%d, %d, %d)",
		user_, pop3_ip_, meter_->pop3_nslookup_elapsed,
		meter_->pop3_connect_elapsed, meter_->pop3_banner_elapsed,
		meter_->pop3_auth_elapsed, meter_->pop3_uidl_elapsed,
		meter_->pop3_list_elapsed, meter_->pop3_recv_elapsed,
		meter_->pop3_quit_elapsed, meter_->pop3_total_elapsed,
		(int) meter_->total_list, (int) meter_->total_uidl,
		(int) meter_->total_size, (int) meter_->recved_count,
		(int) meter_->recved_size, (int) meter_->recved_speed);

	if (db.sql_update(sql.c_str()) == false)
		logger_error("sql(%s) error", sql.c_str());
	else
		logger("insert into pop3_tbl OK!");
}
