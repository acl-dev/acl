#pragma once
#include "../acl_cpp_define.hpp"

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

} // namespace acl
