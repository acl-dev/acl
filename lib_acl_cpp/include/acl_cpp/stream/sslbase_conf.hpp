#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/noncopyable.hpp"

namespace acl {

class string;
class sslbase_io;

class ACL_CPP_API ssl_sni_checker {
public:
	ssl_sni_checker() {}
	virtual ~ssl_sni_checker() {}

	/**
	 * Virtual method used to check whether input sni host is valid. Subclasses
	 * must implement
	 * @param sni {const char*} SNI field sent by client
	 * @param host {acl::string&} Host field extracted from sni
	 * @return {bool} Whether check is valid
	 */
	virtual bool check(sslbase_io* io, const char* sni, string& host) = 0;
};

enum {
	ssl_ver_def,  // Don't set the SSL version.
	ssl_ver_3_0,  // Not support.
	tls_ver_1_0,  // Not support.
	tls_ver_1_1,  // Not support.
	tls_ver_1_2,
	tls_ver_1_3,
};

class ACL_CPP_API sslbase_conf : public noncopyable {
public:
	sslbase_conf() : checker_(NULL), ver_min_(ssl_ver_def), ver_max_(ssl_ver_def) {}
	virtual ~sslbase_conf() {}

	/**
	 * Pure virtual method, create SSL IO object
	 * @param nblock {bool} Whether it is non-blocking mode
	 * @return {sslbase_io*}
	 */
	virtual sslbase_io* create(bool nblock) = 0;

	/**
 	 * Set SSL/TLS version
 	 * @param ver_min {int} Minimum version number, internal default value is
 	 * tls_ver_1_2
 	 * @param ver_max {int} Maximum version number, internal default value is
 	 * tls_ver_1_3
 	 * @return {bool} Returns true indicates setting was successful, otherwise
 	 * indicates version setting is not supported
	 */
	virtual bool set_version(int ver_min, int ver_max) {
		(void) ver_min;
		(void) ver_max;
		return false;
	}

	/**
	 * Get set SSL version number
	 * @param ver_min {int&} Used to store minimum version number
	 * @param ver_max {int&} Used to store maximum version number
	 */
	void get_version(int& ver_min, int& ver_max) {
		ver_min = ver_min_;
		ver_max = ver_max_;
	}

	/**
	 * Convert version number to string
	 * @param v {int}
	 * @return {const char*}
	 */
	static const char* version_s(int v) {
		switch (v) {
		case ssl_ver_3_0:
			return "ssl_ver_3_0";
		case tls_ver_1_0:
			return "tls_ver_1_0";
		case tls_ver_1_1:
			return "tls_ver_1_1";
		case tls_ver_1_2:
			return "tls_ver_1_2";
		case tls_ver_1_3:
			return "tls_ver_1_3";
		default:
			return "ssl_ver_def";
		}
	}
//public:
	/**
	 * Load CA root certificate (each configuration instance only needs to call
	 * this method once)
	 * @param ca_file {const char*} Full path of CA certificate file
	 * @param ca_path {const char*} Directory where multiple CA certificate files
	 * are located
	 * @return {bool} Whether loading CA root certificate was successful
	 * Note: If both ca_file and ca_path are non-empty, will load all certificates
	 * in sequence
	 */
	virtual bool load_ca(const char* ca_file, const char* ca_path) {
		(void) ca_file;
		(void) ca_path;
		return false;
	}

	/**
	 * Add a server/client's own certificate. Can call this method multiple times
	 * to load multiple certificates
	 * @param crt_file {const char*} Full path of certificate file, non-empty
	 * @param key_file {const char*} Full path of key file, non-empty
	 * @param key_pass {const char*} Password of key file. Can write NULL if there
	 * is no key password
	 * @return {bool} Whether adding certificate was successful
	 */
	virtual bool add_cert(const char* crt_file, const char* key_file,
		const char* key_pass) {
		(void) crt_file;
		(void) key_file;
		(void) key_pass;
		return false;
	}

	// Only for compatibility with old API
	bool add_cert(const char* crt_file, const char* key_file) {
		return add_cert(crt_file, key_file, NULL);
	}

	/**
	 * Add a server/client's own certificate. Can call this method multiple times
	 * to load multiple certificates
	 * @param crt_file {const char*} Full path of certificate file, non-empty
	 * @return {bool} Whether adding certificate was successful
	 * @deprecated use add_cert(const char*, const char*, const char*)
	 */
	virtual bool add_cert(const char* crt_file) {
		(void) crt_file;
		return false;
	}

	/**
	 * Add server/client's key (each configuration instance only needs to call this
	 * method once)
	 * @param key_file {const char*} Full path of key file, non-empty
	 * @param key_pass {const char*} Password of key file. Can write NULL if there
	 * is no key password
	 * @return {bool} Whether setting was successful
	 * @deprecated use add_cert(const char*, const char*, const char*)
	 */
	virtual bool set_key(const char* key_file, const char* key_pass) {
		(void) key_file;
		(void) key_pass;
		return false;
	}

	// Only for compatibility with old API
	bool set_key(const char* key_file) {
		return set_key(key_file, NULL);
	}

	/**
	 * When in server mode, whether to enable session cache function, helps improve
	 * SSL handshake efficiency
	 * @param on {bool}
	 * Note: This function is only effective for server mode
	 */
	virtual void enable_cache(bool on) {
		(void) on;
	}

	/**
	 * Set SNI checker class object sent by client
	 * @param checker {ssl_sni_checker*}
	 */
	void set_sni_checker(ssl_sni_checker* checker) {
		checker_ = checker;
	}

	/**
	 * Get set SNI checker object
	 * @return {ssl_sni_checker*}
	 */
	ssl_sni_checker* get_sni_checker() const {
		return checker_;
	}

protected:
	ssl_sni_checker* checker_;
	int ver_min_;
	int ver_max_;
};

} // namespace acl

