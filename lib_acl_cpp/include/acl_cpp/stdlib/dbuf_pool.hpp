#pragma once
#include "acl_cpp/acl_cpp_define.hpp"

struct ACL_DBUF_POOL;

namespace acl
{

/**
 * 内存链管理类，该类仅提供内存分配函数，在整个类对象被析构时该内存链会被一次性地释放，
 * 该类适合于需要频繁分配一些大小不等的小内存的应用；
 * 该类实际上是封装了 lib_acl 中的 ACL_DBUF_POOL 结构及方法
 */
class ACL_CPP_API dbuf_pool
{
public:
	dbuf_pool(size_t block_size = 8192);
	~dbuf_pool();

	/**
	 * 分配指定长度的内存
	 * @param len {size_t} 需要分配的内存长度，当内存比较小时(小于构造函数中的
	 *  block_size)时，所分配的内存是在 dbuf_pool 所管理的内存链上，当内存较大时
	 *  会直接使用 malloc 进行分配
	 * @return {void*} 新分配的内存地址
	 */
	void* dbuf_alloc(size_t len);

	/**
	 * 分配指定长度的内存并将内存区域清零
	 * @param len {size_t} 需要分配的内存长度
	 * @return {void*} 新分配的内存地址
	 */
	void* dbuf_calloc(size_t len);

	/**
	 * 根据输入的字符串动态创建新的内存并将字符串内存进行复制，类似于 strdup
	 * @param s {const char*} 源字符串
	 * @return {char*} 新复制的字符串地址
	 */
	char* dbuf_strdup(const char* s);

	/**
	 * 根据输入的内存数据动态创建内存并将数据进行复制
	 * @param s {const void*} 源数据内存地址
	 * @param len {size_t} 源数据长度
	 * @return {void*} 新复制的数据地址
	 */
	void* dbuf_memdup(const void* s, size_t len);

private:
	ACL_DBUF_POOL* pool_;
};

} // namespace acl
