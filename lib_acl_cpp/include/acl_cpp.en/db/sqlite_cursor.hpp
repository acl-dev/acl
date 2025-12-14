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
	 * Constructor
	 * @param q {query&} SQL query object, will be converted to SQL string in the constructor
	 */
	sqlite_cursor(query& q);
	~sqlite_cursor(void);

	/**
	 * Get query SQL statement
	 * @return {const string&}
	 */
	const string& get_sql(void) const {
		return sql_;
	}

	/**
	 * When traversing query result set, can get result row after each query through this method
	 * @return {db_row*}
	 */
	db_row* get_row(void) const {
		return row_;
	}

	/**
	 * Called by db_sqlite class to initialize names_ field names
	 * @param name {const char*} Database table column name
	 */
	void add_column_name(const char* name);
	
	/**
	 * Add column value
	 * @param n {long long}
	 */
	void add_column_value(long long n);

	/**
	 * Add column value
	 * @param n {double}
	 */
	void add_column_value(double n);

	/**
	 * Add column value
	 * @param s {cont char*} The lifetime of this parameter is determined by stmt_
	 */
	void add_column_value(const char* s);

	/**
	 * Create row record object to store query result rows
	 */
	void create_row(void);

	/**
	 * During traversal, db_sqlite::next method will first call this method to clear previous query results
	 */
	void clear(void);

private:
	friend class db_sqlite;

	typedef int (*free_sqlite3_stmt_fn)(sqlite3_stmt*);

	string sql_;
	sqlite3_stmt* stmt_;
	free_sqlite3_stmt_fn free_callback;

	// Database table field names
	std::vector<const char*> names_;

	dbuf_guard* dbuf_;	// Memory allocator
	db_row* row_;		// For query statements, used to store results
};

}

#endif // !defined(ACL_DB_DISABLE)

