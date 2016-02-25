#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include <map>
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/session/session.hpp"

namespace acl
{

class redis;
class redis_client_cluster;

class ACL_CPP_API redis_session : public session
{
public:
	redis_session(redis_client_cluster& cluster, size_t max_conns,
		time_t ttl = 0, const char* sid = NULL);
	~redis_session();

	// 基类虚函数，向 redis 服务端设置哈希属性值
	bool set(const char* name, const char* value);

	// 基类虚函数，向 redis 服务端设置哈希属性值
	bool set(const char* name, const void* value, size_t len);

	// 基类虚函数，从 redis 服务端的哈希对象中获得对应属性的值
	const session_string* get_buf(const char* name);

	// 基类虚函数，从 redis 服务端的哈希对象中删除某个属性值
	bool del(const char* name);

	// 基类纯虚函数，从 redis 中删除数据
	bool remove();

	// 基类纯虚函数，从 redis 中获得数据
	bool get_attrs(std::map<string, session_string>& attrs);

	// 基类虚函数，从 redis 中获得数据
	bool get_attrs(const std::vector<string>& names,
		std::vector<session_string>& values);

	// 基类纯虚函数，向 redis 中添加或修改数据
	bool set_attrs(const std::map<string, session_string>& attrs);

protected:
	//重新设置 session 在 redis 上的缓存时间
	bool set_timeout(time_t ttl);

private:
	redis_client_cluster& cluster_;
	redis* command_;
	size_t max_conns_;
	std::map<string, session_string*> buffers_;
};

} // namespace acl
