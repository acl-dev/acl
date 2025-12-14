#pragma once
#include "../acl_cpp_define.hpp"
#include <vector>
#include <map>
#include "../stdlib/string.hpp"
#include "../connpool/connect_manager.hpp"

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

namespace acl {

class sslbase_conf;

class redis_client_pool;

/**
 * Redis client cluster class. By registering objects of this class into redis client command class (redis_command),
 * all client commands automatically support cluster version redis commands.
 * redis client cluster class. The class's object is set in the redis_command
 * using redis_command::set_cluster(redis_cluster*), and all the redis client
 * command will support the redis cluster mode.
 */
class ACL_CPP_API redis_client_cluster : public connect_manager {
public:
	/**
	 * Constructor;
	 * constructor
	 * @param max_slot {int} Maximum hash slot value;
	 * the max hash-slot value of keys
	 */
	explicit redis_client_cluster(int max_slot = 16384);
	~redis_client_cluster();

	/**
	 * Get corresponding connection pool based on hash slot value;
	 * get one connection pool with the given slot
	 * @param slot {int} Hash slot value;
	 *  the hash-slot value of key
	 * @return {redis_client_pool*} Returns NULL if corresponding hash slot does not exist;
	 *  return the connection pool of the hash-slot, and return NULL
	 *  when the slot not exists
	 */
	redis_client_pool* peek_slot(int slot);

	/**
	 * Dynamically set redis service address corresponding to hash slot value. When this function is called, internally has thread lock protection;
	 * dynamicly set redis-server addr with one slot, which is protected
	 * by thread mutex internal, no one will be set if the slot were
	 * beyyond the max hash-slot
	 * @param slot {int} Hash slot value;
	 *  the hash-slot
	 * @param addr {const char*} Redis server address;
	 *  one redis-server addr
	 */
	void set_slot(int slot, const char* addr);

	/**
	 * Given a node in redis cluster, automatically discover node addresses corresponding to all hash slots
	 * according one node of the cluster, auto find all nodes with all
	 * slots range
	 * @param addr {const char*} Address of a service node in cluster, format ip:port
	 *  on server node's addr of the cluster, addr format is "ip:port"
	 * @param max_conns {size_t} Maximum connection limit for connection pool built with each node in cluster
	 *  the max connections limit for each connection pool
	 * @param conn_timeout {int} Connection timeout
	 *  set the connection timeout
	 * @param rw_timeout {int} IO read/write timeout
	 *  set the network io timeout
	 */
	void set_all_slot(const char* addr, size_t max_conns,
		int conn_timeout = 30, int rw_timeout = 30);

	/**
	 * Dynamically clear redis service address corresponding to hash slot, to facilitate recalculating position.
	 * Internally has thread lock protection mechanism;
	 * dynamicly remove one slot and redis-server addr mapping, which is
	 * protected by thread mutex
	 * @param slot {int} Hash slot value;
	 *  hash-slot value
	 */
	void clear_slot(int slot);

	/**
	 * Get maximum hash slot value;
	 * get the max hash-slot
	 * @return {int}
	 */
	int get_max_slot() const {
		return max_slot_;
	}

	//////////////////////////////////////////////////////////////////////
	/**
	 * Set threshold for protocol redirect count. Default value is 15;
	 * set redirect limit for MOVE/ASK, default is 15
	 * @param max {int} Redirect count threshold. Only effective when this value > 0;
	 *  the redirect times limit for MOVE/ASK commands
	 */
	void set_redirect_max(int max);

	/**
	 * Set threshold for protocol redirect count;
	 * get redirect limit of MOVE/ASK commands in one redis redirect process
	 * @return {int}
	 */
	int get_redirect_max() const {
		return redirect_max_;
	}

	/**
	 * Allowed sleep time (milliseconds) when redirect count >= 2. Default value is 100 milliseconds. Benefit of this is
	 * when a redis service master node goes offline, it takes time for other slave nodes to upgrade to master node (determined
	 * by cluster-node-timeout configuration item in redis.conf). So to avoid errors within redirect count range, need to wait
	 * for slave nodes to upgrade to master node;
	 * if redirect happenning more than 2 in one redis command process,
	 * the process can sleep for a one avoiding redirect too fast, you
	 * can set the waiting time with microsecond here, default value is
	 * 100 microseconds; this only happends when redis-server died.
	 * @param n {int} Rest time during each redirect (milliseconds), default value is 100 milliseconds;
	 * microseonds to sleep when redirect times are more than 2,
	 * default is 100 ms
	 */
	void set_redirect_sleep(int n);

	/**
	 * Get time set by set_redirect_sleep or default time;
	 * get sleep time set by set_redirect_sleep function
	 * @return {int} Unit is milliseconds;
	 *  return sleep value in microsecond
	 */
	int get_redirect_sleep() const {
		return redirect_sleep_;
	}

	/**
	 * Set SSL communication configuration handle. Internal default value is NULL. If SSL connection
	 * configuration object is set, internally switches to SSL communication mode
	 * set SSL communication with Redis-server if ssl_conf not NULL
	 * @param ssl_conf {sslbase_conf*}
	 * @return {redis_client_cluster&}
	 */
	redis_client_cluster& set_ssl_conf(sslbase_conf* ssl_conf);

	/**
	 * Set connection password for a redis service
	 * set the password of one redis-server
	 * @param addr {const char*} Address of a specified redis server. When value of this parameter is
	 *  default, sets default connection password for all redis servers in cluster
	 *  the specified redis-server's addr, the default password of all
	 *  redis-server will be set when the addr's value is 'default'
	 * @param pass {const char*} Connection password of specified redis server
	 *  the password of the specified redis-server
	 * @return {redis_client_cluster&}
	 */
	redis_client_cluster& set_password(const char* addr, const char* pass);

	/**
	 * Get mapping table of service nodes and connection passwords in redis cluster
	 * get all passwords of the redis cluster
	 * @return {const std::map<string, string>&}
	 */
	const std::map<string, string>& get_passwords() const {
		return passwds_;
	}

	/**
	 * Get connection password of redis node at given address. Returns NULL indicates not set
	 * get the connection password of the specified addr for one redis,
	 * NULL will be returned if password wasn't set
	 * @param addr {const char*}
	 * @return {const char*} return the specified node's connection password,
	 *  NULL returned if no password been set
	 */
	const char* get_password(const char* addr) const;

	/**
	 * Redirect to target redis node
	 * @param addr {const char*} Target redis service address
	 * @param max_conns {size_t} Maximum connection count in connection pool
	 * @return {redis_client*} Get connection communication object with target redis node
	 */
	redis_client* redirect(const char* addr, size_t max_conns);

	/**
	 * Get connection object based on slot number of redis cluster
	 * @param slot {int} Storage slot number corresponding to redis cluster key value
	 * @return {redis_client*} Get connection communication object with target redis node
	 */
	redis_client* peek_conn(int slot);

protected:
	/**
	 * Base class pure virtual function, used to create connection pool object. After this function returns, base class sets network connection and IO timeout
	 * virtual function of base class, which is used to create
	 * the connection pool
	 * @param addr {const char*} Server listening address, format: ip:port;
	 * the server addr for the connection pool, such as ip:port
	 * @param count {size_t} Size limit of connection pool. When this value is 0, there is no limit
	 * the max connections in one connection pool, if it's 0 there
	 * is no limit of the connections pool.
	 * @param idx {size_t} Index position of this connection pool object in collection (starts from 0);
	 * the index of the connection pool in pool array
	 */
	connect_pool* create_pool(const char* addr, size_t count, size_t idx);

private:
	int max_slot_;
	const char** slot_addrs_;
	std::vector<char*> addrs_;
	int redirect_max_;
	int redirect_sleep_;
	std::map<string, string> passwds_;
	sslbase_conf* ssl_conf_;

	redis_client* reopen(redis_command& cmd, redis_client* conn);
	redis_client* move(redis_command& cd, redis_client* conn, 
			const char* ptr, int nretried);
	redis_client* ask(redis_command& cd, redis_client* conn, 
			const char* ptr, int nretried);
	redis_client* cluster_down(redis_command& cd, redis_client* conn, 
			const char* ptr, int nretried);

public:
	const redis_result* run(redis_command& cmd, size_t nchild, int* timeout = NULL);
};

} // namespace acl

#endif // !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

