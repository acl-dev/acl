#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/noncopyable.hpp"
#include "http_header.hpp"

#ifndef ACL_CLIENT_ONLY

namespace acl {

class session;
class socket_stream;
class HttpServletRequest;
class HttpServletResponse;

/**
 * Base class for handling HTTP client requests. Subclasses need to inherit this class
 */
class ACL_CPP_API HttpServlet : public noncopyable {
public:
	/**
	 * Constructor
	 * @param stream {socket_stream*} When running under control of acl_master server framework,
	 *  this parameter must be non-empty. When running in CGI mode under apache, this parameter
	 *  is set to NULL. In addition, this function internally will not close stream connection. Application should handle stream object
	 *  closing itself, which facilitates integration with acl_master architecture
	 * @param session {session*} One session object per HttpServlet object
	 */
	HttpServlet(socket_stream* stream, session* session);
	explicit HttpServlet(socket_stream* stream);

	/**
	 * Constructor (this function is deprecated, please use other construction methods)
	 * @param stream {socket_stream*} When running under control of acl_master server framework,
	 *  this parameter must be non-empty. When running in CGI mode under apache, this parameter
	 *  is set to NULL. In addition, this function internally will not close stream connection. Application should handle stream object
	 *  closing itself, which facilitates integration with acl_master architecture
	 * @param memcache_addr {const char*}
	 */
	//@ACL_DEPRECATED
	HttpServlet(socket_stream* stream, const char* memcache_addr);

	HttpServlet();
	virtual ~HttpServlet() = 0;

	session& getSession() const {
		return *session_;
	}

	socket_stream* getStream() const {
		return stream_;
	}

	/**
	 * Set local character set. If local character set is set, when receiving HTTP request data, will
	 * automatically convert request's character set to local character set. This function must be called before doRun to be effective
	 * @param charset {const char*} Local character set. If this pointer is empty,
	 *  clears local character set
	 * @return {HttpServlet&}
	 */
	HttpServlet& setLocalCharset(const char* charset);

	/**
	 * Set HTTP session process IO read/write timeout. This function must be called before doRun to be effective
	 * @param rw_timeout {int} Read/write timeout (seconds)
	 * @return {HttpServlet&}
	 */
	HttpServlet& setRwTimeout(int rw_timeout);

	/**
	 * For POST method, this method sets whether to parse Form body data. Default is to parse.
	 * This function must be called before doRun to be effective. When body data is data stream or MIME format,
	 * even if this method is called to set parsing data, body data will not be parsed
	 * @param yes {bool} Whether to parse
	 * @return {HttpServlet&}
	 */
	HttpServlet& setParseBody(bool yes);

	/**
	 * For POST method, this method sets maximum length for parsing body data. If body data exceeds this length, this function
	 * must be called before doRun to be effective
	 * @param length {int} Maximum length limit. If request body data length is too large, directly
	 *  returns false. If this value <= 0, internally does not limit body data length. Internal default value before calling this function is 0
	 * @return {HttpServlet&}
	 */
	HttpServlet& setParseBodyLimit(int length);
	
	/**
	 * HttpServlet object starts running, receives HTTP requests, and calls following doXXX virtual functions.
	 * @return {bool} Returns processing result. Returns false indicates processing failed, should close connection.
	 *  Returns true indicates processing succeeded. After calling this function, should continue to determine whether to maintain long connection
	 *  based on whether request/response objects require maintaining long connection
	 */
	bool start();

	/**
	 * HttpServlet object starts running, receives HTTP requests, and calls following doXXX virtual functions.
	 * This function first calls start process, then decides whether to maintain long connection with client based on start's return result and whether request/response
	 * objects require maintaining long connection
	 * @return {bool} Returns processing result. Returns false indicates processing failed or processing succeeded but not maintaining
	 *  long connection, should close connection
	 */
	virtual bool doRun();

	/**
	 * HttpServlet object starts running, receives HTTP requests, and calls following doXXX virtual functions
	 * @param session {session&} Object for storing session data
	 * @param stream {socket_stream*} When running under control of acl_master server framework,
	 *  this parameter must be non-empty. When running in CGI mode under apache, this parameter
	 *  is set to NULL. In addition, this function internally will not close stream connection. Application should handle stream object
	 *  closing itself, which facilitates integration with acl_master architecture
	 * @return {bool} Returns processing result
	 */
	virtual bool doRun(session& session, socket_stream* stream = NULL);

	/**
	 * HttpServlet object starts running, receives HTTP requests, and calls following doXXX virtual functions.
	 * Calling this function means using memcached to store session data
	 * @param memcached_addr {const char*} memcached server address, format: IP:PORT
	 * @param stream {socket_stream*} Same meaning as above
	 * @return {bool} Returns processing result
	 */
	virtual bool doRun(const char* memcached_addr, socket_stream* stream);

protected:
	/**
	 * Virtual function called when HTTP request is GET method
	 */
	virtual bool doGet(HttpServletRequest&, HttpServletResponse&);

	/**
	 * Virtual function called when HTTP request is websocket method
	 */
	virtual bool doWebSocket(HttpServletRequest&, HttpServletResponse&);

	/**
	 * Old websocket handling interface, please override doWebSocket method above
	 */
	virtual bool doWebsocket(HttpServletRequest&, HttpServletResponse&);

	/**
	 * Virtual function called when HTTP request is POST method
	 */
	virtual bool doPost(HttpServletRequest&, HttpServletResponse&);

	/**
	 * Virtual function called when HTTP request is PUT method
	 */
	virtual bool doPut(HttpServletRequest&, HttpServletResponse&);

	/**
	 * Virtual function called when HTTP request is PATCH method
	 */
	virtual bool doPatch(HttpServletRequest&, HttpServletResponse&);

	/**
	 * Virtual function called when HTTP request is CONNECT method
	 */
	virtual bool doConnect(HttpServletRequest&, HttpServletResponse&);

	/**
	 * Virtual function called when HTTP request is PURGE method. This method is used when clearing SQUID cache
	 */
	virtual bool doPurge(HttpServletRequest&, HttpServletResponse&);

	/**
	 * Virtual function called when HTTP request is DELETE method
	 */
	virtual bool doDelete(HttpServletRequest&, HttpServletResponse&);

	/**
	 * Virtual function called when HTTP request is HEAD method
	 */
	virtual bool doHead(HttpServletRequest&, HttpServletResponse&);

	/**
	 * Virtual function called when HTTP request is OPTION method
	 */
	virtual bool doOptions(HttpServletRequest&, HttpServletResponse&);

	/**
	 * Virtual function called when HTTP request is PROPFIND method
	 */
	virtual bool doPropfind(HttpServletRequest&, HttpServletResponse&);

	/**
	 * Virtual function called when HTTP request method is unknown
	 * @param method {const char*} Other unknown request methods
	 */
	virtual bool doOther(HttpServletRequest&, HttpServletResponse&,
		const char* method);

	/**
	 * Virtual function called when HTTP request method is unknown
	 */
	virtual bool doUnknown(HttpServletRequest&, HttpServletResponse&);

	/**
	 * Virtual function called when HTTP request error occurs
	 */
	virtual bool doError(HttpServletRequest&, HttpServletResponse&);

protected:
	HttpServletRequest* req_;
	HttpServletResponse* res_;
	bool parse_body_;

private:
	session* session_;
	socket_stream* stream_;
	bool  first_;
	char* local_charset_;
	int   rw_timeout_;
	int   parse_body_limit_;
	bool  try_old_ws_;

	void init();
};

} // namespace acl

#endif // ACL_CLIENT_ONLY

