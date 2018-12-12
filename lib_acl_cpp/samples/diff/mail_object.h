#pragma once

class mail_object : public acl::diff_object
{
public:
	/**
	 * 构造函数
	 * @param manager {diff_manager&}
	 * @param key {const char*} 以字符串方式表示的键，非空字符串
	 * @param val {const char*} 以字符串方式表示的值，非空字符串
	 */
	mail_object(acl::diff_manager& manager, const char* key, const char* val);

	void set_ctime(long long n);

public:
	// override: 基类纯虚函数的实现
	const char* get_key() const;

	// override: 基类纯虚函数的实现
	const char* get_val() const;

	// override: 基类纯函数的实现
	bool operator== (const acl::diff_object& obj) const;

	// @override
	bool check_range(long long from, long long to) const;

private:
	const char* key_;
	const char* val_;
	long long ctime_;

	// 析构函数声明为私有的，从而要求动态创建本类对象
	~mail_object();
};
