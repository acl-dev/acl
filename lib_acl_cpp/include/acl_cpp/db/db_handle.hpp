#pragma once
#include "../acl_cpp_define.hpp"
#include <vector>
#include "../stdlib/string.hpp"
#include "../stdlib/noncopyable.hpp"
#include "../connpool/connect_client.hpp"

#if !defined(ACL_DB_DISABLE)

namespace acl {

/**
 * 数据库查询结果集的行记录类型定义
 */
class ACL_CPP_API db_row : public noncopyable
{
public:
	/**
	 * 构造函数
	 * @param names {const std::vector<const char*>&} 数据库表中字段名列表
	 */
	db_row(const std::vector<const char*>& names);
	~db_row(void);

	/**
	 * 取得数据表中的某个对应下标值的字段名
	 * @param ifield {size_t} 下标值
	 * @return {const char*} 返回空说明该下标值越界
	 */
	const char* field_name(size_t ifield) const;

	/**
	 * 从查询结果的记录行中根据字段名取得相应的字段值
	 * @param name {const char*} 数据表的字段名
	 * @return {const char*} 对应的字段值，为空则表示字段值不存在或
	 *  字段名非法
	 */
	const char* field_value(const char* name) const;

	/**
	 * 从查询结果的记录行中根据字段名取得相应的字段值，
	 * 功能与 field_value 相同
	 * @param name {const char*} 数据表的字段名
	 * @return {const char*} 对应的字段值，为空则表示字段值不存在或
	 *  字段名非法
	 */
	const char* operator[](const char* name) const;

	/**
	 * 从查询结果的记录行中取得对应下标的字段值
	 * @param ifield {size_t} 下标值，该值应 < 字段名的个数
	 * @return {const char*} 对应的字段值，为空则表示下标值非法或
	 *  字段值不存在
	 */
	const char* field_value(size_t ifield) const;

	/**
	 * 从查询结果的记录行中取得对应下标的字段值，功能与 field_value 相同
	 * @param ifield {size_t} 下标值，该值应 < 字段名的个数
	 * @return {const char*} 对应的字段值，为空则表示下标值非法或
	 *  字段值不存在
	 */
	const char* operator[](size_t ifield) const;

	/**
	 * 从查询结果的记录行中取得对应下标的整数类型的字段值
	 * @param ifield {size_t} 下标值
	 * @param null_value {int} 当结果为空时，返回此值表示未有相应结果
	 * @return {int} 当返回值与用户输入的 null_value 相同表明没有查到结果
	 */
	int field_int(size_t ifield, int null_value = 0) const;

	/**
	 * 从查询结果的记录行中取得字段名的整数类型的字段值
	 * @param name {const char*} 下标值
	 * @param null_value {int} 当结果为空时，返回此值表示未有相应结果
	 * @return {int} 当返回值与用户输入的 null_value 相同表明没有查到结果
	 */
	int field_int(const char* name, int null_value = 0) const;

	
	/**
	 * 从查询结果的记录行中取得对应下标的整数类型的字段值
	 * @param ifield {size_t} 下标值
	 * @param null_value {acl_int64} 当结果为空时，返回此值表示未有相应结果
	 * @return {acl_int64} 当返回值与用户输入的 null_value 值相同时表明
	 *  没有查到结果
	 */
#if defined(_WIN32) || defined(_WIN64)
	__int64 field_int64(size_t ifield, __int64 null_value = 0) const;
#else
	long long int field_int64(size_t ifield,
		long long int null_value = 0) const;
#endif

	/**
	 * 从查询结果的记录行中取得字段名的整数类型的字段值
	 * @param name {const char*} 下标值
	 * @param null_value {acl_int64} 当结果为空时，返回此值表示未有相应结果
	 * @return {acl_int64} 当返回值与用户输入的 null_value 值相同时表明
	 *  没有查到结果
	 */
#if defined(_WIN32) || defined(_WIN64)
	__int64 field_int64(const char* name, __int64 null_value = 0) const;
#else
	long long int field_int64(const char* name,
		long long int null_value = 0) const;
#endif

	/**
	 * 从查询结果的记录行中取得字段名的浮点类型的字段值
	 * @param ifield {size_t} 下标值
	 * @param null_value {double} 当结果为空时，返回此值表示未有相应结果
	 * @return {double} 当返回值与用户输入的 null_value 值相同时表明没有
	 *  查到结果
	 */
	double field_double(size_t ifield, double null_value = 0.0) const;

	/**
	 * 从查询结果的记录行中取得字段名的浮点类型的字段值
	 * @param name {const char*} 下标值
	 * @param null_value {double} 当结果为空时，返回此值表示未有相应结果
	 * @return {double} 当返回值与用户输入的 null_value 值相同时表明没有
	 *  查到结果
	 */
	double field_double(const char* name, double null_value = 0.0) const;

	/**
	 * 从查询结果的记录行中取得对应下标的字符串类型的字段值
	 * @param ifield {size_t} 下标值
	 * @return {const char*} 当返回值 NULL 时表明没有查到结果
	 */
	const char* field_string(size_t ifield) const;

	/**
	 * 从查询结果的记录行中取得字段名的字符串类型的字段值
	 * @param name {const char*} 下标值
	 * @return {const char*} 当返回值 NULL 时表明没有查到结果
	 */
	const char* field_string(const char* name) const;

	/**
	 * 从查询结果的记录行中取得对应下标的字符串类型的字段值长度
	 * @param ifield {size_t} 下标值
	 * @return {size_t}
	 */
	size_t field_length(size_t ifield) const;
	/**
	 * 从查询结果的记录行中取得字段名的字符串类型的字段值长度
	 * @param name {const char*} 下标值
	 * @return {size_t}
	 */
	size_t field_length(const char* name) const;

	/**
	 * 向记录行添加一个字段值，添加字段值的顺序应该与字段名的顺序一致
	 * @param value {const char*} 该行记录的某个字段值
	 * @param len {size_t} value 数据长度
	 */
	void push_back(const char* value, size_t len);

	/**
	 * 行记录中字段值的个数
	 * @return {size_t}
	 */
	size_t length(void) const;

	/**
	 * 清除结果值（即 values_）
	 */
	void clear(void);

private:
	// 数据表的字段名集合的引用
	const std::vector<const char*>& names_;

	// 数据结果行的字段集合
	std::vector<const char*> values_;

	// 数据结果行字段长度集合
	std::vector<size_t> lengths_;
};

/**
 * 数据库查询结果的行记录集合类型定义
 */
class ACL_CPP_API db_rows : public noncopyable
{
public:
	db_rows();
	virtual ~db_rows();

	/**
	 * 从查询的行记录集合中根据表字段名对应的字段值取出结果记录集合
	 * @param name {const char*} 数据表字段名(不区分大小写)
	 * @param value {const char*} 数据表字段值(区分大小写)
	 * @return {const std::vector<const db_row*>&} 返回行记录集合类型对象，
	 *  可以通过调用 db_rows.empty() 来判断结果是否为空
	 */
	const std::vector<const db_row*>& get_rows(
		const char* name, const char* value);

	/**
	 * 取得所有的查询结果集
	 * @return {const std::vector<db_row*>&} 返回行记录集合类型对象，
	 *  可以通过调用 db_rows.empty() 来判断结果是否为空
	 */
	const std::vector<db_row*>& get_rows() const;

	/**
	 * 从查询的行记录集合中根据索引下标取得对应的某行记录
	 * @param idx {size_t} 索引下标，该值应该 < 结果集大小
	 * @return {const db_row*} 返回空表示输入下标值非法或字段值本身
	 *  为空
	 */
	const db_row* operator[](size_t idx) const;

	/**
	 * 判断结果集是否为空
	 * @return {bool} 是否为空
	 */
	bool empty() const;

	/**
	 * 结果集的行记录个数
	 * @return {size_t} 行记录个数
	 */
	size_t length() const;

public:
	// 数据表字段名
	std::vector<const char*> names_;

	// 查询结果行集合，其中的元素 db_row 必须是动态添加进去的，
	// 因为在本类对象析构时会自动 delete rows_ 中的所有元素对象
	std::vector<db_row*> rows_;

	// 临时结果行集合
	std::vector<const db_row*> rows_tmp_;

	// 存储临时结果集对象
	void* result_tmp_;

	// 用来释放临时结果集对象
	void (*result_free)(void* result);
};

class db_pool;
class query;

/**
 * 数据库操作句柄对象类型
 */
class ACL_CPP_API db_handle : public connect_client
{
public:
	db_handle(void);
	virtual ~db_handle(void);

	/////////////////////////////////////////////////////////////////////

	/**
	 * 基类 connect_client 虚函数的实现
	 * @return {bool} 打开数据库连接是否成功
	 */
	bool open();

	/////////////////////////////////////////////////////////////////////

	/**
	 * 返回数据库的类型描述
	 * @return {const char*}
	 */
	virtual const char* dbtype() const = 0;

	/**
	 * 获得上次数据库操作的出错错误号
	 * @return {int}
	 */
	virtual int get_errno() const
	{
		return -1;
	}

	/**
	 * 获得上次数据库操作的出错错描述
	 * @return {const char*}
	 */
	virtual const char* get_error() const
	{
		return "unkonwn error";
	}

	/**
	 * 纯虚接口，子类必须实现此接口用于打开数据库
	 * @param charset {const char*} 打开数据库连接时采用的字符集
	 * @return {bool} 打开是否成功
	 */
	virtual bool dbopen(const char* charset = NULL) = 0;

	/**
	 * 数据库是否已经打开了
	 * @return {bool} 返回 true 表明数据库已经打开了
	 */
	virtual bool is_opened() const = 0;

	/**
	 * 纯虚接口，子类必须实现此接口用于判断数据表是否存在
	 * @return {bool} 是否存在
	 */
	virtual bool tbl_exists(const char* tbl_name) = 0;

	/**
	 * 纯虚接口，子类必须实现此接口用于关闭数据库
	 * @return {bool} 关闭是否成功
	 */
	virtual bool close() = 0;

	/**
	 * 纯虚接口，子类必须实现此接口用于执行 SELECT SQL 语句
	 * @param sql {const char*} 标准的 SQL 语句，非空，并且一定得要注意该
	 *  SQL 语句必须经过转义处理，以防止 SQL 注入攻击
	 * @param result {db_rows*} 如果非空，则将查询结果填充进该结果对象中，
	 *  否则，会引用 db_handle 内部的一个临时存储对象
	 * @return {bool} 执行是否成功
	 */
	virtual bool sql_select(const char* sql, db_rows* result = NULL) = 0;

	/**
	 * 纯虚接口，子类必须实现此接口用于执行 INSERT/UPDATE/DELETE SQL 语句
	 * @param sql {const char*} 标准的 SQL 语句，非空，并且一定得要注意该
	 *  SQL 语句必须经过转义处理，以防止 SQL 注入攻击
	 * @return {bool} 执行是否成功
	 */
	virtual bool sql_update(const char* sql) = 0;

	/**
	 * 开始执行事务
	 * @return {bool}
	 */
	virtual bool begin_transaction() { return false; }

	/**
	 * 提交事务
	 * @return {bool}
	 */
	virtual bool commit() { return false; }

	/**
	 * 事务回滚
	 * @return {bool}
	 */
	virtual bool rollback() { return false; }

	/**
	 * 更安全易用的查询过程，调用此函数功能等同于 sql_select，只是查询
	 * 对象 query 构建的 sql 语句是安全的，可以防止 sql 注入，该方法
	 * 执行 SELECT SQL 语句
	 * @param query {query&}
	 * @param result {db_rows*} 如果非空，则将查询结果填充进该结果对象中，
	 *  否则，会引用 db_handle 内部的一个临时存储对象
	 * @return {bool} 执行是否成功
	 */
	bool exec_select(query& query, db_rows* result = NULL);

	/**
	 * 更安全易用的更新过程，调用此函数功能等同于 sql_update，只是查询
	 * 对象 query 构建的 sql 语句是安全的，可以防止 sql 注入，该方法
	 * 执行 INSERT/UPDATE/DELETE SQL 语句
	 * @param query {query&}
	 * @return {bool} 执行是否成功
	 */
	bool exec_update(query& query);

	/**
	 * 虚接口，为防止 sql 注入，用户应针对字符串字段调用此函数将一些特殊
	 * 字符进行转义，该接口对常见的特殊字符进行了转义，子类也可以实现自己
	 * 的转义方法
	 * @param in {const char*} 输入字符串
	 * @param len {size_t} 字符串长度
	 * @param out {string&} 存储转换结果
	 * @return {string&} 子类应该返回输入的缓冲区的引用，以便于用户在拼接
	 *  SQL 的时候比较方便
	 */
	virtual string& escape_string(const char* in, size_t len, string& out);

	/**
	 * 上次 sql 操作影响的记录行数
	 * @return {int} 影响的行数，-1 表示出错
	 */
	virtual int affect_count() const = 0;

	/////////////////////////////////////////////////////////////////////

	/**
	 * 获得执行 SQL 语句后的结果
	 * @return {const db_rows*}，返回结果若非空，则用完后需要调用
	 *  free_result() 以释放结果对象
	 */
	const db_rows* get_result() const;

	/**
	 * 从查询的行记录集合中根据表字段名对应的字段值取出结果记录集合
	 * @param name {const char*} 数据表字段名(不区分大小写)
	 * @param value {const char*} 数据表字段值(区分大小写)
	 * @return {const std::vector<const db_row*>*} 返回行记录集合类型对象，
	 *  若返回结果集非空，则必须调用 free_result() 以释放结果对象
	 */
	const std::vector<const db_row*>* get_rows(
		const char* name, const char* value);

	/**
	 * 取得所有的查询结果集
	 * @return {const std::vector<db_row*>*} 返回行记录集合类型对象，
	 *  若返回结果集非空，则必须调用 free_result() 以释放结果对象
	 */
	const std::vector<db_row*>* get_rows() const;

	/**
	 * 获得执行 SQL 语句后的第一行结果，针对唯一键的数据查询比较方便
	 * @return {const db_row*} 返回空表示查询结果为空，否则， 则必须调用
	 * free_result() 函数来释放中间的结果内存，否则会引起内存泄露
	 */
	const db_row* get_first_row() const;

	/**
	 * 释放上次查询的结果，当查询完成后，调用该函数来释放上次查询的结果，该函数被
	 * 多次调用并无害处，因为当第一次调用时会自动将内部变量 result_ 置空,
	 * 另外，要求子类必须在每次执行 SQL 查询前先调用此方法，以免用户忘记
	 * 调用而造成内存泄露；此外，本类对象在析构时会自动再调用本方法释放可能
	 * 未释放的内存
	 */
	void free_result();

	/**
	 * 获得某个对应下标值的行记录
	 * @param idx {size_t} 下标值，必须小于查询结果的总数
	 * @return {const db_row*} 结果，如果为空，则有可能是下标越界，
	 *  也有可能是结果为空
	 */
	const db_row* operator[](size_t idx) const;

	/**
	 * 取得查询(sql_select)结果的行记录数
	 * @return {size_t} 结果行记录数，若为 0 则表示结果为空
	 */
	size_t length() const;

	/**
	 * 查询(sql_select)执行完后结果是否为空
	 * @return {bool} 返回 true 表示查询结果为空
	 */
	bool empty() const;

	/**
	 * 输出数据库查询结果
	 * @param max {size_t} 输出至屏幕的行记录数的最大值限制，如果该值为 0
	 *  则输出所有的结果集
	 */
	void print_out(size_t max = 0) const;

	/////////////////////////////////////////////////////////////////
	/**
	 * 设置本实例的唯一 ID
	 * @param id {const char*} 唯一 ID
	 * @return {db_handle&}
	 */
	db_handle& set_id(const char* id);

	/**
	 * 获得本实例的唯一 ID
	 * @return {const char*} 为空时，表示未曾设置过唯一ID
	 */
	const char* get_id() const
	{
		return id_;
	}

	/**
	 * 设置本数据库连接句柄当前被使用的时间
	 * @param now {time_t}
	 * @return {db_handle&}
	 */
	db_handle& set_when(time_t now);

	/**
	 * 获得该连接句柄上次被使用的时间
	 * @return {time_t}
	 */
	time_t get_when() const
	{
		return when_;
	}

	/**
	 * 当采用动态加载方式加载动态库时，可以使用此函数设置动态库的加载全路径
	 */
	static void set_loadpath(const char* path);

	/**
	 * 当设置了动态库的动态加载全路径时，可以通过本函数获得动态库加载全路径
	 * @return {const char*} 当未设置时则返回 NULL
	 */
	static const char* get_loadpath();

protected:
	// 临时结果对象
	db_rows* result_;

	// 实例唯一 ID
	char* id_;

	// 该数据库连接句柄最近被使用的时间
	time_t when_;
};

} // namespace acl

#endif // !defined(ACL_DB_DISABLE)
