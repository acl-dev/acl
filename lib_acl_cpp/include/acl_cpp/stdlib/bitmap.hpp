#pragma once

namespace acl {

class bitmap
{
public:
	/**
	 * 构造函数
	 * @param buf {const void*} 源数据内存位映射区
	 * @param len {size_} buf 位映射内存区中被置位的数量
	 */
	bitmap(const void* buf, size_t len);

	/**
	 * 构造函数
	 * @param len {size_t} 最大容纳的位映射的数量
	 */
	bitmap(size_t len);

	~bitmap();

	/**
	 * 将所给数值映射在位集合中
	 * @param n {size_t}
	 * @return {bool} 返回 true 表示添加成功，否则表示该值越界或已经存在
	 */
	bool bit_set(size_t n);

	/**
	 * 判断所给数据是否已经被设置在位映射中
	 * @param n {size_t}
	 * @return {bool} 判断指定数值是否存在于位映射集合中
	 */
	bool bit_isset(size_t n);

	/**
	 * 将指定数值从位集合中去除
	 * @param n {size_t}
	 * @return {bool} 返回 false 表示该值越界或不存在于位集合中
	 */
	bool bit_unset(size_t n);

	/**
	 * 将bitmap信息拷贝到buf中
	 * @param buf {void*}存放拷贝结果
	 * @param len {size_t} buf的最大长度
	 * @return {size_t} 返回成功拷贝的内存长度，返回 0 表示 buf 太小
	 */
	size_t tobuf(void* buf, size_t len);

	/**
	 * 从buf中设置当前bitmap信息
	 * @param buf {const void*} 要设置bitmap信息
	 * @param len {size_t} buf的长度
	 * @return true 成功，false失败
	 */
	bool frombuf(const void* buf, size_t len);

	/**
	 * 重置当前的bitmap为 0
	 */
	void reset(void);

	/**
	 * 获取当前位映射存储空间的大小
	 * @return {size_t}
	 */
	size_t size(void);

	/**
	 * 获取当前已经设置的个数
	 * @return {size_t}
	 */
	size_t count(void);

	/**
	 * 当前bitmap是否已满
	 * @return {bool}
	 */
	bool full(void);

private:
	unsigned char *bmp_;
	size_t size_;
	size_t count_;

	//从新统计count数量
	void recount(void);
};

} // namespace acl
