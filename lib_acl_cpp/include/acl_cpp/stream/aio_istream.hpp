#pragma once
#include "../acl_cpp_define.hpp"
#include <list>
#include "aio_handle.hpp"
#include "aio_timer_callback.hpp"
#include "aio_stream.hpp"

namespace acl {

class aio_istream;

/**
 * Delayed asynchronous read data class, base class is aio_timer_callback (see
 * aio_handle.hpp).
 * The so-called delayed asynchronous read is to put the asynchronous read
 * stream (aio_istream) into a timer,
 * unbind the asynchronous read operation of the asynchronous stream (i.e.,
 * remove it from aio_handle's monitoring),
 * and then start the asynchronous read operation when the specified time
 * arrives (re-bind the asynchronous read operation
 * of the asynchronous stream in the timer_callback callback). At the same time,
 * the timer automatically destroys itself
 * (calls the destroy method). Therefore, if the user inherits the
 * aio_timer_reader class and the subclass is not allocated
 * on the heap, the destroy method must be overridden, and resource release
 * operations should be performed in the subclass's
 * destroy. If the subclass does not override destroy, the base class
 * aio_timer_reader's destroy will be automatically called
 * when the timer ends - this class calls delete this, which will cause an
 * illegal memory release operation.
 * 
 */
class ACL_CPP_API aio_timer_reader : public aio_timer_callback {
public:
	aio_timer_reader()
	: in_(NULL)
	, delay_gets_(false)
	, delay_timeout_(0)
	, delay_nonl_(true)
	, delay_count_(0) {}

	/**
	 * Called in aio_istream to release the class object. Subclasses should
	 * implement this function.
	 */
	virtual void destroy() {
		delete this;
	}

protected:
	virtual ~aio_timer_reader() {}
	/**
	 * Callback function when reading data with delay, inherited from
	 * aio_timer_callback class.
	 */
	virtual void timer_callback(unsigned int id);

private:
	// Allow aio_istream to directly modify private member variables of this class
	friend class aio_istream;

	aio_istream* in_;
	//int   read_delayed_;
	bool  delay_gets_;
	int   delay_timeout_;
	bool  delay_nonl_;
	int   delay_count_;
};

/**
 * Asynchronous read data stream class definition. This class can only be
 * instantiated on the heap.
 * When destructing, the close function must be called to release this class
 * object.
 */
class ACL_CPP_API aio_istream : virtual public aio_stream {
public:
	/**
	 * Constructor
	 * @param handle {aio_handle*} Asynchronous event engine handle
	 */
	explicit aio_istream(aio_handle* handle);

	/**
	 * Constructor, creates an asynchronous read stream object and hooks the read
	 * process and close/timeout process.
	 * @param handle {aio_handle*} Asynchronous event engine handle
	 * @param fd {int} Connection socket handle
	 */
#if defined(_WIN32) || defined(_WIN64)
	aio_istream(aio_handle* handle, SOCKET fd);
#else
	aio_istream(aio_handle* handle, int fd);
#endif

	/**
	 * Add a callback class object pointer for when data is readable. If the
	 * callback class object already exists,
	 * it will only make the object in an open and available state.
	 * @param callback {aio_callback*} Subclass callback class object inheriting
	 * from aio_callback.
	 * When the asynchronous stream has data, the read_callback interface in this
	 * callback class object will be called first.
	 */
	void add_read_callback(aio_callback* callback);

	/**
	 * Delete from the read callback object collection
	 * @param callback {aio_read_callback*} Callback object to be deleted.
	 * If this value is empty, all callback objects will be deleted.
	 * @return {int} Returns the number of callback objects deleted from the
	 * callback object collection.
	 */

	/**
	 * Delete callback object from the read callback object collection.
	 * @param callback {aio_callback*} Subclass object pointer inheriting from
	 * aio_callback.
	 *  If this value is empty, all read callback objects will be deleted.
	 * @return {int} Returns the number of callback objects deleted from the
	 * callback object collection.
	 */
	int del_read_callback(aio_callback* callback = NULL);

	/**
	 * Disable a callback class object in the callback object collection, but do
	 * not delete it from the callback
	 * object collection, it just won't be called.
	 * @param callback {aio_callback*} Subclass object pointer inheriting from
	 * aio_callback.
	 *  If this value is empty, all read callback objects will be disabled.
	 * @return {int} Returns the number of callback objects disabled in the
	 * callback object collection.
	 */
	int disable_read_callback(aio_callback* callback = NULL);

	/**
	 * Enable all callback objects to be called.
	 * @param callback {aio_callback*} Subclass object pointer inheriting from
	 * aio_callback.
	 *  If this value is empty, all read callback objects will be enabled.
	 * @return {int} Returns the number of enabled callback objects.
	 */
	int enable_read_callback(aio_callback* callback = NULL);

	/**
	 * Asynchronously read a line of data. When delayed asynchronous read is used,
	 * if this process is called continuously,
	 * only the last delayed read operation will take effect.
	 * @param timeout {int} Read timeout time (seconds). If 0, it means
	 *  wait forever until a complete line of data is read or an error occurs.
	 * @param nonl {bool} Whether to automatically remove trailing carriage return
	 * and line feed characters.
	 * @param delay {long long int} If the peer sends data relatively quickly, when
	 * this parameter
	 * is greater than 0, the peer's data can be received with delay. This value
	 * controls the delay time
	 *  for reading data (unit: microseconds).
	 * @param callback {aio_timer_reader*} Callback function class object when the
	 * timer expires.
	 *  When delay > 0, if this value is empty, the default object will be used.
	 */
	void gets_await(int timeout = 0, bool nonl = true,
		long long int delay = 0, aio_timer_reader* callback = NULL);

	/**
	 * same as gets_await();
	 */
	void gets(int timeout = 0, bool nonl = true,
		long long int delay = 0, aio_timer_reader* callback = NULL) {
		gets_await(timeout, nonl, delay, callback);
	}

	/**
	 * Asynchronously read data. When delayed asynchronous read is used, if this
	 * process is called continuously,
	 * only the last delayed read operation will take effect.
	 * @param count {int} The amount of data required to be read. If 0, it will
	 * return as long as there is data
	 * available to read. Otherwise, it will continue until read timeout, read
	 * error, or the required number of bytes is read.
	 * @param timeout {int} Read timeout time (seconds). If 0, it means
	 *  wait forever until the required data is read or an error occurs.
	 * @param delay {long long int} If the peer sends data relatively quickly, when
	 * this parameter
	 * is greater than 0, the peer's data can be received with delay. This value
	 * controls the delay time
	 *  for reading data (unit: microseconds).
	 * @param callback {aio_timer_reader*} Callback function class object when the
	 * timer expires.
	 *  If this value is empty, the default object will be used.
	 */
	void read_await(int count = 0, int timeout = 0,
		long long int delay = 0, aio_timer_reader* callback = NULL);

	/**
	 * same as read_await()
	 */
	void read(int count = 0, int timeout = 0,
		long long int delay = 0, aio_timer_reader* callback = NULL) {
		read_await(count, timeout, delay, callback);
	}

	/**
	 * Asynchronously wait for the connection stream to be readable. This function
	 * sets the read monitoring state of the asynchronous stream.
	 * When data is readable, the callback function is triggered, and the user is
	 * responsible for reading the data.
	 * @param timeout {int} Read timeout time (seconds). When this value is 0,
	 * there is no read timeout.
	 */
	void readable_await(int timeout = 0);

	/**
	 * same as readable_await()
	 */
	void read_wait(int timeout = 0) {
		readable_await(timeout);
	}

	/**
	 * Disable the asynchronous read state of the asynchronous stream, remove the
	 * asynchronous stream from the asynchronous engine's
	 * monitoring until the user calls any asynchronous read operation (at this
	 * time, the asynchronous engine will
	 * automatically re-monitor the readable state of the stream).
	 */
	void disable_read();

	/**
	 * Set whether the stream uses continuous read functionality.
	 * @param onoff {bool}
	 */
	void keep_read(bool onoff);

	/**
	 * Get whether the stream has continuous read functionality set.
	 * @return {bool}
	 */
	bool keep_read() const;

	/**
	 * Set the maximum length of the receive buffer to avoid buffer overflow.
	 * Default value is 0, meaning no limit.
	 * @param max {int}
	 * @return {aio_istream&}
	 */
	aio_istream& set_buf_max(int max);

	/**
	 * Get the current maximum length limit of the receive buffer.
	 * @return {int} Return value <= 0 means no limit.
	 */
	int get_buf_max() const;

	/**
	 * Clear the internal read_ready flag bit. When the IO engine detects that IO
	 * is readable, it will set
	 * the read_ready flag bit, and will also periodically check whether this flag
	 * bit is set to decide whether
	 * to trigger the read callback process again. After clearing this flag bit
	 * through this method, the IO engine
	 * will not check at the application layer whether the handle is readable, but
	 * will leave it to the OS to determine
	 * whether it is readable.
	 */
	void clear_read_ready();

protected:
	virtual ~aio_istream();

	/**
	 * Virtual function to release dynamic class objects.
	 */
	virtual void destroy();

	/**
	 * Register readable callback function.
	 */
	void enable_read();

private:
	friend class aio_timer_reader;
	aio_timer_reader* timer_reader_;
	std::list<AIO_CALLBACK*> read_callbacks_;

	static int read_callback(ACL_ASTREAM*,  void*, char*, int);
	static int read_wakeup(ACL_ASTREAM* stream, void* ctx);
};

}  // namespace acl

