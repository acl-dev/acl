#pragma once
#include "fiber_cpp_define.hpp"
#include <string>

namespace acl {

class keeper_waiter;
class socket_stream;
class thread_mutex;

/**
 * 鐙珛绾跨▼鐢ㄤ簬棰勫厛涓庢湇鍔″櫒鍒涘缓绌洪棽杩炴帴锛屽鎴风鍙互鐩存帴浠庤杩炴帴姹犱腑鑾峰彇鏂拌繛鎺ワ紝
 * 杩欏浜� ping rtt 杈冮暱锛堝锛�10ms 浠ヤ笂锛夋瘮杈冩湁浠峰€硷紝鍙互鏈夋晥鍦板噺灏戝洜缃戠粶 rtt
 * 閫犳垚鐨勮繛鎺ユ椂闂存崯鑰�
 */
class tcp_keeper : public thread
{
public:
	tcp_keeper(void);
	~tcp_keeper(void);

	/**
	 * 璁剧疆寤虹珛缃戠粶杩炴帴鐨勮秴鏃舵椂闂达紙绉掞級
	 * @param n {int}
	 * @return {tcp_keeper&}
	 */
	tcp_keeper& set_conn_timeout(int n);

	/**
	 * 璁剧疆缃戠粶濂楁帴瀛� IO 璇诲啓瓒呮椂鏃堕棿锛堢锛�
	 * @param n {int}
	 * @return {tcp_keeper&}
	 */
	tcp_keeper& set_rw_timeout(int n);

	/**
	 * 璁剧疆杩炴帴姹犱腑绌洪棽杩炴帴鐨勬渶灏忚繛鎺ユ暟
	 * @param n {int}
	 * @return {tcp_keeper&}
	 */
	tcp_keeper& set_conn_min(int n);

	/**
	 * 璁剧疆杩炴帴姹犱腑绌洪棽杩炴帴鐨勬渶澶ц繛鎺ユ暟
	 * @param n {int}
	 * @return {tcp_keeper&}
	 */
	tcp_keeper& set_conn_max(int n);

	/**
	 * 璁剧疆缃戠粶杩炴帴鐨勭┖闂叉椂闂达紙绉掞級锛岀┖闂叉椂闂磋秴杩囨鍊兼椂杩炴帴灏嗚鍏抽棴
	 * @param ttl {int}
	 * @return {tcp_keeper&}
	 */
	tcp_keeper& set_conn_ttl(int ttl);

	/**
	 * 璁剧疆姣忎釜杩炴帴姹犵殑绌洪棽鏃堕棿锛堢锛夛紝鍗冲綋璇ヨ繛鎺ユ睜鐨勭┖闂叉椂闂磋秴杩囨鍊兼椂
	 * 灏嗚閲婃斁锛屼粠鑰屼究浜庣郴缁熷洖鏀跺唴瀛樿祫婧�
	 * @param ttl {int}
	 * @return {tcp_keeper&}
	 */
	tcp_keeper& set_pool_ttl(int ttl);

	/**
	 * 璁剧疆 rtt 闃€鍊硷紙绉掞級锛屽綋缃戠粶杩炴帴鏃堕棿瓒呰繃姝ゅ€兼椂鎵嶄細鍚敤浠庤繛鎺ユ睜鎻愬彇
	 * 杩炴帴鏂瑰紡锛屽鏋滅綉缁滆繛鎺ユ椂闂村皬浜庢鍊硷紝鍒欑洿鎺ヨ繛鎺ユ湇鍔″櫒
	 * @param rtt {double}
	 * @return {tcp_keeper&}
	 */
	tcp_keeper& set_rtt_min(double rtt);

	/**
	 * 浠� tcp_keeper 瀵硅薄涓彁鍙栧搴斿湴鍧€鐨勭綉缁滆繛鎺ュ鎺�
	 * @param addr {const char*} 鏈嶅姟鍣ㄥ湴鍧€锛屾牸寮忥細ip:port
	 * @param hit {bool*} 闈炵┖鏃讹紝灏嗗瓨鏀捐杩炴帴鏄惁鍦ㄨ繛鎺ユ睜鐨勭┖闂茶繛鎺ヤ腑鍛戒腑
	 * @param sync {bool} 鏄惁閲囩敤鐩磋繛妯″紡锛屽鏋滈噰鐢ㄧ洿杩炴ā寮忥紝鍒欏唴閮ㄤ笉浼�
	 *  閽堝璇ュ湴鍧€棰勫垱杩炴帴姹�
	 * @return {socket_stream*} 杩斿洖 NULL 琛ㄧず杩炴帴澶辫触
	 */
	socket_stream* peek(const char* addr, bool* hit = NULL,
		bool sync = false);

	/**
	 * 鍋滄 tcp_keeper 绾跨▼杩愯
	 */
	void stop(void);

protected:
	// @override
	void* run(void);

private:
	double rtt_min_;
	keeper_waiter* waiter_;
	std::map<std::string, double> addrs_;
	thread_mutex* lock_;

	bool direct(const char* addr, bool& found);
	void remove(const char* addr);
	void update(const char* addr, double cost);
};

} // namespace acl
