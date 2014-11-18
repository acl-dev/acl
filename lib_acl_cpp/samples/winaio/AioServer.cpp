#include "StdAfx.h"
#include <iostream>
#include "lib_acl.h"
#include "acl_cpp/stream/aio_handle.hpp"
#include "acl_cpp/stream/aio_listen_stream.hpp"
#include "acl_cpp/stream/aio_socket_stream.hpp"
#include "AioServer.h"

using namespace acl;

/**
* 延迟读回调处理类
*/
class timer_reader: public aio_timer_reader
{
public:
	timer_reader()
	{
		std::cout << "timer_reader init now" << std::endl;
	}

	~timer_reader()
	{
	}

	// aio_timer_reader 的子类必须重载 destroy 方法
	void destroy()
	{
		std::cout << "timer_reader delete now" << std::endl;
		delete this;
	}

	// 重载基类回调方法
	virtual void timer_callback(unsigned int id)
	{
		std::cout << "timer_reader: timer_callback now" << std::endl;

		// 调用基类的处理过程
		aio_timer_reader::timer_callback(id);
	}
};

/**
* 延迟写回调处理类
*/
class timer_writer: public aio_timer_writer
{
public:
	timer_writer()
	{
		std::cout << "timer_writer init now" << std::endl;
	}

	~timer_writer()
	{
	}

	// aio_timer_reader 的子类必须重载 destroy 方法
	void destroy()
	{
		std::cout << "timer_writer delete now" << std::endl;
		delete this;
	}

	// 重载基类回调方法
	virtual void timer_callback(unsigned int id)
	{
		std::cout << "timer_writer: timer_callback now" << std::endl;

		// 调用基类的处理过程
		aio_timer_writer::timer_callback(0);
	}
};

CAcceptedClientCallback::CAcceptedClientCallback(aio_socket_stream* client)
: client_(client)
, i_(0)
{

}

CAcceptedClientCallback::~CAcceptedClientCallback()
{
	std::cout << "delete io_callback now ..." << std::endl;
}

bool CAcceptedClientCallback::read_callback(char* data, int len)
{
	i_++;
	if (i_ < 10)
		std::cout << ">>gets(i:" << i_ << "): " << data;

	// 如果远程客户端希望退出，则关闭之
	if (strncasecmp(data, "quit", 4) == 0)
	{
		client_->format("Bye!\r\n");
		client_->close();
		return (true);
	}

	// 如果远程客户端希望服务端也关闭，则中止异步事件过程
	else if (strncasecmp(data, "stop", 4) == 0)
	{
		client_->format("Stop now!\r\n");
		client_->close();  // 关闭远程异步流

		// 通知异步引擎关闭循环过程
		client_->get_handle().stop();
		return (true);
	}

	int   delay = 0;

	if (strncasecmp(data, "write_delay", strlen("write_delay")) == 0)
	{
		// 延迟写过程

		const char* ptr = data + strlen("write_delay");
		delay = atoi(ptr);
		if (delay > 0)
		{
			std::cout << ">> write delay " << delay
				<< " second ..." << std::endl;
			timer_writer* timer = new timer_writer();
			client_->write(data, len, delay, timer);
			client_->gets(10, false);
			return (true);
		}
	}
	else if (strncasecmp(data, "read_delay", strlen("read_delay")) == 0)
	{
		// 延迟读过程

		const char* ptr = data + strlen("read_delay");
		delay = atoi(ptr);
		if (delay > 0)
		{
			client_->write(data, len);
			std::cout << ">> read delay " << delay
				<< " second ..." << std::endl;
			timer_reader* timer = new timer_reader();
			client_->gets(10, false, delay, timer);
			return (true);
		}
	}

	// 向远程客户端回写收到的数据
	client_->write(data, len);

	// 继续读下一行数据
	client_->gets(10, false);
	return (true);
}

bool CAcceptedClientCallback::write_callback()
{
	return (true);
}

void CAcceptedClientCallback::close_callback()
{
	// 必须在此处删除该动态分配的回调类对象以防止内存泄露
	delete this;
}

bool CAcceptedClientCallback::timeout_callback()
{
	std::cout << "Timeout ..." << std::endl;
	return (true);
}

///////////////////////////////////////////////////////////////////////////////

CServerCallback::CServerCallback()
{

}

CServerCallback::~CServerCallback()
{

}

bool CServerCallback::accept_callback(aio_socket_stream* client)
{
	// 创建异步客户端流的回调对象并与该异步流进行绑定
	CAcceptedClientCallback* callback = new CAcceptedClientCallback(client);

	// 注册异步流的读回调过程
	client->add_read_callback(callback);

	// 注册异步流的写回调过程
	client->add_write_callback(callback);

	// 注册异步流的关闭回调过程
	client->add_close_callback(callback);

	// 注册异步流的超时回调过程
	client->add_timeout_callback(callback);

	// 从异步流读一行数据
	client->gets(10, false);
	return (true);
}
