#pragma once
#include "../acl_cpp_define.hpp"
#include "HttpServlet.hpp"

#ifndef ACL_CLIENT_ONLY

namespace acl {

class websocket;
class session;
class HttpServletRequest;
class HttpServletResponse;

class ACL_CPP_API WebSocketServlet : public HttpServlet {
public:
	WebSocketServlet();

	/**
	 * Constructor
	 * @param stream {socket_stream*} When running under control of acl_master server framework,
	 *  this parameter must be non-empty. When running in CGI mode under apache, this parameter
	 *  is set to NULL. In addition, this function internally will not close stream connection. Application should handle stream object
	 *  closing itself, which facilitates integration with acl_master architecture
	 * @param session {session*} One session object per HttpServlet object
	 */
	WebSocketServlet(socket_stream* stream, session* session);

	/**
	 * Constructor
	 * @param stream {socket_stream*} When running under control of acl_master server framework,
	 *  this parameter must be non-empty. When running in CGI mode under apache, this parameter
	 *  is set to NULL. In addition, this function internally will not close stream connection. Application should handle stream object
	 *  closing itself, which facilitates integration with acl_master architecture
	 * @param memcache_addr {const char*}
	 */
	WebSocketServlet(socket_stream* stream,
		const char* memcache_addr = "127.0.0.1:11211");

	/**
	 * HttpServlet object starts running, receives HTTP requests, and calls following doXXX virtual functions.
	 * This function first calls start process, then decides whether to maintain long connection with client based on start's return result and whether request/response
	 * objects require maintaining long connection.
	 */

	virtual ~WebSocketServlet();

	// @override
	bool doRun();

	// @override
	bool doRun(session& session, socket_stream* stream = NULL);

	// @override
	bool doRun(const char* memcached_addr, socket_stream* stream);

	/**
	 * Send binary data.
	 * @param buf {const char *} Data to be sent
	 * @param len {int} buf data length
	 * @return {bool} false on error, otherwise true
	 */
	bool sendBinary(const char *buf, int len);

	/**
	 * Send text data.
	 * @param text {const char *} Data to be sent
	 * @return {bool} false on error, otherwise true
	 */
	bool sendText(const char *text);

	/**
	 * Send pong message.
	 * @param buffer {const char *} Data to be sent
	 * @return {bool} false on error, otherwise true
	 */
	bool sendPong(const char *buffer = NULL);

	/**
	 * Send pong message.
	 * @param buffer {const char *} Data to be sent
	 * @return {bool} false on error, otherwise true
	 */
	bool sendPing(const char *buffer = NULL);

protected:
	/**
	 * websocket close message callback
	 */
	virtual void onClose() {}

	/**
	 * websocket ping message callback.
	 * @param payload_len {unsigned long long} Total length of message data
	 * @param finish {bool} Whether this data packet is the last one
	 * @return {bool} false to disconnect.
	 */
	virtual bool onPing(unsigned long long payload_len, bool finish) = 0;

	/**
	 * websocket pong message callback.
	 * @param payload_len {unsigned long long} Total length of message data
	 * @param finish {bool} Whether this data packet is the last one
	 * @return {bool} false to disconnect.
	 */
	virtual bool onPong(unsigned long long payload_len, bool finish) = 0;

	/**
	 * websocket ping message callback.
	 * @param payload_len {unsigned long long} Total length of message data
	 * @param text {bool } true indicates text data, otherwise binary data.
	 * @param finish {bool} Whether this data packet is the last one
	 * @return {bool} false to disconnect.
	 */
	virtual bool onMessage(unsigned long long payload_len,
			bool text, bool finish) = 0;

	/**
	 * Subclasses can call this method in a loop to get data frame body data until return <= 0
	 * @param buf {size_t*} Data buffer used to store result data
	 * @param size {size_t} buf buffer size
	 * @return {int} Length of data read, in the following three cases:
	 *   0: Indicates data frame is normally read completely
	 *  -1: Indicates read error
	 *  >0: Indicates data read, should call this method again to read remaining data
	 */
	int readPayload(void* buf, size_t size);

	/**
	 * Return websocket object. If returns NULL, it indicates websocket connection has not been established yet
	 * @return {websocket*}
	 */
	websocket* get_websocket() const
	{
		return ws_;
	}

private:
	// @override
	bool doWebSocket(HttpServletRequest&, HttpServletResponse&);

private:
	websocket *ws_;
	int   opcode_;
};

} // namespace acl

#endif // ACL_CLIENT_ONLY

