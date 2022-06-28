#include "StdAfx.h"
#include "global/global.h"
#include "ping.h"
#include "ping_store.h"

ping_store::ping_store(std::vector<host_status*>* host_list,
	ping_callback* callback)
: host_list_(host_list)
, callback_(callback)
{

}

ping_store::~ping_store()
{
	delete host_list_;
}

//////////////////////////////////////////////////////////////////////////
// 主线程运行过程

void ping_store::rpc_onover()
{
	logger("store ping results OK!");
	callback_->ping_finish(dbpath_.empty() ? NULL : dbpath_.c_str());
	delete this;
}

//////////////////////////////////////////////////////////////////////////
// 子线程运行过程

static const char* CREATE_HOST_STATUS_TBL =
"create table host_status_tbl\r\n"
"(\r\n"
"ip varchar(32) not null default '',\r\n"
"sent integer not null default 0,\r\n"
"received integer not null default 0,\r\n"
"lost integer not null default 0,\r\n"
"loss float(10,2) not null default 0.00,\r\n"
"minimum float(10,2) not null default 0.00,\r\n"
"maximum float(10,2) not null default 0.00,\r\n"
"average float(10,2) not null default 0.00\r\n"
");\r\n"
"create index host_ip_idx on host_status_tbl(ip);\r\n";

const char* CREATE_PING_STATUS_TBL =
"create table ping_status_tbl\r\n"
"(\r\n"
"ip varchar(32) not null default '',\r\n"
"rtt float(10,2) not null default 0.00,\r\n"
"ttl int not null default 0,\r\n"
"bytes int not null default 0,\r\n"
"seq int not null default 0,\r\n"
"status int not null default 0\r\n"
");\r\n"
"create index ping_ip_idx on ping_status_tbl(ip);\r\n";

void ping_store::rpc_run()
{
	const char* path = global::get_instance().get_path();
	dbpath_.format("%s/ping_store_%ld.db", path, time(NULL));

	acl::db_sqlite db(dbpath_.c_str());
	if (db.open() == false)
		logger_error("open db: %s failed", dbpath_.c_str());
	else if (create_tbl(db) == false)
		logger_error("create table failed for %s", dbpath_.c_str());
	else
	{
		logger("open db(%s) ok", dbpath_.c_str());
		insert_tbl(db);
	}
}

bool ping_store::create_tbl(acl::db_handle& db)
{
	if (db.tbl_exists("host_status_tbl"))
		logger("host_status_tbl exist");
	else if (db.sql_update(CREATE_HOST_STATUS_TBL) == false)
	{
		logger_error("sql(%s) error", CREATE_HOST_STATUS_TBL);
		return (false);
	}
	else
		logger("create table host_status_tbl ok");

	if (db.tbl_exists("ping_status_tbl"))
		logger("ping_status_tbl exist");
	else if (db.sql_update(CREATE_PING_STATUS_TBL) == false)
	{
		logger_error("sql(%s) error", CREATE_PING_STATUS_TBL);
		return (false);
	}
	else
		logger("create table ping_status_tbl ok");
	return (true);
}

void ping_store::insert_tbl(acl::db_handle& db)
{
	std::vector<host_status*>::const_iterator cit = host_list_->begin();
	for (; cit != host_list_->end(); ++cit)
		insert_one(db, *cit);
}

void ping_store::insert_one(acl::db_handle& db, const host_status* status)
{
	acl::string sql;

	sql.format("insert into host_status_tbl(ip, sent, received, lost,"
		" loss, minimum, maximum, average) values('%s', %d, %d,"
		" %d, %0.2f, %0.2f, %0.2f, %0.2f)",
		status->get_ip(), status->get_sent(), status->get_received(),
		status->get_lost(), status->get_loss(),
		status->get_minimum(), status->get_maximum(),
		status->get_average());
	if (db.sql_update(sql.c_str()) == false)
		logger_error("sql(%s) error", sql.c_str());
	else
		logger("insert ip %s to host_status_tbl ok", status->get_ip());

	const std::vector<PING_PKT*>& pkt_list = status->get_pkt_list();
	std::vector<PING_PKT*>::const_iterator cit = pkt_list.begin();
	for (; cit != pkt_list.end(); ++cit)
	{
		sql.format("insert into ping_status_tbl(ip, rtt, ttl, bytes,"
			"seq, status) values('%s', %0.2f, %d, %d, %d, %d)",
			status->get_ip(), (*cit)->rtt_, (*cit)->ttl_,
			(*cit)->bytes_, (*cit)->seq_, (*cit)->status_);
		if (db.sql_update(sql.c_str()) == false)
			logger_error("sql(%s) error", sql.c_str());
	}
	logger("ok, insert ip %s, pkt count: %d",
		status->get_ip(), (int) pkt_list.size());
}