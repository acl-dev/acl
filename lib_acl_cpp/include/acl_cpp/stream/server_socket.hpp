#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/string.hpp"
#include "../stdlib/noncopyable.hpp"
#if defined(_WIN32) || defined(_WIN64)
#include <WinSock2.h>
#endif

#if __cplusplus >= 201103L
#include <memory>
#endif

namespace acl {

class socket_stream;

#if __cplusplus >= 201103L
using shared_stream = std::shared_ptr<socket_stream>;
#endif

enum {
	OPEN_FLAG_NONE           = 0,
	OPEN_FLAG_NONBLOCK       = 1,        // Non-blocking mode
	OPEN_FLAG_REUSEPORT      = (1 << 1), // Port reuse, requires Linux 3.0 or above
	OPEN_FLAG_FASTOPEN       = (1 << 2), // Whether to enable Fast open (experimental stage)
	OPEN_FLAG_EXCLUSIVE      = (1 << 3), // Whether to prohibit address reuse
	OPEN_FLAG_MULTICAST_LOOP = (1 << 4), // Whether to allow receiving loopback packets during multicast
};

/**
 * Server listening socket class. Receives client connections and creates client
 * stream connection objects
 */
class ACL_CPP_API server_socket : public noncopyable {
public:
#if 0
	/**
	 * Constructor. After calling this constructor, need to call class method open
	 * to listen on specified service address
	 * @param backlog {int} Listening socket queue length
	 * @param block {bool} Whether it is blocking mode or non-blocking mode
	 */
	server_socket(int backlog, bool block);
#endif

	/**
	 * Constructor
	 * @param flag {unsigned} Definition see OPEN_FLAG_XXX
	 * @param backlog {int} Listening socket queue length
	 */
	server_socket(unsigned flag, int backlog);

	/**
	 * Constructor. After calling this constructor, calling open method again is
	 * prohibited
	 * @param sstream {ACL_VSTREAM*} Externally created listening stream object.
	 * This class only uses
	 * but does not release it. Application should close this listening object
	 * itself
	 */
	server_socket(ACL_VSTREAM* sstream);

	/**
	 * Constructor. After calling this constructor, calling open method again is
	 * prohibited
	 * @param fd {ACL_SOCKET} Externally created listening handle. This class only
	 * uses but does not release it.
	 *  Application should close this listening handle itself
	 */
#if defined(_WIN32) || defined(_WIN64)
	server_socket(SOCKET fd);
#else
	server_socket(int fd);
#endif

	server_socket();
	~server_socket();

	/**
	 * Start listening on given server address
	 * @param addr {const char*} Server listening address, format:
	 * ip:port. In unix environment, can also be domain socket, format: /path/xxx.
	 * On
	 * Linux platform, if domain socket address is: @xxx format, i.e., first
	 * character is @, then
	 * internally automatically enables Linux abstract domain socket mode (abstract
	 * unix socket)
	 * @return {bool} Whether listening was successful
	 */
	bool open(const char* addr);

	/**
	 * Determine whether current listening socket is open
	 * @return {bool}
	 */
	bool opened() const;

	/**
	 * Close already opened listening socket
	 * @return {bool} Whether closed normally
	 */
	bool close();

	/**
	 * Unbind listening socket from service listening object
	 * @return {SOCKET} Returns unbound handle
	 */
#if defined(_WIN32) || defined(_WIN64)
	SOCKET unbind(void);
#else
	int unbind();
#endif

	/**
	 * Accept client connection and create client connection stream
	 * @param timeout {int} When this value >= 0, uses timeout mode to accept
	 * client connection.
	 *  If client connection is not obtained within specified time, returns NULL
	 * @param etimed {bool*} When this pointer is not NULL, if this function
	 * returns
	 *  NULL due to timeout, this value is set to true
	 * @return {socket_stream*} Returns NULL indicates accept failed or timeout.
	 * Returned stream object needs
	 *  to be deleted by user after use.
	 */
	socket_stream* accept(int timeout = -1, bool* etimed = NULL);

#if __cplusplus >= 201103L
	// Use c++11 shared_ptr method to get client stream object, use stream object
	// more safely

	shared_stream shared_accept(int timeout = -1, bool* etimed = NULL) {
		shared_stream ss(accept(timeout, etimed));
		return ss;
	}
#endif

	/**
	 * Get listening address
	 * @return {const char*} Return value is non-NULL pointer
	 */
	const char* get_addr() const {
		return addr_.c_str();
	}

	/**
	 * After normally listening on server address, calling this function can get
	 * listening socket
	 * @return {int}
	 */
#if defined(_WIN32) || defined(_WIN64)
	SOCKET sock_handle(void) const {
#else
	int sock_handle() const {
#endif
		return fd_;
	}

	/**
	 * Set listening socket's deferred accept function, i.e., only return
	 * connection to application when client connection has data.
	 * Currently this function only supports Linux
	 * @param timeout {int} If client connection does not send data within
	 * specified time,
	 *  also return this connection to application
	 */
	void set_tcp_defer_accept(int timeout);

private:
	int      backlog_;
	unsigned open_flag_;
	bool     unix_sock_;
	string   addr_;

#if defined(_WIN32) || defined(_WIN64)
	SOCKET fd_;
	SOCKET fd_local_;
#else
	int   fd_;
	int   fd_local_;
#endif
};

} // namespace acl

