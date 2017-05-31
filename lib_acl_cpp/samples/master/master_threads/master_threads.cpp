// master_threads.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "lib_acl.h"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/util.hpp"
#include "acl_cpp/stream/server_socket.hpp"
#include "acl_cpp/master/master_threads.hpp"
#include "acl_cpp/event/event_timer.hpp"
#include "acl_cpp/stream/socket_stream.hpp"

static char *var_cfg_debug_msg;

static acl::master_str_tbl var_conf_str_tab[] = {
	{ "debug_msg", "test_msg", &var_cfg_debug_msg },

	{ 0, 0, 0 }
};

static int  var_cfg_debug_enable;
static int  var_cfg_keep_alive;
static int  var_cfg_loop;

static acl::master_bool_tbl var_conf_bool_tab[] = {
	{ "debug_enable", 1, &var_cfg_debug_enable },
	{ "keep_alive", 1, &var_cfg_keep_alive },
	{ "loop_read", 1, &var_cfg_loop },

	{ 0, 0, 0 }
};

static int  var_cfg_io_timeout;

static acl::master_int_tbl var_conf_int_tab[] = {
	{ "io_timeout", 120, &var_cfg_io_timeout, 0, 0 },

	{ 0, 0 , 0 , 0, 0 }
};

static void (*format)(const char*, ...) = acl::log::msg1;

//////////////////////////////////////////////////////////////////////////

class master_timer_test : public acl::event_timer
{
public:
	master_timer_test(acl::socket_stream* stream)
	: max_(0)
	, count_(0)
	, stream_(stream)
	{
		(void) stream_;
	}

	void set_max(int max)
	{
		max_ = max;
	}

protected:
	// 基类虚函数
	virtual void timer_callback(unsigned int id)
	{
		format("timer callback, id: %u\r\n", id);
		if (count_++ >= max_)
		{
			printf("clear all timer task now\r\n");
			clear();
		}
		//else
		//	set_task(1000, 1000000);
	}

	virtual void destroy()
	{
		format("destroy called\r\n");
		delete this;
	}

private:
	int  max_;
	int  count_;
	acl::socket_stream* stream_;

	~master_timer_test()
	{
		format("timer destroy now!\r\n");
	}
};

//////////////////////////////////////////////////////////////////////////

class master_threads_test : public acl::master_threads
{
public:
	master_threads_test()
	{

	}

	~master_threads_test()
	{

	}

protected:
	// 基类纯虚函数：当客户端连接有数据可读或关闭时回调此函数，返回 true 表示
	// 继续与客户端保持长连接，否则表示需要关闭客户端连接
	virtual bool thread_on_read(acl::socket_stream* stream)
	{
		while (true)
		{
			if (on_read(stream) == false)
				return false;
			if (var_cfg_loop == 0)
				break;
		}
		return true;
	}

	bool on_read(acl::socket_stream* stream)
	{
		format("%s(%d)", __FILE__, __LINE__);
		acl::string buf;
		if (stream->gets(buf) == false)
		{
			format("gets error: %s", acl::last_serror());
			format("%s(%d)", __FILE__, __LINE__);
			return false;
		}
		if (buf == "quit")
		{
			stream->puts("bye!");
			return false;
		}

		if (buf == "timer")
		{
			int  max = 0;
			master_timer_test* timer = new master_timer_test(stream);
			timer->keep_timer(true);

			timer->set_task(1000, 1000000);
			max += 2;
			timer->set_task(1001, 1000000);
			max += 2;
			timer->set_task(1002, 1000000);
			max += 2;
			timer->set_task(1003, 1000000);
			max += 2;
			timer->set_max(max);

			// 调用基类方法设置定时器任务
			proc_set_timer(timer);
			stream->format("set timer ok\r\n");
			return true;
		}

		if (buf.empty())
		{
			if (stream->write("\r\n") == -1)
			{
				format("write 1 error: %s", acl::last_serror());
				return false;
			}
		}
		else if (stream->write(buf) == -1)
		{
			format("write 2 error: %s, buf(%s), len: %d",
				acl::last_serror(), buf.c_str(), (int) buf.length());
			return false;
		}
		else if (stream->write("\r\n") == -1)
		{
			format("write 3 client error: %s", acl::last_serror());
			return false;
		}
		return true;
	}

	// 基类虚函数：当接收到一个客户端请求时，调用此函数，允许
	// 子类事先对客户端连接进行处理，返回 true 表示继续，否则
	// 要求关闭该客户端连接
	virtual bool thread_on_accept(acl::socket_stream* stream)
	{
		stream->set_rw_timeout(2);
		format("accept one client, peer: %s, local: %s, var_cfg_io_timeout: %d\r\n",
			stream->get_peer(), stream->get_local(), var_cfg_io_timeout);
		if (stream->format("hello, you're welcome!\r\n") == -1)
			return false;
		return true;
	}

	// 基类虚函数：当客户端连接关闭时调用此函数
	virtual void thread_on_close(acl::socket_stream*)
	{
		format("client closed now\r\n");
	}

	// 基类虚函数：当线程池创建一个新线程时调用此函数
	virtual void thread_on_init()
	{
#ifdef WIN32
		format("thread init: tid: %lu\r\n", GetCurrentThreadId());
#else
		format("thread init: tid: %lu\r\n", pthread_self());
#endif
	}

	// 基类虚函数：当线程池中的一个线程退出时调用此函数
	virtual void thread_on_exit()
	{
#ifdef WIN32
		format("thread exit: tid: %lu\r\n", GetCurrentThreadId());
#else
		format("thread exit: tid: %lu\r\n", pthread_self());
#endif
	}

	// 基类虚函数：服务进程切换用户身份前调用此函数
	virtual void proc_pre_jail()
	{
		format("proc_pre_jail\r\n");
	}

	// 基类虚函数：服务进程切换用户身份后调用此函数
	virtual void proc_on_init()
	{
		format("proc init\r\n");
	}

	// 基类虚函数：服务进程退出前调用此函数
	virtual void proc_on_exit()
	{
		format("proc exit\r\n");
	}
private:
};

int main(int argc, char* argv[])
{
#if 0
	int base = 8, nslice = 1024, nalloc_gc = 1000000;
	unsigned int slice_flag = ACL_SLICE_FLAG_GC2 | ACL_SLICE_FLAG_RTGC_OFF;

	acl_mem_slice_init(base, nslice, nalloc_gc, slice_flag);
#endif

	master_threads_test mt;

	// 设置配置参数表
	mt.set_cfg_int(var_conf_int_tab);
	mt.set_cfg_int64(NULL);
	mt.set_cfg_str(var_conf_str_tab);
	mt.set_cfg_bool(var_conf_bool_tab);

	// 开始运行

	if (argc >= 2 && strcmp(argv[1], "alone") == 0)
	{
		int   task_count = 2, threads_count = 2;
		format = (void (*)(const char*, ...)) printf;
		format("listen: 127.0.0.1:8888\r\n");
		acl::log::stdout_open(true);

		// 单独运行方式
		if (argc >= 3)
			mt.run_alone("127.0.0.1:8888", argv[2],
				task_count, threads_count);
		else
			mt.run_alone("127.0.0.1:8888", NULL,
				task_count, threads_count);
	}

	// acl_master 控制模式运行
	else
	{
#ifdef	WIN32
		int   task_count = 2, threads_count = 2;
		format = (void (*)(const char*, ...)) printf;
		format("listen: 127.0.0.1:8888\r\n");

		// 单独运行方式
		mt.run_alone("127.0.0.1:8888", NULL,
			task_count, threads_count);
#else
		mt.run_daemon(argc, argv);
#endif
	}

	return 0;
}
