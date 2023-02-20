#pragma once
#include <vector>

typedef enum {
	REDIS_OBJECT_UNKOWN,
	REDIS_OBJECT_NIL,
	REDIS_OBJECT_ERROR,
	REDIS_OBJECT_STATUS,
	REDIS_OBJECT_INTEGER,
	REDIS_OBJECT_STRING,
	REDIS_OBJECT_ARRAY,
} redis_object_t;

class redis_client;

/**
 * 对 redis-server 返回结果对象类，对 redis-server 返回的数据进行分析后创建
 * redis_object 类对象。
 * the redis result for redis-server's reply
 */
class redis_object : public acl::noncopyable {
public:
	redis_object(acl::dbuf_pool* dbuf);

	/**
	 * 重载了 new/delete 操作符，在 new 新对象时，使内存的分配在
	 * 内存池进行分配
	 * override new/delete operator, when the new object was created,
	 * memory was alloc in acl::dbuf_pool, which is a memroy pool allocator
	 */
	void *operator new(size_t size, acl::dbuf_pool* pool);
	void operator delete(void* ptr, acl::dbuf_pool* pool);

	/**
	 * 获得当前结果结点的数据类型
	 * get the data type of the reply from redis-server
	 * @return {redis_object_t}
	 *  defined above REDIS_OBJECT_
	 */
	redis_object_t get_type(void) const {
		return object_type_;
	}

	/**
	 * 获得当前结果结点存储的对象的个数
	 * get the number of objects from redis-server
	 * @return {size_t} 返回值与存储类型的对应关系如下：
	 *  the relation between returned value and result type show below:
	 *  REDIS_OBJECT_ERROR: 1
	 *  REDIS_OBJECT_STATUS: 1
	 *  REDIS_OBJECT_INTEGER: 1
	 *  REDIS_OBJECT_STRING: > 0 时表示该字符串数据被切分成非连接内存块的个数
	 *  REDIS_OBJECT_ARRAY: children_->size()
	 */
	size_t get_size(void) const;

	/**
	 * 当返回值为 REDIS_OBJECT_INTEGER 类型时，本方法返回对应的 32 位整数值
	 * get the 32 bits integer for REDIS_OBJECT_INTEGER result
	 * @param success {bool*} 本指针非 NULL 时记录操作过程是否成功
	 *  when not NULL, storing the status of success
	 * @return {int}
	 */
	int get_integer(bool* success = NULL) const;

	/**
	 * 当返回值为 REDIS_OBJECT_INTEGER 类型时，本方法返回对应的 64 位整数值
	 * get the 64 bits integer for REDIS_OBJECT_INTEGER result
	 * @param success {bool*} 本指针非 NULL 时记录操作过程是否成功
	 *  when not NULL, storing the status of success
	 * @return {long long int}
	 */
	long long int get_integer64(bool* success = NULL) const;

	/**
	 * 当返回值为 REDIS_OBJECT_STRING 类型时，本方法返回对应的 double 类型值
	 * get the double value for REDIS_OBJECT_STRING result
	 * @param success {bool*} 本指针非 NULL 时记录操作过程是否成功
	 *  when not NULL, storing the status of success
	 * @return {double}
	 */
	double get_double(bool* success = NULL) const;

	/**
	 * 当返回值为 REDIS_OBJECT_STATUS 类型时，本方法返回状态信息
	 * get operation status for REDIS_OBJECT_STATUS result
	 * @return {const char*} 返回 "" 表示出错
	 *  error if empty string returned
	 */
	const char* get_status() const;

	/**
	 * 当出错时返回值为 REDIS_OBJECT_ERROR 类型，本方法返回出错信息
	 * when some error happened, this can get the error information
	 * @return {const char*} 返回空串 "" 表示没有出错信息
	 *  there was no error information if empty string returned
	 */
	const char* get_error(void) const;

	/**
	 * 返回对应下标的数据(当数据类型非 REDIS_OBJECT_ARRAY 时）
	 * get the string data of associated subscript(just for the type
	 * of no REDIS_OBJECT_ARRAY)
	 * @param i {size_t} 数组下标
	 *  the array's subscript
	 * @param len {size_t*} 当为非 NULL 指针时存储所返回数据的长度
	 *  when not NULL, the parameter will store the length of the result
	 * @return {const char*} 返回 NULL 表示下标越界
	 *  NULL if nothing exists or the subscript is out of bounds
	 */
	const char* get(size_t i, size_t* len = NULL) const;

	/**
	 * 返回所有的数据数组(当数据类型非 REDIS_OBJECT_ARRAY 时）地址
	 * return all data's array if the type isn't REDIS_OBJECT_ARRAY
	 * @return {const char**}
	 */
	const char** get_argv(void) const {
		return (const char**) argv_;
	}

	/**
	 * 返回所有的数据长度数组(当数据类型非 REDIS_OBJECT_ARRAY 时）地址
	 * return all length's array if the type isn't REDIS_OBJECT_ARRAY
	 * @return {const size_t*}
	 */
	const size_t* get_lens(void) const {
		return lens_;
	}

	/**
	 * 返回所有数据的总长度(当数据类型非 REDIS_OBJECT_ARRAY 时）
	 * return the total length of all data for no REDIS_OBJECT_ARRAY
	 * @return {size_t}
	 */
	size_t get_length(void) const;

	/**
	 * 当数据类型为 REDIS_OBJECT_STRING 类型时，该函数将按内存块存放的数据
	 * 存储至连接内存中，但需要注意防止内存溢出
	 * compose a continus data for the slicing chunk data internal
	 * @param buf {acl::string&} 存储结果数据，内部会先调用 buf.clear()
	 *  store the result
	 * @return {int} 数据的总长度，返回值 0 表示内部数组为空
	 *  return the total length of data, 0 if data array has no elements
	 */
	int argv_to_string(acl::string& buf) const;
	int argv_to_string(char* buf, size_t size) const;

	/**
	 * 当数据类型为 REDIS_OBJECT_ARRAY 类型时，该函数返回所有的数组对象
	 * return the objects array when result type is REDIS_OBJECT_ARRAY
	 * @param size {size_t*} 当返回数组非空时，则该地址存放数组长度
	 *  store the array's length if size isn't NULL
	 * @return {const const redis_object*}
	 */
	const redis_object** get_children(size_t* size) const;

	/**
	 * 当数据类型为 REDIS_OBJECT_ARRAY 类型时，该函数返回对应下标的结果对象
	 * get one object of the given subscript from objects array
	 * @param i {size_t} 下标值
	 *  the given subscript
	 * @return {const redis_object*} 当下标值越界或结果不存在时，则返回 NULL
	 *  NULL if subscript is out of bounds or object not exist
	 */
	const redis_object* get_child(size_t i) const;

	/**
	 * 返回构造函数传入的内存池对象
	 * get the memory pool object set in constructor
	 * @return {acl::dbuf_pool*}
	 */
	acl::dbuf_pool* get_dbuf(void) {
		return dbuf_;
	}

	/**
	 * 将整个对象转换成字符串
	 * @param out {acl::string&} 存储结果(以追加方式添加)
	 * @return {const acl::string&}
	 */
	const acl::string& to_string(acl::string& out) const;

private:
	~redis_object(void);

	friend class redis_client;
	void clear(void);

	redis_object& set_type(redis_object_t type);
	redis_object& set_size(size_t size);
	redis_object& put(const char* buf, size_t len);
	redis_object& put(const redis_object* rr, size_t idx);

private:
	redis_object_t object_type_;
	acl::dbuf_pool* dbuf_;

	size_t  size_;
	size_t  idx_;
	const char** argv_;
	size_t* lens_;

	//std::vector<const redis_object*>* children_;
	const redis_object** children_;
	size_t  children_size_;
	size_t  children_idx_;
};
