#include "acl_stdafx.hpp"
#include "acl_cpp/http/http_request.hpp"
#include "acl_cpp/http/http_request_pool.hpp"

namespace acl
{

http_request_pool::http_request_pool(const char* addr,
	int count, size_t idx /* = 0 */)
: connect_pool(addr, count, idx)
{
	conn_timeout_ = 30;
	rw_timeout_ = 30;
}

http_request_pool::~http_request_pool()
{

}

http_request_pool& http_request_pool::set_timeout(int conn_timeout /* = 30 */,
	int rw_timeout /* = 60 */)
{
	conn_timeout_ = conn_timeout;
	rw_timeout_ = rw_timeout;
	return *this;
}

connect_client* http_request_pool::create_connect()
{
	http_request* request = NEW http_request(addr_,
		conn_timeout_, rw_timeout_);
	return request;
}

} // namespace acl
