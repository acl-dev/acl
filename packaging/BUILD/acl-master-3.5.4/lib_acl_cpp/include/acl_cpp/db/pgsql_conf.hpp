#pragma once
#include "../acl_cpp_define.hpp"

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_DB_DISABLE)

namespace acl {

class ACL_CPP_API pgsql_conf
{
public:
	/**
	 * 构造函数
	 * @param dbaddr {const char*} 服务器地址，地址格式为：ip:port，或
	 *  unix_domain_path，当为 unix 域套接口时，应为 unix 域套接口文件
	 *  所在目录且不包含文件名，假设 postgresql 正在监听 unix 域套接口
	 *  的文件为：/tmp/.s.PGSQL.5432，则 dbaddr 地址应设为 /tmp
	 *  注意：注意在连接 unix 域套接口的与 mysql 的不同，mysql 的域套接
	 *  口为全路径
	 * @param dbname {const char*} 数据库名
	 */
	pgsql_conf(const char* dbaddr, const char* dbname);

	/**
	 * 拷贝构造函数
	 * @param conf {const pgsql_conf&} 内部将会创建新配置对象并拷贝该参数
	 *  里的内容项
	 */
	pgsql_conf(const pgsql_conf& conf);

	~pgsql_conf(void);

	/**
	 * 设置连接数据库时的用户账号，当不调用此方法时则不需账号
	 * @param dbuser {const char*} 用户账号，为非空字符串时才有效
	 * @return {pgsql_conf&}
	 */
	pgsql_conf& set_dbuser(const char* dbuser);

	/**
	 * 设置连接数据库时的账号密码，当不调用此方法时则不设密码
	 * @param dbpass {const char*} 账号密码，为非空字符串时才有效
	 * @return {pgsql_conf&}
	 */
	pgsql_conf& set_dbpass(const char* dbpass);

	/**
	 * 设置数据库连接池最大连接上限
	 * @param dblimit {size_t} 连接池最大连接数限制，当为 0 时则不限制
	 * @return {pgsql_conf&}
	 */
	pgsql_conf& set_dblimit(size_t dblimit);

	/**
	 * 设置连接数据库的超时时间
	 * @param timeout {int}
	 * @return {pgsql_conf&}
	 */
	pgsql_conf& set_conn_timeout(int timeout);

	/**
	 * 设置读取数据库结果的超时时间
	 * @param timeout {int}
	 * @return {pgsql_conf&}
	 */
	pgsql_conf& set_rw_timeout(int timeout);

	/**
	 * 设置数据库连接的字符集
	 * @param charset {const char*}
	 * @return {pgsql_conf&}
	 */
	pgsql_conf& set_charset(const char* charset);

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
	char* dbaddr_;
	char* dbname_;
	char* dbkey_;
	char* dbuser_;
	char* dbpass_;
	char* charset_;
	size_t dblimit_;
	int   conn_timeout_;
	int   rw_timeout_;
};

} // namespace acl

#endif // !defined(ACL_CLIENT_ONLY) && !defined(ACL_DB_DISABLE)
