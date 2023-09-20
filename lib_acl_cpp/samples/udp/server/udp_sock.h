#pragma once
#include <vector>
#include "sock_addr.h"

class udp_pkt;
class udp_pkts;

/**
 * UDP 数据包收发类
 */
class udp_sock
{
public:
	udp_sock(void);
	~udp_sock(void);

	/**
	 * 根据 UDP 套接口句柄创建 UDP 对象
	 * @param fd {int} 套接器句柄，该值需 > 0
	 * @return {bool} 返回 false 表示套接口非法
	 */
	bool open(int fd);

	/**
	 * 客户端模式下调用此方法创建 UDP 客户端对象
	 * @param local {const char*} 绑定的本地地址，格式：ip:port
	 * @param peer {const char*} 远端目标地址，格式：ip:port
	 * @return {bool} 返回 false 表示参数非法或绑定地址失败
	 */
	bool client_open(const char* local, const char* peer);

	/**
	 * 服务端模式下调用此方法创建 UDP 服务端对象
	 * @param local {const char*} 绑定的本地地址，格式：ip:port
	 * @return {bool} 返回 false 表示参数非法或绑定地址失败
	 */
	bool server_open(const char* local);

	/**
	 * 调用此方法发送单一数据包
	 * @param data {const void*} 数据包地址
	 * @param len {size_t} 数据包长度
	 * @return {ssize_t} 返回值 < 0 表示发送失败
	 */
	ssize_t send(const void* data, size_t len);

	/**
	 * 调用此方法接收单一数据包
	 * @param buf {void*} 缓冲区地址
	 * @param len {size_t} 缓冲区大小
	 * @return {ssize_t} 返回值 < 0 表示接收失败
	 */
	ssize_t recv(void* buf, size_t len);

	/**
	 * 调用此方法一次性接收多个 UDP 数据包
	 * @param pkts {udp_pkts&} 存放接收到的数据包
	 * @param {int} 返回接收到的数据包个数，返回 < 0 表示出错
	 */
	int recv(udp_pkts& pkts);

	/**
	 * 调用此方法一次性发送多个 UDP 数据包
	 * @param pkts {udp_pkts&} 存放要发送的数据包
	 * @param {int} 返回发送的数据包个数，返回 < 0 表示出错
	 */
	int send(udp_pkts& pkts);

private:
	int             fd_;

	SOCK_ADDR       sa_local_;
	socklen_t       sa_local_len_;

	SOCK_ADDR       sa_peer_;
	socklen_t       sa_peer_len_;

	struct mmsghdr* msgvec_;
	size_t          vlen_;

	bool bind(const char* addr);
	int  recv(std::vector<udp_pkt*>& pkts);
	int  send(std::vector<udp_pkt*>& pkts, size_t max);
};
