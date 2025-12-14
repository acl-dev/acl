#pragma once
#include "master_base.hpp"

#ifndef ACL_CLIENT_ONLY

struct ACL_VSTRING;

namespace acl {

/**
 * Template class for triggers in acl_master server framework. Only one instance of objects of this class can run
 */
class ACL_CPP_API master_trigger : public master_base {
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
	 * @param path {const char*} Full path of configuration file
	 * @param count {int} When this value > 0, after receiving this number of connections and completing them,
	 *  this function will return, otherwise it will continuously loop receiving remote connections
	 * @param interval {int} Trigger time interval (seconds)
	 */
	void run_alone(const char* path = NULL, int count = 1, int interval = 1);

	/**
	 * Get configuration file path
	 * @return {const char*} Returns NULL if configuration file was not set
	 */
	const char* get_conf_path() const;

protected:
	master_trigger();
	virtual ~master_trigger();

	/**
	 * Called when trigger time arrives
	 */
	virtual void on_trigger() = 0;

private:
	// Called by acl_master framework when trigger time arrives
	static void service_main(void*);

	// Callback function called after process switches user identity
	static void service_pre_jail(void*);

	// Callback function called after process switches user identity
	static void service_init(void*);

	// Callback function called when process exits
	static void service_exit(void*);

	// Called when process receives SIGHUP signal
	static int service_on_sighup(void*, ACL_VSTRING*);
};

}  // namespace acl

#endif // ACL_CLIENT_ONLY

