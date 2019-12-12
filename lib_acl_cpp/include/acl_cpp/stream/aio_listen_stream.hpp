#pragma once
#include "../acl_cpp_define.hpp"
#include "aio_stream.hpp"

namespace acl
{

class aio_socket_stream;
class aio_listen_stream;

/**
 * 当异步监听流接收到新的客户端流时调用此回调类中的回调函数，该类为纯虚类，
 * 要求子类必须实现 accept_callback 回调过程
 */
class ACL_CPP_API aio_accept_callback : public aio_callback
{
public:
	aio_accept_callback(void) {}
	virtual ~aio_accept_callback(void) {}

	/**
	 * 当接收到新的客户端流时的回调函数
	 * @param client {aio_socket_stream*} 客户端异步连接流，
	 *  可以对此流进行读写操作
	 * @return {bool} 如果希望关闭该异步监听流，可以返回 false，
	 *  一般不应返回 false
	 */
	virtual bool accept_callback(aio_socket_stream* client) = 0;
};

/**
 * 当异步监听流收到有新连接到达事件时调用此类中的虚函数，在由子类实现该虚函数
 * 中调用 accept() 系统 API 接收客户端连接，该类与上面的 aio_accept_callback
 * 有所不同，在 aio_accept_callback::accept_callback() 被调用时，客户端连接对
 * 象已经被创建，而在 listen_callback() 中，则需要应用自己接收连接对象
 */
class ACL_CPP_API aio_listen_callback : public aio_callback
{
public:
	aio_listen_callback(void) {}
	virtual ~aio_listen_callback(void) {}

	virtual bool listen_callback(aio_listen_stream& ss) = 0;
};

/**
 * 异步监听网络流，该类接收来自于客户端的外来连接，同时该类只能
 * 在堆上分配，不能在栈分配，应用可以调用 close 主动关闭流，流关闭
 * 后该异步流对象自动释放，无需调用 delete 删除该类对象
 *
 */
class ACL_CPP_API aio_listen_stream : public aio_stream
{
public:
	/**
	 * 构造函数，用以构造异步监听流
	 * @param handle {aio_handle*} 异步引擎句柄
	 */
	aio_listen_stream(aio_handle* handle);

	/**
	 * 添加异步监听流接收到新客户端流时的回调函数
	 * @param callback {aio_accept_callback*}
	 */
	void add_accept_callback(aio_accept_callback* callback);

	/**
	 * 添加异步监听流有客户端连接到达时的回调函数
	 * @param callback {aio_listen_stream*}
	 *  注意：本方法与上面 add_accept_callback 的区别，本方法是 reactor
	 *  模式，而 add_accept_callback 则是 proactor 模式
	 */
	void add_listen_callback(aio_listen_callback* callback);

	/**
	 * 当调用 add_listen_callback 方式时，在 aio_listen_callback 子类
	 * 中的函数 listen_callback 里可以调用本方法来获得一个异步连接对象
	 * @return {aio_socket_stream*} 返回 NULL 表示获得连接失败
	 */
	aio_socket_stream* accept(void);

	/**
	 * 开始监听某个指定地址，可以为网络套接口，也可以为域套接口，
	 * @param addr {const char*} 监听地址，TCP监听地址或域监听地址
	 * 格式：
	 *   针对TCP连接：IP:PORT，如：127.0.0.1:9001
	 *   针对域套接口：{path}，如：/tmp/my.sock，在 Linux 平台，还可以支持
	 *   Linux abstract unix domain socket，需要地址首字节为'@'，在 Linux
	 *   平台下，acl 内部如果检测到路径首字节为 '@'，则内部自动切到 Linux
	 *   abstract unix domain socket 监听模式（其中的 @ 符只是用来标记，内
	 *   部的监听地址会自动去掉）
	 * @param flag {unsigned} 创建监听套接口时的打开标志位，见 server_socket.hpp
	 * @return {bool} 监听是否成功
	 */
	bool open(const char* addr, unsigned flag = 0);

	/**
	 * 使用套接字创建监听对象，该套接字句柄必须已经调用了 bind/listen 过程
	 * @param fd {int}
	 * @return {bool} 是否成功
	 */
#if defined(_WIN32) || defined(_WIN64)
	bool open(SOCKET fd);
#else
	bool open(int fd);
#endif

	/**
	 * 使用同步流对象创建非阻塞监听对象
	 * @param vstream {ACL_VSTREAM*} 非空对象
	 * @return {bool} 是否成功
	 */
	bool open(ACL_VSTREAM* vstream);

	/**
	 * 使用非阻塞流对象创建非阻塞监听对象
	 * @param astream {ACL_ASTREAM*} 非空对象
	 * @return {bool} 是否成功
	 */
	bool open(ACL_ASTREAM* astream);

	/**
	 * 获得服务器监听地址
	 * @return {const char*}
	 */
	const char* get_addr(void) const;

	/**
	 * 重载基类方法，当异步流对象销毁时回调此方法
	 */
	virtual void destroy(void);

protected:
	virtual ~aio_listen_stream(void);

private:
	bool listen_hooked_;
	char addr_[256];
	std::list<aio_accept_callback*> accept_callbacks_;
	std::list<aio_listen_callback*> listen_callbacks_;

	void hook_listen(void);
	int accept_callback(aio_socket_stream* conn);
	static int listen_callback(ACL_ASTREAM*, void*);
};

}  // namespace acl
