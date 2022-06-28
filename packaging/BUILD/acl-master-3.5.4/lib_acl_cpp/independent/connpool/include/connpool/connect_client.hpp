#pragma once

namespace acl_min
{

class connect_client
{
public:
	connect_client() : when_(0) {}
	virtual ~connect_client() {}

	/**
	 * 获得该连接对象最近一次被使用的时间截
	 * @return {time_t}
	 */
	time_t get_when() const
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
private:
	time_t when_;
};

} // namespace acl_min
