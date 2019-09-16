#pragma once
#include "../stdlib/noncopyable.hpp"

namespace acl
{

class tcp_manager;
class tcp_pool;
class string;

/**
 * 该类封装了 tcp_manager 管理类，可以动态添加目标服务端地址，同时动态创建与
 * 每一个服务端的连接池
 */
class ACL_CPP_API tcp_ipc : public noncopyable
{
public:
	tcp_ipc(void);
	~tcp_ipc(void);

	/**
	 * 设置与每个服务器所建连接池的最大连接限制
	 * @param max {int} 每个连接池的最大连接限制，当 <= 0 时则不限制连接数
	 * @return {tcp_ipc&}
	 */
	tcp_ipc& set_limit(int max);

	/**
	 * 设置连接池中每个连接了空闲时间，当连接空闲时间超过设置值时将被关闭
	 * @param ttl {int} 空闲连接的最大超时时间
	 * @return {tcp_ipc&}
	 */
	tcp_ipc& set_idle(int ttl);

	/**
	 * 设置每个连接的网络连接超时时间
	 * @param conn_timeout {int} 网络连接超时时间（秒）
	 * @return {tcp_ipc&}
	 */
	tcp_ipc& set_conn_timeout(int conn_timeout);

	/**
	 * 设置每个连接的网络读写超时时间
	 * @param timeout {int} 读写超时时间（秒）
	 * @return {tcp_ipc&}
	 */
	tcp_ipc& set_rw_timeout(int timeout);

	/**
	 * 获得 TCP 管理器对象
	 * @return {tcp_manager&}
	 */
	tcp_manager& get_manager(void) const;

	/**
	 * 可以调用本方法显示添加一个服务器地址，只有当地址不存在时才会添加
	 * @param addr {const char*} 服务器地址，格式：IP:PORT
	 * @return {tcp_ipc&}
	 */
	tcp_ipc& add_addr(const char* addr);

	/**
	 * 根据服务器地址删除指定的连接池对象，当连接池对象正在被引用时，该对象
	 * 不会被删除，而是采用延迟删除方式，当最后一个连接被归还后该连接池对象
	 * 才会被真正删除
	 * @param addr {const char*} 服务器地址，格式：IP:PORT
	 * @return {tcp_ipc&}
	 */
	tcp_ipc& del_addr(const char* addr);

	/**
	 * 检测指定的服务器地址是否成功
	 * @param addr {const char*} 服务器地址，格式：IP:PORT
	 * @return {bool}
	 */
	bool addr_exist(const char* addr);

	/**
	 * 获得当前所有的服务器地址集合
	 * @param addrs {std::vector<string>&} 存储结果集
	 */
	void get_addrs(std::vector<string>& addrs);

	/**
	 * 向服务器发送指定长度的数据包
	 * @param addr {const char*} 指定的目标服务器地址
	 * @param data {const void*} 要发送的数据包地址
	 * @param len {unsigned int} 数据长度
	 * @param out {string*} 当该对象非 NULL 时表明需要从服务器读取响应数据，
	 *  响应结果将被存放在该缓冲区中，如果该对象为 NULL，则表示无需读取
	 *  服务器的响应数据
	 * @return {bool} 发送是否成功
	 */
	bool send(const char* addr, const void* data, unsigned int len,
		string* out = NULL);

	/**
	 * 向所有服务器发送数据包
	 * @param data {const void*} 要发送的数据包地址
	 * @param len {unsigned int} 数据长度
	 * @param exclusive {bool} 发送广播包时，是否加线程锁以防止其它线程
	 *  竞争内部连接池资源
	 * @param check_result {bool} 是否读服务器响应以证明服务器收到了数据
	 * @param nerr {unsigned *} 非 NULL 时存放失败的服务器的个数
	 * @return {size_t} 返回发送到的服务器的数量
	 */
	size_t broadcast(const void* data, unsigned int len,
		bool exclusive = true, bool check_result = false,
		unsigned* nerr = NULL);

private:
	tcp_manager* manager_;
	int max_;
	int ttl_;
	int conn_timeout_;
	int rw_timeout_;

	bool send(tcp_pool&, const void*, unsigned int, string*);
};

} // namespace acl
