#pragma once
#include "../acl_cpp_define.hpp"
#include "../connpool/connect_pool.hpp"

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

namespace acl {

class sslbase_conf;

/**
 * Redis connection pool class. This class inherits from connect_pool, which
 * defines common methods
 * related to TCP connection pools.
 * redis connection pool inherting from connect_pool, which includes
 * TCP connection pool methods.
 */
class ACL_CPP_API redis_client_pool : public connect_pool {
public:
	/**
	 * Constructor
	 * constructor
	 * @param addr {const char*} Server address, format: ip:port
	 *  the redis-server's listening address, format: ip:port
	 * @param count {size_t} Maximum connection limit for connection pool. If this
	 * value is 0, the connection pool
	 *  has no upper limit.
	 *  the max connections for each connection pool. there is
	 *  no connections limit of the pool when the count is 0.
	 * @param idx {size_t} Index position of this connection pool object in the
	 * collection (starting from 0)
	 *  the subscript of the connection pool in the connection cluster
	 */
	redis_client_pool(const char* addr, size_t count, size_t idx = 0);

	~redis_client_pool();

	/**
	 * Set SSL communication configuration handle. Internal default value is NULL.
	 * If SSL connection
	 * configuration object is set, internally switches to SSL communication mode
	 * set SSL communication with Redis-server if ssl_conf not NULL
	 * @param ssl_conf {sslbase_conf*}
	 * @return {redis_client_pool&}
	 */
	redis_client_pool& set_ssl_conf(sslbase_conf* ssl_conf);

	/**
	 * Set connection password for connecting to redis server
	 * @param pass {const char*} Connection password
	 * @return {redis_client_pool&}
	 */
	redis_client_pool& set_password(const char* pass);

	/**
	 * In non-cluster mode, this method is used to select the db after connection
	 * is established
	 * in no-cluster mode, the method is used to select the db after
	 * the connection is created
	 * @param dbnum {int}
	 * @return {redis_client_pool&}
	 */
	redis_client_pool& set_db(int dbnum);

	/**
	 * Get the db corresponding to this connection pool
	 * get the current db of the connections pool
	 * @return {int}
	 */
	int get_db() const {
		return dbnum_;
	}

protected:
	/**
	 * Base class pure virtual function: Call this function to create a new
	 * connection
	 * virtual function in class connect_pool to create a new connection
	 * @return {connect_client*}
	 */
	connect_client* create_connect();

private:
	char* pass_;
	int   dbnum_;
	sslbase_conf* ssl_conf_;
};

} // namespace acl

#endif // !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

