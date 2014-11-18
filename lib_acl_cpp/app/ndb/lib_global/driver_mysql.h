#pragma once
#include "db_driver.h"

struct ACL_DB_POOL;
class db_result;

class driver_mysql : public db_driver
{
public:
	driver_mysql(const char* dbaddr, const char* dbname,
		const char* dbuser, const char* dbpass,
		int dbpool_limit = 50, int dbpool_ping = 30,
		int dbpool_timeout = 60);
	virtual ~driver_mysql(void);

	/**
	 * 创建数据库及数据表，若数据库未存在则创建新的数据库，否则
	 * 在该数据库上创建新的数据表，若数据表也存在则在该数据表上创建
	 * 新的索引，若索引也存在，则直接返回正确
	 * @param dbname {const char*} 数据库名称
	 * @param tbl {const char* tbl} 数据表名
	 * @param idx {const char* idx} 数据表的索引名，若该索引值在数据表
	 *  不存在则创建新的基于该索引名的索引，否则打开已经存在的索引名
	 * @param idx_unique {bool} 索引字段是否要求唯一性
	 * @param user {const char*} 打开该数据库的账号名
	 * @param pass {const char*} 打开该数据库的账号密码
	 * @return {bool} 创建是否成功
	 */
	virtual  bool create(const char* dbname, const char* tbl,
			const char* idx, bool idx_unique = false,
			const char* user = NULL, const char* pass = NULL);

	/**
	 * 打开数据库及数据表，如果数据库、数据表或索引不存在则返回失败
	 * @param dbname {const char*} 数据库名称
	 * @param tbl {const char* tbl} 数据表名
	 * @param idx {const char*} 索引字段名
	 * @param user {const char*} 打开该数据库的账号名
	 * @param pass {const char*} 打开该数据库的账号密码
	 * @return {bool} 创建是否成功
	 */
	virtual bool open(const char* dbname, const char*tbl, const char* idx,
			const char* user = NULL, const char* pass = NULL);

	/**
	 * 添加或修改数据，当索引值对应数据不存在时则添加数据，否则修改
	 * 为新的数据
	 * @param idx_value {const char*} 索引值，必须对应 open 函数中的索引名
	 * @param data {cost void*} 数据地址
	 * @param dlen {size_t} data 数据长度
	 * @return {bool} 返回 false 表示失败，原因请调用 last_error() 获得
	 */
	virtual bool set(const char* idx_value, const void* data, size_t dlen);

	/**
	 * 根据输入的索引值获得数据
	 * @param idx_value {const char*} 索引值，对应 open 打开的索引字段
	 * @return {db_result*} 存储查询结果，如果返回值非空则表示查询成功，
	 *  需要进一步调用 db_result 对象中的函数来获得查询结果；如果返回
	 *  NULL 则表示出错，需要调用 last_error() 获得出错原因
	 */
	virtual db_result* get(const char* idx_value);

	/**
	 * 根据输入的索引值删除数据，同时删除与该数据相关的其它的索引项的值
	 * @param idx_value {const char*} 对应 open 中的索引字段的索引值
	 * @return {bool} 返回 false 表示失败，原因请调用 last_error() 获得，
	 *  删除的条数请调用 affect_count() 获得
	 */
	virtual bool del(const char* idx_value);

	/**
	 * 在调用 set/del 操作时调用此函数可以获得所影响的记录的条数，对于 get
	 * 操作没有必要调用此函数
	 * @return {int} 返回值 >= 0 表示影响记录的条数
	 */
	virtual size_t affect_count() const;

	/**
	 * 调用此函数获得上次操作的错误号
	 * @return {db_error_t} 错误号类型
	 */
	virtual db_error_t last_error() const;

private:
	ACL_DB_POOL* dbpool_;
};
