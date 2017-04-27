#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/connpool/connect_client.hpp"
#include "acl_cpp/db/db_handle.hpp"
#include "acl_cpp/db/db_pgsql.hpp"
#include "acl_cpp/db/pgsql_conf.hpp"
#include "acl_cpp/db/pgsql_pool.hpp"
#endif

namespace acl
{

pgsql_pool::pgsql_pool(const pgsql_conf& conf)
: db_pool(conf.get_dbkey(), conf.get_dblimit())
{
	conf_ = NEW pgsql_conf(conf);
}

pgsql_pool::~pgsql_pool()
{
	delete conf_;
}

connect_client* pgsql_pool::create_connect()
{
	return NEW db_pgsql(*conf_);
}

} // namespace acl
