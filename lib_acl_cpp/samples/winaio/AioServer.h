#pragma once
#include "acl_cpp/stream/aio_listen_stream.hpp"
#include "acl_cpp/stream/aio_socket_stream.hpp"

/**
* 异步客户端流的回调类的子类
*/
class CAcceptedClientCallback : public acl::aio_callback
{
public:
	CAcceptedClientCallback(acl::aio_socket_stream* client);

	~CAcceptedClientCallback();

	/**
	* 实现父类中的虚函数，客户端流的读成功回调过程
	* @param data {char*} 读到的数据地址
	* @param len {int} 读到的数据长度
	* @return {bool} 返回 true 表示继续，否则希望关闭该异步流
	*/
	bool read_callback(char* data, int len);

	/**
	* 实现父类中的虚函数，客户端流的写成功回调过程
	* @return {bool} 返回 true 表示继续，否则希望关闭该异步流
	*/
	bool write_callback();

	/**
	* 实现父类中的虚函数，客户端流的超时回调过程
	*/
	void close_callback();

	/**
	* 实现父类中的虚函数，客户端流的超时回调过程
	* @return {bool} 返回 true 表示继续，否则希望关闭该异步流
	*/
	bool timeout_callback();

private:
	acl::aio_socket_stream* client_;
	int   i_;
};

/**
* 异步监听流的回调类的子类
*/
class CServerCallback : public acl::aio_accept_callback
{
public:
	CServerCallback();
	~CServerCallback();

	/**
	* 基类虚函数，当有新连接到达后调用此回调过程
	* @param client {aio_socket_stream*} 异步客户端流
	* @return {bool} 返回 true 以通知监听流继续监听
	*/
	bool accept_callback(acl::aio_socket_stream* client);
};
