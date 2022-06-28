#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/string.hpp"

namespace acl {

class ACL_CPP_API http_utils
{
public:
	http_utils() {}
	~http_utils() {}

	/**
	 * 从完整的 url 中获得 WEB 服务器地址，格式：domain:port
	 * @param url {const char*} HTTP url，非空
	 * @param addr {char*} 存储结果，存储格式：domain:port
	 * @param size {size_t} out 缓冲区大小
	 * @return {bool} 是否成功获得
	 */
	static bool get_addr(const char* url, char* addr, size_t size);

	/**
	 * 从完整的 url 中获得 WEB 服务器 IP 地址及端口号
	 * @param url {const char*} HTTP url，非空
	 * @param domain {char*} 存储域名
	 * @param size {size_t} domain 内存大小
	 * @param port {unsigned short*} 存储端口号大小
	 * @return {bool} 是否成功获得
	 */
	static bool get_addr(const char* url, char* domain, size_t size,
		unsigned short* port);
};

class ACL_CPP_API http_url {
public:
	http_url(void);
	~http_url(void) {}

	bool parse(const char* url);

public:
	/**
	 * 返回 URL 中的协议类型：http 或 https
	 * @return {const char*}
	 */
	const char* get_proto(void) const {
		return proto_;
	}

	/**
	 * 返回 URL 中的域名字段
	 * @return {const char*} 返回空串则表示没有该字段
	 */
	const char* get_domain(void) const {
		return domain_.c_str();
	}

	/**
	 * 返回根据 URL 提取的 HTTP 协议服务端端口号，内部缺省值为 80
	 * @return {unsigned short}
	 */
	unsigned short get_port(void) const {
		return port_;
	}

	/**
	 * 返回根据 URL 提取的相对路径部分（不含 ? 后面的参数）
	 * @return {const char*}
	 */
	const char* get_url_path(void) const {
		return url_path_.c_str();
	}

	/**
	 * 返回从 URL 中提取的参数字段
	 * @return {const char*}
	 */
	const char* get_url_params(void) const {
		return url_params_.c_str();
	}

	/**
	 * 清理解析过程中的中间状态，以便重复使用该类对象解析下一个 URL
	 */
	void reset(void);

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
