#pragma once
#include "../acl_cpp_define.hpp"
#include <map>
#include <vector>
#if !defined(_WIN32) && !defined(_WIN64)
#include <sys/time.h>
#endif
#include "../stream/aio_socket_stream.hpp"                                
#include "../stdlib/string.hpp"

namespace acl
{

class check_timer;
class aio_socket_stream;

/**
 * 异步连接回调函数处理类
 */
class ACL_CPP_API check_client : public aio_open_callback
{
public:
	check_client(check_timer& timer, const char* addr,
		aio_socket_stream& conn, struct timeval& begin);

	/**
	 * 获得输入的非阻塞 IO 句柄
	 * @return {aio_socket_stream&}
	 */
	aio_socket_stream& get_conn() const
	{
		return conn_;
	}

	/**
	 * 获得传入的服务端地址
	 * @return {const char*}
	 */
	const char* get_addr() const
	{
		return addr_.c_str();
	}

	/**
	 * 设置连接是否是存活的
	 * @param yesno {bool}
	 */
	void set_alive(bool yesno);

	/**
	 * 关闭非阻塞 IO 句柄
	 */
	void close();

public:
	// 以下的函数仅供内部使用
	/**
	 * 当前检测对象是否处于阻塞模式下
	 * @return {bool}
	 */
	bool blocked() const
	{
		return blocked_;
	}

	/**
	 * 在阻塞检测方式下，调用此函数用来设置检测对象是否处于阻塞状态，
	 * 处于阻塞状态时该检测对象是禁止通过调用方法 close 来关闭的
	 * @param on {bool} 设置检测对象是否处于阻塞状态，缺省为处于阻塞状态
	 */
	void set_blocked(bool on);

private:
	// 基类虚函数
	bool open_callback();
	void close_callback();
	bool timeout_callback();

private:
	~check_client() {}

private:
	bool blocked_;
	bool aliving_;
	bool timedout_;
	struct timeval begin_;
	check_timer& timer_;
	aio_socket_stream& conn_;
	string addr_;
};

} // namespace acl
