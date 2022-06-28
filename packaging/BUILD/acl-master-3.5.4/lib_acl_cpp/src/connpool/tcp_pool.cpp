#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/connpool/tcp_client.hpp"
#include "acl_cpp/connpool/tcp_pool.hpp"
#endif

namespace acl
{

tcp_pool::tcp_pool(const char* addr, size_t count, size_t idx /* = 0 */)
: connect_pool(addr, count, idx)
{
}

tcp_pool::~tcp_pool(void)
{
}

connect_client* tcp_pool::create_connect(void)
{
	tcp_client* conn = NEW tcp_client(addr_, conn_timeout_, rw_timeout_);
	return conn;
}

bool tcp_pool::send(const void* data, unsigned int len,
	string* out /* = NULL */)
{
	tcp_client* conn = (tcp_client*) this->peek();
	if (conn == NULL) {
		logger_error("no connection available, addr=%s",
			this->get_addr());
		return false;
	}

	if (!conn->send(data, len, out)) {
		this->put(conn, false);
		return false;
	}

	this->put(conn);
	return true;
}

} // namespace acl
