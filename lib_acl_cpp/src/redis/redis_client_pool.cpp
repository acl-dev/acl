#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/redis/redis_client.hpp"
#include "acl_cpp/redis/redis_client_pool.hpp"
#endif

namespace acl
{

redis_client_pool::redis_client_pool(const char* addr, size_t count,
	size_t idx /* = 0 */)
: connect_pool(addr, count, idx)
, pass_(NULL)
, dbnum_(0)
{
}

redis_client_pool::~redis_client_pool(void)
{
	if (pass_)
		acl_myfree(pass_);
}

redis_client_pool& redis_client_pool::set_password(const char* pass)
{
	if (pass_)
		acl_myfree(pass_);
	if (pass && *pass)
		pass_ = acl_mystrdup(pass);
	else
		pass_ = NULL;
	return *this;
}

redis_client_pool& redis_client_pool::set_db(int dbnum)
{
	if (dbnum > 0)
		dbnum_ = dbnum;
	return *this;
}

connect_client* redis_client_pool::create_connect(void)
{
	redis_client* conn = NEW redis_client(addr_, conn_timeout_,
		rw_timeout_);
	if (pass_)
		conn->set_password(pass_);
	if (dbnum_ > 0)
		conn->set_db(dbnum_);
	return conn;
}

} // namespace acl
