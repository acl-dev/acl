#pragma once

class ServerConnection;

/**
 * 服务端连接 IO 处理的回调处理类
 */
class ServerIOCallback : public acl::aio_callback
{
public:
	ServerIOCallback(ServerConnection* conn);

protected:
	bool read_callback(char* data, int len);
	void close_callback();
	bool timeout_callback();

private:
	ServerConnection* conn_;

	~ServerIOCallback();
};
