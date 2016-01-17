#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include <list>
#include "acl_cpp/db/db_handle.hpp"
#include "acl_cpp/connpool/connect_pool.hpp"

namespace acl {

class db_handle;
class locker;

class ACL_CPP_API db_pool : public connect_pool
{
public:
	/**
	 * 数据库构造函数
	 * @param dbaddr {const char*} 数据库地址
	 * @param count {size_t} 连接池最大连接个数限制
	 * @param idx {size_t} 该连接池对象在集合中的下标位置(从 0 开始)
	 */
	db_pool(const char* dbaddr, size_t count, size_t idx = 0);
	virtual ~db_pool() {};

	/**
	 * 从数据库连接池获得一个数据库对象，并且要求打开数据库连接，即用户不必
	 * 显式地再调用 db_handle::open 过程；
	 * 用完后必须调用 db_pool->put(db_handle*) 将连接归还至数据库连接池，
	 * 由该函数获得的连接句柄不能 delete，否则会造成连接池的内部计数器出错
	 * @param charset {const char*} 打开数据库时采用的字符集
	 * @return {db_handle*} 数据库连接对象，返回空表示出错
	 */
	db_handle* peek_open(const char* charset = NULL);

	/**
	 * 获得当前数据库连接池的最大连接数限制
	 * @return {size_t}
	 */
	size_t get_dblimit() const
	{
		return get_max();
	}

	/**
	 * 获得当前数据库连接池当前的连接数
	 * @return {size_t}
	 */
	size_t get_dbcount() const
	{
		return get_count();
	}

	/**
	 * 设置数据库连接池中空闲连接的生存周期(秒)
	 * @param ttl {int} 生存周期(秒)
	 */
	void set_idle(int ttl)
	{
		set_idle_ttl(ttl);
	}
};

class ACL_CPP_API db_guard : public connect_guard
{
public:
	db_guard(db_pool& pool) : connect_guard(pool) {}
	~db_guard(void);
};

} // namespace acl
