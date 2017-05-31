#include <assert.h>
#include <iostream>
#include "acl_cpp/lib_acl.hpp"
#include "fiber/lib_fiber.hpp"

class fiber_client : public acl::fiber
{
public:
	fiber_client(acl::socket_stream* conn) : conn_(conn) {}

protected:
	// @override 实现基类纯虚函数
	void run(void)
	{
		std::cout << "fiber-" << acl::fiber::self()
			<< ": fd=" << conn_->sock_handle()
			<< ", addr=" << conn_->get_peer() << std::endl;
		echo();
		delete this; // 因为是动态创建的，所以需自动销毁
	}

private:
	acl::socket_stream* conn_;

	~fiber_client(void)
	{
		delete conn_;
	}

	void echo(void)
	{
		char buf[8192];

		// 从客户端读取数据并回显
		while (!conn_->eof())
		{
			int ret = conn_->read(buf, sizeof(buf), false);
			if (ret == -1)
			{
				std::cout << "read error "
					<< acl::last_serror() << std::endl;
				break;
			}
			if (conn_->write(buf, ret) == -1)
			{
				std::cout << "write error "
					<< acl::last_serror() << std::endl;
				break;
			}
		}
	}
};

class fiber_server : public acl::fiber
{
public:
	fiber_server(const char* addr) : addr_(addr) {}

protected:
	// @override
	void run(void)
	{
		// 监听服务地址
		acl::server_socket ss;
		if (ss.open(addr_) == false)
		{
			std::cout << "listen " << addr_.c_str() << " error"
				<< std::endl;
			delete this;
			return;
		}

		std::cout << "listen " << addr_.c_str() << " ok" << std::endl;

		while (true)
		{
			// 等待接收客户端连接
			acl::socket_stream* conn = ss.accept();
			if (conn == NULL)
			{
				std::cout << "accept error" << std::endl;
				break;
			}

			// 创建客户端处理协程
			acl::fiber* fb = new fiber_client(conn);
			fb->start();
		}

		delete this;
	}

private:
	acl::string addr_;

	~fiber_server(void) {}
};

int main(void)
{
	const char* addr = "127.0.0.1:8089";
	
	acl::fiber* fb = new fiber_server(addr); // 创建监听服务协程
	fb->start(); // 启动监听协程

	// 循环调度所有协程，直至所有协程退出
	acl::fiber::schedule();

	return 0;
}
