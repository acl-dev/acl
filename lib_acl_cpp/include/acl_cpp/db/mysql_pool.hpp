#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include "acl_cpp/db/db_pool.hpp"

namespace acl {

class db_handle;

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
	 * @param dblimit {size_t} 数据库连接池的最大连接数限制
	 * @param dbflags {unsigned long} mysql 标记位
	 * @param auto_commit {bool} 是否自动提交
	 * @param conn_timeout {int} 连接数据库超时时间(秒)
	 * @param rw_timeout {int} 与数据库通信时的IO时间(秒)
	 */
	mysql_pool(const char* dbaddr, const char* dbname,
		const char* dbuser, const char* dbpass,
		size_t dblimit = 64, unsigned long dbflags = 0,
		bool auto_commit = true, int conn_timeout = 60,
		int rw_timeout = 60);
	~mysql_pool();

protected:
	// 基类 connect_pool 纯虚函数：创建数据库连接句柄
	connect_client* create_connect();

private:
	char* dbaddr_;  // 数据库监听地址
	char* dbname_;  // 数据库名
	char* dbuser_;  // 数据库账号
	char* dbpass_;  // 数据库账号密码
	unsigned long dbflags_;
	bool  auto_commit_;  // 是否自动提交修改后的数据
	int   conn_timeout_; // 连接数据库的超时时间
	int   rw_timeout_;   // 与数据库通信的超时时间
};

} // namespace acl
