#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/string.hpp"
#include "../stdlib/noncopyable.hpp"

struct ACL_AIO;
struct ACL_EVENT;

namespace acl {

// Event engine type
typedef enum {
	ENGINE_SELECT,  // select mode (supports all platforms)
	ENGINE_POLL,    // poll mode (UNIX platforms only)
	ENGINE_KERNEL,  // kernel mode (win32: iocp, Linux: epoll, FreeBsd: kqueue, Solaris: devpoll
	ENGINE_WINMSG   // win32 GUI message mode
} aio_handle_type;

/**
 * Non-blocking IO event engine class. This class encapsulates system's
 * select/poll/epoll/kqueue/devpoll/iocp,
 */

class aio_timer_callback;
class aio_delay_free;
class aio_timer_delay_free;

class ACL_CPP_API aio_handle : private noncopyable {
public:
	/**
	 * Constructor, automatically creates IO event engine, and automatically
	 * releases in destructor
	 * @param engine_type {aio_handle_type} Engine type to use
	 *  ENGINE_SELECT: select method, supports win32/unix platforms
	 *  ENGINE_POLL: poll method, supports unix platforms
	 * ENGINE_KERNEL: Automatically sets based on efficient kernel engine supported
	 * by each system platform
	 *  ENGINE_WINMSG: win32 GUI message method, supports win32 platform
	 * @param nMsg {unsigned int} If engine_type is ENGINE_WINMSG, when this value
	 * is greater than 0, this asynchronous handle is bound to this message,
	 * otherwise bound to default message.
	 * When engine_type is not ENGINE_WINMSG, this value has no effect on other
	 * asynchronous handles
	 *  
	 */
	aio_handle(aio_handle_type engine_type = ENGINE_SELECT,
		unsigned int nMsg = 0);

	/**
	 * Constructor. Caller passes ACL_AIO handle, and class destructor will not
	 * automatically release this ACL_AIO handle
	 * @param handle {ACL_AIO*} ACL_AIO handle
	 */
	aio_handle(ACL_AIO* handle);

	virtual ~aio_handle();

	/**
	 * For asynchronous read streams, set whether to continuously read. This
	 * configuration item will be inherited by all asynchronous read streams
	 * based on this asynchronous engine handle. Generally aio_handle class objects
	 * are continuously reading by default
	 * @param onoff {bool} Set whether to continuously read
	 */
	void keep_read(bool onoff);

	/**
	 * Get whether asynchronous engine handle has continuous read data function set
	 * @return {bool}
	 */
	bool keep_read() const;

	/**
	 * Set timer
	 * @param callback {aio_timer_callback*} Timer callback function class object
	 * @param delay {int64} Timer time interval (microseconds)
	 * @param id {unsigned int} ID number of a timer task
	 * @return {int64} Timer effective time (microseconds since 1970.1.1)
	 */
#if defined(_WIN32) || defined(_WIN64)
	__int64 set_timer(aio_timer_callback* callback,
		__int64 delay, unsigned int id = 0);
#else
	long long int set_timer(aio_timer_callback* callback,
		long long int delay, unsigned int id = 0);
#endif

	/**
	 * Delete all timer task events in timer
	 * @param callback {aio_timer_callback*} Timer callback function class object
	 * @return {time_t} Timer effective time (microseconds since 1970.1.1)
	 */
#if defined(_WIN32) || defined(_WIN64)
	__int64 del_timer(aio_timer_callback* callback);
#else
	long long int del_timer(aio_timer_callback* callback);
#endif

	/**
	 * Delete timer task with specified ID number in timer
	 * @param callback {aio_timer_callback*} Timer callback function class object
	 * @param id {unsigned int} ID number of a timer task
	 * @return {time_t} Timer effective time (microseconds since 1970.1.1)
	 */
#if defined(_WIN32) || defined(_WIN64)
	__int64 del_timer(aio_timer_callback* callback, unsigned int id);
#else
	long long del_timer(aio_timer_callback* callback, unsigned int id);
#endif

	/**
	 * When timer is in locked state, users cannot release this timer causing
	 * memory leak.
	 * Through this function, timer in locked state can be deferred released by
	 * event
	 * engine when in unlocked state (calls aio_delay_free::destroy()), thereby
	 * avoiding
	 * memory leak problems
	 * @param callback {aio_delay_free*}
	 */
	void delay_free(aio_delay_free* callback);

	/**
	 * Get ACL_AIO handle
	 * @return {ACL_AIO*}
	 */
	ACL_AIO* get_handle() const;

	/**
	 * Get asynchronous engine type
	 * @return {aio_handle_type}
	 */
	aio_handle_type get_engine_type() const;

	/**
	 * Get number of asynchronous streams currently being monitored
	 * @return {int}
	 */
	int length() const;

	/**
	 * Check status of all asynchronous streams and trigger processing of ready
	 * asynchronous streams
	 * @return {bool} Whether asynchronous engine should be stopped
	 */
	bool check();

	/**
	 * Get number of events triggered in this event loop
	 * @return {int}
	 */
	int last_nready() const;

	/**
	 * Notify asynchronous stream engine to stop
	 */
	void stop();

	/**
	 * Reset internal state of asynchronous engine
	 */
	void reset();

	/**
	 * Set DNS server address list, format: ip1:port1;ip2:port2...
	 * @param addrs {const char*} DNS server address list, e.g.:
	 * 8.8.8.8:53;1.1.1.1:53
	 * @param timeout {int} DNS query timeout (seconds)
	 *  Note: set_dns and dns_add perform same function
	 */
	void set_dns(const char* addrs, int timeout);
	void dns_add(const char* addrs, int timeout);

	/**
	 * Delete specified DNS server address list, format: ip1:port1;ip2:port2...
	 * @param addrs {const char*} DNS server address list
	 */
	void dns_del(const char* addrs);

	/**
	 * Clear all set DNS server lists
	 */
	void dns_clear();

	/**
	 * Number of DNS server lists
	 * @return {size_t}
	 */
	size_t dns_size() const;

	/**
	 * Determine whether DNS server list is empty
	 * @return {bool}
	 */
	bool dns_empty() const;
	
	/**
	 * Get DNS server address list
	 * @param out {std::vector<std::pair<acl::string, unsigned short> >&}
	 */
	void dns_list(std::vector<std::pair<string, unsigned short> >& out);

public:
	/**
	 * Set second-level part of asynchronous engine loop's wait time
	 * @param n {int} Set second-level wait time when using
	 * select/poll/epoll/kqueue/devpoll
	 */
	void set_delay_sec(int n);

	/**
	 * Set microsecond-level part of asynchronous engine loop's wait time
	 * @param n {int} Set microsecond-level wait time when using
	 * select/poll/epoll/kqueue/devpoll
	 */
	void set_delay_usec(int n);

	/**
	 * Set time interval for periodically checking all descriptor status during
	 * event loop process.
	 * Internal default value is 100 ms
	 */
	void set_check_inter(int n);

	/**
	 * Set read buffer size of asynchronous stream
	 * @param n {int} Read buffer size
	 */
	void set_rbuf_size(int n);

protected:
	friend class aio_stream;

	/**
	 * Increment asynchronous stream count by 1
	 */
	void increase();

	/**
	 * Virtual callback function when asynchronous stream count increases by 1
	 */
	virtual void on_increase() {}

	/**
	 * Decrement asynchronous stream count by 1
	 */
	void decrease();

	/**
	 * Virtual callback function when asynchronous stream count decreases by 1
	 */
	virtual void on_decrease() {}

private:
	ACL_AIO* aio_;
	bool inner_alloc_;
	bool stop_;
	int  nstream_;
	aio_handle_type engine_type_;
	aio_timer_delay_free* delay_free_timer_;

	void destroy_timer(aio_timer_callback* callback);
	static void on_timer_callback(int event_type, ACL_EVENT*,
		aio_timer_callback *callback);
};

} // namespace acl

