#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/redis/redis.hpp"
#endif

namespace acl
{

redis::redis(redis_client* conn /* = NULL */)
	: redis_command(conn)
	, redis_connection(conn)
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
	, redis_cluster(conn)
{
}

redis::redis(redis_client_cluster* cluster, size_t max_conns /* = 0 */)
	: redis_command(cluster, max_conns)
	, redis_connection(cluster, max_conns)
	, redis_hash(cluster, max_conns)
	, redis_hyperloglog(cluster, max_conns)
	, redis_key(cluster, max_conns)
	, redis_list(cluster, max_conns)
	, redis_pubsub(cluster, max_conns)
	, redis_script(cluster, max_conns)
	, redis_server(cluster, max_conns)
	, redis_set(cluster, max_conns)
	, redis_string(cluster, max_conns)
	, redis_transaction(cluster, max_conns)
	, redis_zset(cluster, max_conns)
	, redis_cluster(cluster, max_conns)
{
}

} // namespace acl
