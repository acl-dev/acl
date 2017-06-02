#pragma once
#include "../acl_cpp_define.hpp"
#include "aio_stream.hpp"

namespace acl
{

class aio_socket_stream;

/**
 * 当异步监听流接收到新的客户端流时调用此回调类中的回调函数，
 * 该类为纯虚类，要求子类必须实现 accept_callback 回调过程
 */
class ACL_CPP_API aio_accept_callback : public aio_callback
{
public:
	aio_accept_callback() {}
	virtual ~aio_accept_callback() {}

	/**
	 * 当接收到新的客户端流时的回调函数
	 * @param client {aio_socket_stream*} 客户端异步连接流，
	 *  可以对此流进行读写操作
	 * @return {bool} 如果希望关闭该异步监听流，可以返回 false，
	 *  一般不应返回 false
	 */
	virtual bool accept_callback(aio_socket_stream* client) = 0;
protected:
private:
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
	 * 开始监听某个指定地址，可以为网络套接口，也可以为域套接口，
	 * @param addr {const char*} 监听地址，TCP监听地址或域监听地址
	 * 格式：
	 *      针对TCP连接：IP:PORT，如：127.0.0.1:9001
	 *      针对域套接口：{path}，如：/tmp/my.sock
	 * @return {bool} 监听是否成功
	 */
	bool open(const char* addr);

	/**
	 * 获得服务器监听地址
	 * @return {const char*}
	 */
	const char* get_addr() const;

	virtual void destroy();
protected:
	virtual ~aio_listen_stream();
private:
	bool accept_hooked_;
	char  addr_[256];
	std::list<aio_accept_callback*> accept_callbacks_;

	void hook_accept();
	static int accept_callback(ACL_ASTREAM*,  void*);
};

}  // namespace acl
