#pragma once
#include "../acl_cpp_define.hpp"
#if defined(_WIN32) || defined(_WIN64)
#include <WinSock2.h>
#endif
#include <string>
#include "istream.hpp"
#include "ostream.hpp"

struct ACL_VSTREAM;

namespace acl {

class ACL_CPP_API socket_stream : public istream , public ostream {
public:
	socket_stream();
	~socket_stream();

	/**
	 * 根据套接字打开的一个网络流
	 * @param fd 套接字
	 * @param udp_mode {bool} 是否是 UDP 方式
	 * @return {bool} 连接是否成功
	 */
#if defined(_WIN32) || defined(_WIN64)
	bool open(SOCKET fd, bool udp_mode = false);
#else
	bool open(int fd, bool udp_mode = false);
#endif

	/**
	 * 根据 ACL_VSTREAM 流打开网络流
	 * @param vstream {ACL_VSTREAM*}
	 * @param udp_mode {bool} 是否是 UDP 方式
	 * @return {bool} 连接是否成功
	 */
	bool open(ACL_VSTREAM* vstream, bool udp_mode = false);

	/**
	 * 连接远程服务器并打开网络连接流
	 * @param addr {const char*} 服务器地址, 若连接域套接口服务器(UNIX平台),
	 *  域套接地址：/tmp/test.sock，在Linux 平台下还可连接抽象域套接字，即
	 *  abastract unix socket，为了与普通基于文件路径的unix域套接地址区别，
	 *  在 acl 库中规定如果地址第一个字节为 @，则认为是 Linux 抽象域套接字
	 *  （abstract unix domain socket）不过需注意该功能仅有 Linux 平台支持,
	 *  举例，如：@/tmp/test.sock；;
	 *  remote_addr[@local_ip]|[#interface], 如:
	 *  1. www.sina.com|80@60.28.250.199, 意思是绑定本网卡地址为:
	 *     60.28.250.199, 远程连接 www.sina.com 的 80 端口;
	 *  2.  211.150.111.12|80@192.168.1.1，表示仅绑定本地地址；
	 *  3. 211.150.111.12|80#eth1，表示仅绑定本地指定网卡;
	 *  4. 如果由OS 自动绑定本地 IP 地址，则可以写为：www.sina.com:80;
	 * @param conn_timeout {int} 连接超时时间(单位值取决于 use_ms)
	 * @param rw_timeout {int} 读写超时时间(单位值取决于 use_ms)
	 * @param unit {time_unit_t} 超时时间的时间单位
	 * @return {bool} 连接是否成功
	 */
	bool open(const char* addr, int conn_timeout, int rw_timeout,
		time_unit_t unit = time_unit_s);

	/**
	 * 绑定本地 UDP 地址，创建 UDP 通信对象
	 * @param addr {const char*} 本机地址，格式：ip:port；该地址也可以为
	 *  UNIX 域套接字或 Linux 抽象域套接字（Linux abstract unix socket）
	 * @param rw_timeout {int} 读写超时时间(秒)
	 * @param flags {unsigned} 该标志位的定义参加 server_socket.hpp
	 * @return {bool} 绑定是否成功
	 */
	bool bind_udp(const char* addr, int rw_timeout = -1, unsigned flags = 0);

	/**
	 * 绑定并创建组播套接口通信对象
	 * @param addr {const char*} 组播 IP 地址
	 * @param iface {const char*} 本机用来收发数据包的 IP 地址
	 * @param port {int} 组播端口号
	 * @param rw_timeout {int} IO 读写超时时间
	 * @param flags {unsigned} 该标志位的定义参加 server_socket.hpp
	 * @return {bool} 绑定是否成功
	 */
	bool bind_multicast(const char* addr, const char* iface, int port,
		int rw_timeout = -1, unsigned flags = 0);

	/**
	 * 当 bind_multicast 成功后，可以调用本方法设置广播包的 TTL 值
	 * @param ttl {int} 该值的范围为 1--255
	 * @return {bool} 设置是否成功
	 */
	bool multicast_set_ttl(int ttl);

	/**
	 * 当 bind_multicast 成功后，可以调用本方法设置广播包的本地 IP 地址
	 * @param iface {const char*}
	 * @return {bool} 设置是否成功
	 */
	bool multicast_set_if(const char* iface);

	/**
	 * 当 bind_multicast 成功后，可以调用本方法设置离开组播
	 * @param addr {const char*} 广播 IP
	 * @param iface {const char*} 本地 IP
	 * @return {bool} 是否成功
	 */
	bool multicast_drop(const char *addr, const char *iface);

	/**
	 * 关闭套接口读操作
	 * @return {bool}
	 */
	bool shutdown_read();

	/**
	 * 关闭套接口写操作
	 * @return {bool}
	 */
	bool shutdown_write();

	/**
	 * 关闭套接口读写操作
	 * @return {bool}
	 */
	bool shutdown_readwrite();

	/**
	 * 获得网络连接流的套接字连接句柄
	 * @return {ACL_SOCKET} 若出错，则返回 - 1(UNIX 平台)
	 *  或 INVALID_SOCKET(win32平台)
	 */
#if defined(_WIN32) || defined(_WIN64)
	SOCKET sock_handle(void) const;
#else
	int   sock_handle() const;
#endif

	/**
	 * 解绑套接字与流对象的绑定关系，同时将套接字返回给用户，即
	 * 将该套接字的管理权交给用户，本流对象在释放时不会关闭该套
	 * 接字，但用户接管该套接字后用完后必须将其关闭
	 * 解绑后除了 close/open 的调用有意义外，其它的调用(包括流对
	 * 象读写在内)都无意义
	 * @return {ACL_SOCKET} 返回 ACL_SOCKET_INVALID 表示该流对象
	 *  已经将套接字解绑
	 */
#if defined(_WIN32) || defined(_WIN64)
	SOCKET unbind_sock(void);
#else
	int    unbind_sock();
#endif

	/**
	 * 获得 socket 的类型
	 * @return {int} 返回值有：AF_INET, AF_INT6, AF_UNIX，出错时返回 -1
	 */
	int sock_type() const;

	/**
	 * 获得远程连接的地址
	 * @param full {bool} 是否获得完整地址，即：IP:PORT，如果该参数
	 *  为 false，则仅返回 IP，否则返回 IP:PORT
	 * @return {const char*} 远程连接地址，若返回值 == '\0' 则表示
	 *  无法获得远程连接地址
	 */
	const char* get_peer(bool full = false) const;

	/**
	 * 获得远程连接的 IP 地址
	 * @return {const char*} 远程连接地址，若返回值 == '\0' 则表示
	 *  无法获得远程连接地址
	 */
	const char* get_peer_ip() const;

	/**
	 * 设置远程连接对象的地址，对于 TCP 传输方式，不需要显示调用此函数
	 * 设置远程对象地址，UDP 传输方式时需要调用此函数设置远程地址，然后
	 * 才可以向远程连接写数据
	 * @param addr {const char*} 远程连接对象的地址，格式：ip:port
	 * @return {bool} 当流对象未打开时返回 false
	 */
	bool set_peer(const char* addr);

	/**
	 * 获得连接的本地地址
	 * @param full {bool} 是否获得完整地址，即：IP:PORT，如果该参数
	 *  为 false，则仅返回 IP，否则返回 IP:PORT
	 * @return {const char*} 该连接的本地地址，若返回值 == "" 则表示
	 *  无法获得本地地址
	 */
	const char* get_local(bool full = false) const;

	/**
	 * 获得连接的本地 IP 地址
	 * @return {const char*} 该连接的本地地址，若返回值 == "" 则表示
	 *  无法获得本地地址
	 */
	const char* get_local_ip() const;

	/**
	 * 设置本地地址
	 * @param addr {const char*} 地址，格式：ip:port
	 * @return {bool} 当流对象未打开时返回 false
	 */
	bool set_local(const char* addr);

	/**
	 * 检查套接口连接的存活状态(内部使用了非阻塞读的方式进行探测)
	 * @return {bool} 当网络连接未打开或已经关闭时该函数返回 false，如果
	 *  连接正常则返回 true
	 */
	bool alive() const;

	/**
	 * 设置 TCP 套接字的 nodelay 功能
	 * @param on {bool} true 表示打开，false 表示关闭
	 * @return {socket_stream&}
	 */
	socket_stream& set_tcp_nodelay(bool on);

	/**
	 * 设置 TCP 套接字的 SO_LINGER 选项
	 * @param on {bool} 是否启用 SO_LINGER 选项
	 * @param linger {int} 当SO_LINGER打开时，取消 timed_wait 的时间，单位为秒
	 * @return {socket_stream&}
	 */
	socket_stream& set_tcp_solinger(bool on, int linger);

	/**
	 * 设置 TCP 套接字的写缓冲区大小
	 * @param size {int} 缓冲区设置大小
	 * @return {socket_stream&}
	 */
	socket_stream& set_tcp_sendbuf(int size);

	/**
	 * 设置 TCP 套接字的读缓冲区大小
	 * @param size {int} 缓冲区设置大小
	 * @return {socket_stream&}
	 */
	socket_stream& set_tcp_recvbuf(int size);

	/**
	 * 设置 TCP 套接字的非阻塞状态
	 * @param on {bool} 是否设置为非阻塞状态，当为 true 时，
	 *  则该套接字被设为非阻塞状态；否则为阻塞状态
	 * @return {socket_stream&}
	 */
	socket_stream& set_tcp_non_blocking(bool on);

	/**
	 * 获得 TCP 套接字是否设置了 nodelay 选项
	 * @return {bool} true 表示打开，false 表示关闭
	 */
	bool get_tcp_nodelay() const;

	/**
	 * 获得 TCP 套接字的 linger 值
	 * @return {int} 返回 -1 表示未设置 linger 选项或内部出错，>= 0
	 *  表示设置了 linger 选项且该值表示套接字关闭后该 TCP 连接在内核中
	 *  维持 TIME_WAIT 状态的逗留时间(秒)
	 */
	int get_tcp_solinger() const;

	/**
	 * 获取 TCP 套接字的写缓冲区大小
	 * @return {int} 缓冲区大小
	 */
	int get_tcp_sendbuf() const;

	/**
	 * 获取 TCP 套接字的读缓冲区大小
	 * @return {int} 缓冲区大小
	 */
	int get_tcp_recvbuf() const;

	/**
	 * 判断当前套接字是否被设置了非阻塞模式
	 * @return {bool}
	 * 注：该方法目前仅支持 UNIX 平台
	 */
	bool get_tcp_non_blocking() const;

private:
	std::string ipbuf_;
	const char* get_ip(const char* addr, std::string& out);
};

} // namespace acl
