#pragma once

#include "../acl_cpp_define.hpp"
#include "../stream/aio_socket_stream.hpp"

namespace acl {

class sslbase_conf;
class mqtt_header;
class mqtt_message;

class ACL_CPP_API mqtt_aclient : public aio_open_callback {
public:
	/**
	 * constructor
	 * @param handle {aio_handle&}
	 * @param ssl_conf {sslbase_conf*} if not NULL, ssl will be used
	 */
	mqtt_aclient(aio_handle& handle, sslbase_conf* ssl_conf = NULL);

	/**
	 * because the subclass object was created dynamically, the method will
	 * be called when the subclass object is to be freed
	 */
	virtual void destroy(void) = 0;

	sslbase_conf* get_ssl_conf(void) const {
		return ssl_conf_;
	}

	/**
	 * connect the remote mqtt server, when connected with the server,
	 * the callback on_connect() will be called
	 * @param addr {const char*} the mqtt server's addr with the format
	 *  ip|port, or domain|port
	 * @param conn_timeout {int} the timeout for connecting to the server
	 * @param rw_timeout {int} the timeout read/write with the server
	 * @return bool {bool} if return false, you should call destroy() to
	 *  delete the subclass object
	 */
	bool open(const char* addr, int conn_timeout, int rw_timeout);

	/**
	 * called when connect or accept one connection
	 * @param conn {aio_socket_stream*}
	 * @return bool {bool} if return false, you should call destroy() to
	 *  delete the subclass object
	 */
	bool open(aio_socket_stream* conn);

	/**
	 * close the connection with the mqtt server async
	 */
	void close(void);

	/**
	 * get the connection with the mqtt server
	 * @return {aio_socket_stream*} return NULL if not connected
	 */
	aio_socket_stream* get_conn(void) const {
		return conn_;
	}

	/**
	 * set the remote host name to specify the SSL SNI for SSL handshake
	 * @param host {const char*} the host name
	 */
	void set_host(const char* host);

public:
	bool send(mqtt_message& message);

public:
	bool get_ns_addr(string& out) const;
	bool get_server_addr(string& out) const;

protected:
	// the subclass should be created dynamically
	virtual ~mqtt_aclient(void);

	// @override dummy
	bool open_callback(void) { return true; }

	// @override
	bool timeout_callback(void);

	// @override
	void close_callback(void);

	// @override
	bool read_wakeup(void);

	// @override
	bool read_callback(char* data, int len);

protected:
	// wait for reading data from peer
	bool message_await(void);

	virtual void on_ns_failed(void) {}
	virtual void on_connect_timeout(void) {}
	virtual void on_connect_failed(void) {}
	virtual bool on_read_timeout(void) { return false; }
	virtual void on_disconnect(void) {};

	virtual bool on_open(void) = 0;
	virtual bool on_header(const mqtt_header&) { return true; };
	virtual bool on_body(const mqtt_message&) = 0;

private:
	aio_handle&        handle_;
	sslbase_conf*      ssl_conf_;
	aio_socket_stream* conn_;
	int conn_timeout_;
	int rw_timeout_;
	string host_;
	struct sockaddr_storage ns_addr_;
	struct sockaddr_storage serv_addr_;
	mqtt_header*  header_;
	mqtt_message* body_;

	bool open_done(void);

	bool handle_ssl_handshake(void);
	int handle_data(char* data, int len);

	bool handle_connect(const ACL_ASTREAM_CTX* ctx);
	static int connect_callback(const ACL_ASTREAM_CTX* ctx);
};

} // namespace acl
