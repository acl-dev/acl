#pragma once
#include <vector>
#include "sock_addr.h"

class udp_pkt;
class udp_pkts;

/**
 * UDP 鏁版嵁鍖呮敹鍙戠被
 */
class udp_sock
{
public:
	udp_sock(void);
	~udp_sock(void);

	/**
	 * 鏍规嵁 UDP 濂楁帴鍙ｅ彞鏌勫垱寤� UDP 瀵硅薄
	 * @param fd {int} 濂楁帴鍣ㄥ彞鏌勶紝璇ュ€奸渶 > 0
	 * @return {bool} 杩斿洖 false 琛ㄧず濂楁帴鍙ｉ潪娉�
	 */
	bool open(int fd);

	/**
	 * 瀹㈡埛绔ā寮忎笅璋冪敤姝ゆ柟娉曞垱寤� UDP 瀹㈡埛绔璞�
	 * @param local {const char*} 缁戝畾鐨勬湰鍦板湴鍧€锛屾牸寮忥細ip:port
	 * @param peer {const char*} 杩滅鐩爣鍦板潃锛屾牸寮忥細ip:port
	 * @return {bool} 杩斿洖 false 琛ㄧず鍙傛暟闈炴硶鎴栫粦瀹氬湴鍧€澶辫触
	 */
	bool client_open(const char* local, const char* peer);

	/**
	 * 鏈嶅姟绔ā寮忎笅璋冪敤姝ゆ柟娉曞垱寤� UDP 鏈嶅姟绔璞�
	 * @param local {const char*} 缁戝畾鐨勬湰鍦板湴鍧€锛屾牸寮忥細ip:port
	 * @return {bool} 杩斿洖 false 琛ㄧず鍙傛暟闈炴硶鎴栫粦瀹氬湴鍧€澶辫触
	 */
	bool server_open(const char* local);

	/**
	 * 璋冪敤姝ゆ柟娉曞彂閫佸崟涓€鏁版嵁鍖�
	 * @param data {const void*} 鏁版嵁鍖呭湴鍧€
	 * @param len {size_t} 鏁版嵁鍖呴暱搴�
	 * @return {ssize_t} 杩斿洖鍊� < 0 琛ㄧず鍙戦€佸け璐�
	 */
	ssize_t send(const void* data, size_t len);

	/**
	 * 璋冪敤姝ゆ柟娉曟帴鏀跺崟涓€鏁版嵁鍖�
	 * @param buf {void*} 缂撳啿鍖哄湴鍧€
	 * @param len {size_t} 缂撳啿鍖哄ぇ灏�
	 * @return {ssize_t} 杩斿洖鍊� < 0 琛ㄧず鎺ユ敹澶辫触
	 */
	ssize_t recv(void* buf, size_t len);

	/**
	 * 璋冪敤姝ゆ柟娉曚竴娆℃€ф帴鏀跺涓� UDP 鏁版嵁鍖�
	 * @param pkts {udp_pkts&} 瀛樻斁鎺ユ敹鍒扮殑鏁版嵁鍖�
	 * @param {int} 杩斿洖鎺ユ敹鍒扮殑鏁版嵁鍖呬釜鏁帮紝杩斿洖 < 0 琛ㄧず鍑洪敊
	 */
	int recv(udp_pkts& pkts);

	/**
	 * 璋冪敤姝ゆ柟娉曚竴娆℃€у彂閫佸涓� UDP 鏁版嵁鍖�
	 * @param pkts {udp_pkts&} 瀛樻斁瑕佸彂閫佺殑鏁版嵁鍖�
	 * @param {int} 杩斿洖鍙戦€佺殑鏁版嵁鍖呬釜鏁帮紝杩斿洖 < 0 琛ㄧず鍑洪敊
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
