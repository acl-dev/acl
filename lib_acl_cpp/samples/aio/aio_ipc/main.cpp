#include "lib_acl.h"
#ifndef WIN32
#include <getopt.h>
#endif
#include <iostream>
#include "acl_cpp/acl_cpp_init.hpp"
#include "acl_cpp/stream/aio_handle.hpp"
#include "acl_cpp/ipc/ipc_server.hpp"
#include "acl_cpp/ipc/ipc_client.hpp"

using namespace acl;

#define MSG_REQ		1
#define MSG_RES		2
#define MSG_STOP	3

class test_client1 : public ipc_client
{
public:
	test_client1()
	{

	}

	~test_client1()
	{

	}

	virtual void on_open()
	{
		// 添加消息回调对象
		this->append_message(MSG_RES);

		// 向消息服务器发送请求消息
		this->send_message(MSG_REQ, NULL, 0);

		// 异步等待消息
		wait();
	}

	virtual void on_close()
	{
		delete this;
	}

	virtual void on_message(int nMsg, void* data, int dlen)
	{
		(void) data;
		(void) dlen;
		std::cout << "test_client1 on message:" << nMsg << std::endl;
		this->send_message(MSG_STOP, NULL, 0);
		this->delete_message(MSG_RES);
		this->get_handle().stop();
		this->close();
	}
protected:
private:
};

// 子线程处理过程

static bool client_main(aio_handle* handle, const char* addr)
{
	// 创建消息连接
	ipc_client* ipc = new test_client1();
	
	// 连接消息服务器
	if (ipc->open(handle, addr, 0) == false)
	{
		std::cout << "open " << addr << " error!" << std::endl;
		delete ipc;
		return (false);
	}

	return (true);
}

static void* thread_callback(void *ctx)
{
	const char* addr = (const char*) ctx;
	aio_handle handle;

	if (client_main(&handle, addr) == false)
	{
		handle.check();
		return (NULL);
	}

	// 消息循环
	while (true)
	{
		if (handle.check() == false)
			break;
	}

	handle.check();

	return (NULL);
}

class test_client2 : public ipc_client
{
public:
	test_client2()
	{

	}

	~test_client2()
	{

	}

	virtual void on_close()
	{
		delete this;
	}

	virtual void on_message(int nMsg, void* data, int dlen)
	{
		(void) data;
		(void) dlen;

		std::cout << "test_client2 on message:" << nMsg << std::endl;

		if (nMsg == MSG_STOP)
		{
			this->close();
			this->get_handle().stop();
		}
		else
			// 回应客户端消息
			this->send_message(MSG_RES, NULL, 0);
	}
protected:
private:
};

class test_server : public ipc_server
{
public:
	test_server()
	{
	}

	~test_server()
	{

	}

	void on_accept(aio_socket_stream* client)
	{
		ipc_client* ipc = new test_client2();

		// 打开异步IPC过程
		ipc->open(client);

		// 添加消息回调对象
		ipc->append_message(MSG_REQ);
		ipc->append_message(MSG_STOP);
		ipc->wait();
	}
protected:
private:
};

static void usage(const char* procname)
{
	printf("usage: %s -h[help] -t[use thread]\n", procname);
}

int main(int argc, char* argv[])
{
	int   ch;
	bool  use_thread = false;

	while ((ch = getopt(argc, argv, "ht")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return (0);
		case 't':
			use_thread = true;
			break;
		default:
			break;
		}
	}

	acl_cpp_init();

	aio_handle handle;

	ipc_server* server = new test_server();

	// 使消息服务器监听 127.0.0.1 的地址
	if (server->open(&handle, "127.0.0.1:0") == false)
	{
		delete server;
		std::cout << "open server error!" << std::endl;
		getchar();
		return (1);
	}

	char  addr[256];
#ifdef WIN32
	_snprintf(addr, sizeof(addr), "%s", server->get_addr());
#else
	snprintf(addr, sizeof(addr), "%s", server->get_addr());
#endif

	if (use_thread)
	{
		acl_pthread_t tid;
		acl_pthread_create(&tid, NULL, thread_callback, addr);
	}
	else
		client_main(&handle, addr);

	while (true)
	{
		if (handle.check() == false)
		{
			std::cout << "stop now!" << std::endl;
			break;
		}
	}

	delete server;
	handle.check();

	std::cout << "server stopped!" << std::endl;
	getchar();
	return (0);
}
