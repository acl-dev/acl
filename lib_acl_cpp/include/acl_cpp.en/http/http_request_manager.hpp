#pragma once
#include "../acl_cpp_define.hpp"
#include "../connpool/connect_manager.hpp"

namespace acl
{

class sslbase_conf;

/**
 * HTTP client request connection pool management class
 */
class ACL_CPP_API http_request_manager : public acl::connect_manager {
public:
	http_request_manager();
	virtual ~http_request_manager();

	/**
	 * Call this function to set SSL client mode
	 * @param ssl_conf {sslbase_conf*}
	 */
	void set_ssl(sslbase_conf* ssl_conf);

protected:
	/**
	 * Base class pure virtual function, used to create connection pool objects. After this function returns,
	 * the base class sets the network connection and network IO timeout for the connection pool
	 * @param addr {const char*} Server listening address, format: ip:port
	 * @param count {size_t} Connection pool size limit, when this value is 0 there is no limit
	 * @param idx {size_t} Index position of this connection pool object in the collection (starting from 0)
	 * @return {connect_pool*} Returns the created connection pool object
	 */
	connect_pool* create_pool(const char* addr, size_t count, size_t idx);

private:
	sslbase_conf* ssl_conf_;
};

} // namespace acl
