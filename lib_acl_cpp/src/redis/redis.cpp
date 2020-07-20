#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/redis/redis.hpp"
#endif

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

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

redis::redis(redis_client_cluster* cluster)
: redis_command(cluster)
, redis_connection(cluster)
, redis_hash(cluster)
, redis_hyperloglog(cluster)
, redis_key(cluster)
, redis_list(cluster)
, redis_pubsub(cluster)
, redis_script(cluster)
, redis_server(cluster)
, redis_set(cluster)
, redis_string(cluster)
, redis_transaction(cluster)
, redis_zset(cluster)
, redis_cluster(cluster)
{
}

redis::redis(redis_client_cluster* cluster, size_t)
: redis_command(cluster)
, redis_connection(cluster)
, redis_hash(cluster)
, redis_hyperloglog(cluster)
, redis_key(cluster)
, redis_list(cluster)
, redis_pubsub(cluster)
, redis_script(cluster)
, redis_server(cluster)
, redis_set(cluster)
, redis_string(cluster)
, redis_transaction(cluster)
, redis_zset(cluster)
, redis_cluster(cluster)
{
}

} // namespace acl

#endif // ACL_CLIENT_ONLY
