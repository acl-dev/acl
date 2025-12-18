#ifndef ACL_PROCTL_INCLUDE_H
#define ACL_PROCTL_INCLUDE_H

#include "../stdlib/acl_define.h"
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Get control process's executable program's path location.
 * @param buf {char*} Storage memory location, returned result's end
 *  will not contain "\" or "/", e.g., "C:\\test_path\\test1_path", will not
 *  be "C:\\test_path\\test1_path\\"
 * @param size {size_t} buf space size
 */
ACL_API void acl_proctl_daemon_path(char *buf, size_t size);

/**
 * Initialize process control framework, must be called before acl_proctl_start.
 * @param progname {const char*} Control process program name
 */
ACL_API void acl_proctl_deamon_init(const char *progname);

/**
 * Control process runs as daemon process, continuously monitors
 * child process status.
 * If child process exits abnormally, restarts child process.
 */
ACL_API void acl_proctl_daemon_loop(void);

/**
 * Start a child process in control process.
 * @param progchild {const char*} Child process's program name
 * @param argc {int} argv array length
 * @param argv {char* []}
 * @return 0: ok; -1: error
 */
ACL_API int acl_proctl_deamon_start_one(const char *progchild, int argc, char *argv[]);

/**
 * Start a child process in standalone mode.
 * @param progname {const char*} Control process program name
 * @param progchild {const char*} Child process program name
 * @param argc {int} argv array length
 * @param argv {char* []} Parameters passed to child process
 */
ACL_API void acl_proctl_start_one(const char *progname,
	const char *progchild, int argc, char *argv[]);

/**
 * Stop a child process in standalone mode.
 * @param progname {const char*} Control process program name
 * @param progchild {const char*} Child process program name
 * @param argc {int} argv array length
 * @param argv {char* []} Parameters passed to child process
 */
ACL_API void acl_proctl_stop_one(const char *progname,
	const char *progchild, int argc, char *argv[]);

/**
 * Stop all child processes in standalone mode.
 * @param progname {const char*} Control process program name
 */
ACL_API void acl_proctl_stop_all(const char *progname);

/**
 * Notify control process to stop all child processes in
 * standalone mode. When child process exits, control process
 * will also automatically exit.
 * @param progname {const char*} Control process program name
 */
ACL_API void acl_proctl_quit(const char *progname);

/**
 * List all service processes currently managed by control process.
 * @param progname {const char*} Control process program name
 */
ACL_API void acl_proctl_list(const char *progname);

/**
 * Probe whether a certain child process is running.
 * @param progname {const char*} Control process program name
 * @param progchild {const char*} Child process program name
 */
ACL_API void acl_proctl_probe(const char *progname, const char *progchild);

/**
 * Child process interface, establishes parent/child control
 * relationship through this interface.
 * @param progname {const char*} Child process program name
 * @param onexit_fn {void (*)(void*)} If not NULL, callback
 *  function called when child process exits
 * @param arg {void*} One of onexit_fn's parameters
 */
ACL_API void acl_proctl_child(const char *progname, void (*onexit_fn)(void *), void *arg);

#ifdef __cplusplus
}
#endif

#endif
