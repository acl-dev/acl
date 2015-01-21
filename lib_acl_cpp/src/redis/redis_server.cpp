#include "acl_stdafx.hpp"
#include "acl_cpp/redis/redis_client.hpp"
#include "acl_cpp/redis/redis_result.hpp"
#include "acl_cpp/redis/redis_server.hpp"

namespace acl
{

redis_server::redis_server(redis_client* conn /* = NULL */)
: redis_command(conn)
{

}

redis_server::~redis_server()
{

}

} // namespace acl
