#include <iostream>
#include <assert.h>
#include "lib_acl.h"
#include "acl_cpp/lib_acl.hpp"
#include "ssl_aio_stream.hpp"

#ifdef WIN32
# ifndef snprintf
#  define snprintf _snprintf
# endif
#endif

using namespace acl;

typedef struct
{
	char  addr[64];
	aio_handle* handle;
	int   connect_timeout;
	int   read_timeout;
	int   nopen_limit;
	int   nopen_total;
	int   nwrite_limit;
	int   nwrite_total;
	int   nread_total;
	int   id_begin;
	bool  debug;
} IO_CTX;

static bool connect_ssl_server(IO_CTX* ctx, int id);

/**
* 客户端异步连接流回调函数类
*/
class ssl_io_callback : public aio_open_callback
{
public:
	/**
	* 构造函数
	* @param ctx {IO_CTX*}
	* @param client {ssl_aio_stream*} 异步连接流
	* @param id {int} 本流的ID号
	*/
	ssl_io_callback(IO_CTX* ctx, ssl_aio_stream* client, int id)
		: client_(client)
		, ctx_(ctx)
		, nwrite_(0)
		, id_(id)
	{
	}

	~ssl_io_callback()
	{
		std::cout << ">>>ID: " << id_ << ", ssl_io_callback deleted now!" << std::endl;
	}

	/**
	* 基类虚函数, 当异步流读到所要求的数据时调用此回调函数
	* @param data {char*} 读到的数据地址
	* @param len {int} 读到的数据长度
	* @return {bool} 返回给调用者 true 表示继续，否则表示需要关闭异步流
	*/
	bool read_callback(char* data, int len)
	{
		string buf(data, len);
		ctx_->nread_total++;
		std::cout << buf.c_str();
		return (true);
	}

	/**
	* 基类虚函数, 当异步流写成功时调用此回调函数
	* @return {bool} 返回给调用者 true 表示继续，否则表示需要关闭异步流
	*/
	bool write_callback()
	{
		ctx_->nwrite_total++;
		nwrite_++;
		return (true);
	}

	/**
	* 基类虚函数, 当该异步流关闭时调用此回调函数
	*/
	void close_callback()
	{
		if (client_->is_opened() == false)
		{
			std::cout << "Id: " << id_ << " connect "
				<< ctx_->addr << " error" << std::endl;

			// 如果是第一次连接就失败，则退出
			if (ctx_->nopen_total == 0)
			{
				std::cout << ", first connect error, quit";
				/* 获得异步引擎句柄，并设置为退出状态 */
				client_->get_handle().stop();
			}
			std::cout << std::endl;
			delete this;
			return;
		}

		/* 获得异步引擎中受监控的异步流个数 */
		int nleft = client_->get_handle().length();
		if (ctx_->nopen_total == ctx_->nopen_limit && nleft == 1)
		{
			std::cout << "Id: " << id_ << " stop now! nstream: "
				<< nleft << std::endl;
			/* 获得异步引擎句柄，并设置为退出状态 */
			client_->get_handle().stop();
		}

		// 必须在此处删除该动态分配的回调类对象以防止内存泄露
		delete this;
	}

	/**
	* 基类虚函数，当异步流超时时调用此函数
	* @return {bool} 返回给调用者 true 表示继续，否则表示需要关闭异步流
	*/
	bool timeout_callback()
	{
		std::cout << "Connect " << ctx_->addr << " Timeout ..." << std::endl;
		client_->close();
		return (false);
	}

	/**
	* 基类虚函数, 当异步连接成功后调用此函数
	* @return {bool} 返回给调用者 true 表示继续，否则表示需要关闭异步流
	*/
	bool open_callback()
	{
		// 连接成功，设置IO读写回调函数
		client_->add_read_callback(this);
		client_->add_write_callback(this);
		ctx_->nopen_total++;

		assert(id_ > 0);
		if (ctx_->nopen_total < ctx_->nopen_limit)
		{
			// 开始进行下一个连接过程
			if (connect_ssl_server(ctx_, id_ + 1) == false)
				std::cout << "connect error!" << std::endl;
		}

		http_header header;
		header.set_url("https://www.google.com.hk/");
		header.set_host("www.google.com.hk");
		header.set_keep_alive(false);
		string buf;
		(void) header.build_request(buf);

		// 异步向服务器发送数据
		client_->write(buf.c_str(), (int) buf.length());

		// 异步从服务器读取一行数据
		client_->gets(ctx_->read_timeout, false);

		// 表示继续异步过程
		return (true);
	}

protected:
private:
	ssl_aio_stream* client_;
	IO_CTX* ctx_;
	int   nwrite_;
	int   id_;
};

static bool connect_ssl_server(IO_CTX* ctx, int id)
{
	// 开始异步连接远程服务器
	ssl_aio_stream* stream = ssl_aio_stream::open(ctx->handle,
		ctx->addr, ctx->connect_timeout, true);
	if (stream == NULL)
	{
		std::cout << "connect " << ctx->addr << " error!" << std::endl;
		std::cout << "stoping ..." << std::endl;
		if (id == 0)
			ctx->handle->stop();
		return (false);
	}

	// 创建连接后的回调函数类
	ssl_io_callback* callback = new ssl_io_callback(ctx, stream, id);

	// 添加连接成功的回调函数类
	stream->add_open_callback(callback);

	// 添加连接失败后回调函数类
	stream->add_close_callback(callback);

	// 添加连接超时的回调函数类
	stream->add_timeout_callback(callback);
	return (true);
}

static void usage(const char* procname)
{
	printf("usage: %s -h[help] -l server_addr -c nconnect"
		" -n io_max -k[use kernel event: epoll/kqueue/devpoll"
		" -t connect_timeout -d[debug]\n", procname);
}

int main(int argc, char* argv[])
{
	bool use_kernel = false;
	int   ch;
	IO_CTX ctx;

	memset(&ctx, 0, sizeof(ctx));
	ctx.connect_timeout = 500;
	ctx.nopen_limit = 10;
	ctx.id_begin = 1;
	ctx.nwrite_limit = 10;
	ctx.debug = false;
	//snprintf(ctx.addr, sizeof(ctx.addr), "74.125.71.19:443");
	//snprintf(ctx.addr, sizeof(ctx.addr), "www.google.com.hk:443");
	snprintf(ctx.addr, sizeof(ctx.addr), "mail.sina.com.cn:443");

	while ((ch = getopt(argc, argv, "hc:n:kl:dt:")) > 0)
	{
		switch (ch)
		{
		case 'c':
			ctx.nopen_limit = atoi(optarg);
			if (ctx.nopen_limit <= 0)
				ctx.nopen_limit = 10;
			break;
		case 'n':
			ctx.nwrite_limit = atoi(optarg);
			if (ctx.nwrite_limit <= 0)
				ctx.nwrite_limit = 10;
			break;
		case 'h':
			usage(argv[0]);
			return (0);
		case 'k':
			use_kernel = true;
			break;
		case 'l':
			snprintf(ctx.addr, sizeof(ctx.addr), "%s", optarg);
			break;
		case 'd':
			ctx.debug = true;
			break;
		case 't':
			ctx.connect_timeout = atoi(optarg);
			break;
		default:
			break;
		}
	}

	ACL_METER_TIME("-----BEGIN-----");
	acl::acl_cpp_init();

	aio_handle handle(use_kernel ? ENGINE_KERNEL : ENGINE_SELECT);
	ctx.handle = &handle;

	if (connect_ssl_server(&ctx, ctx.id_begin) == false)
	{
		std::cout << "enter any key to exit." << std::endl;
		getchar();
		return (1);
	}

	std::cout << "Connect " << ctx.addr << " ..." << std::endl;

	while (true)
	{
		// 如果返回 false 则表示不再继续，需要退出
		if (handle.check() == false)
			break;
		//std::cout << ">>> Loop Check ..." << std::endl;
	}

	acl::string buf;

	buf << "total open: " << ctx.nopen_total
		<< ", total write: " << ctx.nwrite_total
		<< ", total read: " << ctx.nread_total;

	ACL_METER_TIME(buf.c_str());

	std::cout << "enter any key to exit." << std::endl;
	getchar();
	return (0);
}
