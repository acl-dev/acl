#pragma once
#include "../acl_cpp_define.hpp"
#include <stdarg.h>
#include "../stdlib/string.hpp"
#include "aio_handle.hpp"
#include "aio_timer_callback.hpp"
#include "aio_stream.hpp"

struct iovec;

namespace acl {

class aio_ostream;

/**
 * Delayed asynchronous write data class. Base class is aio_timer_callback (see
 * aio_handle.hpp).
 * So-called delayed asynchronous write is to put asynchronous write stream
 * (aio_ostream) in timer, unbind
 * asynchronous write operation of this asynchronous stream (i.e., remove from
 * aio_handle's monitoring),
 * then start asynchronous write operation when specified time arrives (rebind
 * asynchronous stream's
 * asynchronous write operation in timer_callback callback), and timer
 * automatically destroys (calls destroy method).
 * So if user inherits aio_timer_writer class, and subclass is not allocated on
 * heap,
 * must override destroy method, and perform resource release related operations
 * in subclass's destroy.
 * If subclass does not override destroy, when timer ends, internally will
 * automatically call
 * base class aio_timer_writer's destroy--this class calls delete this, which
 * will cause
 * illegal memory release operation)
 * 
 */
class ACL_CPP_API aio_timer_writer : public aio_timer_callback {
public:
	aio_timer_writer();

	/**
	 * Called in aio_istream to release class object. Subclass should implement
	 * this function
	 */
	virtual void destroy() {
		delete this;
	}

protected:
	virtual ~aio_timer_writer();

	/**
	 * Callback function when delayed read data. Inherited from aio_timer_callback
	 * class
	 */
	virtual void timer_callback(unsigned int id);
private:
	friend class aio_ostream;

	aio_ostream* out_;
	//int   write_delayed_;
	acl::string buf_;
};

/**
 * Asynchronous write data stream class definition. This class can only be
 * instantiated on heap. Need to call close
 * function during destruction to release this class object
 */
class ACL_CPP_API aio_ostream : virtual public aio_stream {
public:
	/**
	 * Constructor
	 * @param handle {aio_handle*} Asynchronous event engine handle
	 */
	aio_ostream(aio_handle* handle);

	/**
	 * Constructor. Create asynchronous write stream object, and hook write process
	 * and close/timeout process
	 * @param handle {aio_handle*} Asynchronous event engine handle
	 * @param fd {int} Connection socket handle
	 */
#if defined(_WIN32) || defined(_WIN64)
	aio_ostream(aio_handle* handle, SOCKET fd);
#else
	aio_ostream(aio_handle* handle, int fd);
#endif

	/**
	 * Add callback class object pointer when writable. If this callback class
	 * object already exists, only
	 * enables this object to be in open available state
	 * @param callback {aio_callback*} Subclass callback class object inheriting
	 * aio_callback.
	 * When asynchronous stream has data, will first call write_callback interface
	 * in this callback class object
	 */
	void add_write_callback(aio_callback* callback);

	/**
	 * Delete from write callback object collection
	 * @param callback {aio_callback*} Write callback object to be deleted.
	 *  If this value is empty, deletes all callback write objects
	 * @return {int} Returns number of callback objects deleted from callback
	 * object collection
	 */
	int del_write_callback(aio_callback* callback = NULL);

	/**
	 * Disable a callback class object in callback object class collection, but
	 * does not delete from callback class object
	 * collection, just not called
	 * @param callback {aio_callback*} Write callback object to be disabled.
	 *  If this value is empty, disables all write callback objects
	 * @return {int} Returns number of callback objects disabled from callback
	 * object collection
	 */
	int disable_write_callback(aio_callback* callback = NULL);

	/**
	 * Enable all callback objects to be called
	 * @param callback {aio_callback*} Enable specified write callback object.
	 *  If this value is empty, enables all write callback objects
	 * @return {int} Returns number of enabled write callback objects
	 */
	int enable_write_callback(aio_callback* callback = NULL);

	/**
	 * Asynchronously write specified number of bytes of data. When completely
	 * written successfully or error or timeout occurs, will
	 * call user-registered callback function. In delayed asynchronous write, when
	 * this process is called continuously within a function,
	 * each delayed asynchronous write operation will be added to delayed write
	 * queue to ensure each delayed asynchronous write operation can be executed
	 * when its timer arrives
	 * @param data {const void*} Data address
	 * @param len {int} Data length
	 * @param delay {int64} If this value > 0, uses delayed send mode (unit:
	 * microseconds)
	 * @param callback {aio_timer_writer*} Callback function class object when
	 * timer arrives
	 */
	void write_await(const void* data, int len, long long int delay = 0,
		aio_timer_writer* callback = NULL);

	/**
	 * same as write_await();
	 */
	void write(const void* data, int len, long long int delay = 0,
		aio_timer_writer* callback = NULL) {
		write_await(data, len, delay, callback);
	}

	/**
	 * When sending data in datagram mode, can call this method to send data packet
	 * to target address
	 * @param data {const void*} Data address
	 * @param len {int} Data length
	 * @param dest_addr {const char*} Target address, format: ip|port
	 * @param flags {int} Send flag bits. Please refer to flags description in
	 * system sendto() api
	 * @return {int} Returns -1 indicates send failed
	 */
	int sendto(const void* data, int len, const char* dest_addr, int flags = 0);

	/**
	 * When sending data in datagram mode, can call this method to send data packet
	 * to target address
	 * @param data {const void*} Data address
	 * @param len {int} Data length
	 * @param dest_addr {const sockaddr*} Target address, format: ip|port
	 * @param addrlen {int} dest_addr address length
	 * @param flags {int} Send flag bits. Please refer to flags description in
	 * system sendto() api
	 * @return {int} Returns -1 indicates send failed
	 */
	int sendto(const void* data, int len,
		const struct sockaddr* dest_addr, int addrlen, int flags = 0);

	/**
	 * Asynchronously write data to stream. When stream error, write timeout or
	 * write success occurs, will trigger event notification process,
	 * similar to system's writev
	 * @param iov {const struct iovec*} Data collection array
	 * @param count {int} Length of iov array
	 */
	void writev_await(const struct iovec *iov, int count);

	/**
	 * same as writev_await()
	 */
	void writev(const struct iovec *iov, int count) {
		writev_await(iov, count);
	}

	/**
	 * Asynchronously write data in formatted way. When completely written
	 * successfully or error or timeout occurs, will
	 * call user-registered callback function
	 * @param fmt {const char*} Format string
	 */
	void format_await(const char* fmt, ...) ACL_CPP_PRINTF(2, 3);

	/**
	 * please use format_await() instead
	 */
	void format(const char* fmt, ...) ACL_CPP_PRINTF(2, 3)
	{
		va_list ap;
		va_start(ap, fmt);
		vformat_await(fmt, ap);
		va_end(ap);
	}

	/**
	 * Asynchronously write data in formatted way. When completely written
	 * successfully or error or timeout occurs, will
	 * call user-registered callback function
	 * @param fmt {const char*} Format string
	 * @param ap {va_list} Data value list
	 */
	void vformat_await(const char* fmt, va_list ap);

	/**
	 * same as vformat_await()
	 */
	void vformat(const char* fmt, va_list ap) {
		vformat_await(fmt, ap);
	}

	/**
	 * Asynchronously wait for connection stream to be writable. This function sets
	 * asynchronous stream's write monitoring state. When writable,
	 * callback function is triggered, user is responsible for data reading
	 * @param timeout {int} Write timeout (seconds). When this value is <= 0, there
	 * is no write timeout
	 */
	void writable_await(int timeout = 0);

	/**
	 * same as writable_await()
	 */
	void write_wait(int timeout = 0) {
		writable_await(timeout);
	}

	/**
	 * Disable asynchronous stream's asynchronous write state, remove this
	 * asynchronous stream from asynchronous engine's monitoring
	 * events until user calls any write operation, then automatically opens
	 * asynchronous write state
	 * (at this time, this stream will be monitored by asynchronous engine again)
	 */
	void disable_write();

	/**
	 * Get data length in send queue
	 * @return {size_t}
	 */
	size_t pending_length() const;

protected:
	virtual ~aio_ostream();

	/**
	 * Virtual function for releasing dynamic class objects
	 */
	virtual void destroy();

	/**
	 * Register write process
	 */
	void enable_write();

private:
	friend class aio_timer_writer;
	std::list<aio_timer_writer*>* timer_writers_;
	std::list<AIO_CALLBACK*> write_callbacks_;

	static int write_callback(ACL_ASTREAM*, void*);
	static int write_wakup(ACL_ASTREAM*, void*);
};

}  // namespace acl

