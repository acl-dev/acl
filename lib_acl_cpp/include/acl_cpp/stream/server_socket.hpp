#pragma once
#include "../acl_cpp_define.hpp"
#if defined(_WIN32) || defined(_WIN64)
#include <WinSock2.h>
#endif

namespace acl {

class socket_stream;

/**
 * 服务端监听套接口类，接收客户端连接，并创建客户端流连接对象
 */
class ACL_CPP_API server_socket
{
public:
	/**
	 * 构造函数，调用本构造函数后需调用类方法 open 来监听指定服务地址
	 * @param backlog {int} 监听套接口队列长度
	 * @param block {bool} 是阻塞模式还是非阻塞模式
	 */
	server_socket(int backlog = 128, bool block = true);

	/**
	 * 构造函数，调用本构造函数后禁止再调用 open 方法
	 * @param sstream {ACL_VSTREAM*} 外部创建的监听流对象，本类仅使用
	 *  但并不释放，由应用自行关闭该监听对象
	 */
	server_socket(ACL_VSTREAM* sstream);

	/**
	 * 构造函数，调用本构造函数后禁止再调用 open 方法
	 * @param fd {ACL_SOCKET} 外部创建的监听句柄，本类仅使用但并不释放，
	 *  由应用自行关闭该监听句柄
	 */
#if defined(_WIN32) || defined(_WIN64)
	server_socket(SOCKET fd);
#else
	server_socket(int fd);
#endif

	~server_socket();

	/**
	 * 开始监听给定服务端地址
	 * @param addr {const char*} 服务器监听地址，格式为：
	 *  ip:port；在 unix 环境下，还可以是域套接口，格式为：
	 *   /path/xxx
	 * @return {bool} 监听是否成功
	 */
	bool open(const char* addr);

	/**
	 * 关闭已经打开的监听套接口
	 * @return {bool} 是否正常关闭
	 */
	bool close();

	/**
	 * 接收客户端连接并创建客户端连接流
	 * @param timeout {int} 在阻塞模式下，当该值 > 0 时，采用超时
	 *  方式接收客户端连接，若在指定时间内未获得客户端连接，则返回 NULL
	 * @return {socket_stream*} 返回空表示接收失败
	 */
	socket_stream* accept(int timeout = 0);

	/**
	 * 获得监听的地址
	 * @return {const char*} 返回值非空指针
	 */
	const char* get_addr() const
	{
		return addr_;
	}

	/**
	 * 当正常监听服务器地址后调用本函数可以获得监听套接口
	 * @return {int}
	 */
#if defined(_WIN32) || defined(_WIN64)
	SOCKET sock_handle() const
#else
	int sock_handle() const
#endif
	{
		return fd_;
	}

	/**
	 * 设置监听套接字的延迟接收功能，即当客户端连接上有数据时才将该连接返回
	 * 给应用，目前该功能仅支持 Linux
	 * @param timeout {int} 如果客户端连接在规定的时间内未发来数据，
	 *  也将该连接返回给应用
	 */
	void set_tcp_defer_accept(int timeout);

private:
	int   backlog_;
	bool  block_;
	bool  unix_sock_;
	char  addr_[64];

#if defined(_WIN32) || defined(_WIN64)
	SOCKET fd_;
	SOCKET fd_local_;
#else
	int   fd_;
	int   fd_local_;
#endif
};

} // namespace acl
