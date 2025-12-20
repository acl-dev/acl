#pragma once
#include "../acl_cpp_define.hpp"
#if defined(_WIN32) || defined(_WIN64)
#include <WinSock2.h>
#endif
#include <string>
#include "istream.hpp"
#include "ostream.hpp"

struct ACL_VSTREAM;

namespace acl {

class ACL_CPP_API socket_stream : public istream , public ostream {
public:
	socket_stream();
	~socket_stream();

	/**
	 * Open a connection using an existing socket handle.
	 * @param fd Socket handle.
	 * @param udp_mode {bool} Whether in UDP mode.
	 * @return {bool} Whether opening was successful.
	 */
#if defined(_WIN32) || defined(_WIN64)
	bool open(SOCKET fd, bool udp_mode = false);
#else
	bool open(int fd, bool udp_mode = false);
#endif

	/**
	 * Open using ACL_VSTREAM stream object.
	 * @param vstream {ACL_VSTREAM*}
	 * @param udp_mode {bool} Whether in UDP mode.
	 * @return {bool} Whether opening was successful.
	 */
	bool open(ACL_VSTREAM* vstream, bool udp_mode = false);

	/**
	 * Connect to remote server and open connection.
	 * @param addr {const char*} Server address. Can be UNIX domain socket address
	 * (UNIX platform),
	 * socket address like /tmp/test.sock. On Linux platform, abstract socket
	 * handle can also be used,
	 * abstract unix socket. To distinguish from ordinary file path unix socket
	 * address,
	 * ACL library stipulates that the first byte of the address is @, which means
	 * Linux abstract socket
	 * (abstract unix domain socket). Note: This function is only supported on
	 * Linux platform,
	 *  example: @/tmp/test.sock;
	 *  remote_addr[@local_ip]|[#interface], e.g.:
	 *  1. www.sina.com|80@60.28.250.199, meaning bind local address to:
	 *     60.28.250.199, connect to www.sina.com port 80;
	 *  2.  211.150.111.12|80@192.168.1.1 means bind local address.
	 *  3. 211.150.111.12|80#eth1 means bind to specified interface;
	 * 4. Let OS automatically bind local IP address, can be written as:
	 * www.sina.com:80;
	 * @param conn_timeout {int} Connection timeout time (unit value depends on
	 * use_ms)
	 * @param rw_timeout {int} Read/write timeout time (unit value depends on
	 * use_ms)
	 * @param unit {time_unit_t} Time unit for timeout time.
	 * @return {bool} Whether opening was successful.
	 */
	bool open(const char* addr, int conn_timeout, int rw_timeout,
		time_unit_t unit = time_unit_s);

	/**
	 * Bind local UDP address and create UDP communication object.
	 * @param addr {const char*} Local address, format: ip:port. This address can
	 * also be
	 * UNIX socket handle or Linux abstract socket handle (Linux abstract unix
	 * socket).
	 * @param rw_timeout {int} Read/write timeout time (seconds)
	 * @param flags {unsigned} Definition of this flag bit, see server_socket.hpp
	 * @return {bool} Whether binding was successful.
	 */
	bool bind_udp(const char* addr, int rw_timeout = -1, unsigned flags = 0);

	/**
	 * Bind multicast socket communication object.
	 * @param addr {const char*} Multicast IP address.
	 * @param iface {const char*} Local IP address for receiving data packets.
	 * @param port {int} Multicast port number.
	 * @param rw_timeout {int} IO read/write timeout time.
	 * @param flags {unsigned} Definition of this flag bit, see server_socket.hpp
	 * @return {bool} Whether binding was successful.
	 */
	bool bind_multicast(const char* addr, const char* iface, int port,
		int rw_timeout = -1, unsigned flags = 0);

	/**
	 * After bind_multicast succeeds, you can call this method to set multicast TTL
	 * value.
	 * @param ttl {int} Value range is 1--255.
	 * @return {bool} Whether setting was successful.
	 */
	bool multicast_set_ttl(int ttl);

	/**
	 * After bind_multicast succeeds, you can call this method to set multicast
	 * local IP address.
	 * @param iface {const char*}
	 * @return {bool} Whether setting was successful.
	 */
	bool multicast_set_if(const char* iface);

	/**
	 * After bind_multicast succeeds, you can call this method to leave multicast.
	 * @param addr {const char*} Multicast IP.
	 * @param iface {const char*} Local IP.
	 * @return {bool} Whether successful.
	 */
	bool multicast_drop(const char *addr, const char *iface);

	/**
	 * Close socket read end.
	 * @return {bool}
	 */
	bool shutdown_read();

	/**
	 * Close socket write end.
	 * @return {bool}
	 */
	bool shutdown_write();

	/**
	 * Close socket read and write ends.
	 * @return {bool}
	 */
	bool shutdown_readwrite();

	/**
	 * Get underlying socket connection handle.
	 * @return {ACL_SOCKET} Returns -1 (UNIX platform)
	 *  or INVALID_SOCKET (win32 platform) if not connected.
	 */
#if defined(_WIN32) || defined(_WIN64)
	SOCKET sock_handle(void) const;
#else
	int   sock_handle() const;
#endif

	/**
	 * Unbind socket handle binding relationship, and return socket handle to user.
	 * Ownership of socket handle is transferred to user. When user destroys and
	 * releases it, it will close this
	 * socket handle. After user takes over this socket handle, it should not be
	 * closed
	 * again. Note: See close/open documentation for details. Other calls (such as
	 * read/write operations) are not affected.
	 * @return {ACL_SOCKET} Returns ACL_SOCKET_INVALID to indicate socket handle
	 *  has been unbound.
	 */
#if defined(_WIN32) || defined(_WIN64)
	SOCKET unbind_sock(void);
#else
	int    unbind_sock();
#endif

	/**
	 * Get socket address family.
	 * @return {int} Return values include: AF_INET, AF_INT6, AF_UNIX. Returns -1
	 * on error.
	 */
	int sock_type() const;

	/**
	 * Get remote connection address.
	 * @param full {bool} Whether to return full address format (IP:PORT). If this
	 * parameter
	 *  is false, only IP is returned. Otherwise, IP:PORT is returned.
	 * @return {const char*} Remote connection address. If return value == '\0', it
	 * means
	 *  unable to get remote connection address.
	 */
	const char* get_peer(bool full = false) const;

	/**
	 * Get remote connection IP address.
	 * @return {const char*} Remote connection address. If return value == '\0', it
	 * means
	 *  unable to get remote connection address.
	 */
	const char* get_peer_ip() const;

	/**
	 * Set remote connection object address. For TCP transmission mode, there is no
	 * need to call this function
	 * to set remote object address. For UDP transmission mode, you need to call
	 * this function to set remote address before
	 * you can read/write to remote.
	 * @param addr {const char*} Remote connection object address, format: ip:port
	 * @return {bool} Returns false when connection is not opened.
	 */
	bool set_peer(const char* addr);

	/**
	 * Get local address of connection.
	 * @param full {bool} Whether to return full address format (IP:PORT). If this
	 * parameter
	 *  is false, only IP is returned. Otherwise, IP:PORT is returned.
	 * @return {const char*} Local address of connection. If return value == "", it
	 * means
	 *  unable to get local address.
	 */
	const char* get_local(bool full = false) const;

	/**
	 * Get local IP address of connection.
	 * @return {const char*} Local address of connection. If return value == "", it
	 * means
	 *  unable to get local address.
	 */
	const char* get_local_ip() const;

	/**
	 * Set local address.
	 * @param addr {const char*} Address, format: ip:port
	 * @return {bool} Returns false when connection is not opened.
	 */
	bool set_local(const char* addr);

	/**
	 * Detect socket connection status (internally uses ping-like method for
	 * detection).
	 * @param tc1 {double*} When not empty, records first stage timeout (ms).
	 * @param tc2 {double*} When not empty, records second stage timeout (ms).
	 * @return {bool} This function returns false when connection is not opened or
	 * already closed. Otherwise
	 *  returns true.
	 */
	bool alive(double* tc1 = NULL, double* tc2 = NULL) const;

	/**
	 * Set TCP socket nodelay option.
	 * @param on {bool} true means enable, false means disable.
	 * @return {socket_stream&}
	 */
	socket_stream& set_tcp_nodelay(bool on);

	/**
	 * Set TCP socket SO_LINGER option.
	 * @param on {bool} Whether to enable SO_LINGER option.
	 * @param linger {int} When SO_LINGER is enabled, timeout to cancel timed_wait
	 * time, unit is seconds.
	 * @return {socket_stream&}
	 */
	socket_stream& set_tcp_solinger(bool on, int linger);

	/**
	 * Set TCP socket send buffer size.
	 * @param size {int} Buffer size to be set.
	 * @return {socket_stream&}
	 */
	socket_stream& set_tcp_sendbuf(int size);

	/**
	 * Set TCP socket receive buffer size.
	 * @param size {int} Buffer size to be set.
	 * @return {socket_stream&}
	 */
	socket_stream& set_tcp_recvbuf(int size);

	/**
	 * Set TCP socket non-blocking state.
	 * @param on {bool} Whether to set as non-blocking state. When true,
	 * socket handle will be set to non-blocking state, otherwise to blocking
	 * state.
	 * @return {socket_stream&}
	 */
	socket_stream& set_tcp_non_blocking(bool on);

	/**
	 * Get whether TCP socket has nodelay option enabled.
	 * @return {bool} true means enabled, false means disabled.
	 */
	bool get_tcp_nodelay() const;

	/**
	 * Get TCP socket linger value.
	 * @return {int} Returns -1 to indicate SO_LINGER option is not set. Otherwise
	 * returns >= 0
	 * to indicate SO_LINGER option is set and this value represents the timeout
	 * (seconds) for TCP connection
	 *  to maintain TIME_WAIT state after socket is closed.
	 */
	int get_tcp_solinger() const;

	/**
	 * Get TCP socket send buffer size.
	 * @return {int} Buffer size.
	 */
	int get_tcp_sendbuf() const;

	/**
	 * Get TCP socket receive buffer size.
	 * @return {int} Buffer size.
	 */
	int get_tcp_recvbuf() const;

	/**
	 * Determine whether current socket is in non-blocking mode.
	 * @return {bool}
	 * Note: This method currently only supports UNIX platform.
	 */
	bool get_tcp_non_blocking() const;

public:
	bool set_zerocopy(bool yes);
	bool wait_iocp(int ms) const;

private:
	std::string ipbuf_;
	const char* get_ip(const char* addr, std::string& out);
};

} // namespace acl

