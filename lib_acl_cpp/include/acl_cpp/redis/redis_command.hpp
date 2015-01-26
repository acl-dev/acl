#pragma once
#include "acl_cpp/acl_cpp_define.hpp"

namespace acl
{

class redis_client;
class redis_result;

/**
 * redis 客户端命令类的纯虚父类
 */
class ACL_CPP_API redis_command
{
public:
	redis_command(redis_client* conn = NULL);
	virtual ~redis_command() = 0;

	/**
	 * 在重复使用一个继承于 redis_command 的子类操作 redis 时，需要在下一次
	 * 调用前调用本方法以释放上次操作的临时对象
	 */
	void reset();

	/**
	 * 在使用连接池方式时，通过本函数将从连接池获得的连接对象(redis_client)与
	 * redis 客户端命令进行关联
	 */
	void set_client(redis_client* conn);

	/**
	 * 获得当前 redis 客户端命令的连接对象
	 * @return {redis_client*}
	 */
	redis_client* get_client() const
	{
		return conn_;
	}

	/**
	 * 获得本次 redis 操作过程的结果
	 * @return {redis_result*}
	 */
	const redis_result* get_result() const;

protected:
	redis_client* conn_;

	const redis_result** scan_keys(const char* cmd, const char* key,
		int& cursor, size_t& size, const char* pattern,
		const size_t* count);
};

} // namespace acl
