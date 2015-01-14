#include "acl_stdafx.hpp"
#include "acl_cpp/redis/redis_client.hpp"
#include "acl_cpp/redis/redis_list.hpp"

namespace acl
{

redis_list::redis_list(redis_client& conn)
: conn_(conn)
{

}

redis_list::~redis_list()
{

}

} // namespace acl
