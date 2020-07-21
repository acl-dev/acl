#include <iostream>
#include <assert.h>
#include "lib_acl.h" // just for getopt on Windows
#include "acl_cpp/stdlib/util.hpp"
#include "acl_cpp/acl_cpp_init.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stream/aio_handle.hpp"
#include "acl_cpp/stream/aio_istream.hpp"
#include "acl_cpp/stream/aio_listen_stream.hpp"
#include "acl_cpp/stream/aio_socket_stream.hpp"

static int   __max = 0;
static int   __timeout = 0;

/**
 * 延迟读回调处理类
 */
class timer_reader: public acl::aio_timer_reader
{
public:
	timer_reader(int delay)
	{
		delay_ = delay;
		std::cout << "timer_reader init, delay: " << delay << std::endl;
	}

protected:
	~timer_reader(void) {}

	// aio_timer_reader 的子类必须重载 destroy 方法
	// @override
	void destroy(void)
	{
		std::cout << "timer_reader delete, delay: "  << delay_ << std::endl;
		delete this;
	}

	// 重载基类回调方法
	// @override
	void timer_callback(unsigned int id)
	{
		std::cout << "timer_reader(" << id
			<< "): timer_callback, delay: " << delay_ << std::endl;

		// 调用基类的处理过程
		aio_timer_reader::timer_callback(id);
	}

private:
	int   delay_;
};

/**
 * 延迟写回调处理类
 */
class timer_writer: public acl::aio_timer_writer
{
public:
	timer_writer(int delay)
	{
		delay_ = delay;
		std::cout << "timer_writer init, delay: " << delay << std::endl;
	}

protected:
	~timer_writer(void) {}

	// aio_timer_reader 的子类必须重载 destroy 方法
	// @override
	void destroy(void)
	{
		std::cout << "timer_writer delete, delay: " << delay_ << std::endl;
		delete this;
	}

	// 重载基类回调方法
	// @override
	void timer_callback(unsigned int id)
	{
		std::cout << "timer_writer(" << id << "): timer_callback, delay: "
			<< delay_ << std::endl;

		// 调用基类的处理过程
		acl::aio_timer_writer::timer_callback(id);
	}

private:
	int   delay_;
};

/**
 * 异步客户端流的回调类的子类
 */
class io_callback : public acl::aio_callback
{
public:
	io_callback(acl::aio_socket_stream* client)
	: client_(client)
	, i_(0) {}

protected:
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
		i_++;
		if (i_ < 5) {
			std::cout << ">>gets(i:" << i_ << "): "
				<< data << std::endl;
		}

		// 如果远程客户端希望退出，则关闭之
		if (strncasecmp(data, "quit", 4) == 0) {
			client_->format("Bye!\r\n");
			client_->close();
			return false;
		}

		// 如果远程客户端希望服务端也关闭，则中止异步事件过程
		else if (strncasecmp(data, "stop", 4) == 0) {
			client_->format("Stop now!\r\n");
			client_->close();  // 关闭远程异步流

			// 通知异步引擎关闭循环过程
			client_->get_handle().stop();
		}

		// 向远程客户端回写收到的数据

		int   delay = 0;

		if (!strncasecmp(data, "write_delay", strlen("write_delay"))) {
			// 延迟写过程

			const char* ptr = data + strlen("write_delay");
			delay = atoi(ptr);
			if (delay > 0) {
				std::cout << ">> write delay " << delay
					<< " second ..." << std::endl;
				timer_writer* timer = new timer_writer(delay);
				client_->write(data, len, delay * 1000000, timer);
				client_->gets(10, false);
				return true;
			}
		} else if (!strncasecmp(data, "read_delay", strlen("read_delay"))) {
			// 延迟读过程

			const char* ptr = data + strlen("read_delay");
			delay = atoi(ptr);
			if (delay > 0) {
				client_->write(data, len);
				std::cout << ">> read delay " << delay
					<< " second ..." << std::endl;
				timer_reader* timer = new timer_reader(delay);
				client_->gets(10, false, delay * 1000000, timer);
				return true;
			}
		}

		client_->write(data, len);
		//client_->gets(10, false);
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
		return (false);
	}

private:
	acl::aio_socket_stream* client_;
	int  i_;
};

/**
 * 异步监听流的回调类的子类
 */
class io_accept_callback : public acl::aio_accept_callback
			 , public acl::aio_listen_callback
{
public:
	io_accept_callback(void) {}
	~io_accept_callback(void)
	{
		printf(">>io_accept_callback over!\n");
	}

	/**
	 * 基类 aio_accept_callback 虚函数，当有新连接到达后调用此回调过程
	 * @param client {aio_socket_stream*} 异步客户端流
	 * @return {bool} 返回 true 以通知监听流继续监听
	 */
	bool accept_callback(acl::aio_socket_stream* client)
	{
		printf("proactor accept one\r\n");
		return handle_client(client);
	}

	/**
	 * 基类 aio_listen_callback 虚函数，当有新连接到达后调用此回调过程
	 * @param server {acl::aio_listen_stream&} 异步监听流
	 * @return {bool}
	 */
	bool listen_callback(acl::aio_listen_stream& server)
	{
		// reactor 模式下需要用户自己调用 accept 方法
		acl::aio_socket_stream* client = server.accept();
		if (client == NULL) {
			printf("accept error %s\r\n", acl::last_serror());
			return false;
		}

		printf("reactor accept one\r\n");
		return handle_client(client);
	}

private:
	bool handle_client(acl::aio_socket_stream* client)
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

		// 当限定了行数据最大长度时
		if (__max > 0) {
			client->set_buf_max(__max);
		}

		// 从异步流读一行数据
		client->gets(__timeout, false);
		return true;
	}
};

class mytimer : public acl::aio_timer_callback {
public:
	mytimer(long long delay) : id_(0), last_(time(NULL)), delay_(delay) {}
	~mytimer(void) {}

protected:
	// @override
	void destroy(void) {
		delete this;
	}

	// @override
	void timer_callback(unsigned int id) {
		time_t now = time(NULL);
		delay_ += 1000000;
		this->set_task(id_, delay_);

		printf("timer id=%u, delay=%ld, next delay=%lld\r\n",
			id, (long) (now - last_), delay_);

		last_ = now;
		id_ = id;
	}

private:
	unsigned int id_;
	time_t last_;
	long long delay_;
};

static void usage(const char* procname)
{
	printf("usage: %s -h[help]\r\n"
		"	-l ip:port\r\n"
		"	-L line_max_length\r\n"
		"	-t timeout\r\n"
		"	-r [use reactor mode other proactor mode, default: proactor mode]\r\n"
		"	-k[use kernel event: epoll/iocp/kqueue/devpool]\r\n",
		procname);
}

int main(int argc, char* argv[])
{
	bool use_kernel = false, use_reactor = false;
	int  ch;
	acl::string addr(":9001");

	while ((ch = getopt(argc, argv, "l:hkL:t:r")) > 0) {
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
		case 'L':
			__max = atoi(optarg);
			break;
		case 't':
			__timeout = atoi(optarg);
			break;
		case 'r':
			use_reactor = true;
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

	long long delay = 1000000;
	mytimer* timer = new mytimer(delay);
	timer->keep_timer(true);
	handle.set_timer(timer, delay);

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

	if (use_reactor) {
		sstream->add_listen_callback(&callback);
	} else {
		sstream->add_accept_callback(&callback);
	}

	std::cout << "Listen: " << addr.c_str() << " ok!" << std::endl;

	while (true) {
		// 如果返回 false 则表示不再继续，需要退出
		if (!handle.check()) {
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
