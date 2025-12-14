#pragma once
#include "../acl_cpp_define.hpp"
#include <time.h>
#include "../connpool/connect_client.hpp"
#include "../stdlib/string.hpp"
#include "../mime/rfc2047.hpp"

#ifndef ACL_CLIENT_ONLY

namespace acl {

class socket_stream;

typedef class memcache mem_cache;

/**
 * memcached client communication protocol library, supports connection pool and
 * automatic reconnection.
 */
class ACL_CPP_API memcache : public connect_client
{
public:
	/**
	* Constructor
	* @param addr {const char*} memcached server listening address, format:
	*  ip:port, e.g.: 127.0.0.1:11211
	* @param conn_timeout {int} Connection server timeout time (seconds)
	* @param rw_timeout {int} Network IO timeout time (seconds)
	*/
	memcache(const char* addr = "127.0.0.1:11211", int conn_timeout = 30,
		int rw_timeout = 10);

	~memcache();

	/**
	 * Set key prefix. The actual key consists of prefix+original key. By default,
	 * there is
	 * no prefix. When multiple applications share the same memcached server, each
	 * application should set a different
	 * key prefix. This can prevent different applications' keys from conflicting.
	 * @param keypre {const char*} When not empty, sets key prefix. When empty,
	 * removes key prefix.
	 * @return {memcache&}
	 */
	memcache& set_prefix(const char* keypre);

	/**
	 * Whether to automatically reconnect when connection is broken. Default is
	 * automatic reconnection.
	 * @param onoff {bool} When true, it means when connection is broken,
	 * automatically reconnect.
	 * @return {memcache&}
	 */
	memcache& auto_retry(bool onoff);

	/**
	 * Set whether to encode KEY value. By default, key is not encoded. When
	 * application's key
	 * may contain special characters, you can use this function to encode the key.
	 * @param onoff {bool} When true, it means internally need to encode key.
	 * @return {memcache&}
	 */
	memcache& encode_key(bool onoff);

	/**
	* Modify or add new data cache in memcached.
	* @param key {const char*} Key value.
	* @param klen {size_t} Key value length.
	* @param dat {const void*} Data.
	* @param dlen {size_t} Data length of data.
	* @param timeout {time_t} Storage timeout time (seconds)
	* @param flags {unsigned short} Storage flag bits.
	* @return {bool} Whether successful.
	*/
	bool set(const char* key, size_t klen,
		const void* dat, size_t dlen,
		time_t timeout = 0, unsigned short flags = 0);

	/**
	* Modify or add new data cache in memcached.
	* @param key {const char*} String key value.
	* @param dat {const void*} Data.
	* @param dlen {size_t} Data length of data.
	* @param timeout {time_t} Storage timeout time (seconds)
	* @param flags {unsigned short} Storage flag bits.
	* @return {bool} Whether successful.
	*/
	bool set(const char* key, const void* dat, size_t dlen,
		time_t timeout = 0, unsigned short flags = 0);

	/**
	* Extend the survival period of an existing key in memcached. Because currently
	* libmemcached does not
	* provide this interface, the implementation of this function first calls get
	* to get the corresponding data value, and then
	 * calls set to set this key value with new timeout.
	* @param key {const char*} Key value.
	* @param klen {size_t} Key value length.
	* @param timeout {time_t} Survival time (seconds)
	* @return {bool} Whether successful.
	*/
	bool set(const char* key, size_t klen, time_t timeout = 0);

	/**
	* Extend the survival period of an existing key in memcached. Because currently
	* libmemcached does not
	* provide this interface, the implementation of this function first calls get
	* to get the corresponding data value, and then
	 * calls set to set this key value with new timeout.
	* @param key {const char*} String key value.
	* @param timeout {time_t} Survival time (seconds)
	* @return {bool} Whether successful.
	*/
	bool set(const char* key, time_t timeout = 0);

	/**
	 * Streaming format upload. When uploading large data, call this function to
	 * send header.
	 * @param key {const char*} Key value string.
	 * @param dlen {size_t} Total length of data to be uploaded.
	 * @param timeout {time_t} Data survival time (seconds)
	 * @param flags {unsigned short} Storage flag bits.
	 * @return {bool} Whether successful.
	 */
	bool set_begin(const char* key, size_t dlen,
		time_t timeout = 0, unsigned short flags = 0);

	/**
	 * Call this function in a loop to upload data value. Internally automatically
	 * checks whether the sum of already uploaded data
	 * reaches the total length set by set_begin function. When reached,
	 * automatically adds a "\r\n" terminator.
	 * Users should not call this function to upload data again, but should start a
	 * new upload process.
	 * @param data {const void*} Data address pointer.
	 * @param len {data} Data length of data.
	 * @return {bool} Whether successful.
	 */
	bool set_data(const void* data, size_t len);

	/**
	* Get corresponding value cache from memcached.
	* @param key {const char*} String key value.
	* @param klen {size_t} Key value length.
	* @param buf {string&} Buffer to store returned cache. Internally first clears
	* this buffer.
	* @param flags {unsigned short*} Store returned flag bits.
	* @return {bool} Returns true to indicate correctly got value. Returns false to
	* indicate key value corresponding
	*  data does not exist or error.
	*/
	bool get(const char* key, size_t klen, string& buf,
		unsigned short* flags = NULL);

	/**
	* Get corresponding value cache from memcached.
	* @param key {const char*} String key value.
	* @param buf {string&} Buffer to store returned cache. Internally first clears
	* this buffer.
	* @param flags {unsigned short*} Store returned flag bits.
	* @return {bool} Returns true to indicate correctly got value. Returns false to
	* indicate key value corresponding
	*  data does not exist or error.
	*/
	bool get(const char* key, string& buf, unsigned short* flags = NULL);

	/**
	 * Streaming format to get data from server, following memcached protocol.
	 * @param key {const void*} Key value.
	 * @param klen {size_t} Key value length.
	 * @param flags {unsigned short*} Store returned flag bits.
	 * @return {int} Length of data to be returned. Return values are as follows:
	 *   0: Indicates not found.
	 *  -1: Indicates error.
	 *  >0: Indicates data length.
	 */
	int get_begin(const void* key, size_t klen, unsigned short* flags = NULL);

	/**
	 * Streaming format to get data from server, following memcached protocol.
	 * @param key {const char*} Key value string.
	 * @param flags {unsigned short*} Store returned flag bits.
	 * @return {int} Length of data to be returned. Return values are as follows:
	 *   0: Indicates not found.
	 *  -1: Indicates error.
	 *  >0: Indicates data length.
	 */
	int get_begin(const char* key, unsigned short* flags = NULL);

	/**
	 * Streaming format to get data from server. Call this function in a loop to
	 * read data.
	 * @param buf {void*} Buffer address.
	 * @param size {size_t} Buffer size.
	 * @return {int} Size of data read this time. Return values are as follows:
	 *  0: Indicates data finished.
	 *  > 0: Indicates length of data read this time.
	 *  -1: Indicates error.
	 */
	int  get_data(void* buf, size_t size);

	/**
	* Delete data from memcached.
	* @param key {const char*} Key value.
	* @param klen {size_t} Key value length.
	* @return {bool} Whether deletion was successful.
	*/
	bool del(const char* key, size_t klen);

	/**
	* Delete data from memcached.
	* @param key {const char*} String key value.
	* @return {bool} Whether deletion was successful.
	*/
	bool del(const char* key);

	/**
	* Get error message string from last memcached operation.
	* @return {const char*} Error message string, may be empty.
	*/
	const char* last_serror() const;

	/**
	* Get error code from last memcached operation.
	* @return {int} Error code.
	*/
	int  last_error() const;

	/**
	* Open memcached connection. Because set/get/del operations will automatically
	* reconnect
	* memcached connection, you generally do not need to explicitly call this
	* function to open memcached
	* connection.
	* @return {bool} Whether opening was successful.
	*/
	virtual bool open();

	/**
	* Close memcached connection. Generally this function does not need to be
	* called, because when the object
	* is destroyed, it will automatically call this function.
	*/
	void close();

	/**
	* List some properties of memcached connection, for debugging.
	*/
	void property_list();

private:
	bool set(const string& key, const void* dat, size_t dlen,
		time_t timeout, unsigned short flags);
	bool get(const string& key, string& buf, unsigned short* flags);
	const string& build_key(const char* key, size_t klen);

	string* keypre_;         // When not empty, string buffer, prepends new KEY value before KEY value
	rfc2047 coder_;          // Encoder when KEY needs to be encoded.
	bool  encode_key_;       // Whether KEY needs to be encoded.

	bool  opened_;           // Whether opened.
	bool  retry_;            // Whether supports automatic reconnection.
	char* addr_;             // Server address (ip:port)
	int   enum_;             // Error code, used for error handling.
	string ebuf_;            // Store error message.
	string kbuf_;            // Store converted KEY value buffer.

	size_t content_length_;  // When streaming format upload/download, this value records total data length.
	size_t length_;          // Sum of already uploaded/downloaded data.

	socket_stream* conn_;    // Server connection object.
	string req_line_;        // Store request line.
	string res_line_;        // Store response line.
	bool error_happen(const char* line);
};

} // namespace acl

#endif // ACL_CLIENT_ONLY

