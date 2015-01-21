#include "acl_stdafx.hpp"
#include "acl_cpp/redis/redis_client.hpp"
#include "acl_cpp/redis/redis_result.hpp"
#include "acl_cpp/redis/redis_script.hpp"

namespace acl
{

redis_script::redis_script(redis_client* conn /* = NULL */)
: redis_command(conn)
{

}

redis_script::~redis_script()
{

}

} // namespace acl
