#include "StdAfx.h"
#include "global/global.h"
#include "global/passwd_crypt.h"
#include "net_store.h"

net_store::net_store(const char* smtp_addr, int smtp_port,
	const char* pop3_addr, int pop3_port,
	const char* user, const char* pass,
	const char* recipients, net_store_callback* callback,
	bool store /* = false */)
	: smtp_addr_(smtp_addr)
	, smtp_port_(smtp_port)
	, pop3_addr_(pop3_addr)
	, pop3_port_(pop3_port)
	, user_(user)
	, pass_(pass)
	, recipients_(recipients)
	, callback_(callback)
	, store_(store)
{

}

net_store::~net_store()
{

}

//////////////////////////////////////////////////////////////////////////

bool create_option_tbl(acl::db_handle& db, const char* tbl_name,
	const char* sql_create)
{
	if (db.tbl_exists(tbl_name))
	{
		logger("table(%s) exist", tbl_name);
		return (true);
	}
	else if (db.sql_update(sql_create) == false)
	{
		logger_error("sql(%s) error", sql_create);
		return (false);
	}
	else
	{
		logger("create table %s ok", tbl_name);
		return (true);
	}
}

//////////////////////////////////////////////////////////////////////////

static const char* CREATE_OPT_TBL =
"create table option_tbl"
"(\r\n"
"name varchar(128) not null default '',\r\n"
"value varchar(256) not null default '',\r\n"
"PRIMARY KEY(name)"
")";

static acl::db_handle* open_option_tbl()
{
	const char* path = global::get_instance().get_path();
	acl::string dbpath;
	dbpath.format("%s/net_store.db", path);

	acl::db_handle* db = new acl::db_sqlite(dbpath.c_str());
	if (db->open() == false)
	{
		logger_error("open db: %s failed", dbpath.c_str());
		delete db;
		return NULL;
	}
	else if (create_option_tbl(*db, "option_tbl", CREATE_OPT_TBL) == false)
	{
		delete db;
		return NULL;
	}
	else
		return db;
}

bool net_store::get_key(const char* name, acl::string& out)
{
	acl::db_handle* db = open_option_tbl();
	if (db == NULL)
		return false;
	acl::string sql;
	sql.format("select value from option_tbl where name='%s'", name);
	if (db->sql_select(sql.c_str()) == false)
	{
		delete db;
		return false;
	}

	const acl::db_row* row = db->get_first_row();
	if (row == NULL)
	{
		delete db;
		return false;
	}
	const char* value = (*row)["value"];
	if (value == NULL)
	{
		db->free_result();
		delete db;
		return false;
	}
	out = value;
	return true;
}

bool net_store::set_key(const char* name, const char* value)
{
	acl::db_handle* db = open_option_tbl();
	if (db == NULL)
		return false;

	acl::string sql;
	sql.format("insert into option_tbl(name, value) values('%s', '%s')",
		name, value);
	if (db->sql_update(sql.c_str()) == false)
	{
		sql.format("update option_tbl set value='%s' where name='%s'",
			value, name);
		db->sql_update(sql.c_str());
		delete db;
		return false;
	}
	else
	{
		delete db;
		return true;
	}
}

//////////////////////////////////////////////////////////////////////////
// 主线程中运行

void net_store::rpc_onover()
{
	callback_->load_db_callback(smtp_addr_.c_str(), smtp_port_,
		pop3_addr_.c_str(), pop3_port_, user_.c_str(),
		pass_.c_str(), recipients_.c_str(), store_);

	delete this;
}

//////////////////////////////////////////////////////////////////////////
// 子线程中运行

static const char* CREATE_MAIL_TBL =
"create table mail_tbl\r\n"
"(\r\n"
"smtp_addr varchar(128) not null,\r\n"
"smtp_port int not null,\r\n"
"pop3_addr varchar(128) not null default '',\r\n"
"pop3_port int not null,\r\n"
"user varchar(128) not null default '',\r\n"
"pass varchar(128) not null default '',\r\n"
"recipients varchar(256) not null default '',\r\n"
"PRIMARY KEY(user)"
")";

void net_store::rpc_run()
{
	const char* path = global::get_instance().get_path();
	dbpath_.format("%s/net_store.db", path);

	acl::db_sqlite db(dbpath_.c_str());
	if (db.open() == false)
		logger_error("open db: %s failed", dbpath_.c_str());
	else if (create_mail_tbl(db, "mail_tbl", CREATE_MAIL_TBL) == false)
		logger_error("create table failed for %s", dbpath_.c_str());
	else if (store_)
		save_mail_db(db);
	else
		load_mail_db(db);
}

bool net_store::create_mail_tbl(acl::db_handle& db, const char* tbl_name,
				const char* sql_create)
{
	if (db.tbl_exists(tbl_name))
	{
		logger("table(%s) exist", tbl_name);
		return (true);
	}
	else if (db.sql_update(sql_create) == false)
	{
		logger_error("sql(%s) error", sql_create);
		return (false);
	}
	else
	{
		save_mail_db(db);
		logger("create table %s ok", tbl_name);
		return (true);
	}
}

void net_store::save_mail_db(acl::db_handle& db)
{
	acl::string sql;

	// 先删除原来所有的记录
	sql.format("delete from mail_tbl");
	db.sql_update(sql.c_str());

	acl::string smtp_addr, pop3_addr, user, pass, recipients;
	db.escape_string(smtp_addr_.c_str(), smtp_addr_.length(), smtp_addr);
	db.escape_string(pop3_addr_.c_str(), pop3_addr_.length(), pop3_addr);
	db.escape_string(user_.c_str(), user_.length(), user);
	db.escape_string(pass_.c_str(), pass_.length(), pass);

	ACL_ARGV* tokens = acl_argv_split(recipients_.c_str(), "\t,;\r\n");
	ACL_ITER iter;
	acl::string buf;
	acl_foreach(iter, tokens)
	{
		if (iter.i > 0)
			buf << ",";
		buf << (char*) iter.data;
	}
	acl_argv_free(tokens);
	db.escape_string(buf.c_str(), recipients_.length(), recipients);

	// 密码需要加密存储
	char* pass_crypted = passwd_crypt(pass.c_str());
	sql.format("insert into mail_tbl(smtp_addr, smtp_port, pop3_addr,"
		" pop3_port, user, pass, recipients)"
		" values('%s', %d, '%s', %d, '%s', '%s', '%s')",
		smtp_addr.c_str(), smtp_port_, pop3_addr.c_str(), pop3_port_,
		user.c_str(), pass_crypted, recipients.c_str());
	acl_myfree(pass_crypted);

	const char* ptr = sql.c_str();
	ptr = pop3_addr.c_str();

	if (db.sql_update(sql.c_str()) == false)
		logger_error("sql(%s) error", sql.c_str());
}

void net_store::load_mail_db(acl::db_handle& db)
{
	acl::string sql;
	sql.format("select smtp_addr, smtp_port, pop3_addr, pop3_port, user, pass, recipients from mail_tbl");
	if (db.sql_select(sql.c_str()) == false)
	{
		logger_error("sql(%s) error", sql.c_str());
		return;
	}
	
	const acl::db_row* first_row = db.get_first_row();
	if (first_row == NULL)
		return;
	const char* ptr = (*first_row)["smtp_addr"];
	if (ptr)
		smtp_addr_ = ptr;
	ptr = (*first_row)["smtp_port"];

	int n;
	if (ptr && (n = atoi(ptr)) > 0)
		smtp_port_ = n;

	ptr = (*first_row)["pop3_addr"];
	if (ptr)
		pop3_addr_ = ptr;

	ptr = (*first_row)["pop3_port"];
	if (ptr && (n = atoi(ptr)) > 0)
		pop3_port_ = n;

	ptr = (*first_row)["user"];
	if (ptr)
		user_ = ptr;

	ptr = (*first_row)["pass"];
	if (ptr)
	{
		// 对加密的密码需要先解密
		char* pass_plain = passwd_decrypt(ptr);
		if (pass_plain)
		{
			pass_ = pass_plain;
			acl_myfree(pass_plain);
		}
	}

	ptr = (*first_row)["recipients"];
	if (ptr)
		recipients_ = ptr;
}