#pragma once

class ClientConnection;

/**
 * 处理待处理客户端连接对象的定时器，该定时器会定时启动，
 * 查看客户端管理器里是否还有未处理客户端连接并进行处理
 */
class ManagerTimer : public acl::event_timer
{
public:
	ManagerTimer() {}

	/**
	 * 销毁动态对象
	 */
	virtual void destroy();

	/**
	 * 静态方法，用来将客户端连接描述字传递给服务端
	 * @param client {ClientConnection*} 非空对象
	 */
	static bool transfer(ClientConnection* client);

protected:
	// 基类虚函数
	virtual void timer_callback(unsigned int id);

private:
	~ManagerTimer() {}
};
