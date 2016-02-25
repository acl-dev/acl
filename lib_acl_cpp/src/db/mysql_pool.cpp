#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/connpool/connect_client.hpp"
#include "acl_cpp/db/db_handle.hpp"
#include "acl_cpp/db/db_mysql.hpp"
#include "acl_cpp/db/mysql_conf.hpp"
#include "acl_cpp/db/mysql_pool.hpp"
#endif

namespace acl
{

mysql_pool::mysql_pool(const char* dbaddr, const char* dbname,
	const char* dbuser, const char* dbpass, int dblimit /* = 64 */,
	unsigned long dbflags /* = 0 */, bool auto_commit /* = true */,
	int conn_timeout /* = 60 */, int rw_timeout /* = 60 */,
	const char* charset /* = "utf8" */)
	: db_pool(dbaddr, dblimit)
{
	acl_assert(dbaddr && *dbaddr);
	acl_assert(dbname && *dbname);

	conf_ = NEW mysql_conf(dbaddr, dbname);

	if (dbuser && *dbuser)
		conf_->set_dbuser(dbuser);
	if (dbpass && *dbpass)
		conf_->set_dbpass(dbpass);
	if (charset && *charset)
		conf_->set_charset(charset);
	conf_->set_dbflags(dbflags);
	conf_->set_dblimit(dblimit);
	conf_->set_auto_commit(auto_commit);
	conf_->set_conn_timeout(conn_timeout);
	conf_->set_rw_timeout(rw_timeout);
}

mysql_pool::mysql_pool(const mysql_conf& conf)
: db_pool(conf.get_dbkey(), conf.get_dblimit())
{
	conf_ = NEW mysql_conf(conf);
}

mysql_pool::~mysql_pool()
{
	delete conf_;
}

connect_client* mysql_pool::create_connect()
{
	return NEW db_mysql(*conf_);
}

} // namespace acl
