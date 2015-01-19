#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include "acl_cpp/redis/redis_command.hpp"

namespace acl
{

class redis_client;
class redis_result;
class string;

class ACL_CPP_API redis_pubsub : public redis_command
{
public:
	redis_pubsub(redis_client* conn = NULL);
	~redis_pubsub();

	/////////////////////////////////////////////////////////////////////

	/**
	 *
	 * @return {int} 成功发送至订阅该频道的订阅者数量
	 *  -1：表示出错
	 *  0：没有订阅者
	 *  > 0：订阅该频道的订阅者数量
	 */
	int publish(const char* channel, const char* msg, size_t len);
	int subscribe(const char* first_channel, ...);
	int subscribe(const std::vector<string>& channels);
	int unsubscribe(const char* first_channel, ...);
	int unsubscribe(const std::vector<string>& channels);
	bool get_message(string& channel, string& msg);

private:
	int subop(const char* cmd, const std::vector<string>& channels);
	int check_channel(const redis_result* obj, const char* cmd,
		const string& channel);
};

} // namespace acl
