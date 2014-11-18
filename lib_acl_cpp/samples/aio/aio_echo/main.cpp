#include <iostream>
#include <assert.h>
#include "lib_acl.h"
#include "acl_cpp/stream/aio_handle.hpp"
#include "acl_cpp/stream/aio_istream.hpp"
#include "acl_cpp/stream/aio_listen_stream.hpp"
#include "acl_cpp/stream/aio_socket_stream.hpp"

using namespace acl;

/**
 * 异步客户端流的回调类的子类
 */
class io_callback : public aio_callback
{
public:
	io_callback(aio_socket_stream* client)
		: client_(client)
	{
	}

	~io_callback()
	{
		std::cout << "delete io_callback now ..." << std::endl;
	}

	/**
	 * 实现父类中的虚函数，客户端流的读成功回调过程
	 * @param data {char*} 读到的数据地址
	 * @param len {int} 读到的数据长度
	 * @return {bool} 返回 true 表示继续，否则希望关闭该异步流
	 */
	bool read_callback(char* data, int len)
	{
		string buf;
		buf.copy(data, len);

		// 向远程客户端回写收到的数据

		buf += "\r\n";
		client_->write(buf.c_str(), buf.length());

		//client_->read(6); // 如果每次读数据长度相同，则不必显式调用 read 过程
		return true;
	}

	/**
	 * 实现父类中的虚函数，客户端流的写成功回调过程
	 * @return {bool} 返回 true 表示继续，否则希望关闭该异步流
	 */
	bool write_callback()
	{
		return true;
	}

	/**
	 * 实现父类中的虚函数，客户端流的超时回调过程
	 */
	void close_callback()
	{
		// 必须在此处删除该动态分配的回调类对象以防止内存泄露
		delete this;
	}

	/**
	 * 实现父类中的虚函数，客户端流的超时回调过程
	 * @return {bool} 返回 true 表示继续，否则希望关闭该异步流
	 */
	bool timeout_callback()
	{
		std::cout << "Timeout ..." << std::endl;
		return true;
	}

private:
	aio_socket_stream* client_;
};

/**
 * 异步监听流的回调类的子类
 */
class io_accept_callback : public aio_accept_callback
{
public:
	io_accept_callback() {}
	~io_accept_callback()
	{
		printf(">>io_accept_callback over!\n");
	}

	/**
	 * 基类虚函数，当有新连接到达后调用此回调过程
	 * @param client {aio_socket_stream*} 异步客户端流
	 * @return {bool} 返回 true 以通知监听流继续监听
	 */
	bool accept_callback(aio_socket_stream* client)
	{
		// 创建异步客户端流的回调对象并与该异步流进行绑定
		io_callback* callback = new io_callback(client);

		// 注册异步流的读回调过程
		client->add_read_callback(callback);

		// 注册异步流的写回调过程
		client->add_write_callback(callback);

		// 注册异步流的关闭回调过程
		client->add_close_callback(callback);

		// 注册异步流的超时回调过程
		client->add_timeout_callback(callback);

		// 从异步流读数据
		client->read(6);
		return true;
	}
};

static void usage(const char* procname)
{
	printf("usage: %s -h[help] -k[use kernel event: epoll/iocp/kqueue/devpool]\n", procname);
}

int main(int argc, char* argv[])
{
	bool use_kernel = true;
	int  ch;

	while ((ch = getopt(argc, argv, "hk")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return 0;
		case 'k':
			use_kernel = true;
			break;
		default:
			break;
		}
	}

	// 构建异步引擎类对象
	aio_handle handle(use_kernel ? ENGINE_KERNEL : ENGINE_SELECT);

	// 创建监听异步流
	aio_listen_stream* sstream = new aio_listen_stream(&handle);
	const char* addr = "127.0.0.1:9001";

	// 初始化ACL库(尤其是在WIN32下一定要调用此函数，在UNIX平台下可不调用)
	acl_init();

	// 监听指定的地址
	if (sstream->open(addr) == false)
	{
		std::cout << "open " << addr << " error!" << std::endl;
		sstream->close();
		// XXX: 为了保证能关闭监听流，应在此处再 check 一下
		handle.check();

		getchar();
		return 1;
	}

	// 创建回调类对象，当有新连接到达时自动调用此类对象的回调过程
	io_accept_callback callback;
	sstream->add_accept_callback(&callback);
	std::cout << "Listen: " << addr << " ok!" << std::endl;

	while (true)
	{
		// 如果返回 false 则表示不再继续，需要退出
		if (handle.check() == false)
		{
			std::cout << "aio_server stop now ..." << std::endl;
			break;
		}
	}

	// 关闭监听流并释放流对象
	sstream->close();

	// XXX: 为了保证能关闭监听流，应在此处再 check 一下
	handle.check();

	return 0;
}
