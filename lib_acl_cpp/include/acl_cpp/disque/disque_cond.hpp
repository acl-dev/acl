#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/noncopyable.hpp"

#ifndef ACL_CLIENT_ONLY

namespace acl
{

/**
 * 在添加任务时，此类指定的任务的添加条件限定
 */
class ACL_CPP_API disque_cond : public noncopyable
{
public:
	disque_cond();
	~disque_cond();

	int get_replicate() const
	{
		return replicate_;
	}

	int get_delay() const
	{
		return delay_;
	}

	int get_retry() const
	{
		return retry_;
	}

	int get_ttl() const
	{
		return ttl_;
	}

	int get_maxlen() const
	{
		return maxlen_;
	}

	bool is_async() const
	{
		return async_;
	}

	disque_cond& set_replicate(int n);
	disque_cond& set_delay(int n);
	disque_cond& set_retry(int n);
	disque_cond& set_ttl(int n);
	disque_cond& set_maxlen(int n);
	disque_cond& set_async(bool on);

private:
	int replicate_;
	int delay_;
	int retry_;
	int ttl_;
	int maxlen_;
	bool async_;
};

} // namespace acl

#endif // ACL_CLIENT_ONLY
