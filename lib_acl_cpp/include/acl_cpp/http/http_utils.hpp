#pragma once
#include "acl_cpp/acl_cpp_define.hpp"

namespace acl {

class ACL_CPP_API http_utils
{
public:
	http_utils() {}
	~http_utils() {}

	/**
	 * 从完整的 url 中获得 WEB 服务器地址，格式：ip/domain:port
	 * @param url {const char*} HTTP url，非空
	 * @param out {char*} 存储结果
	 * @param size {size_t} out 缓冲区大小
	 * @return {bool} 是否成功获得
	 */
	static bool get_addr(const char* url, char* out, size_t size);
};

} // namespace acl
