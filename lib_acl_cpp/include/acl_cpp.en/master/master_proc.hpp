#pragma once
#include "master_base.hpp"

#ifndef ACL_CLIENT_ONLY

struct ACL_VSTREAM;
struct ACL_EVENT;
struct ACL_VSTRING;

namespace acl {

class socket_stream;

/**
 * Template class for process mode in acl_master server framework. Only one instance of objects of this class can run
 */
class ACL_CPP_API master_proc : public master_base {
public:
	/**
	 * Start running. Calling this function indicates that the service process runs under the control of acl_master service framework,
	 * generally used in production environment
	 * @param argc {int} First parameter passed from main, indicates number of parameters
	 * @param argv {char**} Second parameter passed from main
	 */
	void run_daemon(int argc, char** argv);

	/**
	 * Processing function when running standalone. Users can call this function to perform necessary debugging work
	 * @param addrs {const char*} Service listening address list, format: IP:PORT, IP:PORT...
	 * @param path {const char*} Full path of configuration file
	 * @param count {int} When this value > 0, after receiving this number of connections and completing them,
	 *  this function will return, otherwise it will continuously loop receiving remote connections
	 * @return {bool} Whether listening was successful
	 */
	bool run_alone(const char* addrs, const char* path = NULL, int count = 1);

	/**
	 * Get configuration file path
	 * @return {const char*} Returns NULL if configuration file was not set
	 */
	const char* get_conf_path() const;

protected:
	master_proc();
	virtual ~master_proc();

	/**
	 * Pure virtual function: Called when a client connection is received
	 * @param stream {aio_socket_stream*} Newly received client asynchronous stream object
	 * Note: After this function returns, the stream connection will be closed. Users should not actively close this stream
	 */
	virtual void on_accept(socket_stream* stream) = 0;

private:
	// Callback function called when a client connection is received
	static void service_main(void*, ACL_VSTREAM *stream);

	// Callback function called when listening on a service address
	static void service_on_listen(void*, ACL_VSTREAM*);

	// Callback function called after process switches user identity
	static void service_pre_jail(void*);

	// Callback function called after process switches user identity
	static void service_init(void*);

	// Callback function called before process exits
	static int service_pre_exit(void*);

	// Callback function called when process exits
	static void service_exit(void*);

	// Called when process receives SIGHUP signal
	static int service_on_sighup(void*, ACL_VSTRING*);

private:
	// In standalone running mode, this function is called when listening socket has a new connection
	static void listen_callback(int event_type, ACL_EVENT*,
		ACL_VSTREAM*, void* context);

private:
	bool stop_;
	int  count_limit_;
	int  count_;
};

}  // namespace acl

#endif // ACL_CLIENT_ONLY

