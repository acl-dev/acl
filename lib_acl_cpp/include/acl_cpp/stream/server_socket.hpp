#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/string.hpp"
#include "../stdlib/noncopyable.hpp"
#if defined(_WIN32) || defined(_WIN64)
#include <WinSock2.h>
#endif

namespace acl {

class socket_stream;

enum {
	OPEN_FLAG_NONE      = 0,
	OPEN_FLAG_NONBLOCK  = 1,	// 非阻塞模式
	OPEN_FLAG_REUSEPORT = 1 << 1,	// 端口复用，要求 Linux3.0 以上
	OPEN_FLAG_EXCLUSIVE = 1 << 2,	// 是否禁止复用地址
};

/**
 * 服务端监听套接口类，接收客户端连接，并创建客户端流连接对象
 */
class ACL_CPP_API server_socket : public noncopyable
{
public:
#if 0
	/**
	 * 构造函数，调用本构造函数后需调用类方法 open 来监听指定服务地址
	 * @param backlog {int} 监听套接口队列长度
	 * @param block {bool} 是阻塞模式还是非阻塞模式
	 */
	server_socket(int backlog, bool block);
#endif

	/**
	 * 构造函数
	 * @param flag {unsigned} 定义参见 OPEN_FLAG_XXX
	 * @param backlog {int} 监听套接口队列长度
	 */
	server_socket(unsigned flag, int backlog);

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

	server_socket(void);
	~server_socket(void);

	/**
	 * 开始监听给定服务端地址
	 * @param addr {const char*} 服务器监听地址，格式为：
	 *  ip:port；在 unix 环境下，还可以是域套接口，格式为：/path/xxx，在
	 *  Linux 平台下，如果域套接口地址为：@xxx 格式，即第一个字母为 @ 则
	 *  内部自动启用 Linux 下的抽象域套接字方式（abstract unix socket）
	 * @return {bool} 监听是否成功
	 */
	bool open(const char* addr);

	/**
	 * 判断当前监听套接口是否打开着
	 * @return {bool}
	 */
	bool opened(void) const;

	/**
	 * 关闭已经打开的监听套接口
	 * @return {bool} 是否正常关闭
	 */
	bool close(void);

	/**
	 * 将监听套接口从服务监听对象中解绑
	 * @return {SOCKET} 返回被解绑的句柄
	 */
#if defined(_WIN32) || defined(_WIN64)
	SOCKET unbind(void);
#else
	int unbind(void);
#endif

	/**
	 * 接收客户端连接并创建客户端连接流
	 * @param timeout {int} 当该值 > 0 时，采用超时方式接收客户端连接，
	 *  若在指定时间内未获得客户端连接，则返回 NULL
	 * @param etimed {bool*} 当此指针非 NULL 时，如果因超时导致该函数返回
	 *  NULL，则此值被置为 true
	 * @return {socket_stream*} 返回空表示接收失败或超时
	 */
	socket_stream* accept(int timeout = 0, bool* etimed = NULL);

	/**
	 * 获得监听的地址
	 * @return {const char*} 返回值非空指针
	 */
	const char* get_addr(void) const
	{
		return addr_.c_str();
	}

	/**
	 * 当正常监听服务器地址后调用本函数可以获得监听套接口
	 * @return {int}
	 */
#if defined(_WIN32) || defined(_WIN64)
	SOCKET sock_handle(void) const
#else
	int sock_handle(void) const
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
	int      backlog_;
	unsigned open_flag_;
	bool     unix_sock_;
	string   addr_;

#if defined(_WIN32) || defined(_WIN64)
	SOCKET fd_;
	SOCKET fd_local_;
#else
	int   fd_;
	int   fd_local_;
#endif
};

} // namespace acl
