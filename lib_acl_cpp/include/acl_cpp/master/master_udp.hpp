#pragma once
#include "../acl_cpp_define.hpp"
#include "master_base.hpp"
#include "../stdlib/thread_mutex.hpp"

#ifndef ACL_CLIENT_ONLY

struct ACL_VSTRING;

namespace acl {

class ACL_CPP_API master_udp : public master_base {
public:
	/**
	 * Start running. Calling this function indicates that the service process runs
	 * under the control of acl_master service framework,
	 * generally used in production environment
	 * @param argc {int} First parameter passed from main, indicates number of
	 * parameters
	 * @param argv {char**} Second parameter passed from main
	 */
	void run_daemon(int argc, char** argv);

	/**
	 * Processing function when running standalone. Users can call this function to
	 * perform necessary debugging work
	 * @param addrs {const char*} Service listening address list, format: IP:PORT,
	 * IP:PORT...
	 * @param path {const char*} Full path of configuration file
	 * @param count {unsigned int} Number of service loops. After reaching this
	 * value, function automatically returns;
	 * if this value is 0, it means the program continuously loops processing
	 * incoming requests without returning
	 * @return {bool} Whether listening was successful
	 */
	bool run_alone(const char* addrs, const char* path = NULL,
		unsigned int count = 1);

protected:
	// Objects of this class cannot be directly instantiated
	master_udp();
	virtual ~master_udp();

	/**
	 * Pure virtual function: When UDP stream has data to read, callback subclass
	 * this function. This method is called in child thread
	 * @param stream {socket_stream*}
	 */
	virtual void on_read(socket_stream* stream) = 0;

	/**
	 * Virtual method callback when UDP address is successfully bound. This method
	 * is called in child thread
	 */
	virtual void proc_on_bind(socket_stream&) {}

	/**
	 * Virtual method callback when UDP address is unbound. This method is called
	 * in child thread
	 */
	virtual void proc_on_unbind(socket_stream&) {}

	/**
	 * Virtual method that will be called when thread is initialized
	 */
	virtual void thread_on_init() {}

	/**
	 * Get collection of locally listening socket stream objects
	 * @return {const std::vector<socket_stream*>&}
	 */
	const std::vector<socket_stream*>& get_sstreams() const {
		return sstreams_;
	}

	/**
	 * Get configuration file path
	 * @return {const char*} Returns NULL if configuration file was not set
	 */
	const char* get_conf_path() const;

public:
	void lock();
	void unlock();

private:
	std::vector<socket_stream*> sstreams_;
	thread_mutex lock_;

	void run(int argc, char** argv);
	void push_back(socket_stream* ss);
	void remove(socket_stream* ss);

private:
	// Callback function called when a client connection is received
	static void service_main(void*, ACL_VSTREAM*);

	// Callback function after address is successfully bound
	static void service_on_bind(void*, ACL_VSTREAM*);

	// Callback function when address is unbound
	static void service_on_unbind(void*, ACL_VSTREAM*);

	// Callback function called after process switches user identity
	static void service_pre_jail(void*);

	// Callback function called after process switches user identity
	static void service_init(void*);

	// Callback function called when process exits
	static int service_pre_exit(void*);

	// Callback function called when process exits
	static void service_exit(void*);

	// Callback function called when thread starts
	static void thread_init(void*);

	// Called when process receives SIGHUP signal
	static int service_on_sighup(void*, ACL_VSTRING*);
};

} // namespace acl

#endif // ACL_CLIENT_ONLY

