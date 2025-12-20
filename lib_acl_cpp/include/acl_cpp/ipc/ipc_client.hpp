#pragma once
#include "../acl_cpp_define.hpp"
#include <list>
#include "../stream/aio_socket_stream.hpp"

namespace acl {

typedef struct MSG_HDR 
{
	int nMsg;
	int dlen;
#if defined(_WIN32) || defined(_WIN64)
	__int64   magic;
#else
	long long int magic;
#endif
} MSG_HDR;

typedef enum
{
	IO_WAIT_HDR,
	IO_WAIT_DAT
} io_status;

class aio_handle;
class ipc_adapter;
class aio_socket_stream;
class socket_stream;

/**
 * Asynchronous IP message class
 */
class ACL_CPP_API ipc_client : private aio_open_callback
{
public:
#if defined(_WIN32) || defined(_WIN64)
	ipc_client(__int64 magic = -1);
#else
	ipc_client(long long int magic = -1);
#endif
	virtual ~ipc_client();

	/**
	 * Direct destroy interface. Subclasses can override this interface
	 */
	virtual void destroy()
	{
		delete this;
	}

	/**
	 * Called when calling open function successfully connects to message server
	 */
	virtual void on_open() {}

	/**
	 * Callback interface when asynchronous stream closes
	 */
	virtual void on_close() {}

	/**
	 * Callback function when message is received. Subclasses must implement this
	 * interface
	 * @param nMsg {int} User-added custom message value
	 * @param data {void*} Message data
	 * @param dlen {int} Length of message data
	 */
	virtual void on_message(int nMsg, void* data, int dlen);

	/**
	 * Establish connection with message server and create asynchronous stream
	 * @param handle {aio_handle*} Asynchronous engine handle
	 * @param addr {const char*} Message server listening address, format:
	 *  IP:PORT (supports _WIN32/UNIX), unix_path (UNIX only)
	 * @param timeout {int} Connection timeout
	 */
	bool open(aio_handle* handle, const char* addr, int timeout);

	/**
	 * Asynchronous stream has been established. Call this function to complete
	 * ipc_client connection process
	 * @param client {aio_socket_stream*} Asynchronous connection stream
	 */
	void open(aio_socket_stream* client);

	/**
	 * Establish connection with message server and create synchronous stream
	 * @param addr {const char*} Message server listening address, format:
	 *  IP:PORT (supports _WIN32/UNIX), unix_path (UNIX only)
	 * @param timeout {int} Connection timeout
	 */
	bool open(const char* addr, int timeout);

	/**
	 * Synchronous stream has been established. Call this function to complete
	 * ipc_client connection process
	 * @param client {socket_stream*} Asynchronous connection stream
	 */
	void open(socket_stream* client);

	/**
	 * Message stream has been created. Call this function to open IPC channel
	 */
	void wait();

	/**
	 * Actively close message stream
	 */
	void close();

	/**
	 * Whether connection stream is normally open
	 * @return {bool}
	 */
	bool active() const;

	/**
	 * Add callback process object for specified message
	 * @param nMsg {int} Message number
	 */
	void append_message(int nMsg);

	/**
	 * Delete callback process object for specified message
	 * @param nMsg {int} Message number
	 */
	void delete_message(int nMsg);

	/**
	 * Send message
	 * @param nMsg {int} Message number
	 * @param data {const void*} Data
	 * @param dlen {int} Data length
	 */
	void send_message(int nMsg, const void* data, int dlen);

	/**
	 * Get asynchronous stream handle
	 * @return {aio_socket_stream*}
	 */
	aio_socket_stream* get_async_stream() const;

	/**
	 * Get asynchronous engine handle
	 */
	aio_handle& get_handle() const;

	/**
	 * Get synchronous stream handle
	 * @return {socket_stream*}
	 */
	socket_stream* get_sync_stream() const;
protected:
	/**
	 * Trigger message process
	 * @param nMsg {int} Message ID
	 * @param data {void*} Address of received message data
	 * @param dlen {int} Length of received message data
	 */
	void trigger(int nMsg, void* data, int dlen);
private:
#if defined(_WIN32) || defined(_WIN64)
	__int64   magic_;
#else
	long long int magic_;
#endif
	char* addr_;
	std::list<int> messages_;
	//aio_handle* handle_;
	aio_socket_stream* async_stream_;
	socket_stream* sync_stream_;
	socket_stream* sync_stream_inner_;
	bool closing_;

	io_status status_;
	MSG_HDR hdr_;

	// Base class virtual functions

	virtual bool read_callback(char* data, int len);
	virtual bool write_callback();
	virtual void close_callback();
	virtual bool timeout_callback();
	virtual bool open_callback();
};

}  // namespace acl

