#include "acl_stdafx.hpp"
#include "acl_cpp/redis/redis_client.hpp"
#include "acl_cpp/redis/redis_zset.hpp"

namespace acl
{

redis_zset::redis_zset(redis_client* conn /* = NULL */)
: redis_command(conn)
{

}

redis_zset::~redis_zset()
{

}

} // namespace acl
