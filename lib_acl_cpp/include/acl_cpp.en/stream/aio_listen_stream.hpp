#pragma once
#include "../acl_cpp_define.hpp"
#include "aio_stream.hpp"

namespace acl {

class aio_socket_stream;
class aio_listen_stream;

/**
 * When asynchronous listening stream receives new client stream, calls callback function in this callback class. This class is pure virtual class,
 * requires subclasses to implement accept_callback callback process
 */
class ACL_CPP_API aio_accept_callback : public aio_callback {
public:
	aio_accept_callback() {}
	virtual ~aio_accept_callback() {}

	/**
	 * Callback function when new client stream is received
	 * @param client {aio_socket_stream*} Client asynchronous connection stream.
	 *  Can perform read/write operations on this stream
	 * @return {bool} If want to close this asynchronous listening stream, can return false.
	 *  Generally should not return false
	 */
	virtual bool accept_callback(aio_socket_stream* client) = 0;
};

/**
 * When asynchronous listening stream receives event that new connection arrives, calls virtual function in this class. In subclass implementation of this virtual function
 * calls accept() system API to receive client connection. This class is different from aio_accept_callback above.
 * When aio_accept_callback::accept_callback() is called, client connection object
 * has already been created. In listen_callback(), application needs to receive connection object itself
 */
class ACL_CPP_API aio_listen_callback : public aio_callback {
public:
	aio_listen_callback() {}
	virtual ~aio_listen_callback() {}

	virtual bool listen_callback(aio_listen_stream& ss) = 0;
};

/**
 * Asynchronous listening network stream. This class receives incoming connections from clients. This class can only
 * be allocated on heap, cannot be allocated on stack. Application can call close to actively close stream. After stream closes,
 * this asynchronous stream object is automatically released, no need to call delete to delete this class object
 *
 */
class ACL_CPP_API aio_listen_stream : public aio_stream {
public:
	/**
	 * Constructor, used to construct asynchronous listening stream
	 * @param handle {aio_handle*} Asynchronous engine handle
	 */
	aio_listen_stream(aio_handle* handle);

	/**
	 * Add callback function when asynchronous listening stream receives new client stream
	 * @param callback {aio_accept_callback*}
	 */
	void add_accept_callback(aio_accept_callback* callback);

	/**
	 * Add callback function when asynchronous listening stream has client connection arriving
	 * @param callback {aio_listen_stream*}
	 *  Note: Difference between this method and add_accept_callback above. This method is reactor
	 *  mode, while add_accept_callback is proactor mode
	 */
	void add_listen_callback(aio_listen_callback* callback);

	/**
	 * When using add_listen_callback method, can call this method in aio_listen_callback subclass's
	 * function listen_callback to get an asynchronous connection object
	 * @return {aio_socket_stream*} Returns NULL indicates getting connection failed
	 */
	aio_socket_stream* accept();

	/**
	 * Start listening on a specified address. Can be network socket or domain socket.
	 * @param addr {const char*} Listening address, TCP listening address or domain listening address
	 * Format:
	 *   For TCP connection: IP:PORT, e.g.: 127.0.0.1:9001
	 *   For domain socket: {path}, e.g.: /tmp/my.sock. On Linux platform, can also support
	 *   Linux abstract unix domain socket, requires first byte of address to be '@'. On Linux
	 *   platform, if ACL internally detects first character of path is '@', internally automatically switches to Linux
	 *   abstract unix domain socket listening mode (@ character is only used as marker, internal
	 *   listening address will automatically remove it)
	 * @param flag {unsigned} Open flag bits when creating listening socket, see server_socket.hpp
	 * @param qlen {int} Specifies length of listening socket listening queue
	 * @return {bool} Whether listening was successful
	 */
	bool open(const char* addr, unsigned flag = 0, int qlen = 128);

	/**
	 * Create listening object using socket. This socket handle must have already called bind/listen process
	 * @param fd {int}
	 * @return {bool} Whether successful
	 */
#if defined(_WIN32) || defined(_WIN64)
	bool open(SOCKET fd);
#else
	bool open(int fd);
#endif

	/**
	 * Create non-blocking listening object using synchronous stream object
	 * @param vstream {ACL_VSTREAM*} Non-empty object
	 * @return {bool} Whether successful
	 */
	bool open(ACL_VSTREAM* vstream);

	/**
	 * Create non-blocking listening object using non-blocking stream object
	 * @param astream {ACL_ASTREAM*} Non-empty object
	 * @return {bool} Whether successful
	 */
	bool open(ACL_ASTREAM* astream);

	/**
	 * Get server listening address
	 * @return {const char*}
	 */
	const char* get_addr() const;

	/**
	 * Override base class method. Called when asynchronous stream object is destroyed
	 */
	virtual void destroy();

protected:
	virtual ~aio_listen_stream();

private:
	bool listen_hooked_;
	char addr_[256];
	std::list<aio_accept_callback*> accept_callbacks_;
	std::list<aio_listen_callback*> listen_callbacks_;

	void enable_listen();
	int accept_callback(aio_socket_stream* conn);
	static int listen_callback(ACL_ASTREAM*, void*);
};

}  // namespace acl

