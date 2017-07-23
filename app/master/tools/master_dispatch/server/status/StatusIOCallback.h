#pragma once

class StatusConnection;

/**
 * 服务端连接 IO 处理的回调处理类
 */
class StatusIOCallback : public acl::aio_callback
{
public:
	StatusIOCallback(StatusConnection* conn);

protected:
	// 基类 aio_callback 虚函数

	bool read_wakeup();
	void close_callback();
	bool timeout_callback();

private:
	StatusConnection* conn_;

	~StatusIOCallback();
};
