#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/string.hpp"
#include "stream_hook.hpp"

struct ACL_VSTREAM;

namespace acl {

class sslbase_conf;
class atomic_long;

class ACL_CPP_API sslbase_io : public stream_hook {
public:
	/**
	 * Constructor
	 * @param conf {sslbase_conf&} Class object for configuring each SSL connection
	 * @param server_side {bool} Whether it is server mode. Because handshake methods differ between client mode and server
	 *  mode, this parameter is used to distinguish them
	 * @param nblock {bool} Whether it is non-blocking mode
	 */
	sslbase_io(sslbase_conf& conf, bool server_side, bool nblock = false);
	virtual ~sslbase_io();

	/**
	 * SSL handshake pure virtual method
	 * @return {bool}
	 */
	virtual bool handshake() = 0;

	/**
	 * Get SSL version of current connection. Definition reference see sslbase_conf.hpp.
	 * @return 0 indicates cannot obtain
	 */
	virtual int get_version() const {
		return 0;
	}

	/**
	 * Get SSL version of current connection, represented as string
	 * @return {const char*} Returns non-empty version string
	 */
	const char* get_version_s() const;

	/**
	 * Set socket to blocking mode/non-blocking mode
	 * @param yes {bool} When false, set to blocking mode, otherwise set to non-blocking mode
	 */
	void set_non_blocking(bool yes);

	/**
	 * Determine whether the currently set SSL IO is blocking mode or non-blocking mode
	 * @return {bool} Returns true indicates non-blocking mode, otherwise blocking mode
	 */
	bool is_non_blocking() const {
		return nblock_;
	}

	/**
	 * Determine whether SSL handshake was successful
	 * @return {bool}
	 */
	bool handshake_ok() const {
		return handshake_ok_;
	}

	/**
	 * Client uses this to set SNI HOST field
	 * @param host {const char*}
	 */
	void set_sni_host(const char* host, const char* prefix = NULL,
		const char* suffix = NULL);

	/**
	 * Server sets whether client sent SNI information
	 * @param yes {bool}
	 */
	void set_has_sni(bool yes);

	/**
	 * Server determines whether client sent SNI information
	 * @return {bool}
	 */
	bool has_sni() const {
		return has_sni_;
	}

	/**
	 * Set binding object for this SSL IO object, convenient for applications to handle their own business logic
	 * @param ctx {void*}
	 */
	void set_ctx(void* ctx);

	/**
	 * Get binding object set by set_ctx()
	 * @return {void*}
	 */
	void* get_ctx() const {
		return ctx_;
	}

	/**
	 * Get passed SSL configuration item
	 * @return {sslbase_conf&}
	 */
	sslbase_conf& get_conf() {
		return base_conf_;
	}

protected:
	sslbase_conf& base_conf_;
	bool server_side_;
	bool nblock_;
	bool handshake_ok_;
	atomic_long* refers_;
	ACL_VSTREAM* stream_;
	string sni_host_;	// Just for client to set SNI.
	bool has_sni_;		// Just for server to check SNI.
	void* ctx_;		// The context for every SSL IO.
};

} // namespace acl

