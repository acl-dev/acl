#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/db/mysql_pool.hpp"
#include "acl_cpp/db/mysql_conf.hpp"
#include "acl_cpp/db/mysql_manager.hpp"
#endif

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_DB_DISABLE)

namespace acl {

mysql_manager::mysql_manager(time_t idle_ttl /* = 120 */)
: idle_ttl_(idle_ttl)
{
}

mysql_manager::~mysql_manager()
{
	std::map<string, mysql_conf*>::iterator it;
	for (it = dbs_.begin(); it != dbs_.end(); ++it) {
		delete it->second;
	}
}

mysql_manager& mysql_manager::add(const char* dbaddr, const char* dbname,
	const char* dbuser, const char* dbpass, size_t dblimit /* = 64 */,
	unsigned long dbflags /* = 0 */, bool auto_commit /* = true */,
	int conn_timeout /* = 60 */, int rw_timeout /* = 60 */,
	const char* charset /* = "utf8" */)
{
	const char* ptr = strchr(dbaddr, '@');
	if (ptr != NULL) {
		ptr++;
	} else {
		ptr = dbaddr;
	}
	acl_assert(*ptr);

	string key;
	key.format("%s@%s", dbname, ptr);
	key.lower();

	std::map<string, mysql_conf*>::iterator it = dbs_.find(key);
	if (it != dbs_.end()) {
		delete it->second;
		dbs_.erase(it);
	}

	mysql_conf* conf = NEW mysql_conf(dbaddr, dbname);

	if (dbuser && *dbuser) {
		conf->set_dbuser(dbuser);
	}
	if (dbpass && *dbpass) {
		conf->set_dbpass(dbpass);
	}
	if (charset && *charset) {
		conf->set_charset(charset);
	}
	conf->set_dblimit(dblimit);
	conf->set_dbflags(dbflags);
	conf->set_auto_commit(auto_commit);
	conf->set_conn_timeout(conn_timeout);
	conf->set_rw_timeout(rw_timeout);

	dbs_[key] = conf;
	// 调用基类 connect_manager::set 方法添加
	set(key.c_str(), dblimit);

	return *this;
}

mysql_manager& mysql_manager::add(const mysql_conf& conf)
{
	const char* key = conf.get_dbkey();
	std::map<string, mysql_conf*>::iterator it = dbs_.find(key);
	if (it != dbs_.end()) {
		delete it->second;
		dbs_.erase(it);
	}

	mysql_conf* mc = NEW mysql_conf(conf);
	dbs_[key] = mc;
	// 调用基类 connect_manager::set 方法添加
	set(key, conf.get_dblimit());

	return *this;
}

connect_pool* mysql_manager::create_pool(const char* key, size_t, size_t)
{
	std::map<string, mysql_conf*>::iterator it = dbs_.find(key);
	if (it == dbs_.end()) {
		logger_error("db key: %s not exists", key);
		return NULL;
	}

	mysql_conf* conf = it->second;
	mysql_pool* dbpool = NEW mysql_pool(*conf);

	if (idle_ttl_ > 0) {
		dbpool->set_idle_ttl(idle_ttl_);
	}

	return dbpool;
}

} // namespace acl

#endif // !defined(ACL_CLIENT_ONLY) && !defined(ACL_DB_DISABLE)
