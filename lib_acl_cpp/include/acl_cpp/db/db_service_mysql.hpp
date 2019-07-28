#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/string.hpp"
#include "../db/db_service.hpp"

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_DB_DISABLE)

namespace acl {

class ACL_CPP_API db_service_mysql : public db_service
{
	/**
	 * 当为 mysql 数据库时的构造函数
	 * @param dbaddr {const char*} mysql 服务器地址
	 * @param dbname {const char*} 数据库名
	 * @param dbuser {const char*} 数据库用户名
	 * @param dbpass {const char*} 数据库用户密码
	 * @param dbflags {unsigned long} 数据库连接标志位
	 * @param auto_commit {bool} 数据修改时是否自动提交
	 * @param conn_timeout {int} 数据库连接超时时间
	 * @param rw_timeout {int} 数据库操作时IO读写超时时间
	 * @param dblimit {size_t} 数据库连接池的个数限制
	 * @param nthread {int} 子线程池的最大线程数
	 * @param win32_gui {bool} 是否是窗口类的消息，如果是，则内部的
	 *  通讯模式自动设置为基于 _WIN32 的消息，否则依然采用通用的套接
	 *  口通讯方式
	 */
	db_service_mysql(const char* dbaddr, const char* dbname,
		const char* dbuser, const char* dbpass,
		unsigned long dbflags = 0, bool auto_commit = true,
		int conn_timeout = 60, int rw_timeout = 60,
		size_t dblimit = 100, int nthread = 2, bool win32_gui = false);

	~db_service_mysql(void);

private:
	// 数据库服务器地址
	string dbaddr_;
	// 数据库名
	string dbname_;
	// 数据库用户名
	string dbuser_;
	// 数据库用户密码
	string dbpass_;
	// 数据库连接标志位
	unsigned long dbflags_;
	// 修改数据时是否自动提交数据
	bool auto_commit_;
	// 连接数据库超时时间
	int conn_timeout_;
	// 数据库操作时的读写超时时间
	int rw_timeout_;

	// 基类纯虚函数
	virtual db_handle* db_create(void);
};

}

#endif // !defined(ACL_CLIENT_ONLY) && !defined(ACL_DB_DISABLE)
