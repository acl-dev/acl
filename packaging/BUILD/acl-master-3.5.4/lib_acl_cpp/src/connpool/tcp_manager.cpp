#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/connpool/tcp_pool.hpp"
#include "acl_cpp/connpool/tcp_manager.hpp"
#endif

namespace acl
{

tcp_manager::tcp_manager(void)
{
}

tcp_manager::~tcp_manager(void)
{
}

connect_pool* tcp_manager::create_pool(const char* addr, size_t count,
	size_t idx)
{
	tcp_pool* pool = NEW tcp_pool(addr, count, idx);
	return pool;
}

} // namespace acl
