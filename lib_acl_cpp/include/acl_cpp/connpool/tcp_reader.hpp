#pragma once
#include "../stdlib/noncopyable.hpp"

namespace acl {

class socket_stream;
class string;

/**
 * tcp ipc communication reader class. Internally automatically reads new data
 * packets.
 */
class ACL_CPP_API tcp_reader : public noncopyable {
public:
	tcp_reader(socket_stream& conn);
	~tcp_reader() {}

	/**
	 * Read data from peer. Each call reads only one data packet.
	 * @param out {string&} Buffer to store data packet. Internally uses append
	 * method, and automatically clears out buffer.
	 */
	bool read(string& out);

	/**
	 * Get connection object.
	 * @return {acl::socket_stream&}
	 */
	acl::socket_stream& get_conn() const {
		return *conn_;
	}

private:
	socket_stream* conn_;
};

} // namespace acl
