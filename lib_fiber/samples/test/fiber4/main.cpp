#include <assert.h>
#include <iostream>
#include "acl_cpp/lib_acl.hpp"
#include "fiber/lib_fiber.hpp"

class fiber_client : public acl::fiber
{
public:
	fiber_client(acl::socket_stream* conn) : conn_(conn) {}

protected:
	// @override 瀹炵幇鍩虹被绾櫄鍑芥暟
	void run(void)
	{
		std::cout << "fiber-" << acl::fiber::self()
			<< ": fd=" << conn_->sock_handle()
			<< ", addr=" << conn_->get_peer() << std::endl;
		echo();
		delete this; // 鍥犱负鏄姩鎬佸垱寤虹殑锛屾墍浠ラ渶鑷姩閿€姣�
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

		// 浠庡鎴风璇诲彇鏁版嵁骞跺洖鏄�
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
		// 鐩戝惉鏈嶅姟鍦板潃
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
			// 绛夊緟鎺ユ敹瀹㈡埛绔繛鎺�
			acl::socket_stream* conn = ss.accept();
			if (conn == NULL)
			{
				std::cout << "accept error" << std::endl;
				break;
			}

			// 鍒涘缓瀹㈡埛绔鐞嗗崗绋�
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
	
	acl::fiber* fb = new fiber_server(addr); // 鍒涘缓鐩戝惉鏈嶅姟鍗忕▼
	fb->start(); // 鍚姩鐩戝惉鍗忕▼

	// 寰幆璋冨害鎵€鏈夊崗绋嬶紝鐩磋嚦鎵€鏈夊崗绋嬮€€鍑�
	acl::fiber::schedule();

	return 0;
}
