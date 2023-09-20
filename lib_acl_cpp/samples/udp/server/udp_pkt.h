#pragma once
#include <vector>
#include "sock_addr.h"

class udp_sock;

/**
 * UDP 数据包类
 */
class udp_pkt
{
public:
	/**
	 * 构造函数
	 * @param size {size_t} 指定了每个 udp 数据包的最大值
	 */
	udp_pkt(size_t size = 1460);
	~udp_pkt(void);

	/**
	 * 设置要发送的数据包数据
	 * @param data {const void*} 数据包地址，非 NULL
	 * @param len {size_t} 数据包大小，须 > 0 且 <= 构造函数里设置的最大值
	 * @return {bool} 参数非法时返回 false
	 */
	bool set_data(const void* data, size_t len);

	/**
	 * 设置数据包接收端地址，本方法仅用在客户端方式下
	 * @param addr {const char*} udp 目标地址
	 * @return {bool} 返回 false 表示地址非法
	 */
	bool set_peer(const char* addr);

	/**
	 * 获得读到的数据包内容，需先通过 get_dlen 获得数据大小
	 * @return {void*}
	 */
	void* get_data(void) const
	{
		return iov_.iov_base;
	}

	/**
	 * 获得读到的数据包大小
	 * @return {size_t}
	 */
	size_t get_dlen(void) const
	{
		return iov_.iov_len;
	}

	/**
	 * 当读到数据包后，本方法返回发送端的端口号
	 * @return {int} 返回值 < 0 表示出错
	 */
	int get_port(void) const;

	/**
	 * 当读到数据包后，本方法返回发送端的 IP 地址
	 * @return {const char*} 返回 NULL 表示无法获得发送端地址
	 */
	const char* get_ip(void) const;

private:
	friend class udp_sock;

	struct iovec iov_;		// 存放收/发的数据地址及长度
	const size_t size_;		// 构造时设定后不能再修改
	SOCK_ADDR    addr_;		// 源地址
	socklen_t    addr_len_;
	char         ipbuf_[64];
};

/**
 * UDP 数据包集合对象
 */
class udp_pkts
{
public:
	/**
	 * 构造函数
	 * @param max {size_t} 指定接收 UDP 时最大的接收数据包个数，该值需 > 0
	 */
	udp_pkts(size_t max);
	~udp_pkts(void);

	/**
	 * 获得指针下标的数据包对象
	 * @param i {size_t} 下标值
	 * @return {udp_pkt*} 返回 NULL 表示下标越界
	 */
	udp_pkt* operator[](size_t i);

	/**
	 * 获得已读到的或将要发送的数据包个数
	 * @return {size_t}
	 */
	size_t get_npkt(void) const
	{
		return npkt_;
	}

	/**
	 * 设置据包个数，应用可调用此方法设置要发送的数据包个数；
	 * upd_sock 类在读到数据时也会通过此方法设置读到数据包个数
	 * @param n {size_t} 据包个数，该值不能超过构造时指定的最大值
	 */
	void set_npkt(size_t n);

protected:
	friend class udp_sock;

	std::vector<udp_pkt*>& get_pkts(void)
	{
		return pkts_;
	}

private:
	std::vector<udp_pkt*> pkts_;
	size_t max_;
	size_t npkt_;
};
