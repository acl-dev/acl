#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include <map>

namespace acl
{

/**
 * SQL 查询语句查询器，该类会自动对 sql 中的一些特殊字符进行转义，使用方式类似于
 * java hibernate 的 SQL 语句构建方式
 */
class ACL_CPP_API query
{
public:
	query();
	~query();

	/**
	 * 创建 sql 语句，变参方式，用法和 printf 类似
	 * @param sql_fmt {const char*} sql 语句，格式如：
	 *  select * from xxx where name = :name and len >= %d
	 *  其中的 :name, 将由 set_parameter 中的值进行替换, len 为整形值
	 * @return {query&}
	 */
	query& create_sql(const char* sql_fmt, ...) ACL_CPP_PRINTF(2, 3);

	/**
	 * 创建 sql 语句，非变参方式
	 * @param sql {const char*}  sql 语句，格式如：
	 *  select * from xxx where name = :name and len >= :len
	 *  其中的 :name, :len 将由 set_parameter 中的值进行替换
	 * @return {query&}
	 */
	query& create(const char* sql);

	/**
	 * 设置字符串类型的变量值
	 * @param name {const char*} 变量名
	 * @param value {const char*} 变量值
	 * @return {query&}
	 */
	query& set_parameter(const char* name, const char *value);

	/**
	 * 设置字符类型的变量值
	 * @param name {const char*} 变量名
	 * @param value {char} 变量值
	 * @return {query&}
	 */
	query& set_parameter(const char* name, char value);

	/**
	 * 设置 16 位短整类型的变量值
	 * @param name {const char*} 变量名
	 * @param value {short} 变量值
	 * @return {query&}
	 */
	query& set_parameter(const char* name, short value);

	/**
	 * 设置 32 位短整类型的变量值
	 * @param name {const char*} 变量名
	 * @param value {int} 变量值
	 * @return {query&}
	 */
	query& set_parameter(const char* name, int value);

	/**
	 * 设置单精度浮点类型的变量值
	 * @param name {const char*} 变量名
	 * @param value {float} 单精度浮点类型
	 * @param precision {int} 尾数的精度值
	 * @return {query&}
	 */
	query& set_parameter(const char* name, float value, int precision = 8);

	/**
	 * 设置双精度浮点类型的变量值
	 * @param name {const char*} 变量名
	 * @param value {double} 双精度浮点类型
	 * @param precision {int} 尾数的精度值
	 * @return {query&}
	 */
	query& set_parameter(const char* name, double value, int precision = 8);

	/**
	 * 设置 64 位短整类型的变量值
	 * @param name {const char*} 变量名
	 * @param value {long long int} 变量值
	 * @return {query&}
	 */
#if defined(_WIN32) || defined(_WIN64)
	query& set_parameter(const char* name, __int64 value);
#else
	query& set_parameter(const char* name, long long int value);
#endif

	/**
	 * 设置日期(time_t)类型的变量值
	 * @param name {const char*} 变量名
	 * @param value {time_t} 变量值
	 * @param fmt {const char*} 日期格式
	 * @return {query&}
	 */
	query& set_date(const char* name, time_t value,
		const char* fmt = "%Y-%m-%d %H:%M:%S");

	/**
	 * 以变参方式设置变量值
	 * @param name {const char*} 变量名
	 * @param fmt {const char*} 变参值格式
	 * @return {query&}
	 */
	query& set_format(const char* name, const char* fmt, ...)
		ACL_CPP_PRINTF(3, 4);

	/**
	 * 以变参方式设置变量值
	 * @param name {const char*} 变量名
	 * @param fmt {const char*} 变参值格式
	 * @param ap {va_list} 变参值列表
	 * @return {query&}
	 */
	query& set_vformat(const char* name, const char* fmt, va_list ap);

	/**
	 * 对查询 sql 语句进行转义后返回给调用者
	 * @return {const string&}
	 */
	const string& to_string();

	/**
	 * 清空查询器上一次的缓存数据，当该 SQL 查询器对象被多次使用时，应该提前调用
	 * 本函数清除之前的 SQL 查询器状态
	 */
	void reset();

	/**
	 * 对 sql 中的一些特殊字符进行转义处理，以防止 SQL 注入问题
	 * @param in {const char*} 变量值
	 * @param len {size_t} in 数据长度
	 * @param out {string&} 存储转换后的结果的缓冲区，该参数输入后会先被清空
	 * @return {const string&} 转义处理后的结果(其实是 out 的地址引用)
	 */
	static const string& escape(const char* in, size_t len, string& out);

	/**
	 * 将时间转换成 DateTime 格式的字符串(YYYY-MM-DD HH:MM:SS)
	 * @param t {time_t} 时间截
	 * @param out {string&} 存储转换结果的缓冲区
	 * @param fmt {const char*} 日期格式，在 _WIN32 下必须保证该格式的正确性，
	 *  否则 _WIN32 API 会产生断言，格式如："%Y-%m-%d %H:%M:%S"
	 * @return {const char*} 转换后缓冲区地址，若返回 NULL 则表示转换失败
	 */
	static const char* to_date(time_t t, string& out,
		const char* fmt = "%Y-%m-%d %H:%M:%S");

private:
	typedef enum
	{
		DB_PARAM_CHAR,
		DB_PARAM_SHORT,
		DB_PARAM_INT32,
		DB_PARAM_INT64,
		DB_PARAM_FLOAT,
		DB_PARAM_DOUBLE,
		DB_PARAM_STR
	} db_param_type;

	struct query_param
	{
		char type;
		int  dlen;
		int  precision;
		union
		{
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
