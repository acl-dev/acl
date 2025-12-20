#pragma once
#include <stdarg.h>
#include <vector>
#include "../acl_cpp_define.hpp"
#include "../stdlib/string.hpp"
#include "../stdlib/noncopyable.hpp"
#include "../stream/socket_stream.hpp"

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_BEANSTALK_DISABLE)

struct ACL_ARGV;

namespace acl {

/**
 * Message ID starts from 1 and increments (see beanstalkd source code job.c for
 * details:
 *     static uint64 next_id = 1; and in make_job_with_id():
 *     if (id) {
 *         j->r.id = id;
 *         if (id >= next_id) next_id = id + 1;
 *     } else {
 *         j->r.id = next_id++;
 *     }
 * Message priority pri value range is 0 ~ 4,294,968,295 (maximum unsigned
 * value). Smaller value
 * means higher priority. Highest priority is 0.
 * Message default maximum length is 65,535 (maximum short value). This value
 * can be modified by beanstalkd command
 * line parameters. For details, see <beanstalk protocol specification.pdf> in
 * doc/ directory.
 * All operations internally automatically reconnect and retry. Before
 * operations, you need to call watch or use
 * operations. Internally automatically manages these operations. Generally, you
 * need to call open process explicitly, and users should
 * call close after use, which will disconnect from beanstalkd server
 * connection, and internally automatically releases
 * use and watch operations.
 */
class ACL_CPP_API beanstalk : public noncopyable {
public:
	/**
	 * Constructor
	 * @param addr {const char*} beanstalkd address, format: ip:port/domain:port
	 * @param conn_timeout {int} Connection server timeout time (seconds)
	 * @param retry {bool} Whether connection object automatically reconnects.
	 */
	beanstalk(const char* addr, int conn_timeout, bool retry = true);
	~beanstalk();

	/////////////////////////////////////////////////////////////////////
	// Producer interfaces

	/**
	 * Select sending tube to use.
	 * @param tube {const char*} Tube name.
	 * @return {bool} Whether successful.
	 */
	bool use(const char* tube);

	/**
	 * Send message to selected tube (default tube if not selected).
	 * @param data {const void*} Message data address. Can be binary data.
	 * @param len {size_t} Data buffer length.
	 * @param pri {unsigned} Priority. Smaller value means higher priority.
	 * @param delay {unsigned} Indicates how many seconds job needs to wait before
	 * entering ready state.
	 * @param ttr {unsigned} Indicates how many seconds a worker has to execute
	 * this message.
	 * @return {unsigned long long} Returns created message job number,
	 *  return value > 0 means successful, == 0 means failure.
	 *  (Check beanstalkd source code, you can see message number starts from 1)
	 */
	unsigned long long put(const void* data, size_t len,
		unsigned pri = 1024, unsigned delay = 0, unsigned ttr = 60);

	/**
	 * Send message to selected tube (default tube if not selected) in formatted
	 * string format.
	 * @param pri {unsigned} Priority. Smaller value means higher priority.
	 * @param delay {unsigned} Indicates how many seconds job needs to wait before
	 * entering ready state.
	 * @param ttr {unsigned} Indicates how many seconds a worker has to execute
	 * this message.
	 * @param fmt {const char*} Format string.
	 * @return {unsigned long long} Returns created message job number,
	 *  return value > 0 means successful, == 0 means failure.
	 *  (Check beanstalkd source code, you can see message number starts from 1)
	 */
	unsigned long long format_put(unsigned pri, unsigned delay, unsigned ttr,
		const char* fmt, ...) ACL_CPP_PRINTF(5, 6);

	unsigned long long vformat_put(const char* fmt, va_list ap,
		unsigned pri = 1024, unsigned delay = 0, unsigned ttr = 60);

	/**
	 * Send message to selected tube (default tube if not selected) in formatted
	 * string format. All
	 * pri, delay, ttr parameters use default values.
	 * @param fmt {const char*} Format string.
	 * @return {unsigned long long} Returns created message job number,
	 *  return value > 0 means successful, == 0 means failure.
	 *  (Check beanstalkd source code, you can see message number starts from 1)
	 */
	unsigned long long format_put(const char* fmt, ...) ACL_CPP_PRINTF(2, 3);

	/////////////////////////////////////////////////////////////////////
	// Consumer interfaces

	/**
	 * Select tube to get messages from. Add to watch list,
	 * if you don't call this function, default tube (default) will be used.
	 * @param tube {const char*} Message tube name.
	 * @return {unsigned} Return value is number of watched message tubes. Return
	 * value > 0 means success.
	 */
	unsigned watch(const char* tube);

	/**
	 * Remove one tube from watched (watch) message tubes.
	 * @param tube {const char*} Message tube name.
	 * @return {unsigned} Return value is number of remaining watched message
	 * tubes. Return value > 0 means
	 * success (if you need to watch a default message tube, after correctly
	 * calling this function, return value will be 1).
	 *  Return value 0 means this tube was not watched or removal failed.
	 */
	unsigned ignore(const char* tube);

	/**
	 * Remove all watched message tubes.
	 * @return {unsigned} Return value is number of remaining watched message
	 * tubes. Return value > 0 means
	 * success (if you need to watch a default message tube, after correctly
	 * calling this function, return value will be 1).
	 *  Otherwise 0 means removal failed.
	 */
	unsigned ignore_all();

	/**
	 * Get a message from watched tubes and delete it. This operation
	 * waits for timeout. When timeout is -1, it waits indefinitely for messages.
	 * @param buf {string&} Buffer to store obtained message. Internally first
	 * clears this buffer.
	 * @param timeout {int} Timeout value for waiting for available messages. When
	 * -1,
	 * it waits indefinitely. When > 0, if no message is available within this
	 * time, returns.
	 *  When == 0, immediately returns if no message is available.
	 * @return {unsigned long long} Returns obtained message number. Return value >
	 * 0
	 * means correctly got a message. Otherwise means timeout or no message
	 * available. When
	 * return value is 0, you can call get_error() to get error. When TIMED_OUT, it
	 * means
	 * timeout. When DEADLINE_SOON, it means message has been reserved and within
	 * specified ttr
	 *  (timeout time) has not been delete_id.
	 */
	unsigned long long reserve(string& buf, int timeout = -1);

	/**
	 * Delete message with specified ID number from queue.
	 * @param id {unsigned long long} Message number.
	 * @return {bool} Whether deletion was successful.
	 */
	bool delete_id(unsigned long long id);

	/**
	 * Put a message that has been reserved back to ready state (change job state
	 * to "ready").
	 * This message can be obtained by other workers.
	 * @param id {unsigned long long} Message number.
	 * @param pri {unsigned} Priority.
	 * @param delay {unsigned} How many seconds to wait before this message enters
	 * ready state.
	 * @return {bool} Whether successful.
	 */
	bool release(unsigned long long id, unsigned pri = 1024,
		unsigned delay = 0);

	/**
	 * Change a message state to "buried". Buried messages are stored in a FIFO
	 * queue,
	 * and these messages will not be processed by workers until client calls kick
	 * function.
	 * @param id {unsigned long long} Message number.
	 * @param pri {unsigned int} Priority.
	 * @return {bool} Whether successful.
	 */
	bool bury(unsigned long long id, unsigned pri = 1024);

	/**
	 * Extend time for a worker to reserve a message for execution. This is very
	 * useful for messages that need
	 * long time to complete. It can also be used to transfer a message from one
	 * worker that cannot complete it
	 * to another worker. A worker can extend time by calling this function
	 * multiple times
	 * during job execution (e.g., when receiving DEADLINE_SOON, you can call this
	 * function to extend time).
	 * @param id {unsigned long long} Message number.
	 * @return {bool} Whether successful.
	 */
	bool touch(unsigned long long id);

	/////////////////////////////////////////////////////////////////////
	// Common interfaces

	/**
	 * Open beanstalkd server connection. Generally, you don't need to explicitly
	 * call this function to open connection, because
	 * it automatically connects. You need to call this function to enable
	 * automatic reconnection.
	 * @return {bool}  Whether successful.
	 */
	bool open();

	/**
	 * Explicitly close beanstalkd server connection. When object is destroyed, it
	 * will attempt to call this close process,
	 * and this function internally releases tube_used_ and tubes_watched_.
	 */
	void close();

	/**
	 * Notify beanstalkd server to exit (server will close connection after
	 * receiving this command).
	 */
	void quit();

	/**
	 * Get message body for specified message number.
	 * @param buf {string&} Message body buffer to store obtained message.
	 * Internally first clears this buffer.
	 * @param id {unsigned long long} Specified message number.
	 * @return {unsigned long long} Returns obtained ready state message number,
	 *  return value > 0 means got a message. Otherwise means no message available.
	 */
	unsigned long long peek(string& buf, unsigned long long id);

	/**
	 * Get a ready state message from currently watched (watch) tubes.
	 * This message will not be deleted.
	 * @param buf {string&} Message body buffer to store obtained message.
	 * Internally first clears this buffer.
	 * @return {unsigned long long} Returns obtained ready state message number,
	 *  return value > 0 means got a message. Otherwise means no message available.
	 */
	unsigned long long peek_ready(string& buf);

	/**
	 * Get a delayed state message from currently watched (watch) tubes.
	 * This message will not be deleted.
	 * @param buf {string&} Message body buffer to store obtained message.
	 * Internally first clears this buffer.
	 * @return {unsigned long long} Returns obtained delayed state message number,
	 *  return value > 0 means got a message. Otherwise means no message available.
	 */
	unsigned long long peek_delayed(string& buf);

	/**
	 * Get a buried state message from currently watched (watch) tubes.
	 * This message will not be deleted.
	 * @param buf {string&} Message body buffer to store obtained message.
	 * Internally first clears this buffer.
	 * @return {unsigned long long} Returns obtained buried state message number,
	 *  return value > 0 means got a message. Otherwise means no message available.
	 */
	unsigned long long peek_buried(string& buf);

	/**
	 * Move messages in buried or delayed state to ready state, only for currently
	 * used tube.
	 * @param n {unsigned} Indicates maximum number of messages to kick each time,
	 *  server will kick at most n messages.
	 * @return {int} Indicates number of messages actually kicked. Returns -1 to
	 * indicate error.
	 */
	int  kick(unsigned n);

	/**
	 * Get currently used message tube for this client.
	 * @param buf {string&} Buffer to store currently used message tube. Internally
	 * first clears this buffer.
	 * @return {bool} Whether successfully got.
	 */
	bool list_tube_used(string&buf);

	/**
	 * Get list of all existing message tubes (tube).
	 * @param buf {string&} Buffer to store result. Internally first clears this
	 * buffer.
	 * @return {bool} Whether successfully got.
	 */
	bool list_tubes(string& buf);

	/**
	 * Get list of currently watched (watch) message tubes.
	 * @param buf {string&} Buffer to store result. Internally first clears this
	 * buffer.
	 * @return {bool} Whether successfully got.
	 */
	bool list_tubes_watched(string& buf);

	/**
	 * Temporarily pause getting messages from specified message tube (tube).
	 * @param tube {const char*} Message tube name.
	 * @param delay {unsigned} Specified time period.
	 * @return {bool} Whether successful.
	 */
	bool pause_tube(const char* tube, unsigned delay);

	/////////////////////////////////////////////////////////////////////
	// Common interfaces
	const char* get_error() const {
		return errbuf_.c_str();
	}

	socket_stream& get_conn() {
		return conn_;
	}

	/**
	 * Get beanstalkd server address passed to constructor, format: ip:port
	 * @return {const char*} Returns non-empty beanstalkd server address.
	 */
	const char* get_addr() const {
		return addr_;
	}

private:
	char* addr_;
	int   timeout_;
	bool  retry_;
	string  errbuf_;
	char* tube_used_;
	std::vector<char*> tubes_watched_;
	socket_stream conn_;
	unsigned long long peek_fmt(string& buf, const char* fmt, ...)
		ACL_CPP_PRINTF(3, 4);
	bool list_tubes_fmt(string& buf, const char* fmt, ...)
		ACL_CPP_PRINTF(3, 4);

	unsigned ignore_one(const char* tube);
	bool beanstalk_open();
	bool beanstalk_use();
	unsigned beanstalk_watch(const char* tube);
	ACL_ARGV* beanstalk_request(const string& cmdline,
		const void* data = NULL, size_t len = 0);
};

} // namespace acl

#endif // !defined(ACL_CLIENT_ONLY) && !defined(ACL_BEANSTALK_DISABLE)

