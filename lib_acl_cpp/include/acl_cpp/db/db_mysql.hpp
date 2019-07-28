#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/string.hpp"
#include "../db/db_handle.hpp"

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_DB_DISABLE)

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
	unsigned long mysql_libversion(void) const;

	/**
	 * 获得 mysql 客户端库的信息
	 * @return {const char*}
	 */
	const char* mysql_client_info(void) const;

	/**
	 * 直接获得 mysql 的连接句柄，如果返回 NULL 则表示 mysql 还没有打开
	 * 或出错时内部自动关闭了 mysql 连接
	 * @return {MYSQL*}
	 */
	MYSQL* get_conn(void) const
	{
		return conn_;
	}

	/**
	 * 当动态加载 libmysqlclient.so / libmysqlclient.dll 时，可以调用本
	 * 静态函数显式动态加载 mysql 客户端库，如果加载失败，内部会自动产生
	 * 断言，以免运行时出错，也可不调用本函数，使 db_mysql 类对象内部在
	 * 使用时隐式加载 mysql 动态库
	 */
	static void load(void);

	/********************************************************************/
	/*         以下为基类 db_handle 的虚接口                            */
	/********************************************************************/

	/**
	 * @override
	 */
	const char* dbtype(void) const;

	/**
	 * @override
	 */
	int get_errno(void) const;

	/**
	 * @override
	 */
	const char* get_error(void) const;

	/**
	 * @override
	 */
	bool dbopen(const char* charset = NULL);

	/**
	 * @override
	 */
	bool is_opened(void) const;

	/**
	 * @override
	 */
	bool close(void);

	/**
	 * @override
	 */
	bool tbl_exists(const char* tbl_name);

	/**
	 * @override
	 */
	bool sql_select(const char* sql, db_rows* result = NULL);

	/**
	 * @override
	 */
	bool sql_update(const char* sql);

	/**
	 * @override
	 */
	int affect_count(void) const;

	/**
	 * @override
	 * 基类 db_handle 的虚函数，用来表示事务的开始，注意若要使用事务方式，
	 * 则需要在 db_mysql 的构造函数中传入的参数 auto_commit 为 false
	 */
	bool begin_transaction(void);

	/**
	 * @override
	 */
	bool commit(void);

	/**
	 * @override
	 */
	bool rollback(void);

private:
	char*  dbaddr_;  // 数据库监听地址
	char*  dbname_;  // 数据库名
	char*  dbuser_;  // 数据库账号
	char*  dbpass_;  // 数据库账号密码
	string charset_; // 连接数据库采用的字符集

	unsigned long dbflags_;
	int    conn_timeout_;
	int    rw_timeout_;
	bool   auto_commit_;
	MYSQL* conn_;

	bool sane_mysql_query(const char* sql);
	void sane_mysql_init(const char* dbaddr, const char* dbname,
		const char* dbuser, const char* dbpass,
		unsigned long dbflags, bool auto_commit,
		int conn_timeout, int rw_timeout,
		const char* charset);
};

} // namespace acl

#endif // !defined(ACL_CLIENT_ONLY) && !defined(ACL_DB_DISABLE)
