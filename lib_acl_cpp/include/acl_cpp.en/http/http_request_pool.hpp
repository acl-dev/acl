#pragma once
#include "../acl_cpp_define.hpp"
#include "../connpool/connect_pool.hpp"

namespace acl {

class sslbase_conf;

/**
 * HTTP client connection pool class. The parent class of this class is connect_pool. This class only needs to implement
 * the virtual function create_connect in the parent class to have the functionality of the connection pool parent class connect_pool.
 * In addition, the connection objects created by this class are http_request objects, so when calling connect_pool::peek,
 * the returned class is http_request. The caller needs to cast the class object returned by peek to http_request
 * class object, then can use all the functionality of the http_request class, where http_request class is a
 * subclass of connect_client
 */
class ACL_CPP_API http_request_pool : public connect_pool {
public:
	/**
	 * Constructor
	 * @param addr {const char*} Server listening address, format: ip:port(domain:port)
	 * @param count {size_t} Maximum connection limit for connection pool, when this value is 0 there is no limit
	 * @param idx {size_t} Index position of this connection pool object in the collection (starting from 0)
	 */
	http_request_pool(const char* addr, size_t count, size_t idx = 0);
	~http_request_pool();

	/**
	 * Call this function to set SSL client mode
	 * @param ssl_conf {sslbase_conf*}
	 */
	void set_ssl(sslbase_conf* ssl_conf);

protected:
	// Base class pure virtual function. After this function returns, the base class sets the network connection and network IO timeout for this connection pool
	virtual connect_client* create_connect();

private:
	sslbase_conf* ssl_conf_;
};

class ACL_CPP_API http_guard : public connect_guard {
public:
	http_guard(http_request_pool& pool);
	~http_guard();
};

} // namespace acl

