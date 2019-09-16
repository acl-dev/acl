#pragma once
#include "../acl_cpp_define.hpp"

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_DB_DISABLE)

namespace acl {

class ACL_CPP_API mysql_conf
{
public:
	/**
	 * 构造函数
	 * @param dbaddr {const char*} 数据库连接地址，可以为 TCP 套接口或
	 *  UNIX 域套接口，当为 TCP 套接口时，地址格式为：ip:port, 当为 UNIX
	 *  域套接口时，地址格式：/xxx/xxx/xxx.sock
	 * @param dbname {const char*} 数据库名
	 */
	mysql_conf(const char* dbaddr, const char* dbname);

	/**
	 * 拷贝构造函数
	 * @param conf {const mysql_conf&} 内部创建新对象同时拷贝输入对象内容
	 */
	mysql_conf(const mysql_conf& conf);

	~mysql_conf();

	/**
	 * 设置连接数据库时的用户账号
	 * @param dbuser {const char*} 当为非空字符串时指定用户账号
	 * @return {mysql_conf&}
	 */
	mysql_conf& set_dbuser(const char* dbuser);

	/**
	 * 设置连接数据库时的账号密码
	 * @param dbpass {const char*} 当为非空字符串时指定账号密码
	 * @return {mysql_conf&}
	 */
	mysql_conf& set_dbpass(const char* dbpass);

	/**
	 * 设置数据库连接池最大连接上限
	 * @param dblimit {size_t} 连接池最大连接数限制，当为 0 时则不限制
	 * @return {mysql_conf&}
	 */
	mysql_conf& set_dblimit(size_t dblimit);

	/**
	 * 设置 mysql 数据库的一些特殊标志位
	 * @param dbflags {unsigned long}
	 * @return {mysql_conf&}
	 */
	mysql_conf& set_dbflags(unsigned long dbflags);

	/**
	 * 设置当修改数据库内容时是否允许自动提交，默认为自动提交
	 * @param on {bool}
	 * @return {mysql_conf&}
	 */
	mysql_conf& set_auto_commit(bool on);

	/**
	 * 设置连接数据库的超时时间
	 * @param timeout {int}
	 * @return {mysql_conf&}
	 */
	mysql_conf& set_conn_timeout(int timeout);

	/**
	 * 设置读取数据库结果的超时时间
	 * @param timeout {int}
	 * @return {mysql_conf&}
	 */
	mysql_conf& set_rw_timeout(int timeout);

	/**
	 * 设置数据库连接的字符集
	 * @param charset {const char*}
	 * @return {mysql_conf&}
	 */
	mysql_conf& set_charset(const char* charset);

	const char* get_dbaddr() const
	{
		return dbaddr_;
	}

	const char* get_dbname() const
	{
		return dbname_;
	}

	const char* get_dbkey() const
	{
		return dbkey_;
	}

	const char* get_dbuser() const
	{
		return dbuser_;
	}

	const char* get_dbpass() const
	{
		return dbpass_;
	}

	size_t get_dblimit() const
	{
		return dblimit_;
	}

	unsigned long get_dbflags() const
	{
		return dbflags_;
	}

	bool get_auto_commit() const
	{
		return auto_commit_;
	}

	int get_conn_timeout() const
	{
		return conn_timeout_;
	}

	int get_rw_timeout() const
	{
		return rw_timeout_;
	}

	const char* get_charset() const
	{
		return charset_;
	}

private:
	char* dbaddr_;          // 数据库监听地址
	char* dbname_;          // 数据库名
	char* dbkey_;           // dbname@dbaddr
	char* dbuser_;          // 数据库账号
	char* dbpass_;          // 数据库账号密码
	char* charset_;         // 连接数据库时的字符集
	size_t dblimit_;        // 数据库连接池连接数上限
	unsigned long dbflags_; // 打开数据库时的标志位
	bool  auto_commit_;     // 是否自动提交修改后的数据
	int   conn_timeout_;    // 连接数据库的超时时间
	int   rw_timeout_;      // 与数据库通信的超时时间
};

} // namespace acl

#endif // !defined(ACL_CLIENT_ONLY) && !defined(ACL_DB_DISABLE)
