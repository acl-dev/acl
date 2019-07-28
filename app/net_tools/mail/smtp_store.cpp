#include "StdAfx.h"
#include "global/global.h"
#include "smtp_client.h"
#include "smtp_store.h"

smtp_store::smtp_store(const char* user, const char* smtp_ip,
	const SMTP_METER& meter, smtp_callback& callback)
: callback_(callback)
, ok_(false)
{
	user_ = acl_mystrdup(user);
	smtp_ip_ = acl_mystrdup(smtp_ip);

	meter_ = (SMTP_METER*) acl_mycalloc(1, sizeof(SMTP_METER));
	memcpy(meter_, &meter, sizeof(SMTP_METER));
}

smtp_store::~smtp_store()
{
	acl_myfree(user_);
	acl_myfree(smtp_ip_);
	acl_myfree(meter_);
}

//////////////////////////////////////////////////////////////////////////
// 主线程过程

void smtp_store::rpc_onover()
{
	logger("store smtp test results %s!", ok_ ? "OK" : "Failed");
	callback_.smtp_finish(dbpath_.c_str());
	delete this;
}

//////////////////////////////////////////////////////////////////////////
// 子线程过程

static const char* CREATE_TBL =
"create table smtp_tbl\r\n"
"(\r\n"
"user varchar(128) not null,\r\n"
"smtp_ip varchar(32) not null,\r\n"
"nslookup_elapsed float(10,2) not null default 0.00,\r\n"
"connect_elapsed float(10,2) not null default 0.00,\r\n"
"banner_elapsed float(10,2) not null default 0.00,\r\n"
"auth_elapsed float(10,2) not null default 0.00,\r\n"
"mail_elapsed float(10,2) not null default 0.00,\r\n"
"rcpt_elapsed float(10,2) not null default 0.00,\r\n"
"data_elapsed float(10,2) not null default 0.00,\r\n"
"body_elapsed float(10,2) not null default 0.00,\r\n"
"envelope_elapsed float(10,2) not null default 0.00,\r\n"
"total_elapsed float(10,2) not null default 0.00,\r\n"
"smtp_speed int not null default 0\r\n"
");\r\n"
"create index user_idx on smtp_tbl(user);\r\n";

void smtp_store::rpc_run()
{
	const char* path = global::get_instance().get_path();
	dbpath_.format("%s/smtp_store2.db", path);

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

bool smtp_store::create_tbl(acl::db_handle& db)
{
	if (db.tbl_exists("smtp_tbl"))
	{
		logger("smtp_tbl table exist");
		return (true);
	}
	else if (db.sql_update(CREATE_TBL) == false)
	{
		logger_error("sql(%s) error", CREATE_TBL);
		return (false);
	}
	else
	{
		logger("create table smtp_tbl ok");
		return (true);
	}
}

void smtp_store::insert_tbl(acl::db_handle& db)
{
	acl::string sql;

	sql.format("insert into smtp_tbl(user, smtp_ip, nslookup_elapsed, "
		"connect_elapsed, banner_elapsed, auth_elapsed, "
		"mail_elapsed, rcpt_elapsed, data_elapsed, "
		"body_elapsed, envelope_elapsed, total_elapsed, smtp_speed) "
		"values('%s', '%s', %0.2f, %0.2f, %0.2f, %0.2f, %0.2f, "
		"%0.2f, %0.2f, %0.2f, %0.2f, %0.2f, %d)",
		user_, smtp_ip_,
		meter_->smtp_nslookup_elapsed,
		meter_->smtp_connect_elapsed,
		meter_->smtp_banner_elapsed,
		meter_->smtp_auth_elapsed,
		meter_->smtp_mail_elapsed,
		meter_->smtp_rcpt_elapsed,
		meter_->smtp_data_elapsed,
		meter_->smtp_envelope_eplased,
		meter_->smtp_body_elapsed,
		meter_->smtp_total_elapsed,
		meter_->smtp_speed);

	if (db.sql_update(sql.c_str()) == false)
		logger_error("sql(%s) error", sql.c_str());
	else
		logger("insert into smtp_tbl OK!");
}
