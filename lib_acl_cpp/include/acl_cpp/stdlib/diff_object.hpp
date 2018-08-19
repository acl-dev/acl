#pragma once
#include "../acl_cpp_define.hpp"
#include "dbuf_pool.hpp"

namespace acl
{

class diff_manager;

/**
 * 差集比较纯虚类，子类必须继承该类，并实现其中的纯虚方法
 * 该类继承于 dbuf_obj 类，便于由 dbuf_guard 统一管理，统一销毁
 */
class diff_object : public dbuf_obj
{
public:
	/**
	 * 构造函数
	 * @param manager {diff_manager&}
	 */
	diff_object(diff_manager& manager);

	virtual ~diff_object(void) {}

	/**
	 * 纯虚接口，获得该对象的键字符串
	 * @return {const char*} 必须返回非空字符串
	 */
	virtual const char* get_key(void) const = 0;

	/**
	 * 纯虚接口，获得该对象的值字符串
	 * @return {const char*} 必须返回非空字符串
	 */
	virtual const char* get_val(void) const = 0;

	/**
	 * 纯虚接口，用来比较两个对象
	 * @param obj {const diff_object&}
	 * @return {bool} 两个对象是否相等
	 */
	virtual bool operator== (const diff_object& obj) const = 0;

	/**
	 * 是否是不在给定区间范围（闭区间）的多余数据
	 * @param range_from {long long} 起始位置
	 * @param range_to {long long} 结束位置
	 * @return {bool} 是否是超过给定区间范围的多余数据对象
	 */
	virtual bool check_range(long long range_from, long long range_to) const
	{
		(void) range_from;
		(void) range_to;
		return false;
	}

protected:
	diff_manager& manager_;
};

} // namespace acl
