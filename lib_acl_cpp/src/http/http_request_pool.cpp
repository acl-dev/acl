#include "acl_stdafx.hpp"
#include "acl_cpp/http/http_request.hpp"
#include "acl_cpp/http/http_request_pool.hpp"

namespace acl
{

http_request_pool::http_request_pool(const char* addr,
	size_t count, size_t idx /* = 0 */)
: connect_pool(addr, count, idx)
{
}

http_request_pool::~http_request_pool()
{
}

connect_client* http_request_pool::create_connect()
{
	return NEW http_request(addr_, conn_timeout_, rw_timeout_);
}

} // namespace acl
