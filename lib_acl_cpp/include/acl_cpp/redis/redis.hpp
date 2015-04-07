#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include "acl_cpp/redis/redis_connection.hpp"
#include "acl_cpp/redis/redis_hash.hpp"
#include "acl_cpp/redis/redis_hyperloglog.hpp"
#include "acl_cpp/redis/redis_key.hpp"
#include "acl_cpp/redis/redis_list.hpp"
#include "acl_cpp/redis/redis_pubsub.hpp"
#include "acl_cpp/redis/redis_script.hpp"
#include "acl_cpp/redis/redis_server.hpp"
#include "acl_cpp/redis/redis_set.hpp"
#include "acl_cpp/redis/redis_string.hpp"
#include "acl_cpp/redis/redis_transaction.hpp"
#include "acl_cpp/redis/redis_zset.hpp"
#include "acl_cpp/redis/redis_cluster.hpp"

namespace acl
{

/**
 * 该类继承了所有 redis 命令类，因此可以只通过此类对象使用所有的 redis 命令。
 * inherit all the redis command class, which include all the commands
 * of Key, String, Hash, List, Set, SortedSet, Hyperloglog, Pub/Sub,
 * Transaction, Script, Connection, Server.
 */
class ACL_CPP_API redis
	: public redis_connection
	, public redis_hash
	, public redis_hyperloglog
	, public redis_key
	, public redis_list
	, public redis_pubsub
	, public redis_script
	, public redis_server
	, public redis_set
	, public redis_string
	, public redis_transaction
	, public redis_zset
	, public redis_cluster
{
public:
	redis(redis_client* conn = NULL);
	redis(redis_client_cluster* cluster, size_t max_conns);
	~redis() {}
};

} // namespace acl
