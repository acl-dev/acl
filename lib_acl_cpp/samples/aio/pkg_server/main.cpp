#include <iostream>
#include <assert.h>
#include "lib_acl.h"
#include "acl_cpp/lib_acl.hpp"

static int   __timeout = 0;

typedef enum
{
	STATUS_T_HDR,
	STATUS_T_DAT,
} status_t;

// 数据头
struct DAT_HDR
{
	int  len;		// 数据体长度
	char cmd[64];		// 命令字
};

/**
 * 异步客户端流的回调类的子类
 */
class io_callback : public acl::aio_callback
{
public:
	io_callback(acl::aio_socket_stream* client)
	: status_(STATUS_T_HDR)
	, client_(client)
	, i_(0)
	{
	}

	~io_callback(void)
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
		// 当前状态是处理数据头时
		if (status_ == STATUS_T_HDR) {
			// 检验头部长度是否符合要求
			if (len != sizeof(DAT_HDR)) {
				printf("invalid len(%d) != DAT_HDR(%d)\r\n",
					len, (int) sizeof(DAT_HDR));
				return false;
			}

			// 取出数据体长度，并读指定长度的数据体

			DAT_HDR* req_hdr = (DAT_HDR*) data;

			// 将网络字节序转为主机字节序
			req_hdr->len = ntohl(req_hdr->len);
			if (req_hdr->len <= 0) {
				printf("invalid len: %d\r\n", req_hdr->len);
				return false;
			}

			// 修改状态位，表明下一步需要读取数据体
			status_ = STATUS_T_DAT;

			// 异步读指定长度的数据
			client_->read(req_hdr->len, __timeout);
			return true;
		}

		if (status_ != STATUS_T_DAT) {
			printf("invalid status: %d\r\n", (int) status_);
			return false;
		}

		if (i_++ < 10) {
			printf("req len: %d, dat: %s\r\n", len, data);
		}

		// 向远程客户端回写收到的数据

#define	OK	"+OK"
		size_t dat_len = sizeof(OK) - 1;

		DAT_HDR res_hdr;

		// 将主机字节序转为网络字节序
		res_hdr.len = (int) htonl((unsigned long) dat_len);
		ACL_SAFE_STRNCPY(res_hdr.cmd, "ok", sizeof(res_hdr.cmd));

		// 异步写响应数据包: 数据头及数据体

		client_->write(&res_hdr, sizeof(res_hdr));
		client_->write(OK, (int) dat_len);

		// 设置状态为读取下一个数据包
		status_ = STATUS_T_HDR;

		// 从异步流读数据包头
		client_->read(sizeof(DAT_HDR), __timeout);

		return true;
	}

	/**
	 * 实现父类中的虚函数，客户端流的写成功回调过程
	 * @return {bool} 返回 true 表示继续，否则希望关闭该异步流
	 */
	bool write_callback(void)
	{
		return true;
	}

	/**
	 * 实现父类中的虚函数，客户端流的超时回调过程
	 */
	void close_callback(void)
	{
		// 必须在此处删除该动态分配的回调类对象以防止内存泄露
		delete this;
	}

	/**
	 * 实现父类中的虚函数，客户端流的超时回调过程
	 * @return {bool} 返回 true 表示继续，否则希望关闭该异步流
	 */
	bool timeout_callback(void)
	{
		std::cout << "Timeout, delete it ..." << std::endl;
		return false;
	}

private:
	status_t status_;
	acl::aio_socket_stream* client_;
	int      i_;
};

/**
 * 异步监听流的回调类的子类
 */
class io_accept_callback : public acl::aio_accept_callback
{
public:
	io_accept_callback(void) {}
	~io_accept_callback(void)
	{
		printf(">>io_accept_callback over!\n");
	}

	/**
	 * 基类虚函数，当有新连接到达后调用此回调过程
	 * @param client {aio_socket_stream*} 异步客户端流
	 * @return {bool} 返回 true 以通知监听流继续监听
	 */
	bool accept_callback(acl::aio_socket_stream* client)
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

		// 从异步流读数据包头
		client->read(sizeof(DAT_HDR), __timeout);
		return true;
	}
};

static void usage(const char* procname)
{
	printf("usage: %s -h[help]\r\n"
		"	-l ip:port[:1900]\r\n"
		"	-t timeout\r\n"
		"	-k[use kernel event: epoll/iocp/kqueue/devpool]\r\n",
		procname);
}

int main(int argc, char* argv[])
{
	bool use_kernel = false;
	int  ch;
	acl::string addr(":1900");

	while ((ch = getopt(argc, argv, "l:hkt:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'l':
			addr = optarg;
			break;
		case 'k':
			use_kernel = true;
			break;
		case 't':
			__timeout = atoi(optarg);
			break;
		default:
			break;
		}
	}

	// 初始化ACL库(尤其是在WIN32下一定要调用此函数，在UNIX平台下可不调用)
	acl::acl_cpp_init();

	acl::log::stdout_open(true);

	// 构建异步引擎类对象
	acl::aio_handle handle(use_kernel ? acl::ENGINE_KERNEL : acl::ENGINE_SELECT);

	// 创建监听异步流
	acl::aio_listen_stream* sstream = new acl::aio_listen_stream(&handle);

	// 监听指定的地址
	if (!sstream->open(addr.c_str())) {
		std::cout << "open " << addr.c_str() << " error!" << std::endl;
		sstream->close();
		// XXX: 为了保证能关闭监听流，应在此处再 check 一下
		handle.check();

		getchar();
		return 1;
	}

	// 创建回调类对象，当有新连接到达时自动调用此类对象的回调过程
	io_accept_callback callback;
	sstream->add_accept_callback(&callback);
	std::cout << "Listen: " << addr.c_str() << " ok!" << std::endl;

	while (true) {
		// 如果返回 false 则表示不再继续，需要退出
		if (!handle.check()) {
			std::cout << "pkg_server stop now ..." << std::endl;
			break;
		}
	}

	// 关闭监听流并释放流对象
	sstream->close();

	// XXX: 为了保证能关闭监听流，应在此处再 check 一下
	handle.check();

	return 0;
}
