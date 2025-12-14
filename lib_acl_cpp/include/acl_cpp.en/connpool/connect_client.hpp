#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/noncopyable.hpp"

namespace acl {

class connect_pool;

class ACL_CPP_API connect_client : public noncopyable {
public:
	connect_client()
	: conn_timeout_(5)
	, rw_timeout_(5)
	, when_(0)
	, pool_(NULL) {}

	virtual ~connect_client() {}

	/**
	 * Get the timestamp when this connection object was last used
	 * @return {time_t}
	 */
	time_t get_when() const {
		return when_;
	}

	/**
	 * Set the timestamp when this connection object is currently being used
	 */
	void set_when(time_t when) {
		when_ = when;
	}

	/**
	 * Pure virtual function, subclasses must implement this function to connect to the server
	 * @return {bool} Whether the connection was successful
	 */
	virtual bool open() = 0;

	/**
	 * Virtual function, subclasses can implement this method to indicate whether the current connection is normal,
	 * so that the connection pool object can automatically close disconnected connections when detecting connection liveness
	 * @return {bool}
	 */
	virtual bool alive() {
		return true;
	}

	/**
	 * Get the connection pool object reference, created inside connect_pool
	 * The connection object will call set_pool to set the connection pool object handle
	 * @return {connect_pool*}
	 */
	connect_pool* get_pool() const {
		return pool_;
	}

//public:
	/**
	 * Virtual function, this function sets the network connection timeout and network IO timeout,
	 * subclasses can override this virtual function to set internal object timeout
	 * @param conn_timeout {int} Network connection timeout (seconds)
	 * @param rw_timeout {int} Network IO timeout (seconds)
	 */
	virtual void set_timeout(int conn_timeout, int rw_timeout) {
		conn_timeout_ = conn_timeout;
		rw_timeout_   = rw_timeout;
	}

protected:
	int   conn_timeout_;
	int   rw_timeout_;

	friend class connect_pool;

	time_t when_;
	connect_pool* pool_;

	void set_pool(connect_pool* pool) {
		pool_ = pool;
	}
};

} // namespace acl

