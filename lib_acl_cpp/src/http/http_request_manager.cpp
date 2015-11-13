#include "acl_stdafx.hpp"
#include "acl_cpp/http/http_request_pool.hpp"
#include "acl_cpp/http/http_request_manager.hpp"

namespace acl
{

http_request_manager::http_request_manager(
	int conn_timeout /* = 30 */, int rw_timeout /* = 30 */)
	: connect_manager(conn_timeout, rw_timeout)
{
}

http_request_manager::~http_request_manager()
{
}

connect_pool* http_request_manager::create_pool(const char* addr,
	size_t count, size_t idx)
{
	return NEW http_request_pool(addr, count, idx);
}

}  // namespace acl
