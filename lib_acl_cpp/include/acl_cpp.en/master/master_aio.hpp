#pragma once
#include "../stdlib/thread_mutex.hpp"
#include "../stream/aio_handle.hpp"
#include "../stream/aio_listen_stream.hpp"
#include "master_base.hpp"

#ifndef ACL_CLIENT_ONLY

struct ACL_VSTREAM;
struct ACL_VSTRING;

namespace acl {

class aio_handle;
class aio_socket_stream;

/**
 * Template class for single-threaded non-blocking mode in acl_master server
 * framework. Only one instance of objects of this class can run
 */
class ACL_CPP_API master_aio : public master_base, public aio_accept_callback {
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
	 * @param ht {aio_handle_type} Type of event engine
	 * @return {bool} Whether listening was successful
	 */
	bool run_alone(const char* addrs, const char* path = NULL,
		aio_handle_type ht = ENGINE_SELECT);

	/**
	 * Get asynchronous IO event engine handle. Through this handle, users can set
	 * timers and other functions
	 * @return {aio_handle*}
	 */
	aio_handle* get_handle() const;

	/**
	 * In run_alone mode, notify server framework to close engine and exit program
	 */
	void stop();

	/**
	 * Get configuration file path
	 * @return {const char*} Returns NULL if configuration file was not set
	 */
	const char* get_conf_path() const;

protected:
	master_aio();
	virtual ~master_aio();

	/**
	 * Pure virtual function: Called when a client connection is received
	 * @param stream {aio_socket_stream*} Newly received client asynchronous stream
	 * object
	 * @return {bool} If this function returns false, it notifies server framework
	 * to stop receiving
	 *  remote client connections, otherwise continues receiving client connections
	 */
	virtual bool on_accept(aio_socket_stream* stream) = 0;

private:
	aio_handle* handle_;
	/**
	 * Implementation of base class aio_accept_callback virtual function
	 * @param client {aio_socket_stream*} Asynchronous client stream
	 * @return {bool} Returns true to notify listening stream to continue listening
	 */
	virtual bool accept_callback(aio_socket_stream* client);

private:
	thread_mutex lock_;
	void push_back(server_socket* ss);

private:
#if defined(_WIN32) || defined(_WIN64)
	// Callback function called when a client connection is received
	static void service_main(SOCKET, void*);
#else
	static void service_main(int, void*);
#endif

	// Callback function called when listening on a service address
	static void service_on_listen(void*, ACL_VSTREAM*);

	// Callback function called after process switches user identity
	static void service_pre_jail(void*);

	// Callback function called after process switches user identity
	static void service_init(void*);

	// Callback function called when process exits
	static int service_pre_exit(void*);

	// Callback function called when process exits
	static void service_exit(void*);

	// Called when process receives SIGHUP signal
	static int service_on_sighup(void*, ACL_VSTRING*);
};

}  // namespace acl

#endif // ACL_CLIENT_ONLY

