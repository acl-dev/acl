#ifndef ACL_VSTREAM_NET_INCLUDE_H
#define ACL_VSTREAM_NET_INCLUDE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../stdlib/acl_define.h"
#include "../stdlib/acl_vstream.h"

/**
 * Listen on a certain address, create UNIX domain socket and listening socket:
 * @param addr {const char*} Listening address,
 *  e.g., 127.0.0.1:80 for TCP socket (UNIX platform) or
 *  /tmp/test.sock for UNIX domain socket. On Linux platform, if
 *  addr's first character is '@', it is treated as abstract unix
 *  domain path.
 * @param qlen {int} Connection queue length
 * @param flag {unsigned} Listening flag bits, see ACL_INET_FLAG_XXX
 * @param io_bufsize {int} IO buffer size for newly accepted client socket
 * @param rw_timeout {int} IO read/write timeout for newly
 *  accepted client socket, unit is seconds
 * @return {ACL_VSTREAM*} Listening stream pointer
 */
ACL_API ACL_VSTREAM *acl_vstream_listen_ex(const char *addr, int qlen,
		unsigned flag, int io_bufsize, int rw_timeout);

/**
 * Listen on a certain address, create UNIX domain socket and listening socket:
 * @param addr {const char*} Listening address
 *  e.g., 127.0.0.1:80 for TCP socket, e.g., /tmp/test.sock for
 *  UNIX domain socket. When address is ip:0, system can
 *  automatically assign port number
 * @param qlen {int} Connection queue length
 * @return {ACL_VSTREAM*} Listening stream pointer
 */
ACL_API ACL_VSTREAM *acl_vstream_listen(const char *addr, int qlen);

/**
 * Accept a client connection from listening stream.
 * @param listen_stream {ACL_VSTREAM*} Listening stream
 * @param client_stream {ACL_VSTREAM*} Pre-allocated ACL_VSTREAM
 *  structure object. If NULL, internally creates a new
 *  ACL_VSTREAM object and uses this structure space
 * @param ipbuf {char*} If not NULL, stores client's IP address
 * @param bsize {int} If ipbuf is not NULL, indicates ipbuf's space size
 * @return {ACL_VSTREAM*} If not NULL, indicates newly accepted
 *  client connection
 */
ACL_API ACL_VSTREAM *acl_vstream_accept_ex(ACL_VSTREAM *listen_stream,
		ACL_VSTREAM *client_stream, char *ipbuf, int bsize);

/**
 * Accept a client connection from listening stream.
 * @param listen_stream {ACL_VSTREAM*} Listening stream
 * @param ipbuf {char*} If not NULL, stores client's IP address
 * @param bsize {int} If ipbuf is not NULL, indicates ipbuf's space size
 * @return {ACL_VSTREAM*} If not NULL, indicates newly accepted
 *  client connection
 */
ACL_API ACL_VSTREAM *acl_vstream_accept(ACL_VSTREAM *listen_stream,
		char *ipbuf, int bsize);

/**
 * Remote connection function.
 * @param addr {const char*} Connection address, if it is a UNIX domain socket
 *  (on UNIX platform), socket address is /tmp/test.sock. On Linux platform,
 *  if first character is '@', it indicates abstract UNIX domain socket
 *  (Linux abstract unix domain socket). If it is a TCP connection, address
 *  format is: remote_addr[@local_ip]|[#interface], e.g.:
 *  1. www.sina.com|80@60.28.250.199, meaning bind local address to:
 *     60.28.250.199, connect remotely to www.sina.com port 80;
 *  2.  211.150.111.12|80@192.168.1.1 indicates bind local address;
 *  3. 211.150.111.12|80#eth0 indicates bind to specified interface;
 *  4. Let OS automatically bind local IP address, can be written as: www.sina.com|80;
 * @param block_mode {int} Whether connection is blocking or
 *  non-blocking: ACL_BLOCKING, ACL_NON_BLOCKING
 * @param conn_timeout {int} Connection timeout (seconds)
 * @param rw_timeout {int} Read/write timeout after connection
 *  succeeds, unit is seconds
 * @param bufsize {int} Buffer size after connection succeeds
 * @param flags {unsigned*} If not NULL, receives detailed error
 *  information on connection failure, connection's error flag
 *  bits
 * @return {ACL_VSTREAM*} If not NULL, indicates connection
 *  successful, otherwise failed
 */
ACL_API ACL_VSTREAM *acl_vstream_connect2(const char *addr, int block_mode,
	int conn_timeout, int rw_timeout, int bufsize, unsigned *flags);
#define acl_vstream_connect_ex	acl_vstream_connect2

/**
 * Remote connection function.
 * @param addr {const char*} Connection address, same as above
 * @param block_mode {int} Whether connection is blocking or
 *  non-blocking: ACL_BLOCKING, ACL_NON_BLOCKING
 * @param connect_timeout {int} Connection timeout (seconds)
 * @param rw_timeout {int} Read/write timeout after connection
 *  succeeds, unit is seconds
 * @param rw_bufsize {int} Buffer size after connection succeeds
 * @return {ACL_VSTREAM*} If not NULL, indicates connection
 *  successful, otherwise failed
 */
ACL_API ACL_VSTREAM *acl_vstream_connect(const char *addr, int block_mode,
	int connect_timeout, int rw_timeout, int rw_bufsize);

/**
 * Remote connection function.
 * @param addr {const char*} Connection address, same as above
 * @param block_mode {int} Whether connection is blocking or
 *  non-blocking: ACL_BLOCKING, ACL_NON_BLOCKING
 * @param connect_timeout {int} Connection timeout (milliseconds)
 * @param rw_timeout {int} Read/write timeout after connection
 *  succeeds, unit is milliseconds
 * @param rw_bufsize {int} Buffer size after connection succeeds
 * @param flags {unsigned*} If not NULL, receives detailed error
 *  information on connection failure, connection's error flag
 *  bits
 * @return {ACL_VSTREAM*} If not NULL, indicates connection
 *  successful, otherwise failed
 */
ACL_API ACL_VSTREAM *acl_vstream_timed_connect(const char *addr, int block_mode,
	int connect_timeout, int rw_timeout, int rw_bufsize, unsigned *flags);

/**
 * Bind UDP communication, this function binds local UDP address.
 * After binding succeeds, creates ACL_VSTREAM object, user can
 * use ACL_VSTREAM object's read/write interface.
 * @param addr {const char*} Bind UDP address, format: ip:port. If address is ip:0,
 *  system can automatically assign local port number. Additionally
 *  supports UNIX domain socket on UNIX platform, UNIX domain
 *  socket address format is: {path}@udp, where {path} is socket
 *  path, @udp is UDP suffix
 * @param rw_timeout {int} Read/write timeout (seconds)
 * @param flag {unsigned} Flag bits
 * @return {ACL_VSTREAM*} Returns NULL on failure
 */
ACL_API ACL_VSTREAM *acl_vstream_bind(const char *addr, int rw_timeout, unsigned flag);

/**
 * Set stream to UDP IO mode.
 * @param stream {ACL_VSTREAM*}
 */
ACL_API void acl_vstream_set_udp_io(ACL_VSTREAM *stream);

/**
 * Bind multicast address.
 * @param addr {const char*} Multicast IP address
 * @param iface {const char*} Local IP address for receiving packets
 * @param port {int} Local Port for receiving packets
 * @param timeout {int} IO timeout (seconds)
 * @param flag {unsigned} Flag bits
 * @return {ACL_VSTREAM*} Returns NULL on failure
 */
ACL_API ACL_VSTREAM *acl_vstream_bind_multicast(const char *addr,
	const char *iface, int port, int timeout, unsigned flag);

/**
 * Set multicast TTL option (1--255)
 * @param sock {ACL_SOCKET}
 * @param ttl {int}
 * @return {int} Return -1 indicates error, 0 indicates success
 */
ACL_API int acl_multicast_set_ttl(ACL_SOCKET sock, int ttl);

/**
 * Set multicast bound local address.
 * @param sock {ACL_SOCKET}
 * @param addr {const char*}
 * @return {int} Return 0 indicates success, -1 indicates failure
 */
ACL_API int acl_multicast_set_if(ACL_SOCKET sock, const char *addr);

/**
 * Leave multicast group. After this API succeeds, will no longer
 * receive multicast packets.
 * @param sock {ACL_SOCKET}
 * @param addr {const char*} Multicast IP
 * @param iface {const char*} Interface IP to bind
 * @return {int} Return 0 indicates success, -1 indicates failure
 */
ACL_API int acl_multicast_drop(ACL_SOCKET sock, const char *addr, const char *iface);

#ifdef __cplusplus
}
#endif

#endif
