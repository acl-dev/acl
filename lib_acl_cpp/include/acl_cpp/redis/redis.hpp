#pragma once
#include "../acl_cpp_define.hpp"
#include "redis_connection.hpp"
#include "redis_hash.hpp"
#include "redis_hyperloglog.hpp"
#include "redis_key.hpp"
#include "redis_list.hpp"
#include "redis_pubsub.hpp"
#include "redis_script.hpp"
#include "redis_server.hpp"
#include "redis_set.hpp"
#include "redis_string.hpp"
#include "redis_transaction.hpp"
#include "redis_zset.hpp"
#include "redis_cluster.hpp"
#include "redis_geo.hpp"

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
	, public redis_geo
{
public:
	/**
	 * 非集群方式的构造函数
	 * the constructor with no redis cluster
	 * @param conn {redis_client*} 一个 redis 节点的连接对象
	 *  one redis node's connection
	 */
	redis(redis_client* conn = NULL);

	/**
	 * 集群方式的构造函数
	 * the constructor in redis cluster mode
	 * @param cluster {redis_client_cluster*} 集群对象
	 *  the redis cluster object
	 * @param max_conns {size_t} 集群方式下连接每个 redis 服务节点的
	 *  连接池连接上限，如果设为 0，则每个连接池没有上限限制
	 *  the limit of each connections pool in redis cluster mode,
	 *  there is no connections limit of each pool if the max_conns
	 *  is set to 0.
	 */
	redis(redis_client_cluster* cluster, size_t max_conns = 0);

	~redis(void) {}
};

} // namespace acl
