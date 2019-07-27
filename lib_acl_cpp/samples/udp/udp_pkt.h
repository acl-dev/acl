#pragma once
#include <vector>
#include "sock_addr.h"

class udp_sock;

/**
 * UDP 鏁版嵁鍖呯被
 */
class udp_pkt
{
public:
	/**
	 * 鏋勯€犲嚱鏁�
	 * @param size {size_t} 鎸囧畾浜嗘瘡涓� udp 鏁版嵁鍖呯殑鏈€澶у€�
	 */
	udp_pkt(size_t size = 1460);
	~udp_pkt(void);

	/**
	 * 璁剧疆瑕佸彂閫佺殑鏁版嵁鍖呮暟鎹�
	 * @param data {const void*} 鏁版嵁鍖呭湴鍧€锛岄潪 NULL
	 * @param len {size_t} 鏁版嵁鍖呭ぇ灏忥紝椤� > 0 涓� <= 鏋勯€犲嚱鏁伴噷璁剧疆鐨勬渶澶у€�
	 * @return {bool} 鍙傛暟闈炴硶鏃惰繑鍥� false
	 */
	bool set_data(const void* data, size_t len);

	/**
	 * 璁剧疆鏁版嵁鍖呮帴鏀剁鍦板潃锛屾湰鏂规硶浠呯敤鍦ㄥ鎴风鏂瑰紡涓�
	 * @param addr {const char*} udp 鐩爣鍦板潃
	 * @return {bool} 杩斿洖 false 琛ㄧず鍦板潃闈炴硶
	 */
	bool set_peer(const char* addr);

	/**
	 * 鑾峰緱璇诲埌鐨勬暟鎹寘鍐呭锛岄渶鍏堥€氳繃 get_dlen 鑾峰緱鏁版嵁澶у皬
	 * @return {void*}
	 */
	void* get_data(void) const
	{
		return iov_.iov_base;
	}

	/**
	 * 鑾峰緱璇诲埌鐨勬暟鎹寘澶у皬
	 * @return {size_t}
	 */
	size_t get_dlen(void) const
	{
		return iov_.iov_len;
	}

	/**
	 * 褰撹鍒版暟鎹寘鍚庯紝鏈柟娉曡繑鍥炲彂閫佺鐨勭鍙ｅ彿
	 * @return {int} 杩斿洖鍊� < 0 琛ㄧず鍑洪敊
	 */
	int get_port(void) const;

	/**
	 * 褰撹鍒版暟鎹寘鍚庯紝鏈柟娉曡繑鍥炲彂閫佺鐨� IP 鍦板潃
	 * @return {const char*} 杩斿洖 NULL 琛ㄧず鏃犳硶鑾峰緱鍙戦€佺鍦板潃
	 */
	const char* get_ip(void) const;

private:
	friend class udp_sock;

	struct iovec iov_;		// 瀛樻斁鏀�/鍙戠殑鏁版嵁鍦板潃鍙婇暱搴�
	const size_t size_;		// 鏋勯€犳椂璁惧畾鍚庝笉鑳藉啀淇敼
	SOCK_ADDR    addr_;		// 婧愬湴鍧€
	socklen_t    addr_len_;
	char         ipbuf_[64];
};

/**
 * UDP 鏁版嵁鍖呴泦鍚堝璞�
 */
class udp_pkts
{
public:
	/**
	 * 鏋勯€犲嚱鏁�
	 * @param max {size_t} 鎸囧畾鎺ユ敹 UDP 鏃舵渶澶х殑鎺ユ敹鏁版嵁鍖呬釜鏁帮紝璇ュ€奸渶 > 0
	 */
	udp_pkts(size_t max);
	~udp_pkts(void);

	/**
	 * 鑾峰緱鎸囬拡涓嬫爣鐨勬暟鎹寘瀵硅薄
	 * @param i {size_t} 涓嬫爣鍊�
	 * @return {udp_pkt*} 杩斿洖 NULL 琛ㄧず涓嬫爣瓒婄晫
	 */
	udp_pkt* operator[](size_t i);

	/**
	 * 鑾峰緱宸茶鍒扮殑鎴栧皢瑕佸彂閫佺殑鏁版嵁鍖呬釜鏁�
	 * @return {size_t}
	 */
	size_t get_npkt(void) const
	{
		return npkt_;
	}

	/**
	 * 璁剧疆鎹寘涓暟锛屽簲鐢ㄥ彲璋冪敤姝ゆ柟娉曡缃鍙戦€佺殑鏁版嵁鍖呬釜鏁帮紱
	 * upd_sock 绫诲湪璇诲埌鏁版嵁鏃朵篃浼氶€氳繃姝ゆ柟娉曡缃鍒版暟鎹寘涓暟
	 * @param n {size_t} 鎹寘涓暟锛岃鍊间笉鑳借秴杩囨瀯閫犳椂鎸囧畾鐨勬渶澶у€�
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
