#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/noncopyable.hpp"

#ifndef ACL_CLIENT_ONLY

namespace acl {

class dbuf_guard;
class string;
class xml;
class json;
class ostream;
class socket_stream;
class http_header;
class http_client;
class HttpCookie;
class HttpServletRequest;

/**
 * HTTP client response callback class. This class should not be inherited.
 * Users also do not need to
 * dynamically create objects of this class.
 */
class ACL_CPP_API HttpServletResponse : public noncopyable {
public:
	/**
	 * Constructor
	 * @param stream {socket_stream&} Connection stream. Internally will not
	 * automatically close it.
	 */
	explicit HttpServletResponse(socket_stream& stream);
	~HttpServletResponse();

	/**
	 * Set HTTP response body length.
	 * @param n {acl_int64} Body length.
	 */
#if defined(_WIN32) || defined(_WIN64)
	HttpServletResponse& setContentLength(__int64 n);
#else
	HttpServletResponse& setContentLength(long long int n);
#endif

	/**
	 * Set HTTP chunked transfer mode.
	 * @param on {bool} When set to true, even if setContentLength is called,
	 * internally will also use chunked transfer mode. According to HTTP RFC
	 * specification requirements,
	 *  chunked transfer has higher priority than content-length format.
	 * @return {HttpServletResponse&}
	 */
	HttpServletResponse& setChunkedTransferEncoding(bool on);

	/**
	 * Set whether HTTP client maintains connection.
	 * @param on {bool}
	 * @return {HttpServletResponse&}
	 */
	HttpServletResponse& setKeepAlive(bool on);

	/**
	 * Set HTTP response header Content-Type field value. This field value can be
	 * in
	 * text/html or text/html; charset=utf8 format.
	 * @param value {const char*} Field value.
	 * @return {HttpServletResponse&}
	 */
	HttpServletResponse& setContentType(const char* value);

	/**
	 * Set HTTP response body gzip compression format.
	 * @param gzip {bool} Whether to use gzip compression format.
	 * @return {HttpServletResponse&}
	 */
	HttpServletResponse& setContentEncoding(bool gzip);

	/**
	 * Set HTTP response body character set. If setContentType has already set
	 * the character set, this function should not be called again to set the
	 * character set.
	 * @param charset {const char*} Character set of response data.
	 * @return {HttpServletResponse&}
	 */
	HttpServletResponse& setCharacterEncoding(const char* charset);

	/**
	 * Set date format field in HTTP response header.
	 * @param name {const char*} Field name in HTTP response header.
	 * @param value {time_t} Time value.
	 */
	HttpServletResponse& setDateHeader(const char* name, time_t value);

	/**
	 * Set string format field in HTTP response header.
	 * @param name {const char*} Field name in HTTP response header.
	 * @param value {const char*} Field value.
	 */
	HttpServletResponse& setHeader(const char* name, const char* value);

	/**
	 * Set integer format field in HTTP response header.
	 * @param name {const char*} Field name in HTTP response header.
	 * @param value {int} Field value.
	 */
	HttpServletResponse& setHeader(const char* name, int value);

	/**
	 * For partial response, set the offset position of the response body,
	 * subscript starts from 0.
	 * @param from {http_off_t} Response body starting offset position (subscript
	 * starts from 0).
	 * @param to {http_off_t} Response body ending position. This value should be
	 * less than the data length.
	 * @param total {http_off_t} Total data length. When data source is a static
	 * file, this value
	 *  should be equal to the total length of the file.
	 * @return {HttpServletResponse&}
	 */
#if  defined(_WIN32) || defined(_WIN64)
	HttpServletResponse& setRange(__int64 from,
		__int64 to, __int64 total);
#else
	HttpServletResponse& setRange(long long from,
		long long to, long long total);
#endif

	/**
	 * Set status code in HTTP response header: 1xx, 2xx, 3xx, 4xx, 5xx.
	 * @param status {int} HTTP response status code, e.g., 200.
	 */
	HttpServletResponse& setStatus(int status);

	/**
	 * Set to CGI mode. Users generally should not call this directly, because
	 * HttpServlet
	 * will automatically determine whether it is CGI mode.
	 * @param on {bool} Whether in CGI mode.
	 */
	HttpServletResponse& setCgiMode(bool on);

	/**
	 * Set specific location field in HTTP response header.
	 * @param location {const char*} URL, non-empty.
	 * @param status {int} HTTP response status code, generally 3xx.
	 */
	HttpServletResponse& setRedirect(const char* location, int status = 302);

	/**
	 * Add cookie object. This object must be dynamically allocated. Users should
	 * not
	 * manually release this object, as internally will automatically release it.
	 * @param cookie {HttpCookie*}
	 */
	HttpServletResponse& addCookie(HttpCookie* cookie);

	/**
	 * Add cookie.
	 * @param name {const char*} cookie name.
	 * @param value {const char*} cookie value.
	 * @param domain {const char*} cookie storage domain.
	 * @param path {const char*} cookie storage path.
	 * @param expires {time_t} cookie expiration time. When it is a past timestamp,
	 *  this value is the cookie's survival time (seconds).
	 */
	HttpServletResponse& addCookie(const char* name, const char* value,
		const char* domain = NULL, const char* path = NULL,
		time_t expires = 0);

	/**
	 * Encode url to url encoding.
	 * @param out {string&} Storage encoded result.
	 * @param url {const char*} Original url before encoding.
	 */
	void encodeUrl(string& out, const char* url);

	/**
	 * Get HTTP response header.
	 * @return {http_header&}
	 */
	http_header& getHttpHeader() const;

	/**
	 * Send HTTP response body data to client. This function can be called in a
	 * loop.
	 * If setChunkedTransferEncoding is called to set chunked transfer mode,
	 * internally automatically uses chunked transfer mode. When calling this
	 * function, it will automatically
	 * call sendHeader to send HTTP response header. Internally automatically sends
	 * HTTP response header on the first
	 * write. Additionally, when using chunked format for transfer, applications
	 * should
	 * call write(NULL, 0) at least once at the end to indicate data end.
	 * @param data {const void*} Data address.
	 * @param len {size_t} Data length of data.
	 * @return {bool} Whether sending was successful. Returns false to indicate
	 * connection broken.
	 */
	bool write(const void* data, size_t len);

	/**
	 * Send HTTP response body data to client. This function can be called in a
	 * loop. This function
	 * internally calls HttpServletResponse::write(const void*, size_t) process.
	 * Additionally, when using chunked format for transfer, applications should
	 * call write(NULL, 0) at least once at the end
	 * or pass an empty buffer buf.empty() == true.
	 * @param buf {const string&} Data buffer.
	 * @return {bool} Whether sending was successful. Returns false to indicate
	 * connection broken.
	 */
	bool write(const string& buf);

	/**
	 * Send HTTP Xml response body to client.
	 * @param body {const xml&} Data buffer.
	 * @param charset {const char*} Character set encoding.
	 * @return {bool} Whether sending was successful. Returns false to indicate
	 * connection broken.
	 */
	bool write(const xml& body, const char* charset = "utf-8");

	/**
	 * Send HTTP Json response body to client.
	 * @param body {const json&} Data buffer.
	 * @param charset {const char*} Character set encoding.
	 * @return {bool} Whether sending was successful. Returns false to indicate
	 * connection broken.
	 */
	bool write(const json& body, const char* charset = "utf-8");

	/**
	 * Format HTTP client response data in variable parameter format. Internally
	 * automatically calls
	 * HttpServletResponse::write(const void*, size_t) process. When using
	 * chunked format for transfer, applications should call write(NULL, 0)
	 * at least once at the end to indicate data end.
	 * @param fmt {const char*} Variable parameter format string.
	 * @return {int} Returns value > 0 on success, otherwise returns -1.
	 */
	int format(const char* fmt, ...) ACL_CPP_PRINTF(2, 3);

	/**
	 * Format HTTP client response data in variable parameter format. Internally
	 * automatically calls
	 * HttpServletResponse::write(const string&) process. When using chunked
	 * format for transfer, applications should call write(NULL, 0) at least once
	 * at the end to indicate data end.
	 * @param fmt {const char*} Variable parameter format string.
	 * @param ap {va_list} Parameter list.
	 * @return {int} Returns value > 0 on success, otherwise returns -1.
	 */
	int vformat(const char* fmt, va_list ap);

	///////////////////////////////////////////////////////////////////

	/**
	 * Send HTTP response header. Users should call this function before sending
	 * HTTP
	 * response header to client.
	 * @return {bool} Whether sending was successful. Returns false to indicate
	 * connection broken.
	 *  When users use getOutputStream to get socket write stream, this function
	 *  will be automatically called.
	 */
	bool sendHeader();

	/**
	 * Get HTTP response output stream. Users should call sendHeader first to
	 * send HTTP response header, then use this stream to write HTTP body.
	 * @return {ostream&}
	 */
	ostream& getOutputStream() const;

	/**
	 * Get HTTP bidirectional stream object passed in constructor parameter.
	 * @return {socket_stream&}
	 */
	socket_stream& getSocketStream() const;

	/**
	 * Get underlying http_client communication object.
	 * @return {http_client*} May be NULL.
	 */
	http_client* getClient() const {
		return client_;
	}

	/**
	 * Set http request object. This function should currently only be used
	 * internally by HttpServlet.
	 * @param request {HttpServletRequest*}
	 */
	void setHttpServletRequest(HttpServletRequest* request);

private:
	dbuf_guard* dbuf_internal_;
	dbuf_guard* dbuf_;
	socket_stream& stream_;		// Client connection stream
	HttpServletRequest* request_;	// http request object
	http_client* client_;		// http response object
	http_header* header_;		// http response header
	char  charset_[32];		// Character set
	char  content_type_[32];	// content-type type
	bool  head_sent_;		// Whether HTTP response header has been sent
};

}  // namespace acl

#endif // ACL_CLIENT_ONLY

