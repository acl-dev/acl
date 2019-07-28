// master_aio.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "acl_cpp/lib_acl.hpp"

static char *var_cfg_debug_msg;

static acl::master_str_tbl var_conf_str_tab[] = {
	{ "debug_msg", "test_msg", &var_cfg_debug_msg },

	{ 0, 0, 0 }
};

static int  var_cfg_debug_enable;
static int  var_cfg_keep_alive;
static int  var_cfg_send_banner;

static acl::master_bool_tbl var_conf_bool_tab[] = {
	{ "debug_enable", 1, &var_cfg_debug_enable },
	{ "keep_alive", 1, &var_cfg_keep_alive },
	{ "send_banner", 1, &var_cfg_send_banner },

	{ 0, 0, 0 }
};

static int  var_cfg_io_timeout;

static acl::master_int_tbl var_conf_int_tab[] = {
	{ "io_timeout", 120, &var_cfg_io_timeout, 0, 0 },

	{ 0, 0 , 0 , 0, 0 }
};

static void (*format)(const char*, ...) = acl::log::msg1;

using namespace acl;

//////////////////////////////////////////////////////////////////////////
/**
 * 延迟读回调处理类
 */
class timer_reader: public aio_timer_reader
{
public:
	timer_reader(int delay)
	{
		delay_ = delay;
		format("timer_reader init, delay: %d\r\n", delay);
	}

	~timer_reader()
	{
	}

	// aio_timer_reader 的子类必须重载 destroy 方法
	void destroy()
	{
		format("timer_reader delete, delay: %d\r\n", delay_);
		delete this;
	}

	// 重载基类回调方法
	virtual void timer_callback(unsigned int id)
	{
		format("timer_reader(%u): timer_callback, delay: %d\r\n", id, delay_);

		// 调用基类的处理过程
		aio_timer_reader::timer_callback(id);
	}

private:
	int   delay_;
};

/**
 * 延迟写回调处理类
 */
class timer_writer: public aio_timer_writer
{
public:
	timer_writer(int delay)
	{
		delay_ = delay;
		format("timer_writer init, delay: %d\r\n", delay);
	}

	~timer_writer()
	{
	}

	// aio_timer_reader 的子类必须重载 destroy 方法
	void destroy()
	{
		format("timer_writer delete, delay: %d\r\n", delay_);
		delete this;
	}

	// 重载基类回调方法
	virtual void timer_callback(unsigned int id)
	{
		format("timer_writer(%u): timer_callback, delay: %u\r\n", id, delay_);

		// 调用基类的处理过程
		aio_timer_writer::timer_callback(id);
	}

private:
	int   delay_;
};

class timer_test : public aio_timer_callback
{
public:
	timer_test() : aio_timer_callback(true) {}
	~timer_test() {}
protected:
	// 基类纯虚函数
	virtual void timer_callback(unsigned int id)
	{
		format("id: %u\r\n", id);
	}

	virtual void destroy(void)
	{
		delete this;
		format("timer delete now\r\n");
	}
private:
};
/**
 * 异步客户端流的回调类的子类
 */
class io_callback : public aio_callback
{
public:
	io_callback(aio_socket_stream* client)
		: client_(client)
		, i_(0)
	{
	}

	~io_callback()
	{
		format("delete io_callback now ...\r\n");
	}

	/**
	 * 实现父类中的虚函数，客户端流的读成功回调过程
	 * @param data {char*} 读到的数据地址
	 * @param len {int} 读到的数据长度
	 * @return {bool} 返回 true 表示继续，否则希望关闭该异步流
	 */
	virtual bool read_callback(char* data, int len)
	{
		if (++i_ < 10)
			format(">>gets(i: %d): %s\r\n", i_, data);

		// 如果远程客户端希望退出，则关闭之
		if (strncmp(data, "quit", 4) == 0)
		{
			client_->format("Bye!\r\n");
			client_->close();
		}

		// 如果远程客户端希望服务端也关闭，则中止异步事件过程
		else if (strncmp(data, "stop", 4) == 0)
		{
			client_->format("Stop now!\r\n");
			client_->close();  // 关闭远程异步流

			// 通知异步引擎关闭循环过程
			client_->get_handle().stop();
		}

		// 向远程客户端回写收到的数据

		int   delay = 0;

		if (strncmp(data, "write_delay", strlen("write_delay")) == 0)
		{
			// 延迟写过程

			const char* ptr = data + strlen("write_delay");
			delay = atoi(ptr);
			if (delay > 0)
			{
				format(">> write delay %d second ...\r\n", delay);
				timer_writer* timer = new timer_writer(delay);
				client_->write(data, len, delay * 1000000, timer);
				client_->gets(10, false);
				return (true);
			}
		}
		else if (strncmp(data, "read_delay", strlen("read_delay")) == 0)
		{
			// 延迟读过程

			const char* ptr = data + strlen("read_delay");
			delay = atoi(ptr);
			if (delay > 0)
			{
				client_->write(data, len);
				format(">> read delay %d second ...\r\n", delay);
				timer_reader* timer = new timer_reader(delay);
				client_->gets(10, false, delay * 1000000, timer);
				return (true);
			}
		}

		client_->write(data, len);
		//client_->gets(10, false);
		return (true);
	}

	/**
	 * 实现父类中的虚函数，客户端流的写成功回调过程
	 * @return {bool} 返回 true 表示继续，否则希望关闭该异步流
	 */
	virtual bool write_callback()
	{
		return (true);
	}

	/**
	 * 实现父类中的虚函数，客户端流的超时回调过程
	 */
	virtual void close_callback()
	{
		// 必须在此处删除该动态分配的回调类对象以防止内存泄露
		format("Close\r\n");
		delete this;
	}

	/**
	 * 实现父类中的虚函数，客户端流的超时回调过程
	 * @return {bool} 返回 true 表示继续，否则希望关闭该异步流
	 */
	virtual bool timeout_callback()
	{
		format("Timeout ...\r\n");
		return (true);
	}

private:
	aio_socket_stream* client_;
	int   i_;
};

//////////////////////////////////////////////////////////////////////////

class master_aio_test : public master_aio
{
public:
	master_aio_test() { timer_test_ = new timer_test(); }

	~master_aio_test() { }

protected:
	// 基类纯虚函数：当接收到一个新的连接时调用此函数
	virtual bool on_accept(aio_socket_stream* client)
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

		// 写欢迎信息
		if (var_cfg_send_banner)
			client->format("hello, you're welcome\r\n");

		// 从异步流读一行数据
		client->gets(10, false);
		//client->read();
		return true;
	}

	// 基类虚函数：服务进程切换用户身份前调用此函数
	virtual void proc_pre_jail()
	{
		format("proc_pre_jail\r\n");
		// 只有当程序启动后才能获得异步引擎句柄
		handle_ = get_handle();
		//handle_->keep_timer(true); // 允许定时器被重复触发
		// 设置第一个定时任务，每隔1秒触发一次，定时任务ID为0
		handle_->set_timer(timer_test_, 1000000, 0);
	}

	// 基类虚函数：服务进程切换用户身份后调用此函数
	virtual void proc_on_init()
	{
		format("proc init\r\n");
		// 设置第二个定时任务，每隔2秒触发一次，定时任务ID为1
		handle_->set_timer(timer_test_, 2000000, 1);
	}

	// 基类虚函数：服务进程退出前调用此函数
	virtual void proc_on_exit()
	{
		format("proc exit\r\n");
	}
private:
	timer_test* timer_test_;
	aio_handle* handle_;
};
//////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[])
{
#if 0
	int base = 8, nslice = 1024, nalloc_gc = 1000000;
	unsigned int slice_flag = ACL_SLICE_FLAG_GC2 | ACL_SLICE_FLAG_RTGC_OFF;

	acl_mem_slice_init(base, nslice, nalloc_gc, slice_flag);
#endif

	master_aio_test ma;

	// 设置配置参数表
	ma.set_cfg_int(var_conf_int_tab);
	ma.set_cfg_int64(NULL);
	ma.set_cfg_str(var_conf_str_tab);
	ma.set_cfg_bool(var_conf_bool_tab);

	// 开始运行

	if (argc >= 2 && strcmp(argv[1], "alone") == 0)
	{
		const char* addr = "127.0.0.1:8888, 127.0.0.1:8889";

		if (argc >= 3)
			addr = argv[2];
		
		const char* conf;
		if (argc >= 4)
			conf = argv[3];
		else
			conf = NULL;

		format = (void (*)(const char*, ...)) printf;
		format("listen: %s now\r\n", addr);
		ma.run_alone(addr, conf);  // 单独运行方式
	}
	else
	{
#ifdef	WIN32
		const char* addr = "127.0.0.1:8888, 127.0.0.1:8889";
		const char* conf = "./master_aio.cf";

		format = (void (*)(const char*, ...)) printf;
		format("listen: %s now\r\n", addr);
		ma.run_alone(addr, conf);  // 单独运行方式

#else
		ma.run_daemon(argc, argv);  // acl_master 控制模式运行
#endif
	}
	return 0;
}
