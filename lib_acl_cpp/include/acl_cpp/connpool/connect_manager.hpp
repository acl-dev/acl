#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/string.hpp"
#include "../stdlib/locker.hpp"
#include "../stdlib/noncopyable.hpp"
#include <vector>
#include <map>

struct ACL_EVENT;

namespace acl {

class connect_pool;
class connect_monitor;
class thread_pool;

// Internal data structure
struct conns_pools {
	std::vector<connect_pool*> pools;
	size_t  check_next;			// Index for connection check
	size_t  conns_next;			// Next index value to access
	conns_pools() {
		check_next = 0;
		conns_next = 0;
	}
};

struct conn_config {
	string addr;
	size_t max;				// Maximum connections
	size_t min;				// Minimum connections
	int    conn_timeout;
	int    rw_timeout;
	bool   sockopt_timeo;

	conn_config() {
		max           = 0;
		min           = 0;
		conn_timeout  = 5;
		rw_timeout    = 5;
		sockopt_timeo = false;
	}
};

/**
 * Connection pool manager class, responsible for managing and obtaining
 * connection pools.
 */
class ACL_CPP_API connect_manager : public noncopyable {
public:
	connect_manager();
	virtual ~connect_manager();

	/**
	 * Whether connection pools are automatically bound to threads. Mainly used in
	 * coroutine environments. Internal default value is false.
	 * This method should be called before creating connection pools.
	 * @param yes {bool}
	 */
	void bind_thread(bool yes);

	/**
	 * Initialize all server connection pools. This function calls set function to
	 * set each server connection pool.
	 * @param default_addr {const char*} Default server address. When not empty,
	 *  this address will be used when querying internally.
	 * @param addr_list {const char*} List of all servers. When not empty,
	 *  format: IP:PORT:COUNT;IP:PORT:COUNT;IP:PORT;IP:PORT ...
	 *    or  IP:PORT:COUNT,IP:PORT:COUNT,IP:PORT;IP:PORT ...
	 *  e.g.: 127.0.0.1:7777:50;192.168.1.1:7777:10;127.0.0.1:7778
	 * @param count {size_t} When a certain address in addr_list does not have
	 * COUNT information, use this value. When this value is 0, it means no limit
	 * on connection pool size.
	 * @param conn_timeout {int} Connection timeout time (seconds)
	 * @param rw_timeout {int} Network IO timeout time (seconds)
	 * @param sockopt_timeo {bool} Whether to use setsockopt to set read/write
	 * timeout.
	 *  Note: default_addr and addr_list cannot both be empty.
	 */
	void init(const char* default_addr, const char* addr_list,
		size_t count, int conn_timeout = 30, int rw_timeout = 30,
		bool sockopt_timeo = false);

	/**
	* Add client connection pool for a server. This function is called when
	* creating objects. Internal automatic management.
	 * @param addr {const char*} Server address, format: ip:port
	 * Note: When calling this function multiple times, each time a new server
	 * address is added, and it is called in a loop.
	 * @param max {size_t} Connection pool maximum size. When this value is 0, it
	 * means no limit on connection pool size.
	 * @param conn_timeout {int} Connection timeout time (seconds)
	 * @param rw_timeout {int} Network IO timeout time (seconds)
	 * @param sockopt_timeo {bool} Whether to use setsockopt to set read/write
	 * timeout.
	 * @param min {size_t} Minimum connections for this connection pool.
	 */
	void set(const char* addr, size_t max, int conn_timeout = 30,
		int rw_timeout = 30, bool sockopt_timeo = false, size_t min = 0);

	/**
	 * Get connection pool configuration object corresponding to the specified
	 * address.
	 * @param addr {const char*} Target connection pool address.
	 * @param use_first {bool} If target address configuration object does not
	 * exist, whether to use
	 *  the first address configuration object.
	 * @return {const conn_config*} Returns NULL to indicate not found.
	 */
	const conn_config* get_config(const char* addr, bool use_first = false);

	/**
	 * Set retry interval (seconds) after connection pool fails. This function is
	 * called when creating objects.
	 * Internal automatic management.
	 * @param n {int} When value <= 0, connection pool will not retry after
	 * failure.
	 */
	void set_retry_inter(int n);

	/**
	 * Set idle timeout for connections in connection pool.
	 * @param ttl {time_t} Idle timeout for connections. When value < 0, it means
	 * no timeout limit. When == 0, it means immediate timeout. When > 0, it means
	 * connections will be released after this timeout period.
	 */
	void set_idle_ttl(time_t ttl);

	/**
	 * Set interval for automatically checking connections. Default value is 30
	 * seconds.
	 * @param n {int} Interval time.
	 */
	void set_check_inter(int n);

	/**
	 * Remove a connection pool for a certain address from connection pool cluster.
	 * This function is called during object lifecycle management. Generally, it
	 * does not need to be called, because internal automatic management.
	 * @param addr {const char*} Server address (ip:port)
	 */
	void remove(const char* addr);

	/**
	 * Get connection pool for this server based on server address.
	 * @param addr {const char*} redis server address (ip:port)
	 * @param exclusive {bool} Whether to lock connection pool group. When
	 * dynamically managing connection pool cluster, this value should be true.
	 * @param restore {bool} When this method is marked as dead, this parameter
	 * indicates whether to automatically restore it to alive state.
	 * @return {connect_pool*} Returns empty to indicate no such server.
	 */
	connect_pool* get(const char* addr, bool exclusive = true,
		bool restore = false);

	/**
	 * Get a connection pool from connection pool cluster. This function uses
	 * round-robin method to get a server connection pool from connection pool
	 * cluster, thereby ensuring load balancing. This function internally
	 * automatically locks connection pool management.
	 * Additionally, this function is a virtual interface, and subclasses can
	 * implement their own round-robin method.
	 * @return {connect_pool*} Returns a connection pool. Return value will never
	 * be empty.
	 */
	virtual connect_pool* peek();

	/**
	 * Get a connection pool from connection pool cluster. This function uses hash
	 * positioning method to get a server connection pool from cluster.
	 * Subclasses can override this virtual function to implement their own
	 * cluster access method.
	 * This virtual function internally defaults to CRC32 hash algorithm.
	 * @param key {const char*} Key value string. When this value is NULL,
	 * internally automatically switches to round-robin method.
	 * @param exclusive {bool} Whether to lock connection pool group. When
	 * dynamically managing connection pool cluster, this value should be true.
	 * @return {connect_pool*} Returns a connection pool. Return value will never
	 * be empty.
	 */
	virtual connect_pool* peek(const char* key, bool exclusive);

	/**
	 * When users override peek function, they can call this function to lock
	 * connection pool management process.
	 */
	void lock();

	/**
	 * When users override peek function, they can call this function to unlock
	 * connection pool management process.
	 */
	void unlock();

	/**
	 * Get all server connection pools, including default server connection pool.
	 * @return {std::vector<connect_pool*>&}
	 */
	std::vector<connect_pool*>& get_pools();

	/**
	 * Check idle connections in connection pools and release them when timeout.
	 * @param step {size_t} Number of connection pools to check each time.
	 * @param left {size_t*} When not empty, stores total number of remaining
	 * connections.
	 * @return {size_t} Number of idle connections released.
	 */
	size_t check_idle_conns(size_t step, size_t* left = NULL);

	/**
	 * Check abnormal connections in connection pools and close them.
	 * @param step {size_t} Number of connection pools to check each time.
	 * @param left {size_t*} When not empty, stores total number of remaining
	 * connections.
	 * @return {size_t} Number of connections closed.
	 */
	size_t check_dead_conns(size_t step, size_t* left = NULL);

	/**
	 * Maintain minimum connections for all connection pools.
	 * @param step {size_t} Number of connection pools to check each time.
	 * @return {size_t} Total number of remaining connections.
	 */
	size_t keep_min_conns(size_t step);

	/**
	 * Check idle connections in connection pools, release them when timeout, and
	 * maintain minimum connections for each connection pool.
	 * @param step {size_t} Number of connection pools to check each time.
	 * @param check_idle {bool} Whether to release idle connections when timeout.
	 * @param kick_dead {bool} Whether to release dead connections.
	 * @param keep_conns {bool} Whether to maintain minimum connections for each
	 * connection pool.
	 * @param threads {thread_pool*} When not NULL, use this thread pool to handle
	 * kick_dead operation.
	 * @param left {size_t*} When not empty, stores total number of remaining
	 * connections.
	 * @return {size_t} Number of idle connections released.
	 */
	size_t check_conns(size_t step, bool check_idle, bool kick_dead,
		bool keep_conns, thread_pool* threads, size_t* left = NULL);

	/**
	 * Get number of connection pool objects in connection pool cluster.
	 * @return {size_t}
	 */
	size_t size() const;

	/**
	 * Get default server connection pool.
	 * @return {connect_pool*} When init function's default_addr is empty,
	 *  this function returns NULL.
	 */
	connect_pool* get_default_pool() const {
		return default_pool_;
	}

	/**
	 * Print statistics of current redis connection pools.
	 */
	void statistics();

	/**
	 * Start background monitoring thread to monitor connection pool cluster
	 * status.
	 * @param monitor {connect_monitor*} Connection monitor.
	 * @return {bool} Whether monitoring thread was started successfully.
	 * Returning false indicates current process already has a monitoring thread
	 * running. If you need to start again, you need to call stop_monitor first.
	 */
	bool start_monitor(connect_monitor* monitor);

	/**
	 * Stop background monitoring thread.
	 * @param graceful {bool} Whether to wait for all monitoring operations to
	 * close before returning when closing monitoring thread. When connection pool
	 * cluster is destroyed as process space internal cleanup and release,
	 * this value should be set to false so that monitoring thread can exit, and
	 * applications wait for all monitoring connections to close before allowing
	 * monitoring thread to exit.
	 * @return {connect_monitor*} Returns monitor object passed to start_monitor,
	 * and simultaneously internally sets monitor_ member to NULL.
	 */
	connect_monitor* stop_monitor(bool graceful = true);

	/**
	 * Set alive status of a connection pool server. Internal automatic management.
	 * @param addr {const char*} Server address, format: ip:port
	 * @param alive {bool} Whether this server is alive.
	 */
	void set_pools_status(const char* addr, bool alive);

protected:
	/**
	 * Virtual function. Subclasses must implement this function to create
	 * connection pool objects.
	 * @param addr {const char*} Server listening address, format: ip:port
	 * @param count {size_t} Connection pool size limit. When 0, connection pool
	 * has no limit.
	 * @param idx {size_t} Index position of connection pool object in cluster
	 * (starting from 0)
	 * @return {connect_pool*} Returns created connection pool object.
	 */
	virtual connect_pool* create_pool(const char* addr,
		size_t count, size_t idx) = 0;

	////////////////////////////////////////////////////////////////////////

	typedef std::vector<connect_pool*> pools_t;
	typedef pools_t::iterator          pools_it;
	typedef pools_t::const_iterator    pools_cit;

	typedef std::map<unsigned long, conns_pools*> manager_t;
	typedef manager_t::iterator                   manager_it;
	typedef manager_t::const_iterator             manager_cit;

	bool thread_binding_;			// Coroutine environment, each thread binding
	string default_addr_;			// Default server address
	connect_pool* default_pool_;	// Default server connection pool

	std::map<string, conn_config> addrs_;	// All server addresses
	manager_t  manager_;

	locker lock_;				// Lock when accessing pools_
	int  stat_inter_;			// Statistics server timeout interval
	int  retry_inter_;			// Retry interval after connection pool fails
	time_t idle_ttl_;			// Idle timeout for connections
	int  check_inter_;			// Interval for checking connections
	connect_monitor* monitor_;	// Background monitoring thread handle

	void pools_dump(size_t step, std::vector<connect_pool*>& out);
	static size_t pools_release(std::vector<connect_pool*>& pools);

	static size_t check_idle_conns(const std::vector<connect_pool*>& pools);
	static size_t check_dead_conns(const std::vector<connect_pool*>& pools,
		thread_pool* threads = NULL);
	static void keep_min_conns(const std::vector<connect_pool*>& pools,
		thread_pool* threads = NULL);

	// Set server cluster except default server
	void set_service_list(const char* addr_list, int count,
		int conn_timeout, int rw_timeout, bool sockopt_timeo = false);
	conns_pools& get_pools_by_id(unsigned long id);
	connect_pool* create_pool(const conn_config& cf, size_t idx);
	void create_pools_for(pools_t& pools);

	void remove(pools_t& pools, const char* addr);
	void set_status(pools_t& pools, const char* addr, bool alive);

	unsigned long get_id() const;
	void get_key(const char* addr, string& key);
	void get_addr(const char* key, string& addr);
	//connect_pool* add_pool(const char* addr);

	// Callback when thread local storage initializes.
	static void thread_oninit();
	// Callback before thread exits. This function releases internal thread local
	// storage.
	static void thread_onexit(void* ctx);
};

} // namespace acl

