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

	/**
	 * 解析输入的完整或部分 URL
	 * @param url {const char*} 非空完整或部分 URL 字符串
	 * @param domain {string&} 用来存放域名地址信息
	 * @param port {unsigned short*} 用来存放 url 中的端口号
	 * @param url_path {string&} 用来存放 url 中不含域名和参数的部分，即针对类似
	 *  于 URL：http://test.com.cn/cgi-bin/test?name=value 则只提取其中的字
	 *  符串：/cgi-bin/test
	 * @param url_params {string&} 用来存放 url 中的参数部分字符串
	 * @return {bool} 解析 url 是否成功
	 */
	static bool parse_url(const char* url, string& domain,
		unsigned short* port, string& url_path, string& url_params);
};

class ACL_CPP_API http_url {
public:
	http_url(void);
	~http_url(void) {}

	bool parse(const char* url);

public:
	const char* get_proto(void) const {
		return proto_;
	}

	const char* get_domain(void) const {
		return domain_.c_str();
	}

	unsigned short get_port(void) const {
		return port_;
	}

	const char* get_url_path(void) const {
		return url_path_.c_str();
	}

	const char* get_url_params(void) const {
		return url_params_.c_str();
	}

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
