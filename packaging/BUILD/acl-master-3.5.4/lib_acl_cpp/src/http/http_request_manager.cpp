#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/http/http_request_pool.hpp"
#include "acl_cpp/http/http_request_manager.hpp"
#endif

namespace acl
{

http_request_manager::http_request_manager()
: ssl_conf_(NULL)
{
}

http_request_manager::~http_request_manager()
{
}

void http_request_manager::set_ssl(sslbase_conf* ssl_conf)
{
	ssl_conf_ = ssl_conf;
}

connect_pool* http_request_manager::create_pool(const char* addr,
	size_t count, size_t idx)
{
	http_request_pool* pool = NEW http_request_pool(addr, count, idx);
	if (ssl_conf_) {
		pool->set_ssl(ssl_conf_);
	}
	return pool;
}

}  // namespace acl
