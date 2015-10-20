#include "acl_stdafx.hpp"
#include "acl_cpp/redis/redis_client.hpp"
#include "acl_cpp/redis/redis_client_pool.hpp"

namespace acl
{

redis_client_pool::redis_client_pool(const char* addr, size_t count,
	size_t idx /* = 0 */)
: connect_pool(addr, count, idx)
, conn_timeout_(30)
, rw_timeout_(60)
, pass_(NULL)
{
}

redis_client_pool::~redis_client_pool()
{
	if (pass_)
		acl_myfree(pass_);
}

redis_client_pool& redis_client_pool::set_timeout(int conn_timeout,
	int rw_timeout)
{
	conn_timeout_ = conn_timeout;
	rw_timeout_ = rw_timeout;
	return *this;
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

connect_client* redis_client_pool::create_connect()
{
	redis_client* conn = NEW redis_client(addr_, conn_timeout_,
		rw_timeout_);
	if (pass_)
		conn->set_password(pass_);
	return conn;
}

} // namespace acl
