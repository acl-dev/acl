#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/thread.hpp"
#include "../stream/aio_handle.hpp"
#include <vector>

namespace acl {

class aio_handle;
class check_client;
class connect_manager;
class rpc_service;
class socket_stream;
class aio_socket_stream;
class thread_pool;

class ACL_CPP_API connect_monitor : public thread {
public:
	/**
	 * Constructor
	 * @param manager {connect_manager&}
	 * @param check_server {bool} Whether to check server service availability and put unused service addresses into blacklist
	 */
	connect_monitor(connect_manager& manager, bool check_server = false);

	virtual ~connect_monitor();

	/**
	 * When want to use blocking detection for server connections, need to call this function first to open
	 * acl::rpc_service blocking interface processing service. If this function is not called during initialization,
	 * uses non-blocking method for IO detection
	 * @param max_threads {int} Maximum number of threads running in rpc_service service thread pool
	 * @param addr {const char*} Local address where rpc_service service is expected to listen, can
	 *  be local loopback address or use domain socket address on UNIX platform
	 * @return {connect_monitor&}
	 */
	connect_monitor& open_rpc_service(int max_threads,
		const char* addr = NULL);

	/**
	 * Set time interval for starting detection timer
	 * @param n {int} Time interval (seconds)
	 * @return {connect_mointor&}
	 */
	connect_monitor& set_check_inter(int n);

	/**
	 * Set timeout for connecting to detected server
	 * @param n {int} Timeout (seconds)
	 * @return {connect_monitor&}
	 */
	connect_monitor& set_conn_timeout(int n);

	/**
	 * Whether to check server service availability and put unused service addresses into blacklist
	 * @return {bool}
	 */
	bool check_server_on() const {
		return check_server_;
	}

	/**
	 * Set whether to automatically close expired idle connections after connection monitor starts
	 * @param check_idle {bool} Whether to automatically close expired idle connections
	 * @param kick_dead {bool} Whether to check liveness status of all connections and close abnormal connections. When this parameter
	 *  is true, connect_client subclasses must override alive() virtual method, returning whether connection is alive
	 * @param keep_conns {bool} Whether to try to maintain minimum number of connections in each connection pool
	 * @param threads {thread_pool*} When thread pool is not empty, will be used to improve concurrent processing capability
	 * @param step {bool} Number of connection pools to check each time
	 * @return {connect_monitor&}
	 */
	connect_monitor& set_check_conns(bool check_idle, bool kick_dead,
		bool keep_conns, thread_pool* threads = NULL, size_t step = 0);

	/**
	 * Whether to automatically detect and close expired idle connections
	 * @return {bool}
	 */
	bool check_idle_on() const {
		return check_idle_;
	}

	/**
	 * Whether to check abnormal connections and close them
	 * @return {bool}
	 */
	bool kick_dead_on() const {
		return kick_dead_;
	}

	/**
	 * Whether to try to maintain minimum number of connections in each connection pool
	 * @return {bool}
	 */
	bool keep_conns_on() const {
		return keep_conns_;
	}

	/**
	 * When check_idle_on() returns true, returns connection count limit checked each time
	 * @return {size_t}
	 */
	size_t get_check_step() const {
		return check_step_;
	}

	/**
	 * Get thread pool object set earlier
	 * @return {thread_pool*}
	 */
	thread_pool* get_threads() const {
		return threads_;
	}

	/**
	 * Stop detection thread
	 * @param graceful {bool} Whether to gracefully close detection process. If true,
	 *  will wait for all detection connections to close before detection thread returns. Otherwise, detection thread
	 *  directly returns, may cause some connections being detected to not be released. Because of this, if
	 *  connection pool cluster management object is process-wide global, can set this parameter to false. If
	 *  connection pool cluster management object needs to be created and released multiple times during runtime, should set to true
	 */
	void stop(bool graceful);

	/**
	 * Get connect_manager reference object
	 * @return {connect_manager&}
	 */
	connect_manager& get_manager() const {
		return manager_;
	}

	/**
	 * Virtual function. Subclasses can override this function to further determine whether connection is alive. This callback
	 * function runs in current non-blocking detection thread's running space, so there must not be blocking processes in this callback function,
	 * otherwise will block entire non-blocking detection thread
	 * @param checker {check_client&} Check object for server connection. Can use methods in
	 *  check_client class as follows:
	 *  1) get_conn to get non-blocking connection handle
	 *  2) get_addr to get server address
	 *  3) set_alive to set whether connection is alive
	 *  4) close to close connection
	 */
	virtual void nio_check(check_client& checker, aio_socket_stream& conn);

	/**
	 * Synchronous IO detection virtual function. This function runs in a child thread's space in thread pool. Subclasses can
	 * override this function to detect actual application's network connection liveness status. Can have blocking
	 * IO processes in this function
	 * @param checker {check_client&} Check object for server connection
	 *  Methods allowed to call in check_client class:
	 *  1) get_addr to get server address
	 *  2) set_alive to set whether connection is alive
	 *  Methods prohibited to call in check_client class:
	 *  1) get_conn to get non-blocking connection handle
	 *  2) close to close connection
	 */
	virtual void sio_check(check_client& checker, socket_stream& conn);

	/**
	 * Callback method when connection succeeds. Subclasses can implement this method
	 * @param cost {double} Time interval from initiating connection request to timeout (seconds)
	 */
	virtual void on_connected(const check_client&, double cost) {
		(void) cost;
	}

	/**
	 * Callback method when connection times out. Subclasses can implement this method
	 * @param addr {const char*} Detected server address, format: ip:port
	 * @param cost {double} Time interval from initiating connection request to timeout (seconds)
	 */
	virtual void on_timeout(const char* addr, double cost) {
		(void) addr;
		(void) cost;
	}

	/**
	 * Callback method when connection to server is refused. Subclasses can implement this method
	 * @param addr {const char*} Detected server address, format: ip:port
	 * @param cost {double} Time interval from initiating connection request to disconnection (seconds)
	 */
	virtual void on_refused(const char* addr, double cost) {
		(void) addr;
		(void) cost;
	}

public:
	// Although following functions are public, they are only for internal use
	/**
	 * Called when connection with server is established
	 * @param checker {check_client&}
	 */
	void on_open(check_client& checker);

protected:
	// Base class pure virtual function
	virtual void* run();

private:
	bool stop_;
	bool stop_graceful_;
	aio_handle handle_;			// Non-blocking handle of background detection thread
	connect_manager& manager_;		// Connection pool collection management object
	bool check_server_;			// Whether to check server availability
	int   check_inter_;			// Time interval for checking connection pool status (seconds)
	int   conn_timeout_;			// Timeout for connecting to server
	bool  check_idle_;			// Whether to detect and close expired idle connections
	bool  kick_dead_;			// Whether to delete abnormal connections
	bool  keep_conns_;			// Whether to maintain minimum number of connections in each connection pool
	size_t check_step_;			// Limit on number of connection pools checked each time
	thread_pool* threads_;			// When thread pool is not empty, will concurrently execute tasks
	rpc_service* rpc_service_;		// Asynchronous RPC communication service handle
};

} // namespace acl

