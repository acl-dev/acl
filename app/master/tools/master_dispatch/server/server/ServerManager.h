#pragma once
#include <vector>

class ServerConnection;

/**
 * 鏈嶅姟绔繛鎺ュ璞＄鐞嗗櫒
 */
class ServerManager : public acl::singleton <ServerManager>
{
public:
	ServerManager() {}
	~ServerManager() {}

	/**
	 * 娣诲姞鏂扮殑鏈嶅姟绔繛鎺ュ璞�
	 * @param conn {ServerConnection*}
	 */
	void set(ServerConnection* conn);

	/**
	 * 鍒犻櫎鏌愪釜鎸囧畾鐨勬湇鍔＄杩炴帴瀵硅薄
	 */
	void del(ServerConnection* conn);

	/**
	 * 鍙栧嚭鏈嶅姟绔繛鎺ュ璞′腑璐熻浇鏈€浣庣殑涓€涓繛鎺ュ璞�
	 * @return {ServerConnection*} 杩斿洖 NULL 琛ㄧず娌℃湁鍙敤鐨勬湇鍔″璞�
	 */
	ServerConnection* min();

	/**
	 * 鑾峰緱鎵€鏈夌殑鏈嶅姟绔繛鎺ュ璞＄殑涓暟
	 * @return {size_t}
	 */
	size_t length() const
	{
		return conns_.size();
	}

	/**
	 * 缁熻鏈嶅姟鍣ㄩ泦缇や腑鐨勫悇涓湇鍔″瓙杩涚▼瀹炰緥鐨勭姸鎬侊紝骞跺皢涔嬭浆鎹负
	 * JSON/XML 瀵硅薄
	 */
	void buildStatus();

	/**
	 * 灏� json 瀵硅薄杞负瀛楃涓插璞�
	 * @param buf {acl::string&}
	 */
	void statusToJson(acl::string& buf);

	/**
	 * 灏� xml 瀵硅薄杞负瀛楃涓插璞�
	 * @param buf {acl::string&}
	 */
	void statusToXml(acl::string& buf);

private:
	std::vector<ServerConnection*> conns_;
	acl::json   json_;
	acl::xml1   xml_;
	acl::locker lock_;
};
