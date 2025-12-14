#pragma once
#include "../acl_cpp_define.hpp"
#include <list>
#include "../stdlib/locker.hpp"
#include "../stdlib/noncopyable.hpp"

namespace acl {

class connect_manager;
class connect_client;
class thread_pool;

/**
 * When calling connect_pool::put(), the oper parameter can be:
 */
typedef enum {
	cpool_put_oper_none  = 0x00,	// No operation
	cpool_put_check_idle = 0x01,	// Check and close timeout connections
	cpool_put_check_dead = 0x02,	// Check and close abnormal connections
	cpool_put_keep_conns = 0x04,	// Maintain minimum number of connections
} cpool_put_oper_t;

/**
 * Client connection pool class, implements dynamic management of connection pool. This is a base class and needs to implement
 * the virtual function create_connect to create a connection for the server. This class object
 * can be dynamically allocated through set_delay_destroy() to delay destruction time, which is actually
 * dynamic allocation.
 */
class ACL_CPP_API connect_pool : public noncopyable {
public:
	/**
	 * Constructor
	 * @param addr {const char*} Server listening address, format: ip:port(domain:port)
	 * @param max {size_t} Maximum number of connections in connection pool limit. When this value is 0, there is
	 *  no limit on the connection pool's connection count.
	 * @param idx {size_t} Index position of this connection pool object in the collection (starting from 0)
	 */
	connect_pool(const char* addr, size_t max, size_t idx = 0);

	/**
	 * Destructor. When destroying, it should be dynamically allocated.
	 */
	virtual ~connect_pool();

	/**
	 * This interface is used to set timeout time.
	 * @param conn_timeout {int} Network connection timeout time (seconds)
	 * @param rw_timeout {int} Network I/O timeout time (seconds)
	 * @param sockopt_timo {bool} Whether to use setsockopt to set read/write timeout
	 */
	connect_pool& set_timeout(int conn_timeout, int rw_timeout,
		bool sockopt_timo = false);

	/**
	 * Set connection pool minimum connection count. When check_idle/check_dead operations are performed, maintain minimum connections.
	 * @param min {size_t} When > 0, automatically maintain minimum connections.
	 * @return {connect_pool&}
	 */
	connect_pool& set_conns_min(size_t min);

	/**
	 * Set connection pool abnormal connection retry interval time.
	 * @param retry_inter {int} Retry interval time (seconds) after connection is disconnected before reconnecting.
	 *  When this value <= 0, it means do not retry after connection is disconnected. When connection pool is in abnormal state and timeout time
	 *  expires, it will retry to connect. When this parameter is not set, the internal default value is 1 second.
	 * @return {connect_pool&}
	 */
	connect_pool& set_retry_inter(int retry_inter);

	/**
	 * Set idle connection survival period in connection pool.
	 * @param ttl {time_t} Idle connection survival period. When return value < 0, it means idle connections never expire;
	 *  == 0 means never expire; > 0 means will be released after this time period.
	 * @return {connect_pool&}
	 */
	connect_pool& set_idle_ttl(time_t ttl);

	/**
	 * Set automatic connection check time interval. This only affects the check time when calling put function each time.
	 * @param n {int} Time interval, default value is 30 seconds. When calling put function, idle connection check,
	 *  the larger the value, the less frequent. Set to -1 to disable, set to 0 to check every time.
	 * @return {connect_pool&}
	 */
	connect_pool& set_check_inter(int n);

	/**
	 * Get a connection from the connection pool. If there are no available connections, the last returned connection is abnormal
	 * and has not been closed, or the connection pool connection count reaches the maximum limit, NULL will be returned. Otherwise, a
	 * new connection will be created. If connection creation times out, the connection pool will be marked as abnormal state.
	 * @param on {bool} This parameter controls whether to create a new connection when the connection pool has no available connections.
	 *  If false, no new connection will be created.
	 * @param tc {double*} When not empty, stores total timeout time, unit is ms.
	 * @param old {bool*} When not empty, indicates whether the obtained connection is an old connection in the connection pool.
	 *  If *old is true, it means an old connection was used; otherwise, it's a new connection.
	 * @return {connect_client*} Returns NULL to indicate this connection pool object is not available.
	 */
	connect_client* peek(bool on = true, double* tc = NULL, bool* old = NULL);

	/**
	 * Bind a connection object in the current connection pool to the current connection pool object, making it belong to the current
	 * connection pool object.
	 * @param conn {redis_client*}
	 */
	void bind_one(connect_client* conn);

	/**
	 * Release a connection back to the connection pool. If the connection pool corresponding server is not available, or the user does not want to keep
	 * the connection, the connection will be directly released.
	 * @param conn {redis_client*}
	 * @param keep {bool} Whether to keep this connection alive.
	 * @param oper {cpool_put_oper_t} Automatic connection pool flag bit combination, see the
	 *  cpool_put_oper_t type definition above.
	 */
	void put(connect_client* conn, bool keep = true,
		 cpool_put_oper_t oper = cpool_put_check_idle);

	/**
	 * Check idle connections in the connection pool and release expired ones.
	 * @param ttl {time_t} When value >= 0, connections exceeding this time period will be closed.
	 * @param exclusive {bool} Whether internal lock is needed.
	 * @return {size_t} Returns the number of idle connections released.
	 */
	size_t check_idle(time_t ttl, bool exclusive = true);
	size_t check_idle(bool exclusive = true);

	/**
	 * Check connection status and close disconnected connections. Internal automatic locking.
	 * @param threads {thread_pool*} When not empty, use this thread pool to check connection status for better efficiency.
	 * @return {size_t} Number of connections closed.
	 */
	size_t check_dead(thread_pool* threads = NULL);

	/**
	 * Maintain minimum connections set by set_conns_min().
	 * @param threads {thread_pool*} When not empty, use this thread pool to maintain minimum connections for better efficiency.
	 */
	void keep_conns(thread_pool* threads = NULL);

	/**
	 * Set connection pool's alive state.
	 * @param ok {bool} Set whether this connection pool is alive.
	 */
	void set_alive(bool ok /* true | false */);

	/**
	 * Check whether the connection pool is alive. When the connection pool is in abnormal state, call this function to check whether the connection pool should
	 * automatically recover. If recovery is indicated, this connection pool will be set to alive state.
	 * @return {bool} Returns true to indicate the current connection pool is in alive state, false indicates the current
	 *  connection pool is not alive.
	 */
	bool aliving();

	/**
	 * Get connection pool's server address.
	 * @return {const char*} Returns non-empty address.
	 */
	const char* get_addr() const {
		return addr_;
	}

	/**
	 * Get connection pool's maximum connection limit. When this value is 0, it means there is no maximum connection limit.
	 * @return {size_t}
	 */
	size_t get_max() const {
		return max_;
	}

	/**
	 * Get connection pool's current connection count.
	 * @return {size_t}
	 */
	size_t get_count() const {
		return count_;
	}

	/**
	 * Get this connection pool object's index position in the connection pool collection.
	 * @return {size_t}
	 */
	size_t get_idx() const {
		return idx_;
	}

	/**
	 * Reset statistics counter.
	 * @param inter {int} Statistics time interval.
	 */
	void reset_statistics(int inter);

	/**
	 * Get total number of times this connection pool has been used.
	 */
	unsigned long long get_total_used() const {
		return total_used_;
	}

	/**
	 * Get current usage count accumulation value of this connection pool. This value will be reset when reset_statistics is called.
	 * Different from get_total_used(), this value will be reset.
	 * @return {unsigned long long}
	 */
	unsigned long long get_current_used() const {
		return current_used_;
	}

public:
	void set_key(const char* key);
	const char* get_key() const {
		return key_;
	}

	// Increase reference count.
	void refer();

	// Decrease reference count.
	void unrefer();

protected:
	/**
	 * Pure virtual function, needs to be implemented.
	 * @return {connect_client*}
	 */
	virtual connect_client* create_connect() = 0;

	friend class connect_manager;

	/**
	 * Set this connection pool object to delay destruction. When internal reference count is 0, it will be destroyed.
	 */
	void set_delay_destroy();

protected:
	bool  alive_;				// Whether alive
	ssize_t refers_;			// Current connection pool object reference count
	bool  delay_destroy_;			// Whether to delay connection pool destruction
	// When the server corresponding to this connection pool is unavailable, the connection pool object will be retried after this time interval.
	int   retry_inter_;
	time_t last_dead_;			// Timestamp when connection pool object was last detected as dead.

	char  key_[256];			// Key for this connection pool cache
	char  addr_[256];			// Server address corresponding to connection pool, IP:PORT
	int   conn_timeout_;			// Network connection timeout time (seconds)
	int   rw_timeout_;			// Network I/O timeout time (seconds)
	bool  sockopt_timo_;			// Whether to use setsockopt to set timeout
	size_t idx_;				// Index position of this connection pool object in the collection
	size_t max_;				// Maximum connections
	size_t min_;				// Minimum connections
	size_t count_;				// Current connection count
	time_t idle_ttl_;			// Idle connection survival period
	time_t last_check_;			// Timestamp of last check for idle connections
	int   check_inter_;			// Time interval for checking idle connections

	locker lock_;				// Lock when operating pool_
	unsigned long long total_used_;		// Total usage count of this connection pool
	unsigned long long current_used_;	// Usage count within a certain time period
	time_t last_;				// Timestamp of last record
	std::list<connect_client*> pool_;	// Connection pool collection

	size_t check_dead(size_t count);
	size_t check_dead(size_t count, thread_pool& threads);
	void keep_conns(size_t min);
	void keep_conns(size_t min, thread_pool& threads);

	size_t kick_idle_conns(time_t ttl);	// Close expired connections
	connect_client* peek_back();		// Peek from tail
	void put_front(connect_client* conn);	// Put at head

	void count_inc(bool exclusive);		// Increase connection count and reference count
	void count_dec(bool exclusive);		// Decrease connection count and reference count
};

class ACL_CPP_API connect_guard : public noncopyable {
public:
	explicit connect_guard(connect_pool& pool)
	: keep_(true), pool_(pool), conn_(NULL)
	{
	}

	virtual ~connect_guard() {
		if (conn_) {
			pool_.put(conn_, keep_);
		}
	}

	void set_keep(bool keep) {
		keep_ = keep;
	}

	connect_client* peek() {
		conn_ = pool_.peek();
		return conn_;
	}

protected:
	bool keep_;
	connect_pool& pool_;
	connect_client* conn_;
};

} // namespace acl

