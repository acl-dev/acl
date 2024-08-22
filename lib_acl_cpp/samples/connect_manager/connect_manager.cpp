#include "stdafx.h"
#include "connect_pool.h"
#include "connect_manager.h"

connect_manager::connect_manager(size_t min_conns)
: min_conns_(min_conns)
{
}

connect_manager::~connect_manager()
{
}

acl::connect_pool* connect_manager::create_pool(const char* addr,
	size_t max, size_t idx)
{
	acl::connect_pool* pool = new connect_pool(addr, max, idx);
	pool->set_conns_min(min_conns_);
	pool->set_idle_ttl(idle_ttl_);
	return pool;
}
