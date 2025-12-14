#pragma once
#include "../acl_cpp_define.hpp"
#include <list>
#include "../stdlib/dbuf_pool.hpp"
#include "../http/http_type.hpp"

struct HTTP_HDR_RES;
struct HTTP_HDR_REQ;
struct HTTP_HDR_ENTRY;

namespace acl {

class string;
class HttpCookie;

/**
 * HTTP header class, can construct request header and response header.
*/
class ACL_CPP_API http_header : public dbuf_obj {
public:
	/**
	 * Constructor
	 * @param dbuf {dbuf_guard*} When not empty, used as memory pool.
	 */
	http_header(dbuf_guard* dbuf = NULL);

	/**
	 * HTTP request header constructor.
	 * @param url {const char*} Request URL. URL format examples:
	 *   http://www.test.com/
	 *   /cgi-bin/test.cgi
	 *   http://www.test.com/cgi-bin/test.cgi
	 *   http://www.test.com/cgi-bin/test.cgi?name=value
	 *   /cgi-bin/test.cgi?name=value
	 * When url has no parameter part, internally automatically parses URL.
	 * When url has parameter part, internally automatically parses parameters and calls add_param function.
	 * After calling this function, users can call add_param and other functions to add more parameters.
	 * When parameter part only has parameter name but no parameter value, this parameter will be ignored. If you need to add
	 * parameters without values, you should call add_param function to add them.
	 * @param dbuf {dbuf_guard*} When not empty, used as memory pool.
	 * @param encoding {bool} Whether to URL encode parameters in url. When
	 *  true, internally automatically URL encodes parameters in url. Otherwise, url remains original.
	 */
	http_header(const char* url, dbuf_guard* dbuf = NULL,
		bool encoding = true);

	/**
	 * HTTP response header constructor.
	 * @param status {int} Status code, e.g.: 1xx, 2xx, 3xx, 4xx, 5xx
	 * @param dbuf {dbuf_guard*} When not empty, used as memory pool.
	 */
	http_header(int status, dbuf_guard* dbuf = NULL);

	/**
	 * Construct from C structure HTTP response header.
	 * @param hdr_res {const HTTP_HDR_RES&}
	 * @param dbuf {dbuf_guard*} When not empty, used as memory pool.
	 */
	http_header(const HTTP_HDR_RES& hdr_res, dbuf_guard* dbuf = NULL);

	/**
	 * Construct from C structure HTTP request header.
	 * @param hdr_req {const HTTP_HDR_REQ&}
	 * @param dbuf {dbuf_guard*} When not empty, used as memory pool.
	 */
	http_header(const HTTP_HDR_REQ& hdr_req, dbuf_guard* dbuf = NULL);

	virtual ~http_header();

	/**
	 * Reset HTTP header information and clear resources from last call.
	 */
	void reset();

	//////////////////////////////////////////////////////////////////////
	//            Common methods for HTTP request and HTTP response
	//////////////////////////////////////////////////////////////////////

	/**
	 * Set HTTP protocol version.
	 * @param version {const char*} HTTP protocol version number, format: 1.0, 1.1
	 * @return {http_header&}
	 */
	http_header& set_proto_version(const char* version);

	/**
	 * Set whether HTTP header is client request header or server response header.
	 * @param onoff {bool} true means request header, false means response header.
	 * @return {http_header&} Returns reference to this object, convenient for chaining calls.
	 */
	http_header& set_request_mode(bool onoff);

	/**
	 * Add field to HTTP header.
	 * @param name {const char*} Field name, cannot be empty pointer.
	 * @param value {const char*} Field value, cannot be empty pointer.
	 * @param replace {bool} When field name repeats, whether to automatically replace old value.
	 * @return {http_header&} Returns reference to this object, convenient for chaining calls.
	 */
	http_header& add_entry(const char* name, const char* value,
			bool replace = true);

	/**
	 * In already added HTTP header fields, disable a certain field (internal default is enabled state).
	 * @param name {const char*}
	 * @param yes {bool} When true, means disable. false means enable.
	 */
	void disable_header(const char* name, bool yes);
	
	/**
	 * Get specified header field from HTTP header.
	 * @param name {const char*} Field name, cannot be empty pointer.
	 * @return {const char*} Returns NULL to indicate not found.
	 */
	const char* get_entry(const char* name) const;

	/**
	 * Set Content-Length field in HTTP header.
	 * @param n {int64} Length value.
	 * @return {http_header&} Returns reference to this object, convenient for chaining calls.
	 */
#if defined(_WIN32) || defined(_WIN64)
	http_header& set_content_length(__int64 n);

	/**
	 * Get Content-Length value in HTTP header set through set_content_length.
	 * @return {int64}
	 */
	__int64 get_content_length() const
	{
		return content_length_;
	}
#else
	http_header& set_content_length(long long int n);
	long long int get_content_length() const {
		return content_length_;
	}
#endif

	/**
	 * Set Range field in HTTP request header or response header, used for range request response data.
	 * Only WEB servers that support range requests support this.
	 * @param from {http_off_t} Start offset position, subscript starts from 0. This
	 *  value is effective when >= 0.
	 * @param to {http_off_t} End offset position, subscript starts from 0.
	 *  When value in request header is < 0, it means from start position to end position.
	 * @return {http_header&} Returns reference to this object, convenient for chaining calls.
	 */
#if defined(_WIN32) || defined(_WIN64)
	http_header& set_range(__int64 from, __int64 to);
#else
	http_header& set_range(long long from, long long to);
#endif

	/**
	 * Before range response, you need to call this function to set total resource length.
	 * @param total {http_off_t} Total resource length. This parameter must be set to total resource length.
	 * @return {http_header&}
	 */
#if defined(_WIN32) || defined(_WIN64)
	http_header& set_range_total(__int64 total);
#else
	http_header& set_range_total(long long total);
#endif

	/**
	 * Get range offset position value set through set_range.
	 * @param from {http_off_t*} When not empty, stores start position offset.
	 * @param to {http_off_t*} When not empty, stores end position offset.
	 */
#if defined(_WIN32) || defined(_WIN64)
	void get_range(__int64* from, __int64* to);
#else
	void get_range(long long int* from, long long int* to);
#endif

	/**
	 * Set Content-Type field in HTTP header.
	 * @param value {const char*} Content type value.
	 * @return {http_header&} Returns reference to this object, convenient for chaining calls.
	 */
	http_header& set_content_type(const char* value);

	/**
	 * Set Connection field in HTTP header, whether to keep connection alive.
	 * Note: Currently, long connection is not yet supported. Even if you set this flag bit,
	 * after getting response data, connection will still be closed.
	 * @param on {bool} Whether to keep connection alive.
	 * @return {http_header&} Returns reference to this object, convenient for chaining calls.
	 */
	http_header& set_keep_alive(bool on);

	/**
	 * Check whether current header has set keep-alive option.
	 */
	bool get_keep_alive() const {
		return keep_alive_;
	}

	http_header& set_upgrade(const char* value = "websocket");

	const char* get_upgrade() const {
		return upgrade_;
	}

	/**
	 * Add cookie to HTTP header.
	 * @param name {const char*} Cookie name.
	 * @param value {const char*} Cookie value.
	 * @param domain {const char*} Domain name.
	 * @param path {const char*} Storage path.
	 * @param expires {time_t} Expiration time. When parameter value is 0, it means no expiration.
	 *  When > 0, it means expiration time. expires is absolute time, unit is seconds.
	 * @return {http_header&} Returns reference to this object, convenient for chaining calls.
	 */
	http_header& add_cookie(const char* name, const char* value,
		const char* domain = NULL, const char* path = NULL,
		time_t expires = 0);

	/**
	 * Add cookie to HTTP header.
	 * @param cookie {const http_cookie*} Cookie object.
	 * @return {http_header&} Returns reference to this object, convenient for chaining calls.
	 */
	http_header& add_cookie(const HttpCookie* cookie);

	/**
	 * Get cookie object with corresponding name from HTTP header.
	 * @param name {const char*} Cookie name.
	 * @return {const HttpCookie*}
	 */
	const HttpCookie* get_cookie(const char* name) const;

	/**
	 * Convert time to rfc1123 string format and output.
	 */
	static void date_format(char* out, size_t size, time_t t);

	/**
	 * Determine whether it is HTTP request header.
	 * @return {bool} Returns false to indicate it is HTTP response header.
	 */
	bool is_request() const;

	/**
	 * Set flag bit to control whether ? question mark in HTTP request URI is URL encoded (encoded as %3F). This flag bit
	 * affects URL encoding. Internal default is not to URL encode.
	 * @param on {bool} When true, means URL encode.
	 */
	static void uri_unsafe_correct(bool on);

	//////////////////////////////////////////////////////////////////////
	//                        HTTP request methods
	//////////////////////////////////////////////////////////////////////
	
	/**
	 * Build HTTP request header string.
	 * @param buf {string&} Store result string.
	 * @return {bool} Whether request header string was successfully built.
	 */
	bool build_request(string& buf) const;

	/**
	 * Set request URL. URL format examples:
	 * 1. http://www.test.com/
	 * 2. /cgi-bin/test.cgi
	 * 3. http://www.test.com/cgi-bin/test.cgi
	 * 3. http://www.test.com/cgi-bin/test.cgi?name=value
	 * 4. /cgi-bin/test.cgi?name=value
	 * 5. http://www.test.com
	 * When url has no parameter part, internally automatically parses URL.
	 * When url has parameter part, internally automatically parses parameters and calls add_param function.
	 * After calling this function, users can call add_param and other functions to add more parameters.
	 * When parameter part only has parameter name but no parameter value, this parameter will be ignored. If you need to add
	 * parameters without values, you should call add_param function to add them.
	 * @param url {const char*} Request url, cannot be empty pointer.
	 * @param encoding {bool} Whether to URL encode parameters in url. When
	 *  true, internally automatically URL encodes parameters in url. Otherwise, url remains original.
	 * @return {http_header&} Returns reference to this object, convenient for chaining calls.
	 */
	http_header& set_url(const char* url, bool encoding = true);

	/**
	 * Set HOST field in HTTP request header.
	 * @param value {const char*} HOST field value in request header.
	 * @return {http_header&} Returns reference to this object, convenient for chaining calls.
	 */
	http_header& set_host(const char* value);

	/**
	 * Get HOST field in HTTP request header that was set.
	 * @return {const char*} Returns empty pointer to indicate HOST field was not set.
	 */
	const char* get_host() const {
		return host_[0] == 0 ? NULL : host_;
	}

	/**
	 * Set HTTP protocol request method. If you don't call this function, default is GET request.
	 * @param method {http_method_t} HTTP request method.
	 * @return {http_header&} Returns reference to this object, convenient for chaining calls.
	 */
	http_header& set_method(http_method_t method);

	/**
	 * Set HTTP protocol request method. This function allows users to extend HTTP request methods.
	 * Request method set through this function does not affect HTTP request building.
	 * @param method {const char*} Request method.
	 * @return {http_header&} Returns reference to this object, convenient for chaining calls.
	 */
	http_header& set_method(const char* method);

	/**
	 * When it is request header, you can get current request header's request method.
	 * @param buf {string*} Store string representation of request method.
	 * @return {http_method_t}
	 */
	http_method_t get_method(string* buf = NULL) const;

	/**
	 * Set whether HTTP request header accepts compressed data. Response HTTP header field is:
	 * Accept-Encoding: gzip, deflate. Currently only gzip format is supported.
	 * @param on {bool} When true, automatically adds HTTP compression header.
	 * @return {http_header&} Returns reference to this object, convenient for chaining calls.
	 */
	http_header& accept_gzip(bool on);

	/**
	 * When calling add_param/add_int/add_format, whether to automatically override same parameters.
	 * Internal default value is false, i.e., do not override same parameters.
	 * @param yes {bool}
	 * @return {http_header&}
	 */
	http_header& set_param_override(bool yes);

	/**
	 * Add parameter to URL parameter part. When parameter only has parameter name but no parameter value:
	 * 1. When parameter value is empty string, parameter value is empty pointer. URL parameter part only has: {name}
	 * 2. When parameter value is not empty string, parameter value is empty string. URL parameter part is: {name}=
	 * @param name {const char*} Parameter name, cannot be empty pointer.
	 * @param value {const char*} Parameter value. When empty pointer, parameter is not added.
	 * @return {http_header&} Returns reference to this object, convenient for chaining calls.
	 */
	http_header& add_param(const char* name, const char* value);
	http_header& add_int(const char* name, short value);
	http_header& add_int(const char* name, int value);
	http_header& add_int(const char* name, long value);
	http_header& add_int(const char* name, unsigned short value);
	http_header& add_int(const char* name, unsigned int value);
	http_header& add_int(const char* name, unsigned long value);
	http_header& add_format(const char* name, const char* fmt, ...)
		ACL_CPP_PRINTF(3, 4);
#if defined(_WIN32) || defined(_WIN64)
	http_header& add_int(const char* name, __int64 vlaue);
	http_header& add_int(const char* name, unsigned __int64 vlaue);
#else
	http_header& add_int(const char* name, long long int value);
	http_header& add_int(const char* name, unsigned long long int value);
#endif

	http_header& set_ws_origin(const char* url);
	http_header& set_ws_key(const void* key, size_t len);
	http_header& set_ws_key(const char* key);
	http_header& set_ws_protocol(const char* proto);
	http_header& set_ws_version(int ver);

	const char* get_ws_origin() const {
		return ws_origin_;
	}

	const char* get_ws_key() const {
		return ws_sec_key_;
	}

	const char* get_ws_protocol() const {
		return ws_sec_proto_;
	}

	int get_ws_version() const {
		return ws_sec_ver_;
	}

	http_header& set_ws_accept(const char* key);
	const char* get_ws_accept() const {
		return ws_sec_accept_;
	}

	/**
	 * URL redirect.
	 * @param url {const char*} Redirect URL, format:
	 *  http://xxx.xxx.xxx/xxx or /xxx
	 *  When it is the former, automatically extracts HOST field. When it is the latter,
	 *  uses previously set HOST.
	 */
	bool redirect(const char* url);

	/**
	 * Set redirect count. When value == 0, it means no redirect. Otherwise,
	 * redirect count can be set through this value.
	 * @param n {int} Maximum redirect count.
	 * @return {http_header&} Returns reference to this object, convenient for chaining calls.
	 */
	http_header& set_redirect(unsigned int n = 5);

	/**
	 * Get maximum redirect count set through set_redirect.
	 * @return {unsigned int}
	 */
	unsigned int get_redirect() const;

	/**
	 * When redirect is needed, subclasses can override this function to reset some common fields.
	 */
	virtual void redicrect_reset() {}

	//////////////////////////////////////////////////////////////////////
	//                       HTTP response methods
	//////////////////////////////////////////////////////////////////////

	/**
	 * Build HTTP response header string.
	 * @param buf {string&} Store result string.
	 * @return {bool} Whether response header string was successfully built.
	 */
	bool build_response(string& buf) const;

	/**
	 * Set response status code in HTTP response header.
	 * @param status {int} Status code, e.g.: 1xx, 2xx, 3xx, 4xx, 5xx
	 * @return {http_header&} Returns reference to this object, convenient for chaining calls.
	 */
	http_header& set_status(int status);

	/**
	 * Get HTTP status code in response header.
	 * @return {int} HTTP response status code: 1xx, 2xx, 3xx, 4xx, 5xx
	 */
	int get_status() const {
		return status_;
	}

	/**
	 * Set chunked transfer flag in HTTP response header.
	 * @param on {bool}
	 * @return {http_header&}
	 */
	http_header& set_chunked(bool on);

	/**
	 * Determine whether current HTTP response uses chunked transfer mode.
	 * @return {bool}
	 */
	bool chunked_transfer() const {
		return chunked_transfer_;
	}

	/**
	 * Set whether to build CGI format response header.
	 * @param on {bool} Whether CGI format response header.
	 * @return {http_header&} Returns reference to this object, convenient for chaining calls.
	 */
	http_header& set_cgi_mode(bool on);

	/**
	 * Whether CGI mode is enabled.
	 * @return {bool}
	 */
	bool is_cgi_mode() const {
		return cgi_mode_;
	}

	/**
	 * Set whether server response uses gzip format compression.
	 * @param on {bool}
	 * @return {http_header&}
	 */
	http_header& set_transfer_gzip(bool on);

	/**
	 * Get whether current data transfer has set gzip compression format.
	 * @return {bool}
	 */
	bool is_transfer_gzip() const {
		return transfer_gzip_;
	}

private:
	dbuf_guard* dbuf_internal_;
	dbuf_guard* dbuf_;
	bool fixed_;                          // Whether HTTP has been built.
	//char* domain_;  // HTTP server domain name.
	//unsigned short port_;               // HTTP server port.
	char* url_;                           // HTTP request URL
	std::list<HTTP_PARAM*> params_;       // URL parameter list.
	bool param_override_;                 // Whether to override same parameters when adding parameters.
	std::list<HttpCookie*> cookies_;      // cookies list.
	std::list<HTTP_HDR_ENTRY*> entries_;  // HTTP request header field list.
	http_method_t method_;                // HTTP request method.
	char  version_[8];                    // HTTP protocol version number.
	char  method_s_[64];                  // HTTP request method string representation.
	char  host_[256];                     // HOST field in HTTP request header.
	bool keep_alive_;                     // Whether to keep connection alive.
	unsigned int nredirect_;              // Maximum redirect count.
	bool accept_compress_;                // Whether to accept compressed data.
	int  status_;                         // Status code in response header.
	bool is_request_;                     // Request header or response header.
	bool cgi_mode_;                       // Whether CGI response header.
#if defined(_WIN32) || defined(_WIN64)
	__int64 range_from_;                  // In request header, range start position.
	__int64 range_to_;                    // In request header, range end position.
	__int64 range_total_;                 // Total length recorded in range response mode.
	__int64 content_length_;              // HTTP request body length.
#else
	long long int range_from_;            // In request header, range start position.
	long long int range_to_;              // In request header, range end position.
	long long int range_total_;           // Total length recorded in range response mode.
	long long int content_length_;        // HTTP request body length.
#endif
	bool chunked_transfer_;               // Whether chunked transfer mode.
	bool transfer_gzip_;                  // Whether server response uses gzip compression.

	char* upgrade_;
	// just for websocket
	char* ws_origin_;
	char* ws_sec_key_;
	char* ws_sec_proto_;
	int   ws_sec_ver_;
	char* ws_sec_accept_;

	void init();                          // Initialize.
	void clear();
	void build_common(string& buf) const; // Build common header.

	void add_res_cookie(const HTTP_HDR_ENTRY& entry);
	void append_accept_key(const char* sec_key, string& out) const;
	unsigned char* create_ws_key(const void* key, size_t len) const;
};

}  // namespace acl end

