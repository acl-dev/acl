#pragma once
#include "../acl_cpp_define.hpp"
#include <vector>
#include "../stdlib/string.hpp"
#include "../stdlib/noncopyable.hpp"
#include "../http/http_header.hpp"
#include "http_ctype.hpp"
#include "http_type.hpp"

#ifndef ACL_CLIENT_ONLY

namespace acl {

class dbuf_guard;
class istream;
class ostream;
class socket_stream;
class http_client;
class http_mime;
class json;
class xml;
class session;
class HttpSession;
class HttpCookie;
class HttpServletResponse;

/**
 * Class related to HTTP client request parsing. This class should not be
 * inherited. Users also do not need to
 * create this object directly.
 */
class ACL_CPP_API HttpServletRequest : public noncopyable {
public:
	/**
	 * Constructor
	 * @param res {HttpServletResponse&}
	 * @param sess {session*} Object storing session data.
	 * @param stream {socket_stream&} Connection stream. Internally automatically
	 * manages and closes it.
	 * @param charset {const char*} Local character set. When this value is not
	 * empty,
	 * internally automatically converts HTTP request data to local character set
	 * for conversion.
	 * @param body_limit {int} When POST request body is text type,
	 * this parameter limits maximum request body length. When body length exceeds
	 * this value, it returns error. When body is MIME
	 *  format, on is false, this parameter is invalid.
	 */
	HttpServletRequest(HttpServletResponse& res, session* sess,
		socket_stream& stream, const char* charset = NULL,
		int body_limit = 102400);
	~HttpServletRequest();

	/**
	 * Set POST request. This function sets whether to parse Form request body
	 * data. Default is parsing.
	 * This function must be called before doRun to be effective. When body is MIME
	 * format,
	 * even if you call this function to disable parsing, it will still parse MIME
	 * data.
	 * @param yes {bool} Whether to parse.
	 */
	void setParseBody(bool yes);

	/**
	 * Get HTTP client request method: GET, POST, PUT, CONNECT, PURGE
	 * @param method_s {string*} When not empty, stores string format request
	 * method.
	 * @return {http_method_t}
	 */
	http_method_t getMethod(string* method_s = NULL) const;

	/**
	 * Convert HTTP request method type to corresponding string.
	 * @param type {http_method_t}
	 * @param buf  {string&} Store result string.
	 */
	static void methodString(http_method_t type, string& buf);

	/**
	 * Get all cookie objects from HTTP client request.
	 * @return {const std::vector<HttpCookie*>&}
	 */
	const std::vector<HttpCookie*>& getCookies() const;

	/**
	 * Get a certain cookie value from HTTP client request.
	 * @param name {const char*} Cookie name, cannot be empty.
	 * @return {const char*} Cookie value. Returns NULL to indicate this cookie
	 *  does not exist.
	 */
	const char* getCookieValue(const char* name) const;

	/**
	 * Set cookie in HTTP request header.
	 * @param name {const char*} Cookie name, non-empty string.
	 * @param value {const char*} Cookie value, non-empty string.
	 */
	void setCookie(const char* name, const char* value);

	/**
	 * Get a certain field value in HTTP request header.
	 * @param name {const char*} Field name in HTTP request header, cannot be
	 * empty.
	 * @return {const char*} Field value in HTTP request header. Returns NULL
	 *  when not found.
	 */
	const char* getHeader(const char* name) const;

	/**
	 * Get parameter part in HTTP GET request format URL, i.e., part after ?.
	 * @return {const char*} URL decoded parameter part string.
	 *  Returns empty string to indicate URL has no parameters.
	 */
	const char* getQueryString() const;

	/**
	 * Get /cgi-bin/test path part in
	 *  http://test.com.cn/cgi-bin/test?name=value.
	 * @return {const char*} Returns empty string to indicate error.
	 */
	const char* getPathInfo() const;

	/**
	 * Get /cgi-bin/test?name=value path part in
	 *  http://test.com.cn/cgi-bin/test?name=value.
	 * @return {const char*} Returns empty string to indicate error.
	 */
	const char* getRequestUri() const;

	/**
	 * Get HttpSession object related to HTTP session.
	 * @param create {bool} When session does not exist, whether to automatically
	 * create
	 * a session object for this client. When this parameter is false, this
	 * function
	 *  returns session object without binding, and cannot read/write.
	 * @param sid {const char*} When session does not exist and create parameter is
	 * not empty,
	 * if sid is not empty, use this value as user's unique session ID, and add it
	 * to client's
	 *  cookie.
	 * @return {HttpSession&}
	 *  Note: Priority order: COOKIE > create = true > sid != NULL
	 */
	HttpSession& getSession(bool create = true, const char* sid = NULL);

	/**
	 * Get HTTP client connection input stream object.
	 * @return {istream&}
	 */
	istream& getInputStream() const;

	/**
	 * Get HTTP bidirectional stream object passed to constructor parameter.
	 * @return {socket_stream&}
	 */
	socket_stream& getSocketStream() const;

	/**
	 * Get HTTP request body data length.
	 * @return {acl_int64} Returns -1 to indicate it is GET request
	 *  or HTTP request header has no Content-Length field.
	 */
#if defined(_WIN32) || defined(_WIN64)
	__int64 getContentLength() const;
#else
	long long int getContentLength() const;
#endif

	/**
	 * Determine whether client request is range data. This function gets start
	 * address and
	 * end address from request header.
	 * @param range_from {long long int&} Start offset position.
	 * @param range_to {long long int&} End offset position.
	 * @return {bool} When request is range data, returns true. When not range
	 * data, returns false.
	 *  Note: range_from/range_to subscript starts from 0.
	 */
#if defined(_WIN32) || defined(_WIN64)
	bool getRange(__int64& range_from, __int64& range_to);
#else
	bool getRange(long long int& range_from, long long int& range_to);
#endif
	/**
	 * Get Content-Type field value in HTTP request header: Content-Type:
	 * text/html; charset=gb2312
	 * @param part {bool} When true, returns text. Otherwise returns complete
	 *  value, e.g.: text/html; charset=gb2312
	 * @param ctype {http_ctype*} When pointer is not empty, stores parsed
	 * http_ctype information.
	 * @return {const char*} Returns NULL to indicate Content-Type field does not
	 * exist.
	 */
	const char* getContentType(bool part = true, http_ctype* ctype = NULL) const;

	/**
	 * Get charset field value gb2312 in HTTP request header Content-Type:
	 * text/html; charset=gb2312.
	 * @return {const char*} Returns NULL to indicate Content-Type field or
	 *  charset=xxx does not exist.
	 */
	const char* getCharacterEncoding() const;

	/**
	 * Get local character set field string.
	 * @ return {const char*} Returns NULL to indicate local character set was not
	 * set.
	 */
	const char* getLocalCharset() const;

	/**
	 * Get local IP address of HTTP connection.
	 * @return {const char*} Returns empty string to indicate unable to get.
	 */
	const char* getLocalAddr() const;

	/**
	 * Get local PORT number of HTTP connection.
	 * @return {unsigned short} Returns 0 to indicate unable to get.
	 */
	unsigned short getLocalPort() const;

	/**
	 * Get remote client IP address of HTTP connection.
	 * @return {const char*} Returns empty string to indicate unable to get.
	 */
	const char* getRemoteAddr() const;

	/**
	 * Get remote client PORT number of HTTP connection.
	 * @return {unsigned short} Returns 0 to indicate unable to get.
	 */
	unsigned short getRemotePort() const;

	/**
	 * Get Host field set in HTTP request header.
	 * @return {const char*} When empty, it means not found.
	 */
	const char* getRemoteHost() const;

	/**
	 * Get User-Agent field set in HTTP request header.
	 * @return {const char*} When empty, it means not found.
	 */
	const char* getUserAgent() const;

	/**
	 * Get parameter value in HTTP request. Value has been URL decoded
	 * and converted to local character set. For GET request, you can get
	 * parameter value after ? in URL. For POST request, you can get
	 * parameter value after ? in URL, plus all parameter values in body.
	 * @param name {const char*} Parameter name.
	 * @param case_sensitive {bool} Whether case-sensitive when comparing parameter
	 * names.
	 * @return {const char*} Returns parameter value. Returns NULL when parameter
	 * does not exist.
	 */
	const char* getParameter(const char* name,
		bool case_sensitive = false) const;

	/**
	 * When Content-Type in HTTP request header is
	 * multipart/form-data; boundary=xxx format, it indicates file upload request
	 * type.
	 * You can get http_mime object through this function.
	 * @return {const http_mime*} Returns NULL to indicate no MIME data.
	 * Returned value user should not manually release, because HttpServletRequest
	 * object
	 *  will automatically release it.
	 */
	http_mime* getHttpMime();

	/**
	 * When body is text/json or application/json format, you can call this method
	 * to get json
	 * body and parse it. Returns json object on success. This object is internally
	 * automatically managed. When
	 * HttpServletRequest object is destroyed, json object will also be destroyed.
	 * @param body_limit {size_t} Limit body length to prevent memory overflow.
	 * When
	 * body length exceeds this value, returns error. When this value is 0, no
	 * length limit.
	 * @return {json*} Returns successfully parsed json object. Returns NULL for
	 * following reasons:
	 *  1. Data length exceeded.
	 *  2. Invalid json data format.
	 *  3. Error occurred.
	 */
	json* getJson(size_t body_limit = 1024000);

	/**
	 * Same as above function, except that result is stored in user-provided
	 * object.
	 * @param out {json&}
	 * @param body_limit {size_t} Limit body length to prevent memory overflow.
	 * When
	 * body length exceeds this value, returns error. When this value is 0, no
	 * length limit.
	 * @return {bool} Returns false for following reasons:
	 *  1. Data length exceeded.
	 *  2. Invalid json data format.
	 *  3. Error occurred.
	 */
	bool getJson(json& out, size_t body_limit = 1024000);

	/**
	 * When body is text/xml or application/xml format, you can call this method to
	 * get xml
	 * body and parse it. Returns mxl object on success. This object is internally
	 * automatically managed. When
	 * HttpServletRequest object is destroyed, xml object will also be destroyed.
	 * @param body_limit {size_t} Limit body length to prevent memory overflow.
	 * When
	 * body length exceeds this value, returns error. When this value is 0, no
	 * length limit.
	 * @return {xml*} Returns successfully parsed xml object. Returns NULL for
	 * following reasons:
	 *  1. Data length exceeded.
	 *  2. Invalid xml data format.
	 */
	xml* getXml(size_t body_limit = 1024000);

	/**
	 * Same as above function, except that result is stored in user-provided
	 * object.
	 * @param out {xml&}
	 * @param body_limit {size_t} Limit body length to prevent memory overflow.
	 * When
	 * body length exceeds this value, returns error. When this value is 0, no
	 * length limit.
	 * @return {bool} Returns false for following reasons:
	 *  1. Data length exceeded.
	 *  2. Invalid xml data format.
	 *  3. Error occurred.
	 */
	bool getXml(xml& out, size_t body_limit = 1024000);

	/**
	 * Get POST request body data. For other request types, you can directly call
	 * this method to get
	 * request body data.
	 * @param body_limit {size_t} Limit body length to prevent memory overflow.
	 * When
	 * body length exceeds this value, returns error. When this value is 0, no
	 * length limit.
	 * @return {string*} Returns successfully parsed object. Returns NULL for
	 * following reasons:
	 *  1. Data length exceeded.
	 *  2. No request body.
	 *  3. Error occurred.
	 */
	string* getBody(size_t body_limit = 1024000);

	/**
	 * Same as above function, except that result is stored in user-provided
	 * object.
	 * @param out {string&}
	 * @param body_limit {size_t}
	 * @return {bool} Returns false for following reasons:
	 *  1. Data length exceeded.
	 *  2. No request body.
	 *  3. Error occurred.
	 */
	bool getBody(string& out, size_t body_limit = 1024000);

	/**
	 * Get HTTP request data type.
	 * @return {http_request_t} For POST request with file upload, you need to call
	 * this function to determine whether it is file upload request type. When this
	 * function returns HTTP_REQUEST_OTHER,
	 *  users can get request body type string by calling getContentType.
	 */
	http_request_t getRequestType() const;

	/**
	 * Get referer URL of HTTP request page.
	 * @return {const char*} Returns NULL to indicate user directly accessed this
	 * URL.
	 */
	const char* getRequestReferer() const;

	/**
	 * Get http_ctype object parsed from HTTP request header.
	 * @return {const http_ctype&}
	 */
	const http_ctype& getHttpCtype() const;

	/**
	 * Determine whether HTTP client wants to keep connection alive.
	 * @return {bool}
	 */
	bool isKeepAlive() const;

	/**
	 * When client wants to keep connection alive, get timeout value from HTTP
	 * request header.
	 * @return {int} Return value < 0 indicates no Keep-Alive field.
	 */
	int getKeepAlive() const;

	/**
	 * Get HTTP client request version.
	 * @param major {unsigned&} Major version number.
	 * @param minor {unsigned&} Minor version number.
	 * @return {bool} Whether successfully got client request version.
	 */
	bool getVersion(unsigned& major, unsigned& minor) const;

	/**
	 * Get compression algorithms supported by HTTP client.
	 * @param out {std::vector<string>&} Store result.
	 */
	void getAcceptEncoding(std::vector<string>& out) const;

	/*
	 * When HTTP request is POST request, you can call this function to set HTTP request body
	 * IO timeout value (seconds).
	 * @param rw_timeout {int} Read/write timeout time when reading body (seconds).
	 */
	void setRwTimeout(int rw_timeout);

	/**
	 * Get last error code.
	 * @return {http_request_error_t}
	 */
	http_request_error_t getLastError() const;

	/**
	 * When HttpServlet runs in server mode (not CGI mode), you can call this
	 * function to get HTTP client connection request, thereby getting more
	 * parameters.
	 * @return {http_client*} When running in server mode, this function returns
	 * HTTP client
	 * connection non-empty object. When running in CGI mode, returns empty
	 * pointer.
	 */
	http_client* getClient() const;

	/**
	 * Print HTTP request header to file stream.
	 * @param out {ostream&}
	 * @param prompt {const char*} Prompt string.
	 */	 
	void fprint_header(ostream& out, const char* prompt);

	/**
	 * Print HTTP request header to string.
	 * @param out {string&}
	 * @param prompt {const char*} Prompt string.
	 */
	void sprint_header(string& out, const char* prompt);

private:
	dbuf_guard* dbuf_internal_;
	dbuf_guard* dbuf_;
	http_request_error_t req_error_;
	char* cookie_name_;
	HttpServletResponse& res_;
	session* sess_;
	HttpSession* http_session_;
	socket_stream& stream_;
	int  body_limit_;
	bool body_parsed_;

	std::vector<HttpCookie*> cookies_;
	bool cookies_inited_;
	http_client* client_;
	http_method_t method_;
	bool cgi_mode_;
	http_ctype content_type_;
	char* localAddr_;
	char* remoteAddr_;
	char* localCharset_;
	int  rw_timeout_;
	std::vector<HTTP_PARAM*> params_;
	http_request_t request_type_;
	bool parse_body_;
	http_mime* mime_;
	string* body_;
	json* json_;
	xml* xml_;

	bool readHeaderCalled_;
	bool readHeader(string* method_s);

	void add_cookie(char* data);
	void parseParameters(const char* str);
};

} // namespace acl

#endif // ACL_CLIENT_ONLY

