#pragma once
#include "../acl_cpp_define.hpp"
#if defined(_WIN32) || defined(_WIN64)
# include <WinSock2.h>
#endif
#include "aio_istream.hpp"
#include "aio_ostream.hpp"

namespace acl {

/**
 * Callback function class when asynchronous client stream asynchronously
 * connects to remote server. This class is pure virtual class,
 * requires subclasses to implement open_callback callback process
 */
class ACL_CPP_API aio_open_callback : public aio_callback {
public:
	aio_open_callback() {}
	virtual ~aio_open_callback() {}

	virtual bool open_callback() = 0;
protected:
private:
};

struct AIO_OPEN_CALLBACK {
	aio_open_callback* callback;
	bool enable;
};

class aio_handle;

/**
 * Network asynchronous stream class. This class inherits asynchronous
 * read/write streams. This class can only be allocated on heap,
 * cannot be allocated on stack. When this class ends, application does not need
 * to release this class object, because asynchronous stream
 * framework internally automatically releases this class object. Application
 * can call close to actively close stream
 */
class ACL_CPP_API aio_socket_stream : public aio_istream , public aio_ostream {
public:
	/**
	 * Constructor, create network asynchronous client stream
	 * @param handle {aio_handle*} Asynchronous engine handle
	 * @param stream {ACL_ASTREAM*} Non-blocking stream
	 * @param opened {bool} Whether this stream has already normally established
	 * connection with server. If yes, automatically
	 * hooks read/write process and close/timeout process, otherwise only hooks
	 * close/timeout process
	 */
	aio_socket_stream(aio_handle* handle, ACL_ASTREAM* stream, bool opened = false);

	/**
	 * Constructor, create network asynchronous client stream, and hook read/write
	 * process and close/timeout process
	 * @param handle {aio_handle*} Asynchronous engine handle
	 * @param fd {int} Connection socket handle
	 */
#if defined(_WIN32) || defined(_WIN64)
	aio_socket_stream(aio_handle* handle, SOCKET fd);
#else
	aio_socket_stream(aio_handle* handle, int fd);
#endif

	/**
	 * Open connection with remote server, and automatically hook stream's close,
	 * timeout and callback processing process when connection succeeds
	 * @param handle {aio_handle*} Asynchronous engine handle
	 * @param addr {const char*} Address of remote server, address format:
	 *  For TCP: IP:Port or For domain socket: {filePath}
 	 * @param local {const char*} Local network card IP address or network card
 	 * name. When not empty, if first character
 	 * is @, it indicates followed by IP address. If it is #, it indicates
 	 * followed by network card name, e.g.:
 	 * @127.0.0.1 indicates binding to local loopback IP address, #lo indicates
 	 * binding to local loopback network card name
	 * @param timeout {int} Connection timeout (seconds)
	 * @return {aio_socket_stream*} If connection immediately returns failure, this
	 * function returns NULL.
	 * Returning non-NULL object only indicates it is in connection process.
	 * Whether connection times out or fails
	 *  should be judged through callback function
	 */
	static aio_socket_stream* open(aio_handle* handle, const char* addr,
		const char* local, int timeout);
	static aio_socket_stream* open(aio_handle* handle,
	       const char* addr, int timeout);

	/**
	 * Bind UDP socket and create non-blocking object
	 * @param handle {aio_handle*} Asynchronous engine handle
	 * @param addr {const char*} Address of remote server, address format:
	 *  For TCP: IP:Port or For domain socket: {filePath}
	 * @return {aio_socket_stream*} Returns NULL indicates binding failed,
	 * otherwise indicates success
	 */
	static aio_socket_stream* bind(aio_handle* handle, const char* addr);

	/**
	 * Add callback process for open function
	 * @param callback {aio_open_callback*} Callback function
	 */
	void add_open_callback(aio_open_callback* callback);

	/**
	 * Delete from open callback object collection
	 * @param callback {aio_open_callback*} Callback object to be deleted. If this
	 *  value is empty, deletes all callback objects
	 * @return {int} Returns number of callback objects deleted from callback
	 * object collection
	 */
	int del_open_callback(aio_open_callback* callback = NULL);

	/**
	 * Disable a callback class object in callback object class collection, but
	 * does not delete from callback class object
	 * collection, just not called
	 * @param callback {aio_open_callback*} Callback object to be disabled. If this
	 *  value is empty, disables all callback objects
	 * @return {int} Returns number of callback objects disabled from callback
	 * object collection
	 */
	int disable_open_callback(aio_open_callback* callback = NULL);

	/**
	 * Enable all callback objects to be called
	 * @param callback {aio_open_callback*} Enable specified callback object.
	 * If this value is empty, enables all callback objects
	 * @return {int} Returns number of enabled callback objects
	 */
	int enable_open_callback(aio_open_callback* callback = NULL);

	/**
	 * For open process, determine whether connection has already succeeded
	 * @return {bool} Returns true indicates connection succeeded, otherwise
	 * indicates connection has not succeeded yet
	 */
	bool is_opened() const;

protected:
	virtual ~aio_socket_stream();

	/**
	 * Dynamically release asynchronous stream class objects that can only be
	 * allocated on heap through this function
	 */
	virtual void destroy();

	/**
	 * Register callback process when stream connection succeeds
	 */
	void enable_open();

private:
	std::list<AIO_OPEN_CALLBACK*>* open_callbacks_;

	static int open_callback(ACL_ASTREAM*, void*);
};

}  // namespace acl

