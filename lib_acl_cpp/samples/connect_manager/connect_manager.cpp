#include "stdafx.h"
#include "connect_pool.h"
#include "connect_manager.h"

connect_manager::connect_manager()
{
}

connect_manager::~connect_manager()
{
}

acl::connect_pool* connect_manager::create_pool(const char* addr,
	size_t count, size_t idx)
{
	return new connect_pool(addr, count, idx);
}
