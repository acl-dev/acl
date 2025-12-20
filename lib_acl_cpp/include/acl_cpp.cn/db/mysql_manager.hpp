#pragma once
#include "../acl_cpp_define.hpp"
#include <map>
#include "../stdlib/string.hpp"
#include "../connpool/connect_manager.hpp"

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_DB_DISABLE)

namespace acl {

class mysql_conf;

class ACL_CPP_API mysql_manager : public connect_manager
{
public:
	mysql_manager(time_t idle_ttl = 120);
	~mysql_manager();

	/**
	 * 添加一个数据库实例方法一
	 * @param dbaddr {const char*} mysql 服务器地址，格式：IP:PORT，
	 *  在 UNIX 平台下可以为 UNIX 域套接口
	 * @param dbname {const char*} 数据库名
	 * @param dbuser {const char*} 数据库用户
	 * @param dbpass {const char*} 数据库用户密码
	 * @param dblimit {size_t} 数据库连接池的最大连接数限制
	 * @param dbflags {unsigned long} mysql 标记位
	 * @param auto_commit {bool} 是否自动提交
	 * @param conn_timeout {int} 连接数据库超时时间(秒)
	 * @param rw_timeout {int} 与数据库通信时的IO时间(秒)
	 * @param charset {const char*} 连接数据库时的字符集
	 * @return {mysql_manager&}
	 */
	mysql_manager& add(const char* dbaddr, const char* dbname,
		const char* dbuser, const char* dbpass,
		size_t dblimit = 64, unsigned long dbflags = 0,
		bool auto_commit = true, int conn_timeout = 60,
		int rw_timeout = 60, const char* charset = "utf8");

	/**
	 * 添加一个数据库实例方法二
	 * @param conf {const mysql_conf&}
	 * @return {mysql_manager&}
	 */
	mysql_manager& add(const mysql_conf& conf);

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
	std::map<string, mysql_conf*> dbs_;
};

} // namespace acl

#endif // !defined(ACL_CLIENT_ONLY) && !defined(ACL_DB_DISABLE)
