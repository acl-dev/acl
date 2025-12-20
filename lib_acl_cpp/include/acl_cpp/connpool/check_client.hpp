#pragma once
#include "../acl_cpp_define.hpp"
#include <map>
#include <vector>
#if !defined(_WIN32) && !defined(_WIN64)
#include <sys/time.h>
#endif
#include "../stream/aio_socket_stream.hpp"                                
#include "../stdlib/string.hpp"

namespace acl {

class check_timer;
class aio_socket_stream;

/**
 * Asynchronous connection callback function handler class
 */
class ACL_CPP_API check_client : public aio_open_callback {
public:
	check_client(check_timer& timer, const char* addr,
		aio_socket_stream& conn, struct ::timeval& begin);

	/**
	 * Get the input non-blocking IO handle
	 * @return {aio_socket_stream&}
	 */
	aio_socket_stream& get_conn() const {
		return conn_;
	}

	/**
	 * Get the passed server address
	 * @return {const char*}
	 */
	const char* get_addr() const {
		return addr_.c_str();
	}

	/**
	 * Set whether the connection is alive
	 * @param yesno {bool}
	 */
	void set_alive(bool yesno);

	/**
	 * Close the non-blocking IO handle
	 */
	void close();

public:
	// The following functions are for internal use only
	/**
	 * Whether the current check object is in blocking mode
	 * @return {bool}
	 */
	bool blocked() const {
		return blocked_;
	}

	/**
	 * In blocking check mode, call this function to set whether the check object
	 * is in blocking state.
	 * When in blocking state, the check object is prohibited from being closed by
	 * calling the close method
	 * @param on {bool} Set whether the check object is in blocking state, default
	 * is blocking state
	 */
	void set_blocked(bool on);

private:
	// Base class virtual functions
	bool open_callback();
	void close_callback();
	bool timeout_callback();

private:
	~check_client() {}

private:
	bool blocked_;
	bool aliving_;
	bool timedout_;
	struct ::timeval begin_;
	check_timer& timer_;
	aio_socket_stream& conn_;
	string addr_;
};

} // namespace acl
