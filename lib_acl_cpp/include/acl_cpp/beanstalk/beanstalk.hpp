#pragma once
#include <stdarg.h>
#include <vector>
#include "../acl_cpp_define.hpp"
#include "../stdlib/string.hpp"
#include "../stdlib/noncopyable.hpp"
#include "../stream/socket_stream.hpp"

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_BEANSTALK_DISABLE)

struct ACL_ARGV;

namespace acl {

/**
 * 消息 ID 号从 1 开始递增(参加 beanstalkd 的 job.c 源程序中的如下内容：
 *     static uint64 next_id = 1; 及 make_job_with_id() 中的
 *     if (id) {
 *         j->r.id = id;
 *         if (id >= next_id) next_id = id + 1;
 *     } else {
 *         j->r.id = next_id++;
 *     }
 * 消息优先级 pri 的取值范围为 0 ~ 4,294,968,295(最大无符号整数值)，值越小
 * 则优先级别越高，最高级别为 0 级
 * 消息体默认最大长度为 65,535(最大无符号 short 值)，该值可以在启动 beanstalkd 指定
 * 更多内容请参考本项目 doc/ 目录下的 <beanstalk协议介绍.pdf>
 * 本类中的命令过程内部会自动进行连接操作，在重连过程中，如果之前设置了 watch 及 use
 * 队列，则会自动重试这些命令过程，所以一般来说不用显式调用 open 过程；当用户调用了
 * close 函数后，不仅断开了与 beanstalkd 服务器的连接，同时会清除本类对象中存储的
 * use 及 watch 队列
 */
class ACL_CPP_API beanstalk : public noncopyable
{
public:
	/**
	 * 构造函数
	 * @param addr {const char*} beanstalkd 地址，格式：ip:port/domain:port
	 * @param conn_timeout {int} 连接服务器的超时时间(秒)
	 * @param retry {bool} 如果连接断了是否自动重连
	 */
	beanstalk(const char* addr, int conn_timeout, bool retry = true);
	~beanstalk();

	/////////////////////////////////////////////////////////////////////
	// 生产者调用的接口

	/**
	 * 选择所用的发送管道
	 * @param tube {const char*} 管道名称
	 * @return {bool} 是否成功
	 */
	bool use(const char* tube);

	/**
	 * 向所选管道或缺省管理中发送消息
	 * @param data {const void*} 消息数据地址，可以是二进制
	 * @param len {size_t} data 数据体长度
	 * @param pri {unsigned} 优先级，值越小，优先级越高
	 * @param delay {unsigned} 表示将job放入ready队列需要等待的秒数
	 * @param ttr {unsigned} 表示允许一个worker执行该消息的秒数
	 * @return {unsigned long long} 返回所添加消息的消息号，
	 *  如果返回值 > 0 则表示添加成功，若 == 0 则表示添加失败
	 *  (查看 beanstalkd 源码，可以看出消息号从 1 开始增加)
	 */
	unsigned long long put(const void* data, size_t len,
		unsigned pri = 1024, unsigned delay = 0, unsigned ttr = 60);

	/**
	 * 以格式字符串方式向所选管道或缺省管理中发送消息
	 * @param pri {unsigned} 优先级，值越小，优先级越高
	 * @param delay {unsigned} 表示将job放入ready队列需要等待的秒数
	 * @param ttr {unsigned} 表示允许一个worker执行该消息的秒数
	 * @param fmt {const char*} 格式字符串
	 * @return {unsigned long long} 返回所添加消息的消息号，
	 *  如果返回值 > 0 则表示添加成功，若 == 0 则表示添加失败
	 *  (查看 beanstalkd 源码，可以看出消息号从 1 开始增加)
	 */
	unsigned long long format_put(unsigned pri, unsigned delay, unsigned ttr,
		const char* fmt, ...) ACL_CPP_PRINTF(5, 6);

	unsigned long long vformat_put(const char* fmt, va_list ap,
		unsigned pri = 1024, unsigned delay = 0, unsigned ttr = 60);

	/**
	 * 以格式字符串方式向所选管道或缺省管理中发送消息，其中的
	 * 的 pri, delay, ttr 采用默认值
	 * @param fmt {const char*} 格式字符串
	 * @return {unsigned long long} 返回所添加消息的消息号，
	 *  如果返回值 > 0 则表示添加成功，若 == 0 则表示添加失败
	 *  (查看 beanstalkd 源码，可以看出消息号从 1 开始增加)
	 */
	unsigned long long format_put(const char* fmt, ...) ACL_CPP_PRINTF(2, 3);

	/////////////////////////////////////////////////////////////////////
	// 消费者调用的接口

	/**
	 * 选择读取消息的管道，将其加入监控管理列表中，
	 * 不调用本函数，则使用缺省的管道(default)
	 * @param tube {const char*} 消息管道名称
	 * @return {unsigned} 返回值为关注的消息管道数, 返回值 > 0 则表示成功
	 */
	unsigned watch(const char* tube);

	/**
	 * 取消关注(watch)一个接收消息的管道(tube)
	 * @param tube {const char*} 消息管道名称
	 * @return {unsigned} 返回值为剩余的消息关注管道数, 返回值 > 0 则表示
	 *  成功(因至少要关注一个缺省消息管道，所以正确情况下该返回值至少为 1)，
	 *  如果返回值为 0 则说明输入的管道并未被关注或取消关注失败
	 */
	unsigned ignore(const char* tube);

	/**
	 * 取消关注所有的接收消息的管道
	 * @return {unsigned} 返回值为剩余的消息关注管道数, 返回值 > 0 则表示
	 *  成功(因至少要关注一个缺省消息管道，所以正确情况下该返回值至少为 1)，
	 *  返回 0 表示取消关注失败
	 */
	unsigned ignore_all();

	/**
	 * 从消息输出管道中获取一条消息，但并不删除消息，可以设置
	 * 等待超时，如果设为 -1 则永远阻塞等待消息可用
	 * @param buf {string&} 存储获得的一条消息，函数内部会先清空该缓冲区
	 * @param timeout {int} 等待队列服务器返回消息的超时值，当为 -1
	 *  时，则无限期等待，当 > 0 时，则在该时间内若没有消息，则返回，
	 *  当 == 0 时，则立即返回一条消息或返回超时
	 * @return {unsigned long long} 返回所取得的消息号，若返回值 > 0
	 *  表示正确取到一个消息，否则说明出错或超时没有消息可用，其中当
	 *  返回 0 时，如果调用 get_error() 获得的内容为 TIMED_OUT 则表示
	 *  超时了，当为 DEADLINE_SOON 时则表示该消息已经被读取但在规定的 ttr
	 *  (事务时间内) 没有被 delete_id
	 */
	unsigned long long reserve(string& buf, int timeout = -1);

	/**
	 * 从队列服务器中删除指定 ID 号的消息
	 * @param id {unsigned long long} 消息号
	 * @return {bool} 是否成功删除
	 */
	bool delete_id(unsigned long long id);

	/**
	 * 将一个已经被获取的消息重新放回ready队列(并将job状态置为 "ready")，
	 * 让该消息可以被其它连接获得
	 * @param id {unsigned long long} 消息号
	 * @param pri {unsigned} 优先级别
	 * @param delay {unsigned} 在该消息被放入ready队列之前需要等待的秒数
	 * @return {bool} 是否成功
	 */
	bool release(unsigned long long id, unsigned pri = 1024,
		unsigned delay = 0);

	/**
	 * 将一个消息的状态置为 "buried", Buried 消息被放在一个FIFO的链表中，
	 * 在客户端调用kick命令之前，这些消息将不会被服务端处理
	 * @param id {unsigned long long} 消息号
	 * @param pri {unsigned int} 优先级别
	 * @return {bool} 是否成功
	 */
	bool bury(unsigned long long id, unsigned pri = 1024);

	/**
	 * 允许一个worker请求在一个消息获取更多执行的时间。这对于那些需要
	 * 长时间完成的消息是非常有用的，但同时也可能利用TTR的优势将一个消息
	 * 从一个无法完成工作的worker处移走。一个worker可以通过该命令来告诉
	 * 服务端它还在执行该job (比如：在收到DEADLINE_SOON是可以发生给命令)
	 * @param id {unsigned long long} 消息号
	 * @return {bool} 是否成功
	 */
	bool touch(unsigned long long id);

	/////////////////////////////////////////////////////////////////////
	// 其它接口

	/**
	 * 连接 beanstalkd 服务器，通常情况下不需要显示地调用该函数，上述命令
	 * 会自动根据需要自动调用本函数
	 * @return {bool}  否成功
	 */
	bool open();

	/**
	 * 显示关闭与 beanstalkd 的连接，当该类实例析构时会尝试调用关闭过程，
	 * 调用本函数后，类对象内部的 tube_used_ 及 tubes_watched_ 会被释放
	 */
	void close();

	/**
	 * 显示通知 beanstalkd 服务器退出连接(服务器收到此命令后会立即关闭连接)
	 */
	void quit();

	/**
	 * 获取消息队列中指定的消息号的数据
	 * @param buf {string&} 如果消息存在则存储该条消息，函数内部会先清空该缓冲区
	 * @param id {unsigned long long} 指定的消息号
	 * @return {unsigned long long} 返回取得的 ready 状态消息号，
	 *  若返回值 > 0 则说明取得了一个消息，否则表示没有消息可用
	 */
	unsigned long long peek(string& buf, unsigned long long id);

	/**
	 * 获得当前关注 (watch) 管道中的一条 ready 状态消息，
	 * 如果消息不存在也立即返回
	 * @param buf {string&} 如果消息存在则存储该条消息，函数内部会先清空该缓冲区
	 * @return {unsigned long long} 返回取得的 ready 状态消息号，
	 *  若返回值 > 0 则说明取得了一个消息，否则表示没有消息可用
	 */
	unsigned long long peek_ready(string& buf);

	/**
	 * 获得当前关注 (watch) 管道中的一条 delayed 状态消息，
	 * 如果消息不存在也立即返回
	 * @param buf {string&} 如果消息存在则存储该条消息，函数内部会先清空该缓冲区
	 * @return {unsigned long long} 返回取得的 delayed 状态消息号，
	 *  若返回值 > 0 则说明取得了一个消息，否则表示没有消息可用
	 */
	unsigned long long peek_delayed(string& buf);

	/**
	 * 获得当前关注 (watch) 管道中的一条 buried 状态消息，
	 * 如果消息不存在也立即返回
	 * @param buf {string&} 如果消息存在则存储该条消息，函数内部会先清空该缓冲区
	 * @return {unsigned long long} 返回取得的 buried 状态消息号，
	 *  若返回值 > 0 则说明取得了一个消息，否则表示没有消息可用
	 */
	unsigned long long peek_buried(string& buf);

	/**
	 * 该命令只能针对当前正在使用的tube执行；它将 buried
	 * 或者 delayed 状态的消息移动到 ready 队列
	 * @param n {unsigned} 表示每次 kick 消息的上限，
	 *  服务端将最多 kick 的消息数量
	 * @return {int} 表示本次kick操作作用消息的数目，返回 -1 表示出错
	 */
	int  kick(unsigned n);

	/**
	 * 获得客户当前正在使用的消息管道
	 * @param buf {string&} 存储当前使用的消息管道，函数内部会先清空该缓冲区
	 * @return {bool} 是否成功获得
	 */
	bool list_tube_used(string&buf);

	/**
	 * 获得已经存在的所有消息管道(tube)的列表集合
	 * @param buf {string&} 存储结果，函数内部会先清空该缓冲区
	 * @return {bool} 是否成功获得
	 */
	bool list_tubes(string& buf);

	/**
	 * 获得当前关注(watch)的消息管道的集合
	 * @param buf {string&} 存储结果，函数内部会先清空该缓冲区
	 * @return {bool} 是否成功获得
	 */
	bool list_tubes_watched(string& buf);

	/**
	 * 给定时间内暂停从指定消息管道(tube)中获取消息
	 * @param tube {const char*} 消息管道名
	 * @param delay {unsigned} 指定时间段
	 * @return {bool} 是否成功
	 */
	bool pause_tube(const char* tube, unsigned delay);

	/////////////////////////////////////////////////////////////////////
	// 公共接口
	const char* get_error() const
	{
		return errbuf_.c_str();
	}

	socket_stream& get_conn()
	{
		return conn_;
	}

	/**
	 * 返回构造函数中 beanstalkd 的服务器地址，格式：ip:port
	 * @return {const char*} 永远返回非空的 beanstalkd 服务器地址
	 */
	const char* get_addr() const
	{
		return addr_;
	}

private:
	char* addr_;
	int   timeout_;
	bool  retry_;
	string  errbuf_;
	char* tube_used_;
	std::vector<char*> tubes_watched_;
	socket_stream conn_;
	unsigned long long peek_fmt(string& buf, const char* fmt, ...)
		ACL_CPP_PRINTF(3, 4);
	bool list_tubes_fmt(string& buf, const char* fmt, ...)
		ACL_CPP_PRINTF(3, 4);

	unsigned ignore_one(const char* tube);
	bool beanstalk_open();
	bool beanstalk_use();
	unsigned beanstalk_watch(const char* tube);
	ACL_ARGV* beanstalk_request(const string& cmdline,
		const void* data = NULL, size_t len = 0);
};

} // namespace acl

#endif // !defined(ACL_CLIENT_ONLY) && !defined(ACL_BEANSTALK_DISABLE)
