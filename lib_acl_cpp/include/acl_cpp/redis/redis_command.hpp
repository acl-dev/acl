#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include "acl_cpp/redis/redis_result.hpp"

namespace acl
{

class redis_client;

/**
 * redis 客户端命令类的纯虚父类
 */
class ACL_CPP_API redis_command
{
public:
	redis_command(redis_client* conn = NULL);
	virtual ~redis_command() = 0;

	/**
	 * 在重复使用一个继承于 redis_command 的子类操作 redis 时，需要在
	 * 下一次调用前调用本方法以释放上次操作的临时对象
	 */
	void reset();

	/**
	 * 在使用连接池方式时，通过本函数将从连接池获得的连接对象
	 * (redis_client)与 redis 客户端命令进行关联
	 */
	void set_client(redis_client* conn);

	/**
	 * 获得当前 redis 客户端命令的连接对象
	 * @return {redis_client*} 返回 NULL 表示没有连接对象与当前的命令对象
	 *  进行绑定
	 */
	redis_client* get_client() const
	{
		return conn_;
	}

	/**
	 * 获得当前结果结点的数据类型
	 * @return {redis_result_t}
	 */
	redis_result_t get_type() const;

	/**
	 * 当返回值为 REDIS_RESULT_STATUS 类型时，本方法返回状态信息
	 * @return {const char*} 返回 "" 表示出错
	 */
	const char* get_status() const;

	/**
	 * 当出错时返回值为 REDIS_RESULT_ERROR 类型，本方法返回出错信息
	 * @return {const char*} 返回空串 "" 表示没有出错信息
	 */
	const char* get_error() const;

	/**
	 * 获得当前结果结点存储的对象的个数, 该方法可以获得结果为下面两个方法
	 * (get_child/get_value) 所需要的数组元素的个数
	 * @return {size_t} 返回值与存储类型的对应关系如下：
	 *  REDIS_RESULT_ERROR: 1
	 *  REDIS_RESULT_STATUS: 1
	 *  REDIS_RESULT_INTEGER: 1
	 *  REDIS_RESULT_STRING: > 0 时表示该字符串数据被切分成非连接内存块的个数
	 *  REDIS_RESULT_ARRAY: children_->size()
	 */
	size_t get_size() const;

	/**
	 * 当返回值为 REDIS_RESULT_INTEGER 类型时，本方法返回对应的 32 位整数值
	 * @param success {bool*} 本指针非 NULL 时记录操作过程是否成功
	 * @return {int}
	 */
	int get_integer(bool* success = NULL) const;

	/**
	 * 当返回值为 REDIS_RESULT_INTEGER 类型时，本方法返回对应的 64 位整数值
	 * @param success {bool*} 本指针非 NULL 时记录操作过程是否成功
	 * @return {long long int}
	 */
	long long int get_integer64(bool* success = NULL) const;

	/**
	 * 返回对应下标的数据(当数据类型非 REDIS_RESULT_ARRAY 时）
	 * @param i {size_t} 数组下标
	 * @param len {size_t*} 当为非 NULL 指针时存储所返回数据的长度
	 * @return {const char*} 返回 NULL 表示下标越界
	 */
	const char* get(size_t i, size_t* len = NULL) const;

	/**
	 * 判断当前所绑定的 redis 连接流对象(redis_client) 连接是否已经关闭；
	 * 只有内部的 conn_ 流对象非空时调用此函数才有意义
	 * @return {bool}
	 */
	bool eof() const;

	/**
	 * 获得本次 redis 操作过程的结果
	 * @return {redis_result*}
	 */
	const redis_result* get_result() const;

	/**
	 * 当查询结果为数组对象时调用本方法获得一个数组元素对象
	 * @param i {size_t} 数组对象的下标值
	 * @return {const redis_result*} 当结果非数组对象或结果为空或出错时
	 *  该方法返回 NULL
	 */
	const redis_result* get_child(size_t i) const;

	/**
	 * 当从 redis-server 获得的数据是一组字符串类型的结果集时，可以调用
	 * 本函数获得某个指定下标位置的数据
	 * @param i {size_t} 下标（从 0 开始）
	 * @param len {size_t*} 若该指针非空，则存储所返回结果的长度（仅当该
	 *  方法返回非空指针时有效）
	 * @return {const char*} 返回对应下标的值，当返回 NULL 时表示该下标没
	 *  有值，为了保证使用上的安全性，返回的数据总能保证最后是以 \0 结尾，
	 *  在计算数据长度时不包含该结尾符，但为了兼容二进制情形，调用者还是
	 *  应该通过返回的 len 存放的长度值来获得数据的真实长度
	 */
	const char* get_value(size_t i, size_t* len = NULL) const;

protected:
	redis_client* conn_;

	const redis_result** scan_keys(const char* cmd, const char* key,
		int& cursor, size_t& size, const char* pattern,
		const size_t* count);
};

} // namespace acl
