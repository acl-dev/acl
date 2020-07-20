#pragma once
#include "../acl_cpp_define.hpp"
#include <vector>
#include <map>
#include "redis_command.hpp"

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

namespace acl
{

class redis_client;
class redis_result;
class string;

class ACL_CPP_API redis_pubsub : virtual public redis_command
{
public:
	/**
	 * see redis_command::redis_command()
	 */
	redis_pubsub(void);

	/**
	 * see redis_command::redis_command(redis_client*)
	 */
	redis_pubsub(redis_client* conn);

	/**
	 * see redis_command::redis_command(redis_client_cluster*)
	 */
	redis_pubsub(redis_client_cluster* cluster);

	ACL_CPP_DEPRECATED
	redis_pubsub(redis_client_cluster* cluster, size_t max_conns);

	virtual ~redis_pubsub(void);

	/////////////////////////////////////////////////////////////////////

	/**
	 * 将信息发送到指定的频道 channel
	 * post a message to a channel
	 * @param channel {const char*} 所发送消息的目标频道
	 *  the specified channel
	 * @param msg {const char*} 消息内容
	 *  the message to be sent
	 * @param len {size_t} 消息长度
	 *  the message's length
	 * @return {int} 成功发送至订阅该频道的订阅者数量
	 *  the number of clients that received the message
	 *  -1：表示出错
	 *      error happened
	 *   0：没有订阅者
	 *      no client subscribe the channel
	 *  >0：订阅该频道的订阅者数量
	 *      the number of clients that received the message
	 */
	int publish(const char* channel, const char* msg, size_t len);

	/**
	 * 订阅给定的一个或多个频道的信息；在调用本函数后的操作只能发送的命令有：
	 * subscribe、unsubscribe、psubscribe、punsubscribe、get_message，只有
	 * 取消订阅了所有频道（或连接重建）后才摆脱该限制
	 * subscribe one or more channel(s). Once the client enters the
	 * subscribed state it is not supposed to issue any other commands,
	 * except for additional SUBSCRIBE, PSUBSCRIBE, UNSUBSCRIBE
	 * and PUNSUBSCRIBE commands
	 * @param first_channel {const char*} 所订阅的频道列表的第一个非空字符串
	 *  的频道，对于变参列表中的最后一个必须是 NULL
	 *  the first non-NULL channel in the channel list, and the last
	 *  parameter must be NULL indicating the end of the channel list
	 * @return {int} 返回当前已经成功订阅的频道个数（即所订阅的所有频道数量）
	 *  the number of channels subscribed by the current client
	 */
	int subscribe(const char* first_channel, ...);
	int subscribe(const std::vector<const char*>& channels);
	int subscribe(const std::vector<string>& channels);

	/**
	 * 取消订阅给定的一个或多个频道的信息
	 * stop listening for messages posted to the given channels
	 * @param first_channel {const char*} 所取消的所订阅频道列表的第一个频道
	 *  the fist channel in channel list, and the last parameter must be
	 *  NULL indicating the end of the channel list
	 * @return {int} 返回剩余的所订阅的频道的个数
	 *  the rest channels listened by the current client
	 */
	int unsubscribe(const char* first_channel, ...);
	int unsubscribe(const std::vector<const char*>& channels);
	int unsubscribe(const std::vector<string>& channels);

	/**
	* 订阅一个或多个符合给定模式的频道；每个模式以 * 作为匹配符；在调用本函数后的操作
	* 只能发送的命令有：subscribe、unsubscribe、psubscribe、punsubscribe、
	* get_message，只有取消订阅了所有频道（或连接重建）后才摆脱该限制
	* listen for messages published to channels matching the give patterns
	 * @param first_pattern {const char*} 第一个匹配模式串
	 *  the first pattern in pattern list, the last parameter must be NULL
	 *  int the variable args
	 * @return {int} 返回当前已经成功订阅的频道个数（即所订阅的所有频道数量）
	 *  the number of channels listened by the current client
	 */
	int psubscribe(const char* first_pattern, ...);
	int psubscribe(const std::vector<const char*>& patterns);
	int psubscribe(const std::vector<string>& patterns);

	/**
	 * 根据模式匹配串取消订阅给定的一个或多个频道的信息
	 * stop listening for messaged posted to channels matching
	 * the given patterns
	 * @param first_pattern {const char*} 第一个匹配模式串
	 *  the first parttern in a variable args ending with NULL
	 * @return {int} 返回剩余的所订阅的频道的个数
	 *  the rest number of channels be listened by the client
	 */
	int punsubscribe(const char* first_pattern, ...);
	int punsubscribe(const std::vector<const char*>& patterns);
	int punsubscribe(const std::vector<string>& patterns);

	/**
	 * 在订阅频道后可以循环调用本函数从所订阅的频道中获取订阅消息；
	 * 在调用 subscribe 或 psubscribe 后才可调用本函数来获取所订阅的频道的消息
	 * get messages posted to channels after SUBSCRIBE or PSUBSCRIBE
	 * @param channel {string&} 存放当前有消息的频道名
	 *  buffer for storing the channel associate with the msg
	 * @param msg {string&} 存放当前获得的消息内容
	 *  store the message posted to the channel
	 * @param message_type {string*} will store messsage or pmessage
	 * @param pattern {string*} will store pattern set by psubscribe
	 * @param timeout {int} 当该值 >= 0 时，表示等待消息超时时间(秒)
     *  when timeout >= 0, which was used as the waiting time for reading(second)
	 * @return {bool} 是否成功，如果返回 false 则表示出错或超时
	 *  true on success, false on error or waiting timeout
	 */
	bool get_message(string& channel, string& msg, string* message_type = NULL,
		string* pattern = NULL, int timeout = -1);

	/**
	 * 列出当前的活跃频道：活跃频道指的是那些至少有一个订阅者的频道， 订阅模式的
	 * 客户端不计算在内
	 * Lists the currently active channels.
	 * @param channels {std::vector<string>*} 非空时存放频道结果集
	 *  store the active channels
	 * @param first_pattern {const char*} 作为附加的匹配模式第一个匹配字符串，
	 *  该指针可以为 NULL，此时获取指所有的活跃频道；对于变参而言最后一个参数需为 NULL
	 *  the first pattern in a variable args ending with NULL arg, and
	 *  the first arg can be NULL.
	 * @return {int} 返回活跃频道数； -1 表示出错
	 *  the number of active channels. -1 if error
	 *
	 *  操作成功后可以通过以下任一方式获得数据
	 *  1、基类方法 get_value 获得指定下标的元素数据
	 *  2、基类方法 get_child 获得指定下标的元素对象(redis_result），然后再通过
	 *     redis_result::argv_to_string 方法获得元素数据
	 *  3、基类方法 get_result 方法取得总结果集对象 redis_result，然后再通过
	 *     redis_result::get_child 获得一个元素对象，然后再通过方式 2 中指定
	 *     的方法获得该元素的数据
	 *  4、基类方法 get_children 获得结果元素数组对象，再通过 redis_result 中
	 *     的方法 argv_to_string 从每一个元素对象中获得元素数据
	 *  5、在调用方法中传入非空的存储结果对象的地址
	 *
	 */
	int pubsub_channels(std::vector<string>* channels,
		const char* first_pattern, ...);
	int pubsub_channels(const std::vector<const char*>& patterns,
		std::vector<string>* channels);
	int pubsub_channels(const std::vector<string>& patterns,
		std::vector<string>* channels);

	/**
	 * 返回给定频道的订阅者数量， 订阅模式的客户端不计算在内
	 * Returns the number of subscribers (not counting clients
	 * subscribed to patterns) for the specified channels.
	 * @param out {std::map<string, int>&} 存储查询结果，其中 out->first 存放
	 *  频道名，out->second 在座该频道的订阅者数量
	 *  store the results
	 * @param first_channel {const char*} 第一个频道
	 *  该指针可以为 NULL，此时获取指所有的活跃频道；对于变参而言最后一个参数需为 NULL
	 *  the first pattern in a variable args ending with NULL arg, and
	 *  the first arg can be NULL.
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
	 * Returns the number of subscriptions to patterns.
	 * @return {int} 客户端所有订阅模式的总和，-1 表示出错
	 *  the number of patterns all the clients are subscribed to,
	 *  -1 if error.
	 */
	int pubsub_numpat();

private:
	int subop(const char* cmd, const std::vector<const char*>& channels);
	int subop_result(const char* cmd, const std::vector<const char*>& channels);
	int subop(const char* cmd, const std::vector<string>& channels);
	int subop_result(const char* cmd, const std::vector<string>& channels);
	int check_channel(const redis_result* obj, const char* cmd,
		const char* channel);
	int pubsub_numsub(std::map<string, int>& out);
};

} // namespace acl

#endif // !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)
