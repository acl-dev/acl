#pragma once
#include "../acl_cpp_define.hpp"
#include "../db/db_pool.hpp"

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_DB_DISABLE)

namespace acl {

class db_handle;
class mysql_conf;

class ACL_CPP_API mysql_pool : public db_pool
{
public:
	/**
	 * 采用 mysql 数据库时的构造函数
	 * @param dbaddr {const char*} mysql 服务器地址，格式：IP:PORT，
	 *  在 UNIX 平台下可以为 UNIX 域套接口
	 * @param dbname {const char*} 数据库名
	 * @param dbuser {const char*} 数据库用户
	 * @param dbpass {const char*} 数据库用户密码
	 * @param dblimit {int} 数据库连接池的最大连接数限制
	 * @param dbflags {unsigned long} mysql 标记位
	 * @param auto_commit {bool} 是否自动提交
	 * @param conn_timeout {int} 连接数据库超时时间(秒)
	 * @param rw_timeout {int} 与数据库通信时的IO时间(秒)
	 * @param charset {const char*} 连接数据库的字符集(utf8, gbk, ...)
	 */
	mysql_pool(const char* dbaddr, const char* dbname,
		const char* dbuser, const char* dbpass,
		int dblimit = 64, unsigned long dbflags = 0,
		bool auto_commit = true, int conn_timeout = 60,
		int rw_timeout = 60, const char* charset = "utf8");

	/**
	 * 构造函数
	 * @param conf {const mysql_conf&} mysql 数据库连接配置对象
	 */
	mysql_pool(const mysql_conf& conf);
	~mysql_pool();

protected:
	// 基类 connect_pool 纯虚函数：创建数据库连接句柄
	connect_client* create_connect();

private:
	mysql_conf* conf_;
};

} // namespace acl

#endif // !defined(ACL_CLIENT_ONLY) && !defined(ACL_DB_DISABLE)
