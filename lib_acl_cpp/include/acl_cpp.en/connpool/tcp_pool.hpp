#pragma once
#include "../acl_cpp_define.hpp"
#include "connect_pool.hpp"

namespace acl {

class string;
class connect_client;

class ACL_CPP_API tcp_pool : public connect_pool {
public:
	tcp_pool(const char* addr, size_t count, size_t idx = 0);
	virtual ~tcp_pool(void);

	/**
	 * Send a data packet of specified length to the server. This method will
	 * automatically get a connection from the connection pool for sending
	 * @param data {const void*} Address of the data packet to send
	 * @param len {unsigned int} Data length
	 * @param out {string*} When this object is not NULL, it indicates that
	 * response data needs to be read from the server.
	 * The response result will be stored in this buffer. If this object is NULL,
	 * it means there is no need to read the server's response data
	 * @return {bool} Whether the send was successful
	 */
	bool send(const void* data, unsigned int len, string* out = NULL);

protected:
	// @override
	virtual connect_client* create_connect();
};

} // namespace acl

