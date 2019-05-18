#pragma once
#include "../acl_cpp_define.hpp"
#include <vector>
#include "../stdlib/noncopyable.hpp"
#include "../stdlib/string.hpp"

#ifndef ACL_CLIENT_ONLY

namespace acl
{

class redis_result;

/**
 * 在从 disque 队列中获得的任务信息的类
 */
class ACL_CPP_API disque_job : public noncopyable
{
public:
	disque_job();
	~disque_job();

	/**
	 * get the ID of the job
	 * 获得当前任务的 ID 号
	 * @return {const char*}
	 */
	const char* get_id() const
	{
		return id_.c_str();
	}

	/**
	 * get the queue name holding the job
	 * 获得当前任务所在的队列
	 * @return {const char*}
	 */
	const char* get_queue() const
	{
		return queue_.c_str();
	}

	/**
	 * get the job's data
	 * 获得当前任务的消息内容
	 * @return {const string&}
	 */
	const string& get_body() const
	{
		return body_;
	}

	void set_id(const char* id);
	void set_queue(const char* name);
	void set_body(const char* job, size_t len);

	/////////////////////////////////////////////////////////////////////

	bool init(const redis_result& rr);

	const char* get_state() const
	{
		return state_.c_str();
	}

	int get_repl() const
	{
		return repl_;
	}

	int get_ttl() const
	{
		return ttl_;
	}

	long long int get_ctime() const
	{
		return ctime_;
	}

	int get_delay() const
	{
		return delay_;
	}

	int get_retry() const
	{
		return retry_;
	}

	const std::vector<string>& get_nodes_delivered() const
	{
		return nodes_delivered_;
	}

	const std::vector<string>& get_nodes_confirmed() const
	{
		return nodes_confirmed_;
	}

	int get_next_requeue_within() const
	{
		return next_requeue_within_;
	}

	int get_next_awake_within() const
	{
		return next_awake_within_;
	}

private:
	string id_;
	string queue_;
	string state_;
	int repl_;
	int ttl_;
	long long int ctime_;
	int delay_;
	int retry_;
	std::vector<string> nodes_delivered_;
	std::vector<string> nodes_confirmed_;
	int next_requeue_within_;
	int next_awake_within_;
	string body_;

	void set_nodes_delivered(const redis_result& rr);
	void set_nodes_confirmed(const redis_result& rr);
	void set_nodes(const redis_result& rr, std::vector<string>& out);
};

} // namespace acl

#endif // ACL_CLIENT_ONLY
