#pragma once
#include "fiber_cpp_define.hpp"
//#include "acl_cpp/master/master_base.hpp"

struct ACL_VSTREAM;

namespace acl {

class socket_stream;

/**
 * Network service class based on coroutine.
 */
class FIBER_CPP_API master_fiber : public master_base {
public:
	/**
	 * Run this network service under the acl_master framework.
	 * @param argc {int} The size of the parameter array passed in.
	 * @param argv {char**} The parameter array passed in.
	 */
	void run_daemon(int argc, char** argv);

	/**
	 * Start this network service in standalone mode.
	 * @param addrs {const char*} List of local service addresses to monitor;
	 *  The format looks like ip:port, ip:port, ...
	 * @param path {const char*} Specify the configuration file path when it
	 * is not NULL.
	 */
	bool run_alone(const char* addrs, const char* path = NULL);

	/**
	 * Get the configuration file path.
	 * @return {const char*} A return value of NULL indicates that no
	 *  configuration file is set.
	 */
	const char* get_conf_path() const;

	/**
	 * Get the total number of connections of the current service.
	 * @return {long long}
	 */
	long long users_count();

	/**
	 * Increase the number of current connections.
	 * @param n {int} Increase or decrease (can be negative) the value of
	 *  the connection.
	 * @return {long long} Returns the number of modified connections.
	 */
	long long users_count_add(int n);

protected:
	master_fiber();

	virtual ~master_fiber();

	/**
	 * Pure virtual function, called when the coroutine server receives
	 * a client connection.
	 * @param stream {socket_stream&} Client connection object. After this
	 *  function returns, the coroutine service framework will close the
	 *  connection object.
	 */
	virtual void on_accept(socket_stream& stream) = 0;

	/**
	 * The virtual function will be called when the thread is initialized.
	 */
	virtual void thread_on_init() {}

	/**
	 * The virtual function will be called before thre thread exit.
	 */
	virtual void thread_on_exit() {}

private:
	static void service_on_listen(void*, ACL_VSTREAM*);
	static void service_on_accept(void*, ACL_VSTREAM*);
	static void service_pre_jail(void*);
	static void service_init(void*);
	static void thread_init(void*);
	static void thread_exit(void*);
	static int  service_pre_exit(void*);
	static void service_exit(void*);
	static int  service_on_sighup(void*, ACL_VSTRING*);

	void run(int argc, char** argv);
};

} // namespace acl
