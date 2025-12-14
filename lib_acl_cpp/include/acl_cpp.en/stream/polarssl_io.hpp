#pragma once
#include "../acl_cpp_define.hpp"
#include "sslbase_io.hpp"

struct ACL_VSTREAM;

namespace acl {

class polarssl_conf;

/**
 * Processing class for the underlying IO processing of stream/aio_stream stream
 * objects. The read/write process in objects of this class will replace
 * the default underlying IO process in stream/aio_stream stream objects.
 * Objects of this class must be dynamically created (i.e., heap objects).
 * stream/aio_stream stream objects release objects of this class by calling the
 * destroy() method of this class object
 */
class ACL_CPP_API polarssl_io : public sslbase_io {
public:
	/**
	 * Constructor
	 * @param conf {polarssl_conf&} Class object for configuring each SSL
	 * connection
	 * @param server_side {bool} Whether it is server mode, because the handshake
	 * methods differ between client mode and server
	 *  mode, this parameter is used to distinguish them
	 * @param nblock {bool} Whether it is non-blocking mode
	 */
	polarssl_io(polarssl_conf& conf, bool server_side, bool nblock = false);

	/**
	 * @override stream_hook
	 * Destroy SSL IO object
	 */
	void destroy();

	/**
	 * @override sslbase_io
	 * Call this method to perform SSL handshake. In non-blocking IO mode, this
	 * function needs to be used together with handshake_ok()
	 * function to determine whether SSL handshake was successful
	 * @return {bool}
	 *  1. Returns false indicates handshake failed, connection needs to be closed;
	 *  2. When returns true:
	 *  2.1. If in blocking IO mode, it indicates SSL handshake was successful
	 * 2.2. In non-blocking IO mode, it only means IO was successful in this
	 * handshake process, still need to call
	 * handshake_ok() function to determine whether SSL handshake was successful
	 */
	bool handshake();

	/**
	 * Check whether the peer certificate is valid (generally no need to call this
	 * function)
	 * @return {bool}
	 */
	bool check_peer();

protected:
	~polarssl_io();

	// Implement virtual methods of stream_hook class

	// @override stream_hook
	bool open(ACL_VSTREAM* s);

	// @override stream_hook
	bool on_close(bool alive);

	// @override stream_hook
	int read(void* buf, size_t len);

	// @override stream_hook
	int send(const void* buf, size_t len);

private:
	polarssl_conf& conf_;
	void* ssl_;
	void* ssn_;
	void* rnd_;

private:
	static int sock_read(void *ctx, unsigned char *buf, size_t len);
	static int sock_send(void *ctx, const unsigned char *buf, size_t len);
};

} // namespace acl

