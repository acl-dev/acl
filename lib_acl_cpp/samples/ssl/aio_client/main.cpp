#include <iostream>
#include <assert.h>
#include "lib_acl.h"
#include "acl_cpp/lib_acl.hpp"

typedef struct
{
	acl::aio_handle* handle;
	char  addr[64];
	int   connect_timeout;
	int   read_timeout;
	int   nopen_limit;
	int   nopen_total;
	int   nwrite_limit;
	int   nwrite_total;
	int   nread_total;
	int   id_begin;
} IO_CTX;

static bool connect_server(acl::polarssl_conf* ssl_conf, IO_CTX* ctx, int id);

/**
 * 客户端异步连接流回调函数类
 */
class client_io_callback : public acl::aio_open_callback
{
public:
	/**
	 * 构造函数
	 * @param client {aio_socket_stream*} 异步连接流
	 * @param ssl_conf {acl::polarssl_conf*} 非空时指定 SSL 连接方式
	 * @param ctx {IO_CTX*}
	 * @param id {int} 本流的ID号
	 */
	client_io_callback(acl::aio_socket_stream* client,
			acl::polarssl_conf* ssl_conf, IO_CTX* ctx, int id)
		: client_(client)
		, ssl_conf_(ssl_conf)
		, ctx_(ctx)
		, nwrite_(0)
		, nread_(0)
		, id_(id)
	{
	}

	~client_io_callback()
	{
		std::cout << ">>>ID: " << id_
			<< ", io_callback deleted now!" << std::endl;
	}

	/**
	 * 基类虚函数, 当异步流读到所要求的数据时调用此回调函数
	 * @param data {char*} 读到的数据地址
	 * @param len {int｝ 读到的数据长度
	 * @return {bool} 返回给调用者 true 表示继续，否则表示需要关闭异步流
	 */
	bool read_callback(char*, int len)
	{
		nread_ += len;
		ctx_->nread_total++;

		std::cout << ">>>>>>>> current len: " << len
			<< "; total_len: " << nread_ << std::endl;

		client_->close();
		return true;
	}

	/**
	 * 基类虚函数, 当异步流写成功时调用此回调函数
	 * @return {bool} 返回给调用者 true 表示继续，否则表示需要关闭异步流
	 */
	bool write_callback()
	{
		ctx_->nwrite_total++;
		nwrite_++;

		return true;
	}

	/**
	 * 基类虚函数, 当该异步流关闭时调用此回调函数
	 */
	void close_callback()
	{
		if (client_->is_opened() == false)
		{
			std::cout << "Id: " << id_ << " connect "
				<< ctx_->addr << " error: "
				<< acl::last_serror();

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
		std::cout << "Connect " << ctx_->addr
			<< " Timeout ..." << std::endl;
		client_->close();
		return false;
	}

	bool read_wakeup()
	{
		// 取得之前通过 setup_hook 注册的 SSL IO句柄
		acl::polarssl_io* hook =
			(acl::polarssl_io*) client_->get_hook();

		if (hook == NULL)
		{
			std::cout << "get hook error"<< std::endl;
			return false;
		}

		// 尝试进行 SSL 握手
		if (hook->handshake() == false)
		{
			logger_error("ssl handshake failed");
			return false;
		}

		// SSL 握手还未完成，等待本函数再次被触发
		if (hook->handshake_ok() == false)
			return true;

		// 如果 SSL 握手已经成功，则开始读数据
		
		printf("ssl handshake ok\r\n");

		// 由 reactor 模式转为 proactor 模式，从而取消
		// read_wakeup 回调过程
		client_->disable_read();

		// 开始与服务端的读写过程
		return begin_run();
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

		acl::assert_(id_ > 0);
		if (ctx_->nopen_total < ctx_->nopen_limit)
		{
			// 开始进行下一个连接过程
			if (connect_server(ssl_conf_, ctx_, id_ + 1) == false)
				std::cout << "connect error!" << std::endl;
		}

		// 设置 SSL 方式
		if (ssl_conf_)
			return setup_ssl(*ssl_conf_);

		// 开始与服务端的读写过程
		else
			return begin_run();
	}

private:
	acl::aio_socket_stream* client_;
	acl::polarssl_conf* ssl_conf_;
	IO_CTX* ctx_;
	int   nwrite_;
	int   nread_;
	int   id_;

	bool setup_ssl(acl::polarssl_conf& ssl_conf)
	{
		acl::polarssl_io* ssl =
			new acl::polarssl_io(ssl_conf, false, true);

		// 将 SSL IO 过程注册至异步流中
		if (client_->setup_hook(ssl) == ssl)
		{
			std::cout << "open ssl error!" << std::endl;
			ssl->destroy();
			return false;
		}

		// 开始 SSL 握手过程
		if (ssl->handshake() == false)
		{
			client_->remove_hook();
			ssl->destroy();
			return false;
		}

		// 开始异步 SSL 握手过程，满足可读条件时将触发 read_wakeup
		client_->read_wait(10);
		return true;
	}

	bool begin_run(void)
	{
		// 异步向服务器发送数据
		char  buf[8194];

		memset(buf, 'x', sizeof(buf));
		buf[sizeof(buf) - 1] = '\n';
		buf[sizeof(buf) - 2] = '\r';

		client_->write(buf, (int) sizeof(buf));

		// 异步从服务器读取数据
		client_->read();

		return true;
	}
};

static bool connect_server(acl::polarssl_conf* ssl_conf, IO_CTX* ctx, int id)
{
	// 开始异步连接远程服务器
	acl::aio_socket_stream* stream = acl::aio_socket_stream::open(
			ctx->handle, ctx->addr, ctx->connect_timeout);
	if (stream == NULL)
	{
		std::cout << "connect " << ctx->addr << " error!" << std::endl;
		std::cout << "stoping ..." << std::endl;
		if (id == 0)
			ctx->handle->stop();
		return false;
	}

	acl_non_blocking(stream->sock_handle(), ACL_BLOCKING);

	// 创建连接后的回调函数类
	client_io_callback* callback = new
		client_io_callback(stream, ssl_conf, ctx,  id);

	// 添加连接成功的回调函数类
	stream->add_open_callback(callback);

	// 添加连接失败后回调函数类
	stream->add_close_callback(callback);

	// 添加连接超时的回调函数类
	stream->add_timeout_callback(callback);

	return true;
}

static void usage(const char* procname)
{
	printf("usage: %s -h[help] -l server_addr -c nconnect"
		" -n io_max -k[use kernel event: epoll/kqueue/devpoll"
		" -t connect_timeout -S[use_ssl]\n", procname);
}

int main(int argc, char* argv[])
{
	bool use_kernel = false;
	int   ch;
	IO_CTX ctx;
	acl::polarssl_conf* ssl_conf = NULL;

	memset(&ctx, 0, sizeof(ctx));
	ctx.connect_timeout = 5;
	ctx.nopen_limit = 1;
	ctx.id_begin = 1;
	ctx.nwrite_limit = 1;
	acl::safe_snprintf(ctx.addr, sizeof(ctx.addr), "127.0.0.1:9800");

	while ((ch = getopt(argc, argv, "hc:n:kl:t:S")) > 0)
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
			return 0;
		case 'k':
			use_kernel = true;
			break;
		case 'l':
			acl::safe_snprintf(ctx.addr, sizeof(ctx.addr),
				"%s", optarg);
			break;
		case 't':
			ctx.connect_timeout = atoi(optarg);
			break;
		case 'S':
			ssl_conf = new acl::polarssl_conf();
		default:
			break;
		}
	}

	acl::meter_time(__FUNCTION__, __LINE__, "-----BEGIN-----");
	acl::acl_cpp_init();
	acl::log::stdout_open(true);

	acl::aio_handle handle(use_kernel ? acl::ENGINE_KERNEL : acl::ENGINE_SELECT);
	ctx.handle = &handle;

	if (connect_server(ssl_conf, &ctx, ctx.id_begin) == false)
	{
		std::cout << "enter any key to exit." << std::endl;
		getchar();
		return 1;
	}

	std::cout << "Connect " << ctx.addr << " ..." << std::endl;

	while (true)
	{
		// 如果返回 false 则表示不再继续，需要退出
		if (handle.check() == false)
			break;
	}

	acl::string buf;

	buf << "total open: " << ctx.nopen_total
		<< ", total write: " << ctx.nwrite_total
		<< ", total read: " << ctx.nread_total;

	acl::meter_time(__FUNCTION__, __LINE__, buf.c_str());

	delete ssl_conf;

	return 0;
}

