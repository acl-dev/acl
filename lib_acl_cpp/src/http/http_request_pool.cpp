#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/http/http_request.hpp"
#include "acl_cpp/http/http_request_pool.hpp"
#endif

namespace acl
{

http_request_pool::http_request_pool(const char* addr,
	size_t count, size_t idx /* = 0 */)
: connect_pool(addr, count, idx)
, ssl_conf_(NULL)
{
}

http_request_pool::~http_request_pool()
{
}

void http_request_pool::set_ssl(sslbase_conf* ssl_conf)
{
	ssl_conf_ = ssl_conf;
}

connect_client* http_request_pool::create_connect()
{
	http_request* req = NEW http_request(addr_, conn_timeout_, rw_timeout_);
	if (ssl_conf_) {
		req->set_ssl(ssl_conf_);
	}
	return req;
}

//////////////////////////////////////////////////////////////////////////////

http_guard::http_guard(http_request_pool& pool)
: connect_guard(pool)
{
}

http_guard::~http_guard(void)
{
	if (conn_) {
		http_request* req = (http_request*) conn_;
		pool_.put(conn_, keep_ & req->keep_alive());
		conn_ = NULL;
	}
}

} // namespace acl
