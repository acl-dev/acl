#include "acl_stdafx.hpp"
#include "acl_cpp/redis/redis.hpp"

namespace acl
{

redis::redis(redis_client* conn /* = NULL */)
	: redis_connection(conn)
	, redis_hash(conn)
	, redis_hyperloglog(conn)
	, redis_key(conn)
	, redis_list(conn)
	, redis_pubsub(conn)
	, redis_script(conn)
	, redis_server(conn)
	, redis_set(conn)
	, redis_string(conn)
	, redis_transaction(conn)
	, redis_zset(conn)
{

}

redis::~redis()
{

}

} // namespace acl
