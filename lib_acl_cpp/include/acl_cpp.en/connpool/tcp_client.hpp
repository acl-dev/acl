#pragma once
#include "../acl_cpp_define.hpp"
#include "connect_client.hpp"

namespace acl {

class socket_stream;
class tcp_sender;
class tcp_reader;
class string;

class ACL_CPP_API tcp_client : public connect_client {
public:
	tcp_client(const char* addr, int conn_timeout = 10, int rw_timeout = 10);
	virtual ~tcp_client();

	/**
	 * Send a data packet of specified length to the server
	 * @param data {const void*} Address of the data packet to send
	 * @param len {unsigned int} Data length
	 * @param out {string*} When this object is not NULL, it indicates that response data needs to be read from the server.
	 *  The response result will be stored in this buffer. If this object is NULL, it means there is no need to read
	 *  the server's response data
	 * @return {bool} Whether the send was successful
	 */
	bool send(const void* data, unsigned int len, string* out = NULL);

protected:
	// @override
	virtual bool open();

private:
	char* addr_;
	int   conn_timeout_;
	int   rw_timeout_;

	socket_stream* conn_;
	tcp_sender*    sender_;
	tcp_reader*    reader_;

	bool try_open(bool* reuse_conn);
};

} // namespace acl
