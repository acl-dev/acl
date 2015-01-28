#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include <vector>
#include <map>
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
	 * 将信息发送到指定的频道 channel
	 * @param channel {const char*} 所发送消息的目标频道
	 * @param msg {const char*} 消息内容
	 * @param len {size_t} 消息长度
	 * @return {int} 成功发送至订阅该频道的订阅者数量
	 *  -1：表示出错
	 *   0：没有订阅者
	 *  >0：订阅该频道的订阅者数量
	 */
	int publish(const char* channel, const char* msg, size_t len);

	/**
	 * 订阅给定的一个或多个频道的信息；在调用本函数后的操作只能发送的命令有：
	 * subscribe、unsubscribe、psubscribe、punsubscribe、get_message，只有
	 * 取消订阅了所有频道（或连接重建）后才摆脱该限制
	 * @param first_channel {const char*} 所订阅的频道列表的第一个非空字符串
	 *  的频道，对于变参列表中的最后一个必须是 NULL
	 * @return {int} 返回当前已经成功订阅的频道个数（即所订阅的所有频道数量）
	 */
	int subscribe(const char* first_channel, ...);
	int subscribe(const std::vector<const char*>& channels);
	int subscribe(const std::vector<string>& channels);

	/**
	 * 取消订阅给定的一个或多个频道的信息
	 * @param first_channel {const char*} 所取消的所订阅频道列表的第一个频道
	 * @return {int} 返回剩余的所订阅的频道的个数
	 */
	int unsubscribe(const char* first_channel, ...);
	int unsubscribe(const std::vector<const char*>& channels);
	int unsubscribe(const std::vector<string>& channels);

	/**
	* 订阅一个或多个符合给定模式的频道；每个模式以 * 作为匹配符；在调用本函数后的操作
	* 只能发送的命令有：
	* subscribe、unsubscribe、psubscribe、punsubscribe、get_message，只有
	* 取消订阅了所有频道（或连接重建）后才摆脱该限制
	 * @param first_pattern {const char*} 第一个匹配模式串
	 * @return {int} 返回当前已经成功订阅的频道个数（即所订阅的所有频道数量）
	 */
	int psubscribe(const char* first_pattern, ...);
	int psubscribe(const std::vector<const char*>& patterns);
	int psubscribe(const std::vector<string>& patterns);

	/**
	 * 根据模式匹配串取消订阅给定的一个或多个频道的信息
	 * @param first_pattern {const char*} 第一个匹配模式串
	 * @return {int} 返回剩余的所订阅的频道的个数
	 */
	int punsubscribe(const char* first_pattern, ...);
	int punsubscribe(const std::vector<const char*>& patterns);
	int punsubscribe(const std::vector<string>& patterns);

	/**
	 * 在订阅频道后可以循环调用本函数从所订阅的频道中获取订阅消息；在调用 subscribe
	 * 或 psubscribe 后才可调用本函数来获取所订阅的频道的消息
	 * @param channel {string&} 存放当前有消息的频道名
	 * @param msg {string&} 存放当前获得的消息内容
	 * @return {bool} 是否成功，如果返回 false 则表示出错
	 */
	bool get_message(string& channel, string& msg);

	/**
	 * 列出当前的活跃频道：活跃频道指的是那些至少有一个订阅者的频道， 订阅模式的
	 * 客户端不计算在内
	 * @param channels {std::vector<string>&} 存放频道结果集
	 * @param first_pattern {const char*} 作为附加的匹配模式第一个匹配字符串，
	 *  该指针可以为 NULL，此时获取指所有的活跃频道；对于变参而言最后一个参数需为 NULL
	 * @return {int} 返回活跃频道数； -1 表示出错
	 *
	 */
	int pubsub_channels(std::vector<string>& channels,
		const char* first_pattern, ...);
	int pubsub_channels(const std::vector<const char*>& patterns,
		std::vector<string>& channels);
	int pubsub_channels(const std::vector<string>& patterns,
		std::vector<string>& channels);

	/**
	 * 返回给定频道的订阅者数量， 订阅模式的客户端不计算在内
	 * @param out {std::map<string, int>&} 存储查询结果，其中 out->first 存放
	 *  频道名，out->second 在座该频道的订阅者数量
	 * @param first_pattern {const char*} 作为附加的匹配模式第一个匹配字符串，
	 *  该指针可以为 NULL，此时获取指所有的活跃频道；对于变参而言最后一个参数需为 NULL
	 * @return {int} 频道的数量，-1 表示出错
	 */
	int pubsub_numsub(std::map<string, int>& out,
		const char* first_channel, ...);
	int pubsub_numsub(const std::vector<const char*>& channels,
		std::map<string, int>& out);
	int pubsub_numsub(const std::vector<string>& channels,
		std::map<string, int>& out);

	/**
	 * 返回订阅模式的数量，这个命令返回的不是订阅模式的客户端的数量， 而是客户端订阅的
	 * 所有模式的数量总和
	 * @return {int} 客户端所有订阅模式的总和，-1 表示出错
	 */
	int pubsub_numpat();

private:
	int subop(const char* cmd, const std::vector<const char*>& channels);
	int subop(const char* cmd, const std::vector<string>& channels);
	int check_channel(const redis_result* obj, const char* cmd,
		const string& channel);
	int pubsub_numsub(std::map<string, int>& out);
};

} // namespace acl
