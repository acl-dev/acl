#pragma once
#include "acl_cpp/acl_cpp_define.hpp"

namespace acl {

class socket_stream;

/**
 * 服务端监听套接口类，接收客户端连接，并创建客户端流连接对象
 */
class ACL_CPP_API server_socket
{
public:
	/**
	 * 构造函数
	 * @param backlog {int} 监听套接口队列长度
	 * @param block {bool} 是阻塞模式还是非阻塞模式
	 */
	server_socket(int backlog = 128, bool block = true);
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
#ifdef	WIN32
	SOCKET sock_handle() const
#else
	int sock_handle() const
#endif
	{
		return fd_;
	}

private:
	int   backlog_;
	bool  block_;
	bool  unix_sock_;
	char  addr_[64];

#ifdef WIN32
	SOCKET fd_;
#else
	int   fd_;
#endif
};

} // namespace acl
