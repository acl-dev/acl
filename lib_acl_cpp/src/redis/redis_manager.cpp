#include "acl_stdafx.hpp"
#include "acl_cpp/redis/redis_pool.hpp"
#include "acl_cpp/redis/redis_manager.hpp"

namespace acl
{

redis_manager::redis_manager()
{

}

redis_manager::~redis_manager()
{

}

connect_pool* redis_manager::create_pool(const char* addr, int count, size_t idx)
{
	redis_pool* pool = NEW redis_pool(addr, count, idx);
	return pool;
}

} // namespace acl
