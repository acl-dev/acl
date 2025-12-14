#pragma once
#include "../acl_cpp_define.hpp"
#include <string>
#include "../stream/aio_socket_stream.hpp"
#if !defined(_WIN32) && !defined(_WIN64)
#include <netinet/in.h>  // just for "struct sockaddr_storage"
#endif

struct HTTP_HDR;
struct HTTP_HDR_RES;
struct HTTP_RES;
struct HTTP_HDR_REQ;
struct HTTP_REQ;

struct ACL_ASTREAM_CTX;

namespace acl {

class string;
class aio_handle;
class aio_socket_stream;
class socket_stream;
class zlib_stream;
class websocket;
class sslbase_conf;
class sslbase_io;
class http_header;

/**
 * HTTP client asynchronous communication class, not only supports standard HTTP communication protocol, but also supports Websocket communication;
 * Both HTTP protocol and Websocket communication support SSL encrypted transmission;
 * Additionally, for HTTP protocol, when user sets it, it can automatically decompress GZIP response data. The data received in callback
 * function on_http_res_body() is decompressed data.
 */
class ACL_CPP_API http_aclient : public aio_open_callback {
public:
	/**
	 * Constructor
	 * @param handle {aio_handle&} Asynchronous communication event handle.
	 * @param ssl_conf {sslbase_conf*} When NULL, automatically enables SSL communication mode.
	 */
	explicit http_aclient(aio_handle& handle, sslbase_conf* ssl_conf = NULL);
	virtual ~http_aclient();

	/**
	 * Callback when object is destroyed. Subclasses must implement.
	 */
	virtual void destroy() = 0;

	/**
	 * Get HTTP request header so that applications can add fields to HTTP request header.
	 * @return {http_header&}
	 */
	http_header& request_header();

	/**
	 * Set whether HTTP protocol automatically decompresses response.
	 * @param on {bool}
	 * @return {http_aclient&}
	 */
	http_aclient& unzip_body(bool on);

	/**
	 * Whether GZIP compression is automatically decompressed.
	 * @return {bool}
	 */
	bool is_unzip_body() const {
		return unzip_;
	}

	/**
	 * In addition to SSL conf set in constructor, you can also call this method to set it. Additionally,
	 * when ssl_conf passed to constructor is NULL, internally automatically sets ssl_enable_ to
	 * false. You can call this method to set ssl_conf, or call enable_ssl()
	 * method to enable ssl.
	 * @param ssl_conf {sslbase_conf*} When NULL, disables SSL.
	 * @return {http_aclient&}
	 */
	http_aclient& set_ssl_conf(sslbase_conf* ssl_conf);

	/**
	 * Get configured SSL conf.
	 * @return {sslbase_conf*} Returns NULL to indicate not set.
	 */
	sslbase_conf* get_ssl_conf() const {
		return ssl_conf_;
	}

	/**
	 * Set SNI field when SSL handshake.
	 * @param sni {const char*} When NULL, automatically uses Host field in HTTP header.
	 * @return {http_request&}
	 */
	http_aclient& set_ssl_sni(const char* sni);

	/**
	 * Set SNI field prefix when SSL handshake.
	 * @param prefix {const char*} SNI prefix.
	 * @return {http_request&}
	 */
	http_aclient& set_ssl_sni_prefix(const char* prefix);

	/**
	 * Set SNI field suffix when SSL handshake.
	 * @param suffix {const char*} SNI suffix.
	 * @return {http_request&}
	 */
	http_aclient& set_ssl_sni_suffix(const char* prefix);

	/**
	 * When ssl_conf in constructor is not empty, you can call this method to set whether to enable SSL
	 * encryption. When ssl_conf is not empty, internally ssl_enable_ default value is true. You can
	 * call this method to disable ssl.
	 * @param yes {bool}
	 * @return {http_aclient&}
	 */
	http_aclient& enable_ssl(bool yes);

	/**
	 * Determine whether SSL is enabled internally.
	 * @return {bool}
	 */
	bool is_enable_ssl() const {
		return ssl_enable_ && ssl_conf_;
	}

	/**
	 * Initialize asynchronous connection to remote WEB server.
	 * @param addr {const char*} Remote WEB server address, format:
	 *  domain:port or ip:port. When address is domain name, internally automatically starts asynchronous DNS resolution
	 *  process. You need to set DNS server address through aio_handle::set_dns() at program initialization.
	 *  When address is IP, no DNS resolution is needed.
	 * @param conn_timeout {int} Connection timeout time (seconds)
	 * @param rw_timeout {int} Network IO read/write timeout time (seconds)
	 * @param local {const char*} Local binding IP address. When not empty, if first character
	 *  is @, it means bind local IP address. If it is #, it means bind to specified interface. Examples:
	 *  @127.0.0.1, #eth1
	 * @return {bool} Returns false to indicate connection failed. Returns true to indicate asynchronous connection started.
	 */
	bool open(const char* addr, int conn_timeout, int rw_timeout,
		const char *local = NULL);

	/**
	 * Asynchronously close connection.
	 */
	void close();

	/**
	 * Get DNS address used for this connection, regardless of success or failure.
	 * @param out {string&} Store result.
	 * @return {bool} Returns false to indicate no available DNS address.
	 */
	bool get_ns_addr(string& out) const;

	/**
	 * After connection succeeds, fails, or times out, you can call this method to get the application server address currently connected to.
	 * @param out {string&} Store result.
	 * @return {bool} Returns false to indicate no connection to application server address.
	 */
	bool get_server_addr(string& out) const;

	/**
	 * After connection succeeds, you can call this method to get asynchronous connection object.
	 * @return {aio_socket_stream*}
	 */
	aio_socket_stream* get_conn() const {
		return conn_;
	}

protected:
	/**
	 * Callback when connection succeeds. Subclasses must implement. Applications should construct HTTP request
	 * in this method and call send_request method to send HTTP request to WEB server.
	 * @return {bool} When this method returns false, internally automatically closes connection.
	 */
	virtual bool on_connect() = 0;

	/**
	 * Callback when DNS resolution fails. When calling this method, internally automatically calls this->destroy().
	 */
	virtual void on_ns_failed() {}

	/**
	 * Callback when connection times out. When calling this method, internally automatically calls this->destroy().
	 */
	virtual void on_connect_timeout() {}

	/**
	 * Callback after connection fails. When calling this method, internally automatically calls this->destroy().
	 */
	virtual void on_connect_failed() {}

	/**
	 * Callback when read timeout.
	 * @return {bool} When timeout callback returns true, internally continues reading data. When
	 *  returns false, connection will be closed, and then on_disconnect() callback method will be called.
	 */
	virtual bool on_read_timeout() {
		return false;
	}

	/**
	 * Callback when connection succeeds or connection closes. Internally calls this method, and then calls
	 * destroy() callback.
	 */
	virtual void on_disconnect() {};

	/**
	 * Callback when receiving WEB server response header.
	 * @param header {const http_header&}
	 * @return {bool} Returns false to close connection, otherwise continues.
	 */
	virtual bool on_http_res_hdr(const http_header& header) {
		(void) header;
		return true;
	}

	/**
	 * Callback when receiving WEB server response body. This method may be called multiple times
	 * until response data is completely read.
	 * @param data {char*} Data read this time. Note: This data may be modified internally.
	 * @param dlen {size_t} Length of data read this time.
	 * @return {bool} Returns false to close connection, otherwise continues.
	 */
	virtual bool on_http_res_body(char* data, size_t dlen) {
		(void) data;
		(void) dlen;
		return true;
	}

	/**
	 * Callback when HTTP response is completely read.
	 * @param success {bool} Whether successfully read HTTP response completely.
	 * @return {bool} When successful, returns false to close connection.
	 */
	virtual bool on_http_res_finish(bool success) {
		(void) success;
		return true;
	}

	/**
	 * Callback when websocket handshake succeeds.
	 * @return {bool} Returns false to indicate need to close connection, otherwise continues.
	 */
	virtual bool on_ws_handshake() {
		// Initialize asynchronous websocket reading.
		this->ws_read_wait(0);
		return true;
	}

	/**
	 * Callback when websocket handshake fails.
	 * @param status {int} HTTP response status returned by server.
	 */
	virtual void on_ws_handshake_failed(int status) {
		(void) status;
	}

	/**
	 * Callback when receiving a text type frame.
	 * @return {bool} Returns true to indicate continue reading. Returns false to indicate need to close connection.
	 */
	virtual bool on_ws_frame_text() { return true; }

	/**
	 * Callback when receiving a binary type frame.
	 * @return {bool} Returns true to indicate continue reading. Returns false to indicate need to close connection.
	 */
	virtual bool on_ws_frame_binary() { return true; }

	/**
	 * Callback when receiving a close frame.
	 */
	virtual void on_ws_frame_closed() {}

	/**
	 * Callback when receiving data in websocket communication mode.
	 * @param data {char*} Address of received data.
	 * @param dlen {size_t} Length of received data.
	 * @return {bool} Returns true to indicate continue reading. Returns false to indicate need to close connection.
	 */
	virtual bool on_ws_frame_data(char* data, size_t dlen) {
		(void) data;
		(void) dlen;
		return true;
	}

	/**
	 * Callback when one frame is completely read.
	 * @return {bool} Returns true to indicate continue reading. Returns false to indicate need to close connection.
	 */
	virtual bool on_ws_frame_finish() { return true; }

	/**
	 * Callback when receiving ping data packet. When this callback returns, if data is not empty, internally automatically
	 * writes pong message. If data is empty, after this method returns, it will not send pong message to peer.
	 * @param data {string&} Received data.
	 */
	virtual void on_ws_frame_ping(string& data) {
		(void) data;
	}

	/**
	 * Callback when receiving pong message.
	 * @param data {string&} Received data.
	 */
	virtual void on_ws_frame_pong(string& data) {
		(void) data;
	}

public:
	/**
	 * Send HTTP request to WEB server. Internally automatically starts HTTP response reading after sending.
	 * @param body {const void*} HTTP request body. When NULL, internally constructs
	 *  HTTP GET request.
	 * @param len {size_t} When body is NULL, indicates body length.
	 */
	void send_request(const void* body, size_t len);

	/**
	 * Send WEBSOCKET handshake request.
	 * @param key {const void*} Key value to be sent.
	 * @param len {size_t} Length of key value.
	 */
	void ws_handshake(const void* key, size_t len);
	void ws_handshake(const char* key = "123456789xxx");

	/**
	 * When calling ws_handshake(), internally calls this callback to set websocket request header information.
	 * Applications can modify request header information through this callback before sending websocket handshake request.
	 */
	virtual void ws_handshake_before(http_header& reqhdr) {
		(void) reqhdr;
	}

	/**
	 * Initialize asynchronous websocket reading.
	 * @param timeout {int} Read timeout. When value <= 0, no read timeout is set,
	 *  otherwise when read timeout expires, timeout callback will be called.
	 *  Note:
	 *  This value is different from rw_timeout in open(). The timeout in open() function is
	 *  for standard HTTP IO process and SSL handshake process. The timeout here is
	 *  specifically for websocket reading timeout. This is mainly because websocket applications are often
	 *  long connections.
	 */
	void ws_read_wait(int timeout = 0);

	/**
	 * Asynchronously send a FRAME_TEXT type data frame.
	 * @param data {char*} Internally may modify this data, original data may be changed.
	 * @param len {size_t} Data length.
	 * @return {bool}
	 */
	bool ws_send_text(char* data, size_t len);

	/**
	 * Asynchronously send a FRAME_BINARY type data frame.
	 * @param data {void*} Internally may modify this data, original data may be changed.
	 * @param len {size_t} Data length.
	 * @return {bool}
	 */
	bool ws_send_binary(void* data, size_t len);

	/**
	 * Asynchronously send a FRAME_PING type data frame.
	 * @param data {void*} Internally may modify this data, original data may be changed.
	 * @param len {size_t} Data length.
	 * @return {bool}
	 */
	bool ws_send_ping(void* data, size_t len);

	/**
	 * Asynchronously send a FRAME_PONG type data frame.
	 * @param data {void*} Internally may modify this data, original data may be changed.
	 * @param len {size_t} Data length.
	 * @return {bool}
	 */
	bool ws_send_pong(void* data, size_t len);

protected:
	// @override dummy
	bool open_callback() { return true; }

	// @override
	bool timeout_callback();

	// @override
	void close_callback();

	// @override
	bool read_wakeup();

	// @override
	bool read_callback(char* data, int len);

protected:
	unsigned           status_;
	int                rw_timeout_;
	int                gzip_header_left_;	// Remaining bytes when gzip decompression header.
	bool               keep_alive_;
	bool               unzip_;		// Whether to automatically decompress response body.
	aio_handle&        handle_;
	sslbase_conf*      ssl_conf_;		// When not empty, enables SSL encryption.
	bool               ssl_enable_;		// Whether SSL encryption is enabled.
	std::string        sni_host_;
	std::string        sni_prefix_;
	std::string        sni_suffix_;
	aio_socket_stream* conn_;
	socket_stream*     stream_;
	http_header*       header_;
	HTTP_HDR_RES*      hdr_res_;
	HTTP_RES*          http_res_;
	websocket*         ws_in_;
	websocket*         ws_out_;
	string*            buff_;
	zlib_stream*       zstream_;		// Decompression stream.
	struct sockaddr_storage ns_addr_;	// DNS server address used.
	struct sockaddr_storage serv_addr_;	// Application server address connected to.

	bool handle_connect(const ACL_ASTREAM_CTX* ctx);
	bool handle_ssl_handshake();

	bool handle_res_hdr(int status);

	bool handle_res_body(char* data, int dlen);
	bool res_plain(char* data, int dlen);
	bool res_unzip(zlib_stream& zstream, char* data, int dlen);

	bool handle_res_body_finish(char* data, int dlen);
	bool res_plain_finish(char* data, int dlen);
	bool res_unzip_finish(zlib_stream& zstream, char* data, int dlen);

	bool handle_websocket();
	bool handle_ws_data();
	bool handle_ws_ping();
	bool handle_ws_pong();
	bool handle_ws_other();

private:
	static int connect_callback(const ACL_ASTREAM_CTX* ctx);
	static int http_res_hdr_cllback(int status, void* ctx);
	static int http_res_callback(int status, char* data, int dlen, void* ctx);
};

} // namespace acl

