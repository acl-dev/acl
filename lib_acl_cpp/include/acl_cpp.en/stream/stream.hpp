#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/string.hpp"
#include "../stdlib/noncopyable.hpp"
#include <map>

struct ACL_VSTREAM;

namespace acl {

/**
 * Time unit type
 */
typedef enum {
	time_unit_s,	// Seconds
	time_unit_ms,	// Milliseconds
	time_unit_us,	// Microseconds
	time_unit_ns,	// Nanoseconds
} time_unit_t;

class stream_hook;
class dbuf_pool;

class ACL_CPP_API stream : public noncopyable {
public:
	stream();
	virtual ~stream() = 0;

	/**
	 * Call this function to close stream connection
	 * @return {bool} true: Close successful; false: Close failed
	 */
	bool close();

	/**
	* Determine whether stream has ended
	* @return {bool} true: Stream has ended; false: Stream has not ended
	*/
	bool eof() const;

	/**
	 * Clear stream end flag, i.e., set eof_ flag to false
	 */
	void clear_eof();

	/**
	* Whether current stream is in open state
	* @return {bool} true: Stream is open; false: Stream is not open
	*/
	bool opened() const;

	/**
	 * Get ACL_VSTREAM stream object of current stream
	 * @return {ACL_VSTREAM*}
	 */
	ACL_VSTREAM* get_vstream() const;

	/**
	 * Unbind binding relationship between ACL_VSTREAM and stream object, and return ACL_VSTREAM
	 * to user, i.e., hand over management of this ACL_VSTREAM to user. This stream object will
	 * not close this ACL_VSTREAM when released, but user must close this ACL_VSTREAM after taking over and using it.
	 * After unbinding, except for close/open calls, other calls
	 * (including stream object read/write) are meaningless
	 * @return {ACL_VSTREAM} Returns NULL indicates stream object has already unbound ACL_VSTREAM
	 */
	ACL_VSTREAM* unbind();

	/**
	 * Set bound object of stream
	 * @param ctx {void*}
	 * @param key {const char* } Key identifying this ctx
	 * @param replace {bool} Whether to allow overwrite when corresponding KEY exists
	 * @return {bool} When replace is false and key already exists, returns false
	 */
	bool set_ctx(void* ctx, const char* key = NULL, bool replace = true);

	/**
	 * Get object bound to stream
	 * @param key {const char* key} When not empty, uses this key to query corresponding ctx object,
	 *  otherwise returns default ctx object
	 * @return {void*}
	 */
	void* get_ctx(const char* key = NULL) const;

	/**
	 * Delete bound object in stream
	 * @param key {const char*} When not empty, deletes ctx object corresponding to this key, otherwise deletes
	 *  default ctx object
	 * @return {void*} Returns NULL when object does not exist. After successful deletion, returns this object
	 */
	void* del_ctx(const char* key = NULL);

	/**
	 * Set read/write timeout of stream. Only effective when called after internal stream object is established
	 * @param n {int} Timeout time. When this value >= 0, enables timeout detection process, otherwise will
	 *  block until readable or error occurs. Unit of this value depends on second parameter
	 * @param use_sockopt {bool} Whether to use setsockopt to set timeout
	 * @return {bool}
	 */
	bool set_rw_timeout(int n, bool use_sockopt = false);

	/**
	 * Set internal timeout unit type. Only effective when called after internal stream object is established
	 * @param unit {time_unit_t} Time unit type
	 */
	void set_time_unit(time_unit_t unit);

	/**
	 * Get read/write timeout of current stream
	 * @param use_sockopt {bool} Whether to get timeout set by setsockopt
	 * @return {int} Get read/write timeout of stream (seconds)
	 */
	int get_rw_timeout(bool use_sockopt = false) const;

	/**
	 * Register read/write stream object. Internally automatically calls hook->open process. If successful, returns NULL
	 * indicating this registered object has been bound to stream, will be unbound and released when stream is released;
	 * If failed, returns pointer same as input parameter. Application can determine whether stream object registration was successful
	 * by checking whether return value is same as input value, and returned registered object should be released by caller on failure.
	 * xxx: Before calling this method, must ensure stream connection has been created
	 * @param hook {stream_hook*} Non-empty object pointer
	 * @return {stream_hook*} Return value different from input value indicates success
	 */
	stream_hook* setup_hook(stream_hook* hook);

	/**
	 * Get currently registered stream read/write object
	 * @return {stream_hook*}
	 */
	stream_hook* get_hook() const;

	/**
	 * Delete currently registered stream read/write object and return this object, restore default read/write process
	 * @return {stream_hook*}
	 */
	stream_hook* remove_hook();

public:
	/**
	 * Because stream has long lifecycle, users using internal buffer in stream object can
	 * appropriately reduce frequent creation and release of buffers
	 * @return {string&}
	 */
	string& get_buf();

	/**
	 * Get dbuf memory allocator with same lifecycle as stream
	 * @return {dbuf_pool&}
	 */
	dbuf_pool& get_dbuf();

protected:
	/**
	 * Open stream object. If stream is already open, will not open again
	 */
	void open_stream(bool is_file = false);

	/**
	 * Reopen stream object. If stream is already open, releases stream object first then opens
	 */
	void reopen_stream(bool is_file = false);

protected:
	stream_hook* hook_;
	ACL_VSTREAM *stream_;
	string* buf_;
	dbuf_pool* dbuf_;

	void* default_ctx_;
	std::map<string, void*>* ctx_table_;

	bool eof_;
	bool opened_;

private:
#if defined(_WIN32) || defined(_WIN64)
	static int read_hook(SOCKET fd, void *buf, size_t len,
		int timeout, ACL_VSTREAM* stream, void *ctx);
	static int send_hook(SOCKET fd, const void *buf, size_t len,
		int timeout, ACL_VSTREAM* stream, void *ctx);

	static int fread_hook(HANDLE fd, void *buf, size_t len,
		int timeout, ACL_VSTREAM* stream, void *ctx);
	static int fsend_hook(HANDLE fd, const void *buf, size_t len,
		int timeout, ACL_VSTREAM* stream, void *ctx);
#else
	static int read_hook(int fd, void *buf, size_t len,
		int timeout, ACL_VSTREAM* stream, void *ctx);
	static int send_hook(int fd, const void *buf, size_t len,
		int timeout, ACL_VSTREAM* stream, void *ctx);

	static int fread_hook(int fd, void *buf, size_t len,
		int timeout, ACL_VSTREAM* stream, void *ctx);
	static int fsend_hook(int fd, const void *buf, size_t len,
		int timeout, ACL_VSTREAM* stream, void *ctx);
#endif
};

} // namespace acl

