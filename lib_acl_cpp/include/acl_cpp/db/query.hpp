#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/string.hpp"
#include "../stdlib/noncopyable.hpp"
#include <map>

#if !defined(ACL_DB_DISABLE)

namespace acl {

/**
 * SQL query statement query builder. This class automatically escapes some
 * special characters in sql. Usage is similar to
 * java hibernate's SQL statement building method
 */
class ACL_CPP_API query : public noncopyable {
public:
	query();
	~query();

	/**
	 * Create sql statement, variable argument method, usage similar to printf
	 * @param sql_fmt {const char*} sql statement, format:
	 *  select * from xxx where name = :name and len >= %d
	 * where :name will be replaced by value in set_parameter, len is integer value
	 * @return {query&}
	 */
	query& create_sql(const char* sql_fmt, ...) ACL_CPP_PRINTF(2, 3);

	/**
	 * Create sql statement, non-variable argument method
	 * @param sql {const char*}  sql statement, format:
	 *  select * from xxx where name = :name and len >= :len
	 *  where :name, :len will be replaced by values in set_parameter
	 * @return {query&}
	 */
	query& create(const char* sql);

	/**
	 * Set variable value of string type
	 * @param name {const char*} Variable name
	 * @param value {const char*} Variable value
	 * @return {query&}
	 */
	query& set_parameter(const char* name, const char *value);

	/**
	 * Set variable value of char type
	 * @param name {const char*} Variable name
	 * @param value {char} Variable value
	 * @return {query&}
	 */
	query& set_parameter(const char* name, char value);

	/**
	 * Set variable value of 16-bit short integer type
	 * @param name {const char*} Variable name
	 * @param value {short} Variable value
	 * @return {query&}
	 */
	query& set_parameter(const char* name, short value);

	/**
	 * Set variable value of 32-bit short integer type
	 * @param name {const char*} Variable name
	 * @param value {int} Variable value
	 * @return {query&}
	 */
	query& set_parameter(const char* name, int value);

	/**
	 * Set variable value of single precision floating point type
	 * @param name {const char*} Variable name
	 * @param value {float} Single precision floating point type
	 * @param precision {int} Mantissa precision value
	 * @return {query&}
	 */
	query& set_parameter(const char* name, float value, int precision = 8);

	/**
	 * Set variable value of double precision floating point type
	 * @param name {const char*} Variable name
	 * @param value {double} Double precision floating point type
	 * @param precision {int} Mantissa precision value
	 * @return {query&}
	 */
	query& set_parameter(const char* name, double value, int precision = 8);

	/**
	 * Set variable value of 64-bit short integer type
	 * @param name {const char*} Variable name
	 * @param value {long long int} Variable value
	 * @return {query&}
	 */
#if defined(_WIN32) || defined(_WIN64)
	query& set_parameter(const char* name, __int64 value);
#else
	query& set_parameter(const char* name, long long int value);
#endif

	/**
	 * Set variable value of date (time_t) type
	 * @param name {const char*} Variable name
	 * @param value {time_t} Variable value
	 * @param fmt {const char*} Date format
	 * @return {query&}
	 */
	query& set_date(const char* name, time_t value,
		const char* fmt = "%Y-%m-%d %H:%M:%S");

	/**
	 * Set variable value in variable argument method
	 * @param name {const char*} Variable name
	 * @param fmt {const char*} Variable argument value format
	 * @return {query&}
	 */
	query& set_format(const char* name, const char* fmt, ...)
		ACL_CPP_PRINTF(3, 4);

	/**
	 * Set variable value in variable argument method
	 * @param name {const char*} Variable name
	 * @param fmt {const char*} Variable argument value format
	 * @param ap {va_list} Variable argument value list
	 * @return {query&}
	 */
	query& set_vformat(const char* name, const char* fmt, va_list ap);

	/**
	 * Return escaped query sql statement to caller
	 * @return {const string&}
	 */
	const string& to_string();

	/**
	 * Clear cached data from previous query. When this SQL query builder object is
	 * used multiple times, should call
	 * this function in advance to clear previous SQL query builder state
	 */
	void reset();

	/**
	 * Escape some special characters in sql to prevent SQL injection problems
	 * @param in {const char*} Variable value
	 * @param len {size_t} in data length
	 * @param out {string&} Buffer for storing converted result. This parameter
	 * will be cleared first after input
	 * @return {const string&} Escaped result (actually reference to out address)
	 */
	static const string& escape(const char* in, size_t len, string& out);

	/**
	 * Convert time to DateTime format string (YYYY-MM-DD HH:MM:SS)
	 * @param t {time_t} Timestamp
	 * @param out {string&} Buffer for storing conversion result
	 * @param fmt {const char*} Date format. On _WIN32, must ensure correctness of
	 * this format,
	 *  otherwise _WIN32 API will generate assertion. Format: "%Y-%m-%d %H:%M:%S"
	 * @return {const char*} Converted buffer address. Returns NULL indicates
	 * conversion failed
	 */
	static const char* to_date(time_t t, string& out,
		const char* fmt = "%Y-%m-%d %H:%M:%S");

private:
	typedef enum {
		DB_PARAM_CHAR,
		DB_PARAM_SHORT,
		DB_PARAM_INT32,
		DB_PARAM_INT64,
		DB_PARAM_FLOAT,
		DB_PARAM_DOUBLE,
		DB_PARAM_STR
	} db_param_type;

	struct query_param {
		char type;
		int  dlen;
		int  precision;
		union {
			char  c;
			short s;
			int   n;
			long long int l;
			double d;
			float f;
			char  S[1];
		} v;
	};

	std::map<string, query_param*> params_;
	string* sql_buf_;
	string sql_;
	string buf_;

	void del_param(const string& key);
	bool append_key(string& buf, char* key);
};

} // namespace acl

#endif // !defined(ACL_DB_DISABLE)

