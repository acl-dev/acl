#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include <list>

namespace acl {

class db_handle;
class locker;

class ACL_CPP_API db_pool
{
public:
	/**
	 * 数据库构造函数
	 * @param dblimit {int} 数据库连接池的最大连接数限制
	 */
	db_pool(int dblimit = 64);
	virtual ~db_pool();

	/**
	 * 从数据库连接池获得一个数据库对象，该函数返回数据库对象，用户需调用
	 * db_handle::open 打开数据库连接，然后再使用该数据库连接对象；
	 * 用完后必须调用 db_pool->put(db_handle*) 将连接归还至数据库连接池，
	 * 由该函数获得的连接句柄不能 delete，否则会造成连接池的内部计数器出错
	 * @return {db_handle*} 返回空，则表示出错
	 */
	db_handle* peek();

	/**
	 * 从数据库连接池获得一个数据库对象，并且要求打开数据库连接，即用户不必
	 * 显式地再调用 db_handle::open 过程；
	 * 用完后必须调用 db_pool->put(db_handle*) 将连接归还至数据库连接池，
	 * 由该函数获得的连接句柄不能 delete，否则会造成连接池的内部计数器出错
	 * @param charset {const char*} 打开数据库时使用的字符集
	 * @return {db_handle*} 数据库连接对象，返回空表示出错
	 */
	db_handle* peek_open(const char* charset = "utf8");

	/**
	 * 将数据库连接放回至连接池中，当从数据库连接池中获得连接
	 * 句柄用完后应该通过该函数放回，不能直接 delete，因为那样
	 * 会导致连接池的内部记数发生错误
	 * @param conn {db_handle*} 数据库连接句柄，该连接句柄可以
	 *  是由 peek 创建的，也可以单独动态创建的
	 * @param keep {bool} 归还给连接池的数据库连接句柄是否继续
	 *  保持连接，如果否，则内部会自动删除该连接句柄
	 */
	void put(db_handle* conn, bool keep = true);

	/**
	 * 将数据库连接池中的过期连接释放掉，以减少对后端数据库的连接数
	 * @param ttl {time_t} 当数据库连接的空闲时间大于等于此值时
	 *  该连接将被释放；当 idle 为 0 时则需要释放所有的数据库连接;
	 *  当为 -1 时则不进行释放连接操作
	 * @param exclusive {bool} 内部是否需要加互斥锁
	 * @return {int} 被释放的数据库连接的个数(>= 0)
	 */
	int check_idle(time_t ttl, bool exclusive = true);

	/**
	 * 设置连接池中空闲连接的生存周期，当通过本函数设置了数据库空闲连接
	 * 的生存周期后(idle >= 0)，则当用户调用 db_pool::put 时会自动检查
	 * 过期连接并关闭，否则内部空闲连接一直保持
	 * @param idle {int} 连接池中空闲连接的连接存活时间，当该值
	 *  为 -1 时，表示不处理空闲连接，为 0 时表示内部不保留任何长连接
	 * @return {db_pool&}
	 */
	db_pool& set_idle(int idle);

	/**
	 * 设置自动检查空闲连接的时间间隔，缺省值为 30 秒
	 * @param n {int} 时间间隔
	 * @return {db_pool&}
	 */
	db_pool& set_check_inter(int n);

	/**
	 * 获得当前数据库连接池的最大连接数限制
	 * @return {int}
	 */
	int get_dblimit() const
	{
		return dblimit_;
	}

	/**
	 * 获得当前数据库连接池当前的连接数
	 * @return {int}
	 */
	int get_dbcount() const
	{
		return dbcount_;
	}
protected:
	/**
	 * 纯虚函数：创建 DB 的方法
	 * @return {db_handle*}
	 */
	virtual db_handle* create() = 0;
private:
	std::list<db_handle*> pool_;
	int   dblimit_;  // 连接池的最大连接数限制
	int   dbcount_;  // 当前已经打开的连接数
	locker* locker_;
	char  id_[128];  // 本类实例的唯一 ID 标识
	time_t ttl_;     // 连接池中空闲连接被释放的超时值
	time_t last_check_;  // 上次检查空闲连接的时间截
	int    check_inter_; // 检查空闲连接的时间间隔

	// 设置本实例的唯一 ID 标识
	void set_id();
};

} // namespace acl
