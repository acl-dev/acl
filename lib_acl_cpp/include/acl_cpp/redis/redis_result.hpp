#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include <vector>

namespace acl
{

typedef enum
{
	REDIS_RESULT_NIL,
	REDIS_RESULT_ERROR,
	REDIS_RESULT_STATUS,
	REDIS_RESULT_INTEGER,
	REDIS_RESULT_STRING,
	REDIS_RESULT_ARRAY,
} redis_result_t;

class dbuf_pool;
class redis_client;

class ACL_CPP_API redis_result
{
public:
	redis_result(dbuf_pool* pool);

	/**
	 * 重载了 new/delete 操作符，在 new 新对象时，使内存的分配在内存池进行分配
	 */
	void *operator new(size_t size, dbuf_pool* pool);
	void operator delete(void* ptr, dbuf_pool* pool);

	/**
	 * 获得当前结果结点的数据类型
	 * @return {redis_result_t}
	 */
	redis_result_t get_type() const
	{
		return result_type_;
	}

	/**
	 * 获得当前结果结点存储的对象的个数
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
	 * 当返回值为 REDIS_RESULT_STATUS 类型时，本方法返回状态信息
	 * @return {const char*} 返回 NULL 表示出错
	 */
	const char* get_status() const;

	/**
	 * 返回对应下标的数据(当数据类型非 REDIS_RESULT_ARRAY 时）
	 * @param i {size_t} 数组下标
	 * @param len {size_t*} 当为非 NULL 指针时存储所返回数据的长度
	 * @return {const char*}
	 */
	const char* get(size_t i, size_t* len = NULL) const;

	/**
	 * 返回所有的数据数组(当数据类型非 REDIS_RESULT_ARRAY 时）地址
	 * @return {const char**}
	 */
	const char** gets_argv() const
	{
		return (const char**) argv_;
	}

	/**
	 * 返回所有的数据长度数组(当数据类型非 REDIS_RESULT_ARRAY 时）地址
	 * @return {const size_t*}
	 */
	const size_t* get_lens() const
	{
		return lens_;
	}

	/**
	 * 返回所有数据的总长度(当数据类型非 REDIS_RESULT_ARRAY 时）
	 * @return {size_t}
	 */
	size_t get_length() const;

	/**
	 * 当数据类型为 REDIS_RESULT_STRING 类型时，该函数将按内存块存放的数据存储至
	 * 连接内存中，但需要注意防止内存溢出
	 * @param buf {string&} 存储结果数据，内部会先调用 buf.clear()
	 * @return {size_t} 数据的总长度
	 */
	size_t argv_to_string(string& buf) const;
	size_t argv_to_string(char* buf, size_t size) const;

	/**
	 * 当数据类型为 REDIS_RESULT_ARRAY 类型时，该函数返回所有的数组对象
	 * @param size {size_t*} 当返回数组非空时，则该地址存放数组长度
	 * @return {const const redis_result*}
	 */
	const redis_result** get_children(size_t* size) const;

	/**
	 * 当数据类型为 REDIS_RESULT_ARRAY 类型时，该函数返回对应下标的结果对象
	 * @param i {size_t} 下标值
	 * @return {const redis_result*} 当下标值越界或结果不存在时，则返回 NULL
	 */
	const redis_result* get_child(size_t i) const;

	/**
	 * 返回构造函数传入的内存池对象
	 * @return {dbuf_pool*}
	 */
	dbuf_pool* get_pool()
	{
		return pool_;
	}

private:
	~redis_result();

	friend class redis_client;
	void reset();

	redis_result& set_type(redis_result_t type);
	redis_result& set_size(size_t size);
	redis_result& put(const char* buf, size_t len);
	redis_result& put(const redis_result* rr, size_t idx);

private:
	redis_result_t result_type_;
	dbuf_pool* pool_;

	size_t  size_;
	size_t  idx_;
	const char** argv_;
	size_t* lens_;

	//std::vector<const redis_result*>* children_;
	const redis_result** children_;
	size_t  children_size_;
	size_t  children_idx_;
};

} // namespace acl
