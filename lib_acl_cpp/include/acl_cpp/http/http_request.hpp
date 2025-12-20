#pragma once
#include "../acl_cpp_define.hpp"
#include <vector>
#include <string>
#include "../connpool/connect_client.hpp"
#include "http_header.hpp"

namespace acl {

class http_client;
class http_pipe;
class socket_stream;
class charset_conv;
class sslbase_conf;
class xml;
class json;

/**
 * HTTP client request class, supports connection reuse, and automatically
 * reconnects when connection times out.
 */
class ACL_CPP_API http_request : public connect_client {
public:
	/**
	 * Constructor. When using this constructor, socket_stream object passed in
	 * will not be closed, and you need to close it yourself.
	 * @param client {socket_stream*} HTTP connection object, can be server-side
	 * connection
	 * or client-side connection. When object is destroyed, whether client object
	 * will be automatically destroyed
	 *  depends on stream_fixed value.
	 * @param conn_timeout {int} When connection is not opened, internally
	 * automatically opens. At this time, this value indicates connection server
	 * timeout time (seconds).
	 *  Network IO read/write timeout time is inherited from base class.
	 * @param unzip {bool} Whether to automatically decompress server response
	 * body.
	 * Note: When actually reusing this object, users should call
	 * request_header::http_header::reset() before each call.
	 * @param stream_fixed {bool} When this value is true, when http_client object
	 * is destroyed, passed client object will not be destroyed, and response
	 * object will be destroyed. When
	 * value is false, when this object is destroyed, client object will also be
	 * destroyed.
	 */
	explicit http_request(socket_stream* client, int conn_timeout = 60,
		bool unzip = true, bool stream_fixed = true);

	/**
	 * Constructor. This constructor internally creates socket_stream object and
	 * closes it.
	 * @param addr {const char*} WEB server address. Address format: domain:port,
	 *  e.g.: www.baidu.com:80, or http://www.baidu.com, or www.baidu.com
	 * @param conn_timeout {int} Remote connection server timeout time (seconds)
	 * @param rw_timeout {int} IO read/write timeout time (seconds)
	 * @param unzip {bool} Whether to automatically decompress server response
	 * body.
	 */
	explicit http_request(const char* addr, int conn_timeout = 60,
		int rw_timeout = 60, bool unzip = true);

	virtual ~http_request();

	/**
	 * Set whether to decompress compressed data when reading server response body.
	 * @param on {bool}
	 * @return {http_request&}
	 */
	http_request& set_unzip(bool on);

	/**
	 * Set client SSL communication mode. Internal default is non-SSL communication
	 * mode.
	 * @param conf {sslbase_conf*} Client SSL configuration object.
	 * @return {http_request&}
	 */
	http_request& set_ssl(sslbase_conf* conf);

	/**
	 * Set SNI field when SSL handshake. After calling this API to set SSL SNI
	 * field, when calling this object's
	 * reset() API, it will automatically restore this API's set value. If you need
	 * to modify it, you need to call
	 * this function again. When this parameter is empty, corresponding field will
	 * be automatically set.
	 * @param sni {const char*} When NULL, automatically uses Host field in HTTP
	 * header.
	 * @return {http_request&}
	 */
	http_request& set_ssl_sni(const char* sni);

	/**
	 * Set SNI field prefix when SSL handshake. After calling this API to set SSL
	 * SNI field prefix,
	 * when calling this object's reset() API, it will automatically restore this
	 * API's set value. If you need to modify it,
	 * you need to call this function again. When this parameter is empty,
	 * corresponding field will be automatically set.
	 * @param prefix {const char*} SNI prefix.
	 * @return {http_request&}
	 */
	http_request& set_ssl_sni_prefix(const char* prefix);

	/**
	 * Set SNI field suffix when SSL handshake. After calling this API to set SSL
	 * SNI field prefix,
	 * when calling this object's reset() API, it will automatically restore this
	 * API's set value. If you need to modify it,
	 * you need to call this function again. When this parameter is empty,
	 * corresponding field will be automatically set.
	 * @param suffix {const char*} SNI suffix.
	 * @return {http_request&}
	 */
	http_request& set_ssl_sni_suffix(const char* prefix);

	/**
	 * Get HTTP request header, and then you can add fields to returned HTTP
	 * request header object
	 * or call http_header::reset() to reset header state.
	 * See http_header class.
	 * @return {http_header&}
	 */
	http_header& request_header();

	/**
	 * Set local character set. When local character set is not empty, boundary
	 * data boundary string conversion.
	 * @param local_charset {const char*} Local character set.
	 * @return {http_header&}
	 */
	http_request& set_local_charset(const char* local_charset);

	/**
	 * Send HTTP GET request and wait for response.
	 * @return {bool}
	 */
	bool get();

	/**
	 * Send HTTP POST request and wait for response.
	 * @param data {const char*} Request body data.
	 * @param len {size_t} Request body buffer length.
	 * @return {bool}
	 */
	bool post(const char* data, size_t len);

	/**
	 * Send HTTP request including HTTP request header and HTTP request body, and
	 * simultaneously
	 * read HTTP response header from HTTP server. For long connections, when
	 * timeout occurs
	 * you can call this function again. Before calling get_body function, you must
	 * call this function (or write_head/write_body).
	 * Generally, after this function sends request data and reads HTTP response
	 * header,
	 * if user needs to continue, you can call get_body() or
	 * http_request::get_clinet()->read_body(char*, size_t)
	 * to read HTTP response body data.
	 * @param data {const void*} Data to be sent. When not empty, automatically
	 * determines
	 *  POST request type, otherwise GET request type.
	 * @param len {size_} When data is not empty, indicates data length.
	 * @return {bool} Whether sending request data and reading HTTP response header
	 * was successful.
	 */
	bool request(const void* data, size_t len);

	/////////////////////////////////////////////////////////////////////

	/**
	 * When using streaming write mode, you need to call this function first to
	 * send HTTP request header.
	 * @return {bool} Whether successful. Only after success can you continue
	 * calling write_body.
	 */
	bool write_head();

	/**
	 * When using streaming write mode, after calling write_head, you can call this
	 * function in a loop
	 * to send HTTP request body data. When data parameter is empty pointer, it
	 * indicates data sending is complete;
	 * After sending request data, this function internally automatically reads
	 * HTTP response header data. Users can
	 * then call get_body/read_body to read HTTP response body.
	 * @param data {const void*} Data address pointer. When parameter value is
	 * empty pointer, it indicates
	 *  data sending is complete.
	 * @param len {size_t} When data is not empty pointer, indicates data length.
	 * @return {bool} Whether sending request was successful.
	 * Note: After application sends data, you must call this function once more,
	 * and data parameter must be empty.
	 */
	bool write_body(const void* data, size_t len);

	/////////////////////////////////////////////////////////////////////

	/**
	 * After sending request data, internally automatically calls HTTP response
	 * header reading process. You can get server response HTTP status code (2xx,
	 * 3xx, 4xx, 5xx) through this function.
	 * Actually, this function internally only calls http_client::response_status
	 * function.
	 * @return {int}
	 */
	int http_status() const;

	/**
	 * Get HTTP response body length.
	 * @return {int64) Return value is -1 when HTTP header does not exist or has no
	 * Content-Length field.
	 */
#if defined(_WIN32) || defined(_WIN64)
	__int64 body_length(void) const;
#else
	long long int body_length() const;
#endif
	/**
	 * HTTP keep-alive (whether response supports long connection).
	 * @return {bool}
	 */
	bool keep_alive() const;

	/**
	 * Get a certain field value in HTTP response header.
	 * @param name {const char*} Field name.
	 * @return {const char*} Field value. Returns empty when not found.
	 */
	const char* header_value(const char* name) const;

	/**
	 * Whether response body is finished.
	 * @return {bool}
	 */
	bool body_finish() const;

	/**
	 * After request succeeds, call this function to get complete server response
	 * body
	 * and store it in specified xml object.
	 * @param out {xml&} HTTP response body data stored in this xml object.
	 * @param to_charset {const char*} When not empty, internally automatically
	 *  converts response data to this character set and stores it in xml object.
	 * @return {bool} Whether reading was successful.
	 * Note: When response body is very large, you should use this function to
	 * avoid memory overflow.
	 */
	bool get_body(xml& out, const char* to_charset = NULL);

	/**
	 * After request succeeds, call this function to get complete server response
	 * body
	 * and store it in specified json object.
	 * @param out {json&} HTTP response body data stored in this json object.
	 * @param to_charset {const char*} When not empty, internally automatically
	 *  converts response data to this character set and stores it in json object.
	 * @return {bool} Whether reading was successful.
	 * Note: When response body is very large, you should use this function to
	 * avoid memory overflow.
	 */
	bool get_body(json& out, const char* to_charset = NULL);

	/**
	 * After request succeeds, call this function to get complete server response
	 * body
	 * and store it in user buffer.
	 * @param out {string&} Store response body.
	 * @param to_charset {const char*} When not empty, internally automatically
	 *  converts response data to this character set and stores it in out buffer.
	 * Note: When response body is very large, you should use this function to
	 * avoid memory overflow.
	 */
	bool get_body(string& out, const char* to_charset = NULL);

	/**
	 * After request succeeds, call this function to read server response body data
	 * and store it in user buffer. Call this function in a loop until data is
	 * finished.
	 * @param buf {char*} Buffer to store server response body.
	 * @param size {size_t} Buffer size.
	 * @return {int} Return value == 0 indicates data finished, < 0 indicates error
	 * or connection closed, > 0 indicates number of bytes read. Users should
	 * continue calling until
	 *  return value <= 0.
	 * Note: This function returns original HTTP response body data, without
	 * decompression or character
	 *  encoding conversion. Users need to handle it themselves.
	 */
	int read_body(char* buf, size_t size);

	/**
	 * After request succeeds, call this function to read HTTP response body. Call
	 * in a loop
	 * to read data. Internally automatically decompresses compressed data. If
	 * set_charset function was called before calling this function
	 * to set local character set, data will be converted to local character set.
	 * @param out {string&} Store result.
	 * @param clean {bool} Whether to internally automatically clear out buffer
	 * each time this function is called.
	 *  buffer.
	 * @param real_size {int*} When pointer is not empty, stores uncompressed
	 * response body
	 * length. When constructor specified automatic decompression mode and response
	 * body length > 0, this
	 * value stores length value should be same as this function's return value.
	 * When no data was read, this
	 *  value stores length value is 0.
	 * @return {int} == 0 indicates finished, but connection not closed. >0
	 * indicates this time read
	 * data length (when decompression is enabled, it indicates decompressed data
	 * length,
	 * which may differ from actual response data. Actual response data length
	 * should be obtained through real_size
	 * parameter); < 0 indicates error or connection closed. At this time, if
	 * real_size is not empty, real_size
	 *  stored value should be 0.
	 * When return value is 0, you can call body_finish function to determine
	 * whether response body is finished.
	 */
	int read_body(string& out, bool clean = false, int* real_size = NULL);

	/**
	 * After request succeeds, call this function to read one line of data from
	 * HTTP server. Call in a loop
	 * to read data until it returns false or body_finish() returns true.
	 * Internally automatically decompresses compressed data. If set_charset
	 * function was called before calling this function
	 * to set local character set, data will be converted to local character set.
	 * @param out {string&} Store result.
	 * @param nonl {bool} Whether to automatically remove "\r\n" or "\n" at the end
	 * of read line.
	 * @param size {size_t*} When pointer is not empty, stores read data length.
	 * @return {bool} Whether one line was read. When returns true, it means one
	 * line was read.
	 * You can determine whether response body is completely read by whether
	 * body_finish() is true.
	 *  When reading one line and nonl = true, if *size = 0, it means false when
	 *  response body is not finished and reading is finished.
	 *  *size stores read data length.
	 */
	bool body_gets(string& out, bool nonl = true, size_t* size = NULL);

	/**
	 * When http_request::request_header().set_range() was called to set
	 * range request, this function can determine whether server supports range.
	 * @return {bool}
	 */
	bool support_range() const;

#if defined(_WIN32) || defined(_WIN64)
	/**
	 * When http_request::request_header().set_range() was called to read data,
	 * after reading response header, this function can get start offset position
	 * of range support function.
	 * @return {acl_int64} When server does not support range format, returns value
	 * < 0.
	 */
	__int64 get_range_from(void) const;

	/**
	 * When http_request::request_header().set_range() was called to read data,
	 * after reading response header, this function can get end offset position of
	 * range support function.
	 * @return {acl_int64} When server does not support range format, returns value
	 * < 0.
	 */
	__int64 get_range_to(void) const;

	/**
	 * When http_request::request_header().set_range() was called to read data,
	 * after reading response header, this function can get total resource size of
	 * range support function (this value
	 * is total HTTP response body size).
	 * @return {acl_int64} When server does not support range format, returns value
	 * < 0.
	 */
	__int64 get_range_max(void) const;
#else
	long long int get_range_from() const;
	long long int get_range_to() const;
	long long int get_range_max() const;
#endif

	/**
	 * Get Set-Cookie list returned by server.
	 * @return {const std::vector<HttpCookie*>*} Returns empty to indicate
	 *  no cookies or error occurred.
	 */
	const std::vector<HttpCookie*>* get_cookies() const;

	/**
	 * Get a certain cookie name from Set-Cookie returned by server.
	 * @param name {const char*} Cookie name.
	 * @param case_insensitive {bool} Whether case-insensitive. true means
	 *  case-insensitive.
	 * @return {const HttpCookie*} Returns NULL to indicate not found.
	 */
	const HttpCookie* get_cookie(const char* name,
		bool case_insensitive = true) const;

	/////////////////////////////////////////////////////////////////////

	/**
	 * Get http_client HTTP connection object used internally. You can use this
	 * object to read
	 * response header and data. See http_client class.
	 * @return {http_client*} Returns empty to indicate not initialized.
	 */
	http_client* get_client() const;

	/**
	 * Reset object state. When reusing the same connection object, call this
	 * function.
	 */
	void reset();

protected:
	/**
	 * Override connect_client virtual function. Explicitly call this function to
	 * open connection to server.
	 * @return {bool} Whether opening was successful.
	 */
	virtual bool open();

private:
	char addr_[128];
	bool unzip_;
	sslbase_conf* ssl_conf_;
	std::string sni_host_;
	std::string sni_prefix_;
	std::string sni_suffix_;
	char local_charset_[64];
	charset_conv* conv_;
        http_client* client_;
	http_header  header_;
	bool cookie_inited_;
	std::vector<HttpCookie*>* cookies_;
#if defined(_WIN32) || defined(_WIN64)
	__int64 range_from_;
	__int64 range_to_;
	__int64 range_max_;
#else
	long long int range_from_;
	long long int range_to_;
	long long int range_max_;
#endif
	// Flag bit when writing HTTP request body to identify whether retry is needed.
	bool need_retry_;

	bool send_request(const void* data, size_t len);
	bool try_open(bool* reuse_conn);
	void close();
	void create_cookies();
	http_pipe* get_pipe(const char* to_charset);
	void set_charset_conv();
	void check_range();
};

} // namespace acl

