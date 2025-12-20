#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/noncopyable.hpp"

struct iovec;

namespace acl {

class socket_stream;

/**
 * TCP IPC communication sender class, internally automatically packages data
 */
class ACL_CPP_API tcp_sender : public noncopyable {
public:
	tcp_sender(socket_stream& conn);
	~tcp_sender();

	/**
	 * Send method
	 * @param data {const void*} Address of the data packet to send
	 * @param len {unsigned int} Data packet length
	 * @return {bool} Whether the send was successful
	 */
	bool send(const void* data, unsigned int len);

	/**
	 * Get the connection stream object
	 * @return {acl::socket_stream&}
	 */
	acl::socket_stream& get_conn() const {
		return *conn_;
	}

private:
	acl::socket_stream* conn_;
	struct iovec* v2_;
};

} // namespace acl

