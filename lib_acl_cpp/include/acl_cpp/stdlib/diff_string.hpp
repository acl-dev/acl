#pragma once
#include "../acl_cpp_define.hpp"
#include "diff_object.hpp"

namespace acl
{

class diff_string : public diff_object
{
public:
	/**
	 * 构造函数
	 * @param manager {diff_manager&}
	 * @param key {const char*} 以字符串方式表示的键，非空字符串
	 * @param val {const char*} 以字符串方式表示的值，非空字符串
	 */
	diff_string(diff_manager& manager, const char* key, const char* val);

	/**
	 * 设置本对象所在的区间值
	 * @param range {long long}
	 */
	void set_range(long long range);

	/**
	 * 获得本对象所在的区间值
	 * @return {long long}
	 */
	long long get_range(void) const
	{
		return range_;
	}

public:
	// override: 基类纯虚函数的实现
	const char* get_key(void) const;

	// override: 基类纯虚函数的实现
	const char* get_val(void) const;

	// override: 基类纯函数的实现
	bool operator== (const diff_object& obj) const;

	// @override
	bool check_range(long long range_from, long long range_to) const;

private:
	const char* key_;
	const char* val_;
	long long range_;

	// 析构函数声明为私有的，从而要求动态创建本类对象
	~diff_string(void);
};

} // namespace acl
