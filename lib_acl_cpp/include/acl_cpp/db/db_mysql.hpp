#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include "acl_cpp/db/db_handle.hpp"

typedef struct st_mysql MYSQL;

namespace acl {

class ACL_CPP_API db_mysql : public db_handle
{
public:
	db_mysql(const char* dbaddr, const char* dbname,
		const char* dbuser, const char* dbpass,
		unsigned long dbflags = 0, bool auto_commit = true,
		int conn_timeout = 60, int rw_timeout = 60);
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

	/************************************************************************/
	/*            以下为基类 db_handle 的虚接口                             */
	/************************************************************************/

	/**
	 * 返回数据库的类型描述
	 * @return {const char*}
	 */
	virtual const char* dbtype() const;

	/**
	 * 获得上次数据库操作的出错错误号
	 * @return {int}
	 */
	virtual int get_errno() const;

	/**
	 * 获得上次数据库操作的出错错描述
	 * @return {const char*}
	 */
	virtual const char* get_error() const;

	/**
	 * 基类 db_handle 的纯虚接口
	 * @param local_charset {const char*} 本地字符集
	 * @return {bool} 打开是否成功
	 */
	virtual bool open(const char* local_charset = "GBK");

	/**
	 * 基类 db_handle 的纯虚接口，数据库是否已经打开了
	 * @return {bool} 返回 true 表明数据库已经打开了
	 */
	virtual bool is_opened() const;

	/**
	 * 基类 db_handle 的纯虚接口
	 * @return {bool} 关闭是否成功
	 */
	virtual bool close(void);

	/**
	 * 基类 db_handle 的纯虚接口，子类必须实现此接口用于判断数据表是否存在
	 * @return {bool} 是否存在
	 */
	virtual bool tbl_exists(const char* tbl_name);

	/**
	 * 基类 db_handle 的纯虚接口
	 * @param sql {const char*} 标准的 SQL 语句，非空，并且一定得要注册该
	 *  SQL 语句必须经过转义处理，以防止 SQL 注入攻击
	 * @return {bool} 执行是否成功
	 */
	virtual bool sql_select(const char* sql);

	/**
	 * 基类 db_handle 的纯虚接口
	 * @param sql {const char*} 标准的 SQL 语句，非空，并且一定得要注册该
	 *  SQL 语句必须经过转义处理，以防止 SQL 注入攻击
	 * @return {bool} 执行是否成功
	 */
	virtual bool sql_update(const char* sql);

	/**
	 * 基类 db_handle 的纯虚接口：上次 sql 操作影响的记录行数
	 * @return {int} 影响的行数，-1 表示出错
	 */
	virtual int affect_count() const;
protected:
private:
	char* dbaddr_;  // 数据库监听地址
	char* dbname_;  // 数据库名
	char* dbuser_;  // 数据库账号
	char* dbpass_;  // 数据库账号密码

	unsigned long dbflags_;
	int   conn_timeout_;
	int   rw_timeout_;
	bool  auto_commit_;
	MYSQL* conn_;

	bool sane_mysql_query(const char* sql);
};

} // namespace acl
