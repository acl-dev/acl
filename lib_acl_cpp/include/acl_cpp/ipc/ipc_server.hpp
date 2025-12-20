#pragma once
#include "../acl_cpp_define.hpp"
#include "../stream/aio_listen_stream.hpp"

namespace acl {

class aio_handle;
class aio_listen_stream;

/**
 * Asynchronous message server, pure virtual class
 */
class ACL_CPP_API ipc_server : private aio_accept_callback
{
public:
	ipc_server();

	virtual ~ipc_server();

	/**
	 * Open asynchronous listening service stream
	 * @param handle {aio_handle*} Asynchronous engine handle, non-empty
	 * @param addr {const char*} Listening address
	 * @return {bool} Whether listening was successful
	 */
	bool open(aio_handle* handle, const char* addr = "127.0.0.1:0");

	/**
	 * After open succeeds, get listening address through this function
	 * @return {const char*} Listening address, format: IP:PORT
	 */
	const char* get_addr() const;

	/**
	 * Get asynchronous stream handle
	 * @return {aio_listen_stream*}
	 */
	aio_listen_stream* get_stream() const;

	/**
	 * Get asynchronous engine handle
	 */
	aio_handle& get_handle() const;

protected:
	/**
	 * Callback function after listening stream is successfully opened
	 * @param addr {const char*} Actual listening address, format: IP:PORT
	 */
	virtual void on_open(const char*addr)
	{
		(void) addr;
	}

	/**
	 * Callback function when listening stream closes
	 */
	virtual void on_close() {}

	/**
	 * Callback function after asynchronous listening stream gets a client
	 * connection
	 * @param client {aio_socket_stream*} Client IPC stream
	 */
	virtual void on_accept(aio_socket_stream* client)
	{
		(void) client;
	}

#if defined(_WIN32) || defined(_WIN64)
	/**
	 * For _WIN32 window message based cases, when open is called, internally
	 * will automatically call create_windows process
	 */
	virtual bool create_window()
	{
		return false;
	}
#endif

private:
	aio_handle* handle_;
	aio_listen_stream* sstream_;

	/**
	 * Base class virtual function, callback process called when new connection
	 * arrives
	 * @param client {aio_socket_stream*} Asynchronous client stream
	 * @return {bool} Returns true to notify listening stream to continue listening
	 */
	virtual bool accept_callback(aio_socket_stream* client);

	/**
	 * Base class virtual function, callback process when listening stream closes
	 */
	virtual void close_callback();

	/**
	 * Base class virtual function, callback process when listening stream times
	 * out
	 */
	virtual bool timeout_callback();
};

}  // namespace acl

