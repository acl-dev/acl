#pragma once
#include <vector>

class ServerConnection;

/**
 * 服务端连接对象管理器
 */
class ServerManager : public acl::singleton <ServerManager>
{
public:
	ServerManager() {}
	~ServerManager() {}

	/**
	 * 添加新的服务端连接对象
	 * @param conn {ServerConnection*}
	 */
	void set(ServerConnection* conn);

	/**
	 * 删除某个指定的服务端连接对象
	 */
	void del(ServerConnection* conn);

	/**
	 * 取出服务端连接对象中负载最低的一个连接对象
	 * @return {ServerConnection*} 返回 NULL 表示没有可用的服务对象
	 */
	ServerConnection* min();

	/**
	 * 获得所有的服务端连接对象的个数
	 * @return {size_t}
	 */
	size_t length() const
	{
		return conns_.size();
	}

	/**
	 * 统计服务器集群中的各个服务子进程实例的状态，并将之转换为
	 * JSON/XML 对象
	 */
	void buildStatus();

	/**
	 * 将 json 对象转为字符串对象
	 * @param buf {acl::string&}
	 */
	void statusToJson(acl::string& buf);

	/**
	 * 将 xml 对象转为字符串对象
	 * @param buf {acl::string&}
	 */
	void statusToXml(acl::string& buf);

private:
	std::vector<ServerConnection*> conns_;
	acl::json   json_;
	acl::xml1   xml_;
	acl::locker lock_;
};
