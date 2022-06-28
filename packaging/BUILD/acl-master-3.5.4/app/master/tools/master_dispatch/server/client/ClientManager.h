#pragma once
#include <vector>

class ClientConnection;

/**
 * 单例类，用来管理客户端连接对象
 */
class ClientManager : public acl::singleton<ClientManager>
{
public:
	ClientManager() {}
	~ClientManager() {}

	/**
	 * 添加客户端连接对象，不能重复添加相同的连接对象，
	 * 否则，内部直接 fatal
	 * @param conn {ClientConnection*} 非空对象
	 */
	void set(ClientConnection* conn);

	/**
	 * 删除客户端对象
	 * @param conn {ClientConnection*} 非空对象
	 */
	void del(ClientConnection* conn);

	/**
	 * 从连接对象集合中弹出一个连接对象，并从集合中删除
	 * @return {ClientConnection*} 如果返回空，则说明没有连接对象
	 */
	ClientConnection* pop();

	size_t length() const
	{
		return conns_.size();
	}

private:
	// 存储客户端连接对象的数组集合
	std::vector<ClientConnection*> conns_;
};
