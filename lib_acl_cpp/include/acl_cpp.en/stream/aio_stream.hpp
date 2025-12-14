#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/noncopyable.hpp"
#include <list>
#include <string>
#if defined(_WIN32) || defined(_WIN64)
#include <WinSock2.h>
#endif

struct ACL_ASTREAM;
struct ACL_VSTREAM;

namespace acl {

/**
 * Asynchronous stream callback class.
 */
class ACL_CPP_API aio_callback : public noncopyable {
public:
	aio_callback() {}
	virtual ~aio_callback() {};

	virtual void close_callback() {}
	virtual bool timeout_callback() {
		return false;
	}

	/**
	 * Read callback virtual function. When the callback object is registered in aio_istream instance's
	 * gets/read readable callback, the asynchronous stream internally reads the data and
	 * directly passes it to the user callback.
	 * @param data {char*} Pointer to the read data
	 * @param len {int} Length of the read data (> 0)
	 * @return {bool} This function returns false to notify the asynchronous stream to close the asynchronous stream.
	 */
	virtual bool read_callback(char* data, int len) {
		(void) data;
		(void) len;
		return true;
	}

	/**
	 * Read callback virtual function. When the callback object is registered in aio_istream instance's
	 * read_wait readable callback, it is called when the asynchronous stream has readable data. On timeout,
	 * timeout_callback is called. On abnormal close, close_callback is called.
	 */
	virtual bool read_wakeup() {
		return true;
	}

	/**
	 * Write success callback virtual function.
	 * @return {bool} This function returns false to notify the asynchronous stream to close the asynchronous stream.
	 */
	virtual bool write_callback() {
		return true;
	}

	/**
	 * Write callback virtual function. When the callback object is registered in aio_ostream instance's
	 * write_wait writable callback, it is called when the asynchronous stream is writable. On timeout,
	 * timeout_callback is called. On abnormal close, close_callback is called.
	 */
	virtual bool write_wakeup() {
		return true;
	}
};

struct AIO_CALLBACK {
	aio_callback* callback;
	bool enable;
};

class aio_handle;
class stream_hook;

/**
 * Asynchronous stream base class. This is a base class and cannot be directly instantiated. It can only be inherited and used.
 * This class can only be allocated on the heap, not on the stack.
 */
class ACL_CPP_API aio_stream : public noncopyable {
public:
	/**
	 * Constructor
	 * @param handle {aio_handle*}
	 */
	aio_stream(aio_handle* handle);

	/**
	 * Close asynchronous stream.
	 * @param flush_out {bool} When true, it needs to wait until all buffered data is written
	 *  before closing. Otherwise, it does not wait for data in the write buffer to be closed.
	 */
	void close(bool flush_out = false);

	/**
	 * Add callback object pointer for when closing. If the callback object already exists, it will only
	 * make the object in an open and available state.
	 * @param callback {aio_callback*} Subclass callback class object inheriting from aio_callback.
	 *  Before the asynchronous stream closes, the close_callback interface in this callback class object will be called first.
	 */
	void add_close_callback(aio_callback* callback);

	/**
	 * Add callback object pointer for when timeout occurs. If the callback object already exists, it will only
	 * make the object in an open and available state.
	 * @param callback {aio_callback*} Subclass callback class object inheriting from aio_callback.
	 *  Before the asynchronous stream closes, the timeout_callback interface in this callback class object will be called first.
	 */
	void add_timeout_callback(aio_callback* callback);

	/**
	 * Delete callback object pointer for when closing.
	 * @param callback {aio_callback*} Subclass object pointer inheriting from aio_callback.
	 *  If this value is empty, all close callback objects will be deleted.
	 * @return {int} Returns the number of callback objects deleted from the callback object collection.
	 */
	int del_close_callback(aio_callback* callback = NULL);

	/**
	 * Delete callback object pointer for when timeout occurs.
	 * @param callback {aio_callback*} Subclass object pointer inheriting from aio_callback.
	 *  If this value is empty, all timeout callback objects will be deleted.
	 * @return {int} Returns the number of callback objects deleted from the callback object collection.
	 */
	int del_timeout_callback(aio_callback* callback = NULL);

	/**
	 * Disable close callback objects, but do not delete them from the close callback object collection.
	 * @param callback {aio_callback*} Subclass object pointer inheriting from aio_callback.
	 *  If this value is empty, all close callback objects will be disabled.
	 * @return {int} Returns the number of callback objects disabled in the callback object collection.
	 */
	int disable_close_callback(aio_callback* callback = NULL);

	/**
	 * Disable timeout callback objects, but do not delete them from the timeout callback object collection.
	 * @param callback {aio_callback*} Subclass object pointer inheriting from aio_callback.
	 *  If this value is empty, all timeout callback objects will be disabled.
	 * @return {int} Returns the number of callback objects disabled in the callback object collection.
	 */
	int disable_timeout_callback(aio_callback* callback = NULL);

	/**
	 * Enable all callback objects to be called.
	 * @param callback {aio_callback*} Specified callback object. If this value is empty,
	 *  all close callback objects will be enabled.
	 * @return {int} Returns the number of enabled callback objects.
	 */
	int enable_close_callback(aio_callback* callback = NULL);

	/**
	 * Enable all callback objects to be called.
	 * @param callback {aio_callback*} Specified callback object. If this value is empty,
	 *  all timeout callback objects will be enabled.
	 * @return {int} Returns the number of enabled callback objects.
	 */
	int enable_timeout_callback(aio_callback* callback = NULL);

	/**
	 * Get asynchronous stream's ACL_ASTREAM.
	 * @return {ACL_ASTREAM*}
	 */
	ACL_ASTREAM* get_astream() const;

	/**
	 * Get synchronous stream ACL_VSTREAM in asynchronous stream.
	 * @return {ACL_VSTREAM*}
	 */
	ACL_VSTREAM* get_vstream() const;

	/**
	 * Get SOCKET handle in asynchronous stream.
	 * @return {ACL_SOCKET} Returns -1(UNIX) or INVALID_SOCKET(win32) if not connected.
	 */
#if defined(_WIN32) || defined(_WIN64)
	SOCKET get_socket(void) const;
	SOCKET sock_handle(void) const
#else
	int get_socket() const;
	int sock_handle() const
#endif
	{
		return get_socket();
	}

	/**
	 * Get remote connection address.
	 * @param full {bool} Whether to return full address format (IP:PORT). If this parameter
	 *  is false, only IP is returned. Otherwise, IP:PORT is returned.
	 * @return {const char*} Remote connection address. If return value == '\0', it means
	 *  unable to get remote connection address.
	 */
	const char* get_peer(bool full = false) const;

	/**
	 * Get local address of connection.
	 * @param full {bool} Whether to return full address format (IP:PORT). If this parameter
	 *  is false, only IP is returned. Otherwise, IP:PORT is returned.
	 * @return {const char*} Local address of connection. If return value == "", it means
	 *  unable to get local address.
	 */
	const char* get_local(bool full = false) const;

	/**
	 * Get asynchronous event engine.
	 * @return {aio_handle&}
	 */
	aio_handle& get_handle() const;

	/**
	 * Update asynchronous event engine.
	 * @param handle {aio_handle&}
	 * Note: This method should only be called once after object creation. Generally, it is used when IO operations are stopped.
	 */
	void set_handle(aio_handle& handle);

	/**
	 * Register read/write hook. Internally automatically calls hook->open process. If successful, returns the previously
	 * registered object (may be NULL). If failed, returns the same pointer as input. Applications can
	 * determine whether registration was successful by checking if return value is different from input value.
	 * xxx: Before calling this method, it must be ensured that the connection has been established.
	 * @param hook {stream_hook*} Non-empty object pointer.
	 * @return {stream_hook*} Return value. If different from input value, it means success.
	 */
	stream_hook* setup_hook(stream_hook* hook);

	/**
	 * Get currently registered read/write hook.
	 * @return {stream_hook*}
	 */
	stream_hook* get_hook() const;

	/**
	 * Remove currently registered read/write hook and return this object, restore default read/write hook.
	 * @return {stream_hook*}
	 */
	stream_hook* remove_hook();

protected:
	aio_handle*  handle_;
	ACL_ASTREAM* stream_;
	stream_hook* hook_;

	virtual ~aio_stream();

	/**
	 * Through this function, dynamically release asynchronous stream objects that are only allocated on the heap.
	 */
	virtual void destroy();

	/**
	 * This function should be called after connection is successfully established to notify the application layer that the asynchronous stream is ready,
	 * and register close and timeout callback functions at the same time.
	 */
	void enable_error();

protected:
	enum {
		// Flag bits indicating whether hook_xxx functions are registered
		STATUS_HOOKED_ERROR = 1,
		STATUS_HOOKED_READ  = 1 << 1,
		STATUS_HOOKED_WRITE = 1 << 2,
		STATUS_HOOKED_OPEN  = 1 << 3,

		// For aio_socket_stream, indicates whether connection has been established
		STATUS_CONN_OPENED  = 1 << 4,
	};
	unsigned status_;
private:
	std::list<AIO_CALLBACK*>* close_callbacks_;
	std::list<AIO_CALLBACK*>* timeout_callbacks_;

	static int close_callback(ACL_ASTREAM*, void*);
	static int timeout_callback(ACL_ASTREAM*, void*);

private:
	std::string ip_peer_;
	std::string ip_local_;

	const char* get_ip(const char* addr, std::string& out);

private:
#if defined(_WIN32) || defined(_WIN64)
	static int read_hook(SOCKET fd, void *buf, size_t len,
		int timeout, ACL_VSTREAM* stream, void *ctx);
	static int send_hook(SOCKET fd, const void *buf, size_t len,
		int timeout, ACL_VSTREAM* stream, void *ctx);

	static int fread_hook(HANDLE fd, void *buf, size_t len,
		int timeout, ACL_VSTREAM* stream, void *ctx);
	static int fsend_hook(HANDLE fd, const void *buf, size_t len,
		int timeout, ACL_VSTREAM* stream, void *ctx);
#else
	static int read_hook(int fd, void *buf, size_t len,
		int timeout, ACL_VSTREAM* stream, void *ctx);
	static int send_hook(int fd, const void *buf, size_t len,
		int timeout, ACL_VSTREAM* stream, void *ctx);

	static int fread_hook(int fd, void *buf, size_t len,
		int timeout, ACL_VSTREAM* stream, void *ctx);
	static int fsend_hook(int fd, const void *buf, size_t len,
		int timeout, ACL_VSTREAM* stream, void *ctx);
#endif
};

}  // namespace acl

