#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/db/pgsql_pool.hpp"
#include "acl_cpp/db/pgsql_conf.hpp"
#include "acl_cpp/db/pgsql_manager.hpp"
#endif

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_DB_DISABLE)

namespace acl {

pgsql_manager::pgsql_manager(time_t idle_ttl /* = 120 */)
: idle_ttl_(idle_ttl)
{
}

pgsql_manager::~pgsql_manager(void)
{
	std::map<string, pgsql_conf*>::iterator it;
	for (it = dbs_.begin(); it != dbs_.end(); ++it) {
		delete it->second;
	}
}

pgsql_manager& pgsql_manager::add(const pgsql_conf& conf)
{
	const char* key = conf.get_dbkey();
	std::map<string, pgsql_conf*>::iterator it = dbs_.find(key);
	if (it != dbs_.end()) {
		delete it->second;
		dbs_.erase(it);
	}

	pgsql_conf* mc = NEW pgsql_conf(conf);
	dbs_[key] = mc;
	// 调用基类 connect_manager::set 方法添加
	set(key, conf.get_dblimit());

	return *this;
}

connect_pool* pgsql_manager::create_pool(const char* key, size_t, size_t)
{
	std::map<string, pgsql_conf*>::iterator it = dbs_.find(key);
	if (it == dbs_.end()) {
		logger_error("db key: %s not exists", key);
		return NULL;
	}

	pgsql_conf* conf = it->second;
	pgsql_pool* dbpool = NEW pgsql_pool(*conf);

	if (idle_ttl_ > 0) {
		dbpool->set_idle_ttl(idle_ttl_);
	}

	return dbpool;
}

} // namespace acl

#endif // !defined(ACL_CLIENT_ONLY) && !defined(ACL_DB_DISABLE)
