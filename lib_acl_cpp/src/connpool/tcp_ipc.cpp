#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/connpool/tcp_manager.hpp"
#include "acl_cpp/connpool/tcp_pool.hpp"
#include "acl_cpp/connpool/tcp_client.hpp"
#include "acl_cpp/connpool/tcp_ipc.hpp"
#endif

namespace acl
{

tcp_ipc::tcp_ipc(void)
: max_(0)
, ttl_(60)
, conn_timeout_(10)
, rw_timeout_(10)
{
	manager_ = NEW tcp_manager;
}

tcp_ipc::~tcp_ipc(void)
{
	delete manager_;
}

tcp_ipc& tcp_ipc::set_limit(int max)
{
	max_ = max;
	return *this;
}

tcp_ipc& tcp_ipc::set_idle(int ttl)
{
	ttl_ = ttl;
	return *this;
}

tcp_ipc& tcp_ipc::set_conn_timeout(int timeout)
{
	conn_timeout_ = timeout;
	return *this;
}

tcp_ipc& tcp_ipc::set_rw_timeout(int timeout)
{
	rw_timeout_ = timeout;
	return *this;
}

tcp_manager& tcp_ipc::get_manager(void) const
{
	acl_assert(manager_);
	return *manager_;
}

tcp_ipc& tcp_ipc::add_addr(const char* addr)
{
	manager_->set(addr, max_, conn_timeout_, rw_timeout_);
	return *this;
}

tcp_ipc& tcp_ipc::del_addr(const char* addr)
{
	manager_->remove(addr);
	return *this;
}

bool tcp_ipc::addr_exist(const char* addr)
{
	return manager_->get(addr) != NULL;
}

void tcp_ipc::get_addrs(std::vector<string>& addrs)
{
	manager_->lock();
	std::vector<connect_pool*>& pools = manager_->get_pools();
	for (std::vector<connect_pool*>::const_iterator cit = pools.begin();
		cit != pools.end(); ++cit) {

		addrs.push_back((*cit)->get_addr());
	}

	manager_->unlock();
}

bool tcp_ipc::send(const char* addr, const void* data, unsigned int len,
	string* out /* = NULL */)
{
	tcp_pool* pool = (tcp_pool*) manager_->peek(addr);
	if (pool == NULL) {
		manager_->set(addr, max_, conn_timeout_, rw_timeout_);
		pool = (tcp_pool*) manager_->peek(addr);
	}
	return send(*pool, data, len, out);
}

bool tcp_ipc::send(tcp_pool& pool, const void* data, unsigned int len,
	string* out)
{
	tcp_client* conn = (tcp_client*) pool.peek();
	if (conn == NULL) {
		logger_error("no connection available, addr=%s",
			pool.get_addr());
		return false;
	}

	if (!conn->send(data, len, out)) {
		pool.put(conn, false);
		return false;
	}

	pool.put(conn);
	return true;
}

size_t tcp_ipc::broadcast(const void* data, unsigned int len,
	bool exclusive /* = true */, bool check_result /* = false */,
	unsigned* nerr /* = NULL */)
{
	size_t n = 0;

	if (exclusive) {
		manager_->lock();
	}

	string dummy;
	std::vector<connect_pool*>& pools = manager_->get_pools();
	for (std::vector<connect_pool*>::iterator it = pools.begin();
		it != pools.end(); ++it) {

		tcp_pool* pool = (tcp_pool*) (*it);
		if (send(*pool, data, len, check_result ? &dummy : NULL)) {
			n++;
		} else if (nerr) {
			(*nerr)++;
		}
		dummy.clear();
	}

	if (exclusive) {
		manager_->unlock();
	}

	return n;
}

} // namespace acl
