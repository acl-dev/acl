#pragma once
#include "../acl_cpp_define.hpp"
#include "session.hpp"

#ifndef ACL_CLIENT_ONLY

namespace acl {

class memcache;

/**
 * session 类，该类使用 memcached 存储 session 数据
 */
class ACL_CPP_API memcache_session : public session
{
public:
	/**
	 * 构造函数
	 * @param cache_addr {const char*} memcached 服务地址，格式：
	 *  IP:PORT，不能为空
	 * @param prefix {const char*} 在 memcached 存储的键值的前缀
	 * @param conn_timeout {int} 连接 memcached 的超时时间(秒)
	 * @param rw_timeout {int} 与 memcached 通讯的 IO 超时时间(秒)
	 * @param ttl {time_t} 生存周期(秒)
	 * @param sid {const char*} session 对应的 sid，当为空时，内部
	 *  会自动生成一个，其它说明请参考基类 session 的说明
	 * @param encode_key {bool} 是否对存储于 memcached 的键值进行编码
	 */
	memcache_session(const char* cache_addr, int conn_timeout = 180,
		int rw_timeout = 300, const char* prefix = NULL,
		time_t ttl = 0, const char* sid = NULL, bool encode_key = true);

	/**
	 * 以输入的 memcached 的连接对象为参数的构造函数
	 * @param cache {memcache*} 输入的 memcached 连接对象
	 * @param auto_free {bool} 当该参数为 true 时，则要求该
	 *  memcached_session 对象析构函数中释放传入的 cache 对象；
	 *  否则则禁止在 memcached_session 的析构函数中释放 cache 对象
	 * @param ttl {time_t} 生存周期(秒)
	 * @param sid {const char*} session 对应的 sid，当为空时，内部
	 *  会自动生成一个，其它说明请参考基类 session 的说明
	 */
	memcache_session(memcache* cache, bool auto_free = false,
		time_t ttl = 0, const char* sid = NULL);

	~memcache_session(void);

	// 基类纯虚函数，从 memcached 中获得数据
	bool get_attrs(std::map<string, session_string>& attrs);

	// 基类纯虚函数，向 memcached 中添加或修改数据
	bool set_attrs(const std::map<string, session_string>& attrs);

	// 基类纯虚函数，从 memcached 中删除数据
	bool remove();

protected:
	//重新设置 session 在 memcached 上的缓存时间
	bool set_timeout(time_t ttl);

private:
	memcache* cache_;
	bool auto_free_;
};

} // namespace acl

#endif // ACL_CLIENT_ONLY
