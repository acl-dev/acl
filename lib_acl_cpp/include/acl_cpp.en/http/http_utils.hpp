#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/string.hpp"

namespace acl {

class ACL_CPP_API http_utils {
public:
	http_utils() {}
	~http_utils() {}

	/**
	 * Get WEB server address from complete url, format: domain:port
	 * @param url {const char*} HTTP url, non-empty
	 * @param addr {char*} Store result, storage format: domain:port
	 * @param size {size_t} out buffer size
	 * @return {bool} Whether successfully obtained
	 */
	static bool get_addr(const char* url, char* addr, size_t size);

	/**
	 * Get WEB server IP address and port number from complete url
	 * @param url {const char*} HTTP url, non-empty
	 * @param domain {char*} Store domain name
	 * @param size {size_t} domain memory size
	 * @param port {unsigned short*} Store port number size
	 * @return {bool} Whether successfully obtained
	 */
	static bool get_addr(const char* url, char* domain, size_t size,
		unsigned short* port);
};

class ACL_CPP_API http_url {
public:
	http_url();
	~http_url() {}

	bool parse(const char* url);

public:
	/**
	 * Return protocol type in URL: http or https
	 * @return {const char*}
	 */
	const char* get_proto() const {
		return proto_;
	}

	/**
	 * Return domain name field in URL
	 * @return {const char*} Returns empty string if this field does not exist
	 */
	const char* get_domain() const {
		return domain_.c_str();
	}

	/**
	 * Return HTTP protocol server port number extracted from URL, internal default
	 * value is 80
	 * @return {unsigned short}
	 */
	unsigned short get_port() const {
		return port_;
	}

	/**
	 * Return relative path part extracted from URL (excluding parameters after ?)
	 * @return {const char*}
	 */
	const char* get_url_path() const {
		return url_path_.c_str();
	}

	/**
	 * Return parameter field extracted from URL
	 * @return {const char*}
	 */
	const char* get_url_params() const {
		return url_params_.c_str();
	}

	/**
	 * Clear intermediate state during parsing process, so that this class object
	 * can be reused to parse the next URL
	 */
	void reset();

private:
	char proto_[16];
	string domain_;
	unsigned short port_;
	string url_path_;
	string url_params_;

	bool parse_url_part(const char* url);
	const char* parse_domain(const char* url);
};

} // namespace acl

