#pragma once
#include "master_base.hpp"
#include "../stdlib/thread_mutex.hpp"

#ifndef ACL_CLIENT_ONLY

struct ACL_VSTREAM;
struct ACL_EVENT;
struct ACL_VSTRING;
struct acl_pthread_pool_t;

namespace acl {

class socket_stream;

/**
 * Thread pool server framework class. This class is pure virtual class.
 * Subclasses need to implement its pure virtual functions.
 * Each process can only have one instance of this class object, otherwise
 * program will be terminated
 */
class ACL_CPP_API master_threads : public master_base {
public:
	/**
	 * Start running. Calling this function means this service process runs under
	 * control of acl_master service framework,
	 * generally used in production machine state
	 * @param argc {int} First parameter passed from main, indicates number of
	 * parameters
	 * @param argv {char**} Second parameter passed from main
	 */
	void run_daemon(int argc, char** argv);

	/**
	 * Processing function when running standalone. Users can call this function
	 * for necessary debugging work
	 * @param addrs {const char*} Listening address list, format: IP:PORT,
	 * IP:PORT...
	 * @param path {const char*} Full path of configuration file
	 * @param count {unsigned int} Number of service loops. Function automatically
	 * returns after reaching this value.
	 * If this value is 0, it means program continuously processes incoming
	 * requests without returning
	 * @param threads_count {int} When this value is greater than 1, automatically
	 * uses thread pool method.
	 * This value is only effective when count != 1, i.e., if count == 1, only runs
	 * once then returns
	 *  and will not start threads to handle client requests
	 * @return {bool} Whether listening was successful
	 * Note: count, threads_count two parameters are no longer effective, will use
	 * configuration values in configuration file
	 * ioctl_use_limit (controls number of connections processed) and
	 * ioctl_max_threads (
	 *  controls maximum number of threads started)
	 */
	bool run_alone(const char* addrs, const char* path = NULL,
		unsigned int count = 1, int threads_count = 1);
	
	/**
	 * Monitor readable state of given stream
	 * @param stream {socket_stream*}
	 */
	void thread_enable_read(socket_stream* stream);

	/**
	 * Stop monitoring readable state of given stream
	 * @param stream {socket_stream*}
	 */
	void thread_disable_read(socket_stream* stream);

	/**
	 * Get configuration file path
	 * @return {const char*} Returns NULL indicates no configuration file is set
	 */
	const char* get_conf_path() const;

	/**
	 * Get number of pending tasks backlogged in current thread pool queue. This
	 * API can help applications decide when
	 * overload protection is needed, discarding subsequent tasks when under high
	 * pressure
	 * @return {size_t}
	 */
	size_t task_qlen() const;

public:
	/**
	 * Get thread pool handle in lib_acl C library
	 * @return {acl_pthread_pool_t*}
	 */
	acl_pthread_pool_t* threads_pool() const;

protected:
	// This class cannot be directly instantiated
	master_threads();
	virtual ~master_threads();

	/**
	 * Pure virtual function: Called when a client connection has data readable or
	 * closes or errors occur
	 * @param stream {socket_stream*}
	 * @return {bool} Returns false indicates connection needs to be closed after
	 * function returns,
	 * otherwise indicates need to maintain long connection. If this stream has
	 * error, application should return false
	 */
	virtual bool thread_on_read(socket_stream* stream) = 0;

	/**
	 * After framework calls thread_on_read and it returns true, will automatically
	 * call this function
	 * to determine whether to monitor stream object as readable
	 * @param stream {socket_stream*}
	 * @return {bool} If returns false, framework no longer monitors this stream
	 * object
	 */
	virtual bool keep_read(socket_stream* stream) {
		(void) stream;
		return true;
	}

	/**
	 * Callback function when a connection is obtained by a thread in thread pool.
	 * Subclasses can do some
	 * initialization work. This function runs in main thread's thread space
	 * @param stream {socket_stream*}
	 * @return {bool} If returns false, it indicates subclass requires closing
	 * connection, and does not
	 *  need to pass this connection to thread_main process
	 */
	virtual bool thread_on_accept(socket_stream* stream) {
		(void) stream;
		return true;
	}

	/**
	 * After receiving a client connection, server calls this function to perform
	 * handshake operation with client.
	 * This function will be called after thread_on_accept
	 * @return {bool} If returns false, it indicates subclass requires closing
	 * connection, and does not
	 *  need to pass this connection to thread_main process
	 */
	virtual bool thread_on_handshake(socket_stream *stream) {
		(void) stream;
		return true;
	}

	/**
	 * Callback function when IO read/write of a network connection times out. If
	 * this function returns true, it
	 * indicates continue waiting for next read/write, otherwise hopes to close
	 * this connection
	 * @param stream {socket_stream*}
	 * @return {bool} If returns false, it indicates subclass requires closing
	 * connection, otherwise requires
	 *  continuing to monitor this connection
	 */
	virtual bool thread_on_timeout(socket_stream* stream) {
		(void) stream;
		return false;
	}

	/**
	 * Callback function when connection bound to a thread closes
	 * @param stream {socket_stream*}
	 * Note: This function will not be called when stream closes after
	 * thread_on_accept returns false
	 */
	virtual void thread_on_close(socket_stream* stream) { (void) stream; }

	/**
	 * Callback function when a new thread in thread pool is created
	 */
	virtual void thread_on_init() {}

	/**
	 * Callback function when a thread in thread pool exits
	 */
	virtual void thread_on_exit() {}

	/**
	 * When child process needs to exit, framework will call this function.
	 * Framework determines whether child process exits based on:
	 * 1) If this function returns true, child process exits immediately,
	 * otherwise:
	 * 2) If all client connections of this child process have closed, child
	 * process exits immediately, otherwise:
	 * 3) Check configuration item (ioctl_quick_abort) in configuration file. If
	 * this configuration item is not 0, then
	 *    child process exits immediately, otherwise:
	 * 4) Wait for all client connections to close before exiting
	 * @param nclients {size_t} Current number of connected clients
	 * @param nthreads {size_t} Current number of busy worker threads in thread
	 * pool
	 * @return {bool} Returns false indicates current child process cannot exit
	 * yet, otherwise indicates current
	 *  child process can exit
	 */
	virtual bool proc_exit_timer(size_t nclients, size_t nthreads) {
		(void) nclients;
		(void) nthreads;
		return true;
	}

private:
	thread_mutex lock_;

	void push_back(server_socket* ss);
	void run(int argc, char** argv);

	// Callback function when a client connection is received
	static int service_main(void*, ACL_VSTREAM*);

	// Callback function when a service address is monitored
	static void service_on_listen(void*, ACL_VSTREAM*);

	// Callback function when a client connection is received, can do some
	// initialization
	static int service_on_accept(void*, ACL_VSTREAM*);

	// When server needs to do some preliminary handshake actions with client after
	// receiving client connection,
	// this function is called. This function will be called after
	// service_on_accept
	static int service_on_handshake(void*, ACL_VSTREAM*);

	// Callback function when client connection read/write times out
	static int service_on_timeout(void*, ACL_VSTREAM*);

	// Callback function when client connection closes
	static void service_on_close(void*, ACL_VSTREAM*);

	// Callback function called after process switches user identity
	static void service_pre_jail(void*);

	// Callback function called after process switches user identity
	static void service_init(void*);

	// Called when process needs to exit, application determines whether process
	// needs to exit
	static int service_exit_timer(void*, size_t, size_t);

	// Callback function called when process exits
	static int service_pre_exit(void*);

	// Callback function called when process exits
	static void service_exit(void*);

	// Callback function called after thread is created
	static int thread_init(void*);

	// Callback function called before thread exits
	static void thread_exit(void*);

	// This function is called when process receives SIGHUP signal
	static int service_on_sighup(void*, ACL_VSTRING*);
};

} // namespace acl

#endif // ACL_CLIENT_ONLY

