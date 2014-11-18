#include "acl_stdafx.hpp"
#include "acl_cpp/http/http_request_pool.hpp"
#include "acl_cpp/http/http_request_manager.hpp"

namespace acl
{

http_request_manager::http_request_manager()
{

}

http_request_manager::~http_request_manager()
{

}

connect_pool* http_request_manager::create_pool(const char* addr,
	int count, size_t idx)
{
	http_request_pool* conns = NEW http_request_pool(addr, count, idx);

	return conns;
}

}  // namespace acl
