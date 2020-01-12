#include <iostream>
#include <assert.h>
#include "lib_acl.h"
#include "acl_cpp/stdlib/util.hpp"
#include "acl_cpp/stdlib/snprintf.hpp"
#include "acl_cpp/acl_cpp_init.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stream/polarssl_conf.hpp"
#include "acl_cpp/stream/polarssl_io.hpp"
#include "acl_cpp/stream/mbedtls_conf.hpp"
#include "acl_cpp/stream/mbedtls_io.hpp"
#include "acl_cpp/stream/aio_handle.hpp"
#include "acl_cpp/stream/aio_istream.hpp"
#include "acl_cpp/stream/aio_listen_stream.hpp"
#include "acl_cpp/stream/aio_socket_stream.hpp"

static int   __max = 0;
static int   __timeout = 0;
static int   __max_used = 0;
static int   __cur_used = 0;

// SSL 模式下的 SSL 配置对象
static acl::sslbase_conf* __ssl_conf;

/**
 * 异步客户端流的回调类的子类
 */
class io_callback : public acl::aio_callback
{
public:
	io_callback(acl::aio_socket_stream* client)
	: client_(client)
	, nread_cnt_(0)
	, nread_(0)
	{
	}

	~io_callback()
	{
		printf("delete io_callback now ...\r\n");
		__cur_used++;
	}

	/**
	 * 读回调虚函数，该回调函数当满足了类 aio_istream 实例中的
	 * gets/read 的可读条件后被调用，由异步框架内部将符合条件的数
	 * 据读出，直接传递给用户的子类
	 * @param data {char*} 读到的数据的指针地址
	 * @param len {int} 读到的数据长度(> 0)
	 * @return {bool} 该函数返回 false 通知异步引擎关闭该异步流
	 */
	bool read_wakeup()
	{
		acl::sslbase_io* hook = (acl::sslbase_io*) client_->get_hook();
		if (hook == NULL) {
			// 非 SSL 模式，异步读取数据
			//client_->read(__timeout);
			client_->gets(__timeout, false);
			return true;
		}

		// 尝试进行 SSL 握手
		if (!hook->handshake()) {
			printf("ssl handshake failed\r\n");
			return false;
		}

		// 如果 SSL 握手已经成功，则开始按行读数据
		if (hook->handshake_ok()) {
			// 由 reactor 模式转为 proactor 模式，从而取消
			// read_wakeup 回调过程
			client_->disable_read();

			// 异步读取数据，将会回调 read_callback
			//client_->read(__timeout);
			client_->gets(__timeout, false);
			return true;
		}

		// SSL 握手还未完成，等待本函数再次被触发
		return true;
	}

	/**
	 * 实现父类中的虚函数，客户端流的读成功回调过程
	 * @param data {char*} 读到的数据地址
	 * @param len {int} 读到的数据长度
	 * @return {bool} 返回 true 表示继续，否则希望关闭该异步流
	 */
	bool read_callback(char* data, int len)
	{
		nread_cnt_++;
		nread_ += len;

		if (nread_cnt_ <= 100 || nread_cnt_ % 1000 == 0) {
			char  buf[256];
			acl::safe_snprintf(buf, sizeof(buf),
				"read len: %d, total read: %d, nread_cnt: %d",
				len, nread_, nread_cnt_);
			acl::meter_time(__FILE__, __LINE__, buf);
		}

		// 如果远程客户端希望退出，则关闭之
		if (strncasecmp(data, "quit", 4) == 0) {
			client_->format("Bye!\r\n");
			client_->close();
			return true;
		}

		// 如果远程客户端希望服务端也关闭，则中止异步事件过程
		if (strncasecmp(data, "stop", 4) == 0) {
			client_->format("Stop now!\r\n");
			client_->close();  // 关闭远程异步流

			// 通知异步引擎关闭循环过程
			client_->get_handle().stop();
		}

		// 向远程客户端回写收到的数据

		client_->write(data, len);
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
		std::cout << "Timeout, delete it ..." << std::endl;
		return false;
	}

private:
	acl::aio_socket_stream* client_;
	int   nread_cnt_;
	int   nread_;
};

/**
 * 异步监听流的回调类的子类
 */
class io_accept_callback : public acl::aio_accept_callback
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
	bool accept_callback(acl::aio_socket_stream* client)
	{
		//acl_non_blocking(client->sock_handle(), ACL_BLOCKING);

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

		// 当限定了行数据最大长度时
		if (__max > 0)
			client->set_buf_max(__max);

		// SSL 模式下，等待客户端发送握手信息
		if (__ssl_conf != NULL) {
			acl::sslbase_io* ssl = __ssl_conf->open(true);

			// 注册 SSL IO 过程的钩子
			if (client->setup_hook(ssl) == ssl) {
				std::cout << "setup_hook error" << std::endl;
				ssl->destroy();
				return false;
			}

			// 将客户端置于读监听状态以触发 read_wakeup 回调过程，
			// SSL 握手过程将在 read_wakeup 中完成
			client->read_wait(__timeout);
		}

		// 非 SSL 模式下，从异步流读一行数据
		else {
			//client->read(__timeout);
			client->gets(__timeout, false);
		}

		return true;
	}
};

static void usage(const char* procname)
{
	printf("usage: %s -h[help]\r\n"
		"  -d path_to_polarssl\r\n"
		"  -l server_addr[ip:port, default: 127.0.0.1:9800]\r\n"
		"  -L line_max_length\r\n"
		"  -t timeout\r\n"
		"  -n conn_used_limit\r\n"
		"  -k[use kernel event: epoll/iocp/kqueue/devpool]\r\n"
		"  -M delay_ms\r\n"
		"  -I check_fds_inter\r\n"
		"  -K ssl_key_file -C ssl_cert_file [in SSL mode]\r\n",
		procname);
}

int main(int argc, char* argv[])
{
	// 事件引擎是否采用内核中的高效模式
	bool use_kernel = false;
	acl::string key_file, cert_file, libpath("../libpolarssl.so");
	acl::string addr("127.0.0.1:9800");
	int  ch, delay_ms = 100, check_fds_inter = 10;

	while ((ch = getopt(argc, argv, "l:d:hkL:t:K:C:n:M:I:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'd':
			libpath = optarg;
			break;
		case 'l':
			addr = optarg;
			break;
		case 'k':
			use_kernel = true;
			break;
		case 'L':
			__max = atoi(optarg);
			break;
		case 't':
			__timeout = atoi(optarg);
			break;
		case 'K':
			key_file = optarg;
			break;
		case 'C':
			cert_file = optarg;
			break;
		case 'n':
			__max_used = atoi(optarg);
			break;
		case 'M':
			delay_ms = atoi(optarg);
			break;
		case 'I':
			check_fds_inter = atoi(optarg);
			break;
		default:
			break;
		}
	}

	acl::log::stdout_open(true);

	if (libpath.find("mbedtls") != NULL) {
		const std::vector<acl::string>& libs = libpath.split2(";");
		if (libs.size() != 3) {
			printf("invalid libpath=%s\r\n", libpath.c_str());
			return 1;
		}
		acl::mbedtls_conf::set_libpath(libs[0], libs[1], libs[2]);

		if (acl::mbedtls_conf::load()) {
			__ssl_conf = new acl::mbedtls_conf(true);
			printf("load %s ok\r\n", libpath.c_str());
		} else {
			key_file.clear();
			cert_file.clear();
			printf("load %s error\r\n", libpath.c_str());
		}
	} else if (libpath.find("polarssl") != NULL) {
		acl::polarssl_conf::set_libpath(libpath);

		if (acl::polarssl_conf::load()) {
			__ssl_conf = new acl::polarssl_conf();
			printf("load %s ok\r\n", libpath.c_str());
		} else {
			key_file.clear();
			cert_file.clear();
			printf("load %s error\r\n", libpath.c_str());
		}
	}

	// 当私钥及证书都存在时才采用 SSL 通信方式
	if (!key_file.empty() && !cert_file.empty() && __ssl_conf) {

		// 允许服务端的 SSL 会话缓存功能
		__ssl_conf->enable_cache(true);

		// 添加本地服务的证书
		if (!__ssl_conf->add_cert(cert_file.c_str())) {
			delete __ssl_conf;
			__ssl_conf = NULL;
			std::cout << "add_cert error: " << cert_file.c_str()
				<< std::endl;
		}

		// 添加本地服务密钥
		else if (!__ssl_conf->set_key(key_file.c_str())) {
			delete __ssl_conf;
			__ssl_conf = NULL;
			std::cout << "set_key error: " << key_file.c_str()
				<< std::endl;
		} else {
			std::cout << "Load cert&key OK!" << std::endl;
		}
	}

	if (__ssl_conf) {
	}

	// 构建异步引擎类对象
	acl::aio_handle handle(use_kernel ? acl::ENGINE_KERNEL : acl::ENGINE_SELECT);

	handle.set_check_inter(check_fds_inter);

	int delay_sec = delay_ms / 1000;
	int delay_usec = (delay_ms - delay_sec * 1000) * 1000;
	handle.set_delay_sec(delay_sec);
	handle.set_delay_usec(delay_usec);

	// 创建监听异步流
	acl::aio_listen_stream* sstream = new acl::aio_listen_stream(&handle);

	// 初始化ACL库(尤其是在WIN32下一定要调用此函数，在UNIX平台下可不调用)
	acl::acl_cpp_init();

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
			std::cout << "aio_server stop now ..." << std::endl;
			break;
		}

		if (__max_used > 0 && __cur_used >= __max_used) {
			break;
		}
	}

	// 关闭监听流并释放流对象
	sstream->close();

	// XXX: 为了保证能关闭监听流，应在此处再 check 一下
	handle.check();

	// 删除 acl::polarssl_conf 动态对象
	delete __ssl_conf;
	return 0;
}
