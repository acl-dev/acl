#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/connpool/connect_client.hpp"
#include "acl_cpp/db/db_handle.hpp"
#include "acl_cpp/db/db_pgsql.hpp"
#include "acl_cpp/db/pgsql_conf.hpp"
#include "acl_cpp/db/pgsql_pool.hpp"
#endif

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_DB_DISABLE)

namespace acl
{

pgsql_pool::pgsql_pool(const pgsql_conf& conf)
: db_pool(conf.get_dbkey(), conf.get_dblimit())
{
	conf_ = NEW pgsql_conf(conf);
}

pgsql_pool::~pgsql_pool(void)
{
	delete conf_;
}

connect_client* pgsql_pool::create_connect(void)
{
	return NEW db_pgsql(*conf_);
}

} // namespace acl

#endif // !defined(ACL_CLIENT_ONLY) && !defined(ACL_DB_DISABLE)
