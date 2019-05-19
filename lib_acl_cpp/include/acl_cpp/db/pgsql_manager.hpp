#pragma once
#include "../acl_cpp_define.hpp"
#include <map>
#include "../stdlib/string.hpp"
#include "../connpool/connect_manager.hpp"

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_DB_DISABLE)

namespace acl {

class pgsql_conf;

class ACL_CPP_API pgsql_manager : public connect_manager
{
public:
	pgsql_manager(time_t idle_ttl = 120);
	~pgsql_manager();

	/**
	 * 添加一个数据库实例方法二
	 * @param conf {const pgsql_conf&}
	 * @return {pgsql_manager&}
	 */
	pgsql_manager& add(const pgsql_conf& conf);

protected:
	/**
	 * 基类 connect_manager 虚函数的实现
	 * @param addr {const char*} 服务器监听地址，格式：ip:port
	 * @param count {size_t} 连接池的大小限制，该值为 0 时则没有限制
	 * @param idx {size_t} 该连接池对象在集合中的下标位置(从 0 开始)
	 * @return {connect_pool*} 返回创建的连接池对象
	 */
	connect_pool* create_pool(const char* addr, size_t count, size_t idx);

private:
	time_t idle_ttl_;       // 数据库连接的空闲过期时间
	std::map<string, pgsql_conf*> dbs_;
};

} // namespace acl

#endif // !defined(ACL_CLIENT_ONLY) && !defined(ACL_DB_DISABLE)
