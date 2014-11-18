#pragma once
#include "acl_cpp/acl_cpp_define.hpp"

namespace acl
{

class connect_pool;

class ACL_CPP_API connect_client
{
public:
	connect_client() : when_(0), pool_(NULL) {}
	virtual ~connect_client() {}

	/**
	 * 获得该连接对象最近一次被使用的时间截
	 * @return {time_t}
	 */
	time_t get_when()
	{
		return when_;
	}

	/**
	 * 设置该连接对象当前被使用的时间截
	 */
	void set_when(time_t when)
	{
		when_ = when;
	}

	/**
	 * 纯虚函数，子类必须实现此函数用于连接服务器
	 * @return {bool} 是否连接成功
	 */
	virtual bool open() = 0;

	/**
	 * 获得连接池对象引用，在 connect_pool 内部创建
	 * 连接对象会调用 set_pool 设置连接池对象句柄
	 * @return {connect_pool*}
	 */
	connect_pool* get_pool() const
	{
		return pool_;
	}

private:
	friend class connect_pool;

	time_t when_;
	connect_pool* pool_;

	void set_pool(connect_pool* pool)
	{
		pool_ = pool;
	}
};

} // namespace acl
