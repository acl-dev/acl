#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/noncopyable.hpp"
#include "http_header.hpp"

namespace acl {

class http_client;
class http_pipe;
class socket_stream;
class xml;
class json;

class ACL_CPP_API http_response : public noncopyable {
public:
	/**
	 * Constructor: socket_stream stream object passed through this constructor
	 * needs to be deleted by user itself.
	 * Internally will not delete it during destruction.
	 * @param client {socket_stream*} Data connection stream, non-empty
	 * Note: This class instance can be used multiple times in long connection, but
	 * must pay attention to
	 * usage order: get_body->response
	 */
	explicit http_response(socket_stream* client);
	virtual ~http_response();

	//////////////////////////////////////////////////////////////////////
	// Methods related to reading request data

	/**
	 * Read HTTP request header from HTTP request client. After calling this
	 * method, can call
	 * get_body/read_body to read HTTP request body data
	 * @return {bool} Whether successful
	 */
	bool read_header();

	/**
	 * Read HTTP request body data in xml format. After calling this function,
	 * should call
	 * response to return response data to client. When this function is called
	 * each time, internal
	 * object will be initialized, so this function can be called multiple times in
	 * multiple sessions
	 * @param out {xml&} Store HTTP request body data in this xml object
	 * @param to_charset {const char*} When this item is not empty, internally
	 * automatically
	 *  converts data to this character set and stores in xml object
	 * @return {bool} Whether normal
	 *  Note: Must first call read_header before calling this function
	 * When request body data is particularly large, should not use this function
	 * to avoid running out of memory
	 */
	bool get_body(xml& out, const char* to_charset = NULL);

	/**
	 * Read HTTP request body data in json format. After calling this function,
	 * should call
	 * response to return response data to client. When this function is called
	 * each time, internal
	 * object will be initialized, so this function can be called multiple times in
	 * multiple sessions
	 * @param out {json&} Store HTTP request body data in this json object
	 * @param to_charset {const char*} When this item is not empty, internally
	 * automatically
	 *  converts data to this character set and stores in json object
	 * @return {bool} Whether normal
	 *  Note: Must first call read_header before calling this function
	 * When request body data is particularly large, should not use this function
	 * to avoid running out of memory
	 */
	bool get_body(json& out, const char* to_charset = NULL);

	/**
	 * Read all HTTP request body data and store in input buffer
	 * @param out {string&} Store request body data
	 * @param to_charset {const char*} When this item is not empty, internally
	 * automatically
	 *  converts data to this character set and stores in out object
	 * Note: When request body data is particularly large, should not use this
	 * function to avoid running out of memory
	 *     Must first call read_header before calling this function
	 */
	bool get_body(string& out, const char* to_charset = NULL);

	/**
	 * Read HTTP request body data and store in input buffer. Can call
	 * this function in a loop until data is read completely.
	 * @param buf {char*} Store partial request body data
	 * @param size {size_t} buf buffer size
	 * @return {int} Return value == 0 indicates normally read complete, < 0
	 * indicates client
	 * closed connection, > 0 indicates data already read. User should keep reading
	 * data until
	 *  return value <= 0
	 * Note: This function reads raw HTTP body data, does not decompress or
	 * character set
	 *     decode. User handles as needed. Must first call read_header
	 *     before calling this function
	 */
	int read_body(char* buf, size_t size);

	//////////////////////////////////////////////////////////////////////
	// Method functions related to data response

	/**
	 * Get HTTP response header object, then add
	 * your own response header fields or http_header::reset() to reset response
	 * header state in returned HTTP response header object.
	 * Reference: http_header class
	 * @return {http_header&}
	 */
	http_header& response_header();

	/**
	 * Send HTTP response data to client. Can call this function in a loop.
	 * <b>Before calling this function, must ensure the following operations:</b>
	 * 1) Must first call read_header && get_body to get HTTP client's request
	 * data;
	 * 2) Must get http_header object through response_header, and set response
	 * header
	 *    fields (e.g.: set_status, set_keep_alive, etc.)
	 * <b>When calling this function, the following operations will occur:</b>
	 * 1) Internally will automatically send HTTP response header on first write;
	 * 2) After setting chunked transfer mode through http_header::set_chunked,
	 * internally automatically uses chunked transfer mode;
	 * 3) When using chunked mode to transfer data, should call this function once
	 * more at the end,
	 * with all parameters set to 0 to indicate data end
	 * @param data {const void*} Data address
	 * @param len {size_t} data data length
	 * @return {bool} Whether sending was successful. If returns false, it
	 * indicates connection interrupted
	 */
	bool response(const void* data, size_t len);

	//////////////////////////////////////////////////////////////////////

	/**
	 * Get http_client HTTP connection stream. Can get
	 * partial data of client request header through returned object. Reference:
	 * http_client class
	 * @return {http_client*} Returns NULL indicates stream error occurred
	 */
	http_client* get_client() const;

	/**
	 * Close HTTP connection stream
	 */
	void close();

private:
	bool debug_;
	bool header_ok_;
	http_client* client_;
	http_header  header_;
	bool head_sent_;
	http_pipe* get_pipe(const char* to_charset);
};

} // namespace acl

