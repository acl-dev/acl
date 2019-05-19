#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/string.hpp"
#include "db_cursor.hpp"

#if !defined(ACL_DB_DISABLE)

struct sqlite3_stmt;

namespace acl
{

class db_row;
class db_sqlite;
class dbuf_guard;

class sqlite_cursor : public db_cursor
{
public:
	/**
	 * 构造方法
	 * @param q {query&} SQL 查询对象，在构造方法内会首先将其转为 Sql 字符串
	 */
	sqlite_cursor(query& q);
	~sqlite_cursor(void);

	/**
	 * 获得查询 SQL 语句
	 * @return {const string&}
	 */
	const string& get_sql(void) const
	{
		return sql_;
	}

	/**
	 * 在遍历查询结构集时，每次查询后可通过本方法获得结果行
	 * @return {db_row*}
	 */
	db_row* get_row(void) const
	{
		return row_;
	}

	/**
	 * 由 db_sqlite 类调用来初始化 names_ 字段名
	 * @param name {const char*} 数据表列名
	 */
	void add_column_name(const char* name);
	
	/**
	 * 添加列值
	 * @param n {long long}
	 */
	void add_column_value(long long n);

	/**
	 * 添加列值
	 * @param n {double}
	 */
	void add_column_value(double n);

	/**
	 * 添加列值
	 * @param s {cont char*} 该参数的生命周期由 stmt_ 决定
	 */
	void add_column_value(const char* s);

	/**
	 * 创建行记录对象，用来存放查询结果行
	 */
	void create_row(void);

	/**
	 * 在遍历过程中，db_sqlite::next 方法会首先调用本方法清除上次的查询结果
	 */
	void clear(void);

private:
	friend class db_sqlite;

	typedef int (*free_sqlite3_stmt_fn)(sqlite3_stmt*);

	string sql_;
	sqlite3_stmt* stmt_;
	free_sqlite3_stmt_fn free_callback;

	// 数据表字段名
	std::vector<const char*> names_;

	dbuf_guard* dbuf_;	// 内存分配器
	db_row* row_;		// 对于查询语句而言，用来存储结果
};

}

#endif // !defined(ACL_DB_DISABLE)
