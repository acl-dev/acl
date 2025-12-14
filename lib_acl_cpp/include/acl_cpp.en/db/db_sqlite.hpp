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

class ACL_CPP_API db_sqlite : public db_handle {
public:
	/**
	 * Constructor (internally automatically loads sqlite dynamic library)
	 * @param charset {const char*} Database character set (gbk, utf-8, ...)
	 */
	db_sqlite(const char* dbfile, const char* charset = "utf-8");
	~db_sqlite(void);

	/**
	 * Manually statically load sqlite dynamic library.
	 * @return {bool}
	 */
	static bool load(void);

	/**
	 * Get the version information of the current sqlite.
	 * @return {const char*}
	 */
	const char* version(void) const;

	/**
	 * After the database is opened, configure the database's pragma options
	 * through this function. The pragma options used need to strictly follow
	 * sqlite's pragma option requirements.
	 * @param pragma {const char*} Pragma option content, format is:
	 *  PRAGMA xxx=xxx
	 *  e.g.: PRAGMA synchronous = NORMAL
	 * @return {bool} Whether configuring the database was successful
	 */
	bool set_conf(const char* pragma);

	/**
	 * After the database is opened, call this function to query pragma options.
	 * @param pragma {const char*} Pragma option content, format is:
	 *  PRAGMA xxx, e.g.: PRAGMA synchronous
	 * @param out {string&} Non-empty buffer to store the result value.
	 * @return {const char*} Returns NULL to indicate the option does not exist or
	 * database is not opened.
	 */
	const char* get_conf(const char* pragma, string& out);

	/**
	 * After the database is opened, call this function to display database pragma
	 * options.
	 * @param pragma {const char*} Specify a pragma option. If this parameter is
	 * empty, display all pragma options, format: PRAGMA xxx, e.g.: PRAGMA synchronous
	 */
	void show_conf(const char* pragma = NULL);

	/**
	 * Get the number of affected records after the database is opened.
	 * @return {int} Number of affected records, -1 indicates error.
	 */
	int affect_total_count(void) const;

	/**
	 * Directly get sqlite's native connection handle. Returns NULL to indicate
	 * sqlite has not been opened.
	 * When this handle is closed, internally automatically closes sqlite.
	 * @return {sqlite3*}
	 */
	sqlite3* get_conn(void) const {
		return db_;
	}

	/**
	 * Prepare cursor.
	 * @param cursor {sqlite_cursor&}
	 * @return {bool}
	 */
	bool prepare(sqlite_cursor& cursor);

	/**
	 * Execute the next step. If it is a query process, bind the query result to
	 * the cursor parameter.
	 * @param cursor {sqlite_cursor&}
	 * @return {bool}
	 */
	bool next(sqlite_cursor& cursor, bool* done);

	/********************************************************************/
	/*            The following are some sqlite3 private interfaces                         */
	/********************************************************************/

	/**
	 * Compile zSql into a prepared statement.
	 * @param zSql {const char*} utf-8 encoded sql.
	 * @param nByte {int} Byte length of zSql string.
	 * @param ppStmt {sqlite3_stmt**} OUT: prepared statement handle.
	 * @param pzTail {const char**} OUT: Pointer to unused part of zSql.
	 * @return {int} Returns SQLITE_OK on success, otherwise returns corresponding
	 * error code.
	 */
	int sqlite3_prepare_v2(const char *zSql,
		int nByte, sqlite3_stmt **ppStmt, const char **pzTail);

	/**
	 * Execute prepared statement.
	 * @param stmt {sqlite3_stmt*} prepared statement
	 * @return {int} Returns SQLITE_BUSY, SQLITE_DONE, SQLITE_ROW,
	 *          SQLITE_ERROR, or SQLITE_MISUSE
	 */
	int sqlite3_step(sqlite3_stmt *stmt);

	/**
	 * Reset prepared statement to initial state.
	 * @param pStmt {sqlite3_stmt*} prepared statement
	 * @return {int} SQLITE_ROW, SQLITE_DONE or SQLITE_OK
	 */
	int sqlite3_reset(sqlite3_stmt *pStmt);

	/**
	 * Release prepared statement resources.
	 * @param stmt {sqlite3_stmt*} prepared statement handle.
	 * @return {int} SQLITE_OK indicates success, otherwise indicates error.
	 */
	int sqlite3_finalize(sqlite3_stmt *stmt);

	/**
	 * Bind binary data parameter.
	 * @param stmt {sqlite3*} prepared statement
	 * @param iCol {int} Parameter index in sql to bind.
	 * @param value {const void*} Parameter value in sql to bind.
	 * @param n {int} Parameter byte length.
	 * @param destory {void(*)(void*)} Parameter destruction callback function.
	 * @return {int} Returns SQLITE_OK on success, otherwise returns corresponding
	 * error code.
	 */
	int sqlite3_bind_blob(sqlite3_stmt *stmt, int iCol,
		const void *value, int n, void(*destory)(void*));

	/**
	 * Bind int parameter.
	 * @param stmt {sqlite3*} prepared statement
	 * @param iCol {int} Parameter index in sql to bind.
	 * @param value {int} Parameter value in sql to bind.
	 * @return {int} Returns SQLITE_OK on success, otherwise returns corresponding
	 * error code.
	 */
	int sqlite3_bind_int(sqlite3_stmt *stmt, int iCol, int value);

	/**
	 * Bind int64 parameter.
	 * @param stmt {sqlite3*} prepared statement
	 * @param iCol {int} Parameter index in sql to bind.
	 * @param value {long long int} Parameter value in sql to bind.
	 * @return {int} Returns SQLITE_OK on success, otherwise returns corresponding
	 * error code.
	 */
	int sqlite3_bind_int64(sqlite3_stmt* stmt, int iCol, long long int value);

	/**
	 * Bind text parameter.
	 * @param stmt {sqlite3*} prepared statement
	 * @param iCol {int} Parameter index in sql to bind.
	 * @param value {const void*} Parameter value in sql to bind.
	 * @param n {int} Parameter byte length.
	 * @param destory {void(*)(void*)} Parameter destruction callback function.
	 * @return {int} Returns SQLITE_OK on success, otherwise returns corresponding
	 * error code.
	 */
	int sqlite3_bind_text(sqlite3_stmt *stmt, int iCol,
		const char *value, int n, void(*destory)(void*));

	/**
	 * Get the number of columns in prepared statement result set.
	 * @param stmt {sqlite3_stmt*} prepared statement
	 * @return {int} Number of columns.
	 */
	int sqlite3_column_count(sqlite3_stmt *stmt);

	/**
	 * Get binary data information of corresponding column in query result.
	 * @param stmt {sqlite3_stmt*} prepared statement
	 * @param iCol {int} Column number.
	 * @return {const void*} Data pointer.
	 */
	const void *sqlite3_column_blob(sqlite3_stmt *stmt, int iCol);

	/**
	 * Get int data information of corresponding column in query result.
	 * @param stmt {sqlite3_stmt*} prepared statement
	 * @param iCol {int} Column number.
	 * @return {int} Data.
	 */
	int sqlite3_column_int(sqlite3_stmt *stmt, int iCol);

	/**
	 * Get int64 data information of corresponding column in query result.
	 * @param stmt {sqlite3_stmt*} prepared statement
	 * @param iCol {int} Column number.
	 * @return {long long int} Data.
	 */
	long long int sqlite3_column_int64(sqlite3_stmt *stmt, int iCol);

	/**
	 * Get utf-8 text data information of corresponding column in query result.
	 * @param stmt {sqlite3_stmt*} prepared statement
	 * @param iCol {int} Column number.
	 * @return {const unsigned char *} Data pointer.
	 */
	const unsigned char *sqlite3_column_text(sqlite3_stmt *stmt, int iCol);

	/**
	 * Get data information of corresponding column in query result, byte length.
	 * @param stmt {sqlite3_stmt*} prepared statement
	 * @param iCol {int} Column number.
	 * @return {const unsigned char *} Data pointer.
	 */
	int sqlite3_column_bytes(sqlite3_stmt *stmt, int iCol);

	/**
	 * Get column name of specified column in select query result.
	 * @param stmt {sqlite3_stmt*} prepared statement
	 * @param iCol {int} Column number.
	 * @return {const char*} Name.
	 */
	const char *sqlite3_column_name(sqlite3_stmt *stmt, int iCol);

	/**
	 * Execute sql statement.
	 * @param sql {const char*} Sql statement to execute.
	 * @param callback {int (*)(void*,int,char**,char**)} callback function.
	 * @param arg {void*} First parameter of callback function.
	 * @param errmsg {char**} Error message.
	 * @return {int} SQLITE_OK indicates success, otherwise indicates error.
	 */
	int sqlite3_exec(const char *sql,
		int(*callback)(void*,int,char**,char**), void *arg, char **errmsg);

	/**
	 * Additional interface for releasing errmsg memory.
	 * @param ptr {void*} Pointer to be released.
	 */
	void sqlite3_free(void* ptr);

	/********************************************************************/
	/*            The following are virtual interfaces of base class db_handle                         */
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
	// sqlite handle
	sqlite3* db_;

	// Data storage file
	string dbfile_;

	// Character set converter
	charset_conv* conv_;

	// Database character set
	string charset_;

	// Function for executing SQL query
	bool exec_sql(const char* sql, db_rows* result = NULL);
};

} // namespace acl

#endif // !defined(ACL_DB_DISABLE)

