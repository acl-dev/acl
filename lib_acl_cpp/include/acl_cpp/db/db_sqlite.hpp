#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/string.hpp"
#include "../db/db_handle.hpp"

#if !defined(ACL_DB_DISABLE)

typedef struct sqlite3 sqlite3;
typedef struct sqlite3_stmt sqlite3_stmt;

namespace acl {

class charset_conv;
class sqlite_cursor;

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
	int affect_total_count(void) const;

	/**
	 * 直接获得 sqlite 的句柄，如果返回 NULL 则表示 sqlite 还没有打开
	 * 或出错时内部自动关闭了 sqlite
	 * @return {sqlite3*}
	 */
	sqlite3* get_conn(void) const
	{
		return db_;
	}

	/**
	 * 准备游标
	 * @param cursor {sqlite_cursor&}
	 * @return {bool}
	 */
	bool prepare(sqlite_cursor& cursor);

	/**
	 * 执行下一步，如果是查询类过程，则将查询结果存入给定的参数中
	 * @param cursor {sqlite_cursor&}
	 * @return {bool}
	 */
	bool next(sqlite_cursor& cursor, bool* done);

	/********************************************************************/
	/*            以下为一些 sqlite3 的私有接口                         */
	/********************************************************************/

	/**
	 * 将zSql初始化为 prepared statement
	 * @param  {const char *zSql} utf-8编码的sql
	 * @param  {int nByte} zSql的最大字节长度
	 * @param  {sqlite3_stmt **ppStmt} OUT: prepared statement句柄
	 * @param  {const char **pzTail} OUT: 指向zSql未使用部分的指针
	 * @return {int} 成功返回 SQLITE_OK，否则返回相应的错误代码
	 */
	int sqlite3_prepare_v2(const char *zSql,
		int nByte, sqlite3_stmt **ppStmt, const char **pzTail);

	/**
	 * 计算 prepared statement
	 * @param {sqlite3_stmt *stmt} prepared statement
	 * @return {int} 返回 SQLITE_BUSY, SQLITE_DONE, SQLITE_ROW,
	 *          SQLITE_ERROR, 或 SQLITE_MISUSE
	 */
	int sqlite3_step(sqlite3_stmt *stmt);

	/**
	 * 将prepared statement重置为初始化状态
	 * @param {sqlite3_stmt *pStmt} prepared statement
	 * @return {int} SQLITE_ROW, SQLITE_DONE 或 SQLITE_OK
	 */
	int sqlite3_reset(sqlite3_stmt *pStmt);

	/**
	 * 释放 prepared statement 资源
	 * @param  {sqlite3_stmt *stmt} prepared statement句柄
	 * @return {int} SQLITE_OK 或其他错误代码
	 */
	int sqlite3_finalize(sqlite3_stmt *pStmt);

	/**
	 * 绑定二进制数据
	 * @param {sqlite3 *stmt} prepared statement
	 * @param {int iCol} 待绑定到sql中的参数索引
	 * @param {const void *value} 待绑定到sql中的参数数值
	 * @param {int n} 参数的字节长度
	 * @param {void(*)(void*)} 传入参数的析构函数
	 * @return {int} 成功返回 SQLITE_OK，否则返回相应的错误代码
	 */
	int sqlite3_bind_blob(sqlite3_stmt *stmt, int iCol,
		const void *value, int n, void(*)(void*));

	/**
	 * 绑定int类型数据
	 * @param {sqlite3 *stmt} prepared statement
	 * @param {int iCol} 待绑定到sql中的参数索引
	 * @param {int value} 待绑定到sql中的参数数值
	 * @return {int} 成功返回 SQLITE_OK，否则返回相应的错误代码
	 */
	int sqlite3_bind_int(sqlite3_stmt *stmt, int iCol, int value);

	/**
	 * 绑定int64数据
	 * @param {sqlite3 *stmt} prepared statement
	 * @param {int iCol} 待绑定到sql中的参数索引
	 * @param {int64_t value} 待绑定到sql中的参数数值
	 * @return {int} 成功返回 SQLITE_OK，否则返回相应的错误代码
	 */
	int sqlite3_bind_int64(sqlite3_stmt*, int iCol, int64_t value);

	/**
	 * 绑定text数据
	 * @param {sqlite3 *stmt} prepared statement
	 * @param {int iCol} 待绑定到sql中的参数索引
	 * @param {const void *value} 待绑定到sql中的参数数值
	 * @param {int n} 参数的字节长度
	 * @param {void(*)(void*)} 传入参数的析构函数
	 * @return {int} 成功返回 SQLITE_OK，否则返回相应的错误代码
	 */
	int sqlite3_bind_text(sqlite3_stmt *stmt, int iCol,
		const char *value, int n, void(*)(void*));

	/**
	 * 返回 prepared statement 结果集的列数
	 * @param {sqlite3_stmt* pStmt} prepared statement
	 * @return {int} 列数量
	 */
	int sqlite3_column_count(sqlite3_stmt *pStmt);

	/**
	 * 返回查询结果的对应列的二进制结果信息
	 * @param {sqlite3_stmt *stmt} prepared statement
	 * @param {int iCol} 列索引
	 * @return {const void *} 数据指针
	 */
	const void *sqlite3_column_blob(sqlite3_stmt *stmt, int iCol);

	/**
	 * 返回查询结果的对应列的int结果信息
	 * @param {sqlite3_stmt *stmt} prepared statement
	 * @param {int iCol} 列索引
	 * @return {int} 数据
	 */
	int sqlite3_column_int(sqlite3_stmt*, int iCol);

	/**
	 * 返回查询结果的对应列的int64结果信息
	 * @param {sqlite3_stmt *stmt} prepared statement
	 * @param {int iCol} 列索引
	 * @return {int64_t} 数据
	 */
	int64_t sqlite3_column_int64(sqlite3_stmt*, int iCol);

	/**
	 * 返回查询结果的对应列的 utf-8 text 结果信息
	 * @param {sqlite3_stmt *stmt} prepared statement
	 * @param {int iCol} 列索引
	 * @return {const unsigned char *} 数据指针
	 */
	const unsigned char *sqlite3_column_text(sqlite3_stmt*, int iCol);

	/**
	 * 返回查询结果的对应列的结果信息数据字节长度
	 * @param {sqlite3_stmt *stmt} prepared statement
	 * @param {int iCol} 列索引
	 * @return {const unsigned char *} 数据指针
	 */
	int sqlite3_column_bytes(sqlite3_stmt*, int iCol);

	/**
	 * 返回select结果集中特定列的名称
	 * @param {sqlite3_stmt* stmt} prepared statement
	 * @param {int iCol} 列索引
	 * @return {const char*} 列名
	 */
	const char *sqlite3_column_name(sqlite3_stmt *stmt, int iCol);

	/**
	 * 执行单条sql语句
	 * @param  {const char *sql} 待执行的sql语句
	 * @param  {int (*callback)(void*,int,char**,char**)} callback函数
	 * @param  {void *arg}callback函数的第一个参数
	 * @param  {char **errmsg} 错误信息
	 * @return {int} SQLITE_OK 或其他错误码
	 */
	int sqlite3_exec(const char *sql,
		int(*callback)(void*,int,char**,char**), void *arg, char **errmsg);

	/**
	 * 为释放 errmsg 而添加的接口
	 * @param {void* ptr} 待释放数据指针
	 * @return {void}
	 */
	void sqlite3_free(void* ptr);

	/********************************************************************/
	/*            以下为基类 db_handle 的虚接口                         */
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
	 */
	bool begin_transaction(void);

	/**
	 * @override
	 */
	bool commit(void);

	/**
	 * @override
	 */
	bool set_busy_timeout(int nMillisecs);


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
	bool exec_sql(const char* sql, db_rows* result = NULL);
};

} // namespace acl

#endif // !defined(ACL_DB_DISABLE)
