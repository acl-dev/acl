#pragma once
#include "../acl_cpp_define.hpp"
#include <vector>
#include "../stdlib/string.hpp"
#include "../stdlib/noncopyable.hpp"
#include "../connpool/connect_client.hpp"

#if !defined(ACL_DB_DISABLE)

namespace acl {

/**
 * Database query result set record row object.
 */
class ACL_CPP_API db_row : public noncopyable {
public:
	/**
	 * Constructor
	 * @param names {const std::vector<const char*>&} Database table field name
	 * list.
	 */
	db_row(const std::vector<const char*>& names);
	~db_row();

	/**
	 * Get field name corresponding to a certain subscript value in data table.
	 * @param ifield {size_t} Subscript value.
	 * @return {const char*} Returns empty to indicate subscript value out of
	 * bounds or does not exist.
	 */
	const char* field_name(size_t ifield) const;

	/**
	 * Get corresponding field value from query result record set by field name.
	 * @param name {const char*} Database table field name.
	 * @return {const char*} Corresponding field value. Returns empty to indicate
	 * field value is NULL or field name is invalid.
	 */
	const char* field_value(const char* name) const;

	/**
	 * Get corresponding field value from query result record set by field name.
	 * Same as field_value.
	 * @param name {const char*} Database table field name.
	 * @return {const char*} Corresponding field value. Returns empty to indicate
	 * field value is NULL or field name is invalid.
	 */
	const char* operator[](const char* name) const;

	/**
	 * Get corresponding field value from query result record set by subscript.
	 * @param ifield {size_t} Subscript value. This value should be < number of
	 * fields.
	 * @return {const char*} Corresponding field value. Returns empty to indicate
	 * subscript value is invalid or field value is NULL.
	 */
	const char* field_value(size_t ifield) const;

	/**
	 * Get corresponding field value from query result record set by subscript.
	 * Same as field_value.
	 * @param ifield {size_t} Subscript value. This value should be < number of
	 * fields.
	 * @return {const char*} Corresponding field value. Returns empty to indicate
	 * subscript value is invalid or
	 *  field value is NULL.
	 */
	const char* operator[](size_t ifield) const;

	/**
	 * Get corresponding field value of integer type from query result record set
	 * by subscript.
	 * @param ifield {size_t} Subscript value.
	 * @param null_value {int} When field is NULL, returns this value to indicate
	 * no corresponding value.
	 * @return {int} Returned value. If user-provided null_value is same as
	 * returned value, no value was found.
	 */
	int field_int(size_t ifield, int null_value = 0) const;

	/**
	 * Get corresponding field value of integer type from query result record set
	 * by field name.
	 * @param name {const char*} Subscript value.
	 * @param null_value {int} When field is NULL, returns this value to indicate
	 * no corresponding value.
	 * @return {int} Returned value. If user-provided null_value is same as
	 * returned value, no value was found.
	 */
	int field_int(const char* name, int null_value = 0) const;

	
	/**
	 * Get corresponding field value of integer type from query result record set
	 * by subscript.
	 * @param ifield {size_t} Subscript value.
	 * @param null_value {acl_int64} When field is NULL, returns this value to
	 * indicate no corresponding value.
	 * @return {acl_int64} Returned value. If user-provided null_value value is
	 * same, then no value was found.
	 */
#if defined(_WIN32) || defined(_WIN64)
	__int64 field_int64(size_t ifield, __int64 null_value = 0) const;
#else
	long long int field_int64(size_t ifield,
		long long int null_value = 0) const;
#endif

	/**
	 * Get corresponding field value of integer type from query result record set
	 * by field name.
	 * @param name {const char*} Subscript value.
	 * @param null_value {acl_int64} When field is NULL, returns this value to
	 * indicate no corresponding value.
	 * @return {acl_int64} Returned value. If user-provided null_value value is
	 * same, then no value was found.
	 */
#if defined(_WIN32) || defined(_WIN64)
	__int64 field_int64(const char* name, __int64 null_value = 0) const;
#else
	long long int field_int64(const char* name,
		long long int null_value = 0) const;
#endif

	/**
	 * Get corresponding field value of floating point type from query result
	 * record set by subscript.
	 * @param ifield {size_t} Subscript value.
	 * @param null_value {double} When field is NULL, returns this value to
	 * indicate no corresponding value.
	 * @return {double} Returned value. If user-provided null_value value is same,
	 * then no value was found.
	 */
	double field_double(size_t ifield, double null_value = 0.0) const;

	/**
	 * Get corresponding field value of floating point type from query result
	 * record set by field name.
	 * @param name {const char*} Subscript value.
	 * @param null_value {double} When field is NULL, returns this value to
	 * indicate no corresponding value.
	 * @return {double} Returned value. If user-provided null_value value is same,
	 * then no value was found.
	 */
	double field_double(const char* name, double null_value = 0.0) const;

	/**
	 * Get corresponding field value of string type from query result record set by
	 * subscript.
	 * @param ifield {size_t} Subscript value.
	 * @return {const char*} Returned value. When NULL, it means no value was
	 * found.
	 */
	const char* field_string(size_t ifield) const;

	/**
	 * Get corresponding field value of string type from query result record set by
	 * field name.
	 * @param name {const char*} Subscript value.
	 * @return {const char*} Returned value. When NULL, it means no value was
	 * found.
	 */
	const char* field_string(const char* name) const;

	/**
	 * Get corresponding field value length of string type from query result record
	 * set by subscript.
	 * @param ifield {size_t} Subscript value.
	 * @return {size_t}
	 */
	size_t field_length(size_t ifield) const;
	/**
	 * Get corresponding field value length of string type from query result record
	 * set by field name.
	 * @param name {const char*} Subscript value.
	 * @return {size_t}
	 */
	size_t field_length(const char* name) const;

	/**
	 * Add a field value to record set. Field value order should match field name
	 * order.
	 * @param value {const char*} A certain field value in record set.
	 * @param len {size_t} Data length of value.
	 */
	void push_back(const char* value, size_t len);

	/**
	 * Number of field values in record set.
	 * @return {size_t}
	 */
	size_t length() const;

	/**
	 * Clear all values in values_.
	 */
	void clear();

private:
	// Database table field name reference list.
	const std::vector<const char*>& names_;

	// Field value list in query result.
	std::vector<const char*> values_;

	// Field value length list in query result.
	std::vector<size_t> lengths_;
};

/**
 * Database query result set record collection object.
 */
class ACL_CPP_API db_rows : public noncopyable
{
public:
	db_rows();
	virtual ~db_rows();

	/**
	 * Get matching record collection from query result record set by database
	 * table field name and corresponding field value.
	 * @param name {const char*} Database table field name (case-insensitive).
	 * @param value {const char*} Database table field value (case-insensitive).
	 * @return {const std::vector<const db_row*>&} Record collection object.
	 *  You can determine whether result is empty by calling db_rows.empty().
	 */
	const std::vector<const db_row*>& get_rows(
		const char* name, const char* value);

	/**
	 * Get all query results.
	 * @return {const std::vector<db_row*>&} Record collection object.
	 *  You can determine whether result is empty by calling db_rows.empty().
	 */
	const std::vector<db_row*>& get_rows() const;

	/**
	 * Get a certain record from query result record set by specified subscript.
	 * @param idx {size_t} Record subscript. This value should be < result set
	 * size.
	 * @return {const db_row*} Returns empty to indicate subscript value is invalid
	 * or field value is empty.
	 */
	const db_row* operator[](size_t idx) const;

	/**
	 * Determine whether result is empty.
	 * @return {bool} Whether empty.
	 */
	bool empty() const;

	/**
	 * Get number of records in result set.
	 * @return {size_t} Number of records.
	 */
	size_t length() const;

public:
	// Database table field names.
	std::vector<const char*> names_;

	// Query result record set. All elements db_row in it are dynamically added,
	// because when destructor is called, it automatically deletes all element
	// objects in rows_.
	std::vector<db_row*> rows_;

	// Temporary record collection.
	std::vector<const db_row*> rows_tmp_;

	// Store temporary result pointer.
	void* result_tmp_;

	// Function pointer for releasing temporary result.
	void (*result_free)(void* result);
};

class db_pool;
class query;

/**
 * Database operation handle base class.
 */
class ACL_CPP_API db_handle : public connect_client {
public:
	db_handle();
	virtual ~db_handle();

	/////////////////////////////////////////////////////////////////////

	/**
	 * Override connect_client virtual function implementation.
	 * @return {bool} Whether database connection was successful.
	 */
	bool open();

	/////////////////////////////////////////////////////////////////////

	/**
	 * Get database connection type.
	 * @return {const char*}
	 */
	virtual const char* dbtype() const = 0;

	/**
	 * Get last database operation error code.
	 * @return {int}
	 */
	virtual int get_errno() const {
		return -1;
	}

	/**
	 * Get last database operation error string.
	 * @return {const char*}
	 */
	virtual const char* get_error() const {
		return "unkonwn error";
	}

	/**
	 * Pure virtual interface. Subclasses must implement this interface to open
	 * database.
	 * @param charset {const char*} Character set used when connecting to database.
	 * @return {bool} Whether successful.
	 */
	virtual bool dbopen(const char* charset = NULL) = 0;

	/**
	 * Whether database is already opened.
	 * @return {bool} Returns true to indicate database is already opened.
	 */
	virtual bool is_opened() const = 0;

	/**
	 * Pure virtual interface. Subclasses must implement this interface to
	 * determine whether specified table exists.
	 * @return {bool} Whether exists.
	 */
	virtual bool tbl_exists(const char* tbl_name) = 0;

	/**
	 * Pure virtual interface. Subclasses must implement this interface to close
	 * database.
	 * @return {bool} Whether closing was successful.
	 */
	virtual bool close() = 0;

	/**
	 * Pure virtual interface. Subclasses must implement this interface to execute
	 * SELECT SQL statement.
	 * @param sql {const char*} Standard SQL statement, cannot be empty.
	 * Note: you must escape SQL statements to prevent SQL injection attacks.
	 * @param result {db_rows*} When not empty, stores query result in this result
	 * object. Otherwise, result is stored in a temporary storage inside db_handle.
	 * @return {bool} Whether execution was successful.
	 */
	virtual bool sql_select(const char* sql, db_rows* result = NULL) = 0;

	/**
	 * Pure virtual interface. Subclasses must implement this interface to execute
	 * INSERT/UPDATE/DELETE SQL statement.
	 * @param sql {const char*} Standard SQL statement, cannot be empty.
	 * Note: you must escape SQL statements to prevent SQL injection attacks.
	 * @return {bool} Whether execution was successful.
	 */
	virtual bool sql_update(const char* sql) = 0;

	/**
	 * Start transaction.
	 * @return {bool}
	 */
	virtual bool begin_transaction() { return false; }

	/**
	 * Commit transaction.
	 * @return {bool}
	 */
	virtual bool commit() { return false; }

	/**
	 * Rollback transaction.
	 * @return {bool}
	 */
	virtual bool rollback() { return false; }

	/**
	 * Execute safe query process. Calling this function is equivalent to
	 * sql_select, except query object's sql is safe, which can prevent
	 * sql injection. This function executes SELECT SQL statement.
	 * @param query {query&}
	 * @param result {db_rows*} When not empty, stores query result in this
	 * result object. Otherwise, result is stored in a temporary storage inside
	 * db_handle.
	 * @return {bool} Whether execution was successful.
	 */
	bool exec_select(query& query, db_rows* result = NULL);

	/**
	 * Execute safe update process. Calling this function is equivalent to
	 * sql_update, except query object's sql is safe, which can prevent sql
	 * injection. This function executes INSERT/UPDATE/DELETE SQL statement.
	 * @param query {query&}
	 * @return {bool} Whether execution was successful.
	 */
	bool exec_update(query& query);

	/**
	 * Interface: To prevent sql injection, users should call this function to
	 * escape string field values.
	 * This interface escapes common special characters in strings. Subclasses can
	 * also implement their own escaping methods.
	 * @param in {const char*} Input string.
	 * @param len {size_t} String length.
	 * @param out {string&} Store escaped result.
	 * @return {string&} Returns reference to out buffer for convenience, so users
	 * can chain calls when building SQL statements.
	 */
	virtual string& escape_string(const char* in, size_t len, string& out);

	/**
	 * Number of records affected by last sql operation.
	 * @return {int} Number of affected records. -1 indicates error.
	 */
	virtual int affect_count() const = 0;

	/////////////////////////////////////////////////////////////////////

	/**
	 * Get result of last executed SQL query.
	 * @return {const db_rows*} Returns result. When not empty, users need to call
	 *  free_result() to release result memory.
	 */
	const db_rows* get_result() const;

	/**
	 * Get matching record collection from query result record set by database
	 * table field name and corresponding field value.
	 * @param name {const char*} Database table field name (case-insensitive).
	 * @param value {const char*} Database table field value (case-insensitive).
	 * @return {const std::vector<const db_row*>*} Record collection object.
	 * Returns result. When not empty, users need to call free_result() to release
	 * result memory.
	 */
	const std::vector<const db_row*>* get_rows(
		const char* name, const char* value);

	/**
	 * Get all query results.
	 * @return {const std::vector<db_row*>*} Record collection object.
	 * Returns result. When not empty, users need to call free_result() to release
	 * result memory.
	 */
	const std::vector<db_row*>* get_rows() const;

	/**
	 * Get first row of last executed SQL query result. More convenient for unique
	 * data queries.
	 * @return {const db_row*} Returns empty to indicate query result is empty or
	 * error occurred.
	 * Users need to call free_result() to release result memory, otherwise memory
	 * leak will occur.
	 */
	const db_row* get_first_row() const;

	/**
	 * Release last query result. After query completes, call this function to
	 * release last query result. This function
	 * can be called multiple times without side effects, because when called
	 * again, it automatically sets internal
	 * result_ to empty. Additionally, to prevent memory leaks, users should call
	 * this function before each SQL query execution.
	 * When object is destroyed, it automatically calls this function to release
	 * any unreleased memory.
	 */
	void free_result();

	/**
	 * Get record corresponding to a certain subscript value.
	 * @param idx {size_t} Subscript value. Should be less than query result set
	 * size.
	 * @return {const db_row*} Returns empty, possibly because subscript is out of
	 * bounds,
	 *  or result is empty.
	 */
	const db_row* operator[](size_t idx) const;

	/**
	 * Get number of records in query (sql_select) result set.
	 * @return {size_t} Number of records. Returns 0 to indicate result is empty.
	 */
	size_t length() const;

	/**
	 * Whether query (sql_select) execution result is empty.
	 * @return {bool} Returns true to indicate query result is empty.
	 */
	bool empty() const;

	/**
	 * Print database query result.
	 * @param max {size_t} Maximum number of records to print. When this value is
	 * 0,
	 *  prints all results.
	 */
	void print_out(size_t max = 0) const;

	/////////////////////////////////////////////////////////////////
	/**
	 * Set instance's unique ID.
	 * @param id {const char*} Unique ID.
	 * @return {db_handle&}
	 */
	db_handle& set_id(const char* id);

	/**
	 * Get instance's unique ID.
	 * @return {const char*} Returns empty to indicate unique ID was not set.
	 */
	const char* get_id() const {
		return id_;
	}

	/**
	 * Set time when database connection handle was last used.
	 * @param now {time_t}
	 * @return {db_handle&}
	 */
	db_handle& set_stamp(time_t now);

	/**
	 * Get time when connection handle was last used.
	 * @return {time_t}
	 */
	time_t get_stamp() const {
		return when_;
	}

	/**
	 * When using dynamic library loading method, you need to call this function to
	 * set dynamic library's full path.
	 */
	static void set_loadpath(const char* path);

	/**
	 * When you need to get dynamic library's full path, you can get dynamic
	 * library's full path through this function.
	 * @return {const char*} Returns NULL when not set.
	 */
	static const char* get_loadpath();

protected:
	// Temporary result storage.
	db_rows* result_;

	// Instance unique ID.
	char* id_;

	// Time when database connection handle was last used.
	time_t when_;
};

} // namespace acl

#endif // !defined(ACL_DB_DISABLE)

