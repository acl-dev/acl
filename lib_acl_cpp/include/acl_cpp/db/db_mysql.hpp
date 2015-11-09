#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/db/db_handle.hpp"

typedef struct st_mysql MYSQL;

namespace acl {

class mysql_conf;

class ACL_CPP_API db_mysql : public db_handle
{
public:
	/**
	 * 构造函数方式一
	 * @param dbaddr {const char*} 数据库监听地址，可以为 TCP 套接口或在 UNIX
	 *  平台下的域套接口，格式如：127.0.0.1:3306，或 /tmp/mysql.sock
	 * @param dbname {const char*} 数据库名称，非 NULL
	 * @param dbuser {const char*} 连接数据库时的用户名
	 * @param dbpass {const char*} 连接数据库时的用户密码
	 * @param dbflags {unsigned long} 连接 MYSQL 时的标志位
	 * @param auto_commit {bool} 当对数据库进行修改时是否自动提交事务
	 * @param conn_timeout {int} 连接数据库的超时时间（秒）
	 * @param rw_timeout {int} 进行数据库操作时的超时时间（秒）
	 * @param charset {const char*} 连接数据库时的本地字符集（gbk, utf8, ...）
	 */
	db_mysql(const char* dbaddr, const char* dbname,
		const char* dbuser, const char* dbpass,
		unsigned long dbflags = 0, bool auto_commit = true,
		int conn_timeout = 60, int rw_timeout = 60,
		const char* charset = "utf8");

	/**
	 * 构造函数方式二：使用参数配置类对象进行构造
	 * @param conf {const mysql_conf&} mysql 数据库连接配置类对象
	 */
	db_mysql(const mysql_conf& conf);
	~db_mysql(void);

	/**
	 * 获得 mysql 客户端库的版本号
	 * @return {unsigned long}
	 */
	unsigned long mysql_libversion() const;

	/**
	 * 获得 mysql 客户端库的信息
	 * @return {const char*}
	 */
	const char* mysql_client_info() const;

	/**
	 * 直接获得 mysql 的连接句柄，如果返回 NULL 则表示 mysql 还没有打开
	 * 或出错时内部自动关闭了 mysql 连接
	 * @return {MYSQL*}
	 */
	MYSQL* get_conn() const
	{
		return conn_;
	}

	/********************************************************************/
	/*         以下为基类 db_handle 的虚接口                            */
	/********************************************************************/

	/**
	 * 返回数据库的类型描述
	 * @return {const char*}
	 */
	const char* dbtype() const;

	/**
	 * 获得上次数据库操作的出错错误号
	 * @return {int}
	 */
	int get_errno() const;

	/**
	 * 获得上次数据库操作的出错错描述
	 * @return {const char*}
	 */
	const char* get_error() const;

	/**
	 * 基类 db_handle 的纯虚接口
	 * @param charset {const char*} 打开数据库连接时采用的字符集，当该
	 *  参数非空时将会覆盖构造函数中传入的字符集
	 * @return {bool} 打开是否成功
	 */
	bool dbopen(const char* charset = NULL);

	/**
	 * 基类 db_handle 的纯虚接口，数据库是否已经打开了
	 * @return {bool} 返回 true 表明数据库已经打开了
	 */
	bool is_opened() const;

	/**
	 * 基类 db_handle 的纯虚接口
	 * @return {bool} 关闭是否成功
	 */
	bool close(void);

	/**
	 * 基类 db_handle 的纯虚接口，子类必须实现此接口用于判断数据表是否存在
	 * @return {bool} 是否存在
	 */
	bool tbl_exists(const char* tbl_name);

	/**
	 * 基类 db_handle 的纯虚接口
	 * @param sql {const char*} 标准的 SELECT SQL 语句，非空，并且一定
	 *  得要注意该 SQL 语句必须经过转义处理，以防止 SQL 注入攻击
	 * @return {bool} 执行是否成功
	 */
	bool sql_select(const char* sql);

	/**
	 * 基类 db_handle 的纯虚接口
	 * @param sql {const char*} 标准的 INSERT/UPDATE/DELETE SQL 语句，
	 *  并且一定得要注意该 SQL 语句必须经过转义处理，以防止 SQL 注入攻击
	 * @return {bool} 执行是否成功
	 */
	bool sql_update(const char* sql);

	/**
	 * 基类 db_handle 的纯虚接口：上次 sql 操作影响的记录行数
	 * @return {int} 影响的行数，-1 表示出错
	 */
	int affect_count() const;

	/**
	 * 基类 db_handle 的虚函数，用来表示事务的开始，注意若要使用事务方式，
	 * 则需要在 db_mysql 的构造函数中传入的参数 auto_commit 为 false
	 * @return {bool}
	 */
	bool begin_transaction();

	/**
	 * 基类 db_handle 的虚函数，用来表示事务的结束
	 * @return {bool}
	 */
	bool commit();

private:
	char* dbaddr_;  // 数据库监听地址
	char* dbname_;  // 数据库名
	char* dbuser_;  // 数据库账号
	char* dbpass_;  // 数据库账号密码
	string charset_; // 连接数据库采用的字符集

	unsigned long dbflags_;
	int   conn_timeout_;
	int   rw_timeout_;
	bool  auto_commit_;
	MYSQL* conn_;

	bool sane_mysql_query(const char* sql);
	void sane_mysql_init(const char* dbaddr, const char* dbname,
		const char* dbuser, const char* dbpass,
		unsigned long dbflags, bool auto_commit,
		int conn_timeout, int rw_timeout,
		const char* charset);
};

} // namespace acl
