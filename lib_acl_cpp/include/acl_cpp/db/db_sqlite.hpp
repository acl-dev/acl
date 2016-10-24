#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/db/db_handle.hpp"

typedef struct sqlite3 sqlite3;

namespace acl {

class charset_conv;

class ACL_CPP_API db_sqlite : public db_handle
{
public:
	/**
	 * 构造函数
	 * @param charset {const char*} 本地字符集(gbk, utf-8, ...)
	 */
	db_sqlite(const char* dbfile, const char* charset = "utf-8");
	~db_sqlite(void);

	/**
	 * 返回当前的 sqlite 的版本信息
	 */
	const char* version(void) const;

	/**
	 * 当数据库打开后通过此函数对数据库的操作引擎进行配置，
	 * 进行配置的内容需要严格遵循 sqlite 本身的配置选项要求
	 * @param pragma {const char*} 配置选项内容，格式为：
	 *  PRAGMA xxx=xxx
	 *  如：PRAGMA synchronous = NORMAL
	 * @return {bool} 配置数据库是否成功
	 */
	bool set_conf(const char* pragma);

	/**
	 * 当数据库打开调用此函数获得数据引擎的配置选项
	 * @param pragma {const char*} 配置选项内容，格式为：
	 *  PRAGMA xxx
	 *  如：PRAGMA synchronous
	 * @param out {string&} 如果返回值非空则存储结果
	 * @return {const char*} 为空则说明该配置不存在或数据库未打开
	 */
	const char* get_conf(const char* pragma, string& out);

	/**
	 * 在数据库打开的情况下输入数据库引擎的配置选项
	 * @param pragma {const char*} 指定的配置选项，如果该参数为空，
	 *  则输出所有的配置选项，格式为：PRAGMA xxx，如：PRAGMA synchronous
	 */
	void show_conf(const char* pragma = NULL);

	/**
	 * 自数据库打开后所有的影响的记录行数
	 * @return {int} 影响的行数，-1 表示出错
	 */
	int affect_total_count() const;

	/**
	 * 直接获得 sqlite 的句柄，如果返回 NULL 则表示 sqlite 还没有打开
	 * 或出错时内部自动关闭了 sqlite
	 * @return {sqlite3*}
	 */
	sqlite3* get_conn() const
	{
		return db_;
	}

	/********************************************************************/
	/*            以下为基类 db_handle 的虚接口                         */
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
	 * @param sql {const char*} 标准的 SELECT SQL 语句，并且一定得要
	 *  注意该 SQL 语句必须经过转义处理，以防止 SQL 注入攻击
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

private:
	// sqlite 引擎
	sqlite3* db_;

	// 数据存储文件
	string dbfile_;

	// 字符集转码器
	charset_conv* conv_;

	// 本地字符集
	string charset_;

	// 真正执行SQL查询的函数
	bool exec_sql(const char* sql);
};

} // namespace acl
