// master_proc.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "acl_cpp/lib_acl.hpp"

static char *var_cfg_debug_msg;

static acl::master_str_tbl var_conf_str_tab[] = {
	{ "debug_msg", "test_msg", &var_cfg_debug_msg },

	{ 0, 0, 0 }
};

static int  var_cfg_debug_enable;

static acl::master_bool_tbl var_conf_bool_tab[] = {
	{ "debug_enable", 1, &var_cfg_debug_enable },

	{ 0, 0, 0 }
};

static int  var_cfg_io_timeout;

static acl::master_int_tbl var_conf_int_tab[] = {
	{ "io_timeout", 120, &var_cfg_io_timeout, 0, 0 },

	{ 0, 0 , 0 , 0, 0 }
};

static void (*format)(const char*, ...) = acl::log::msg1;

//////////////////////////////////////////////////////////////////////////
using namespace acl;

class master_proc_test : public master_proc
{
public:
	master_proc_test() {}
	~master_proc_test() {}
protected:
	/**
	 * 基类纯虚函数：当接收到一个客户端连接时调用此函数
	 * @param stream {aio_socket_stream*} 新接收到的客户端异步流对象
	 * 注：该函数返回后，流连接将会被关闭，用户不应主动关闭该流
	 */
	virtual void on_accept(socket_stream* stream)
	{
		if (stream->format("hello, you're welcome!\r\n") == -1)
			return;
		while (true)
		{
			if (on_read(stream) == false)
				break;
		}
	}

	bool on_read(socket_stream* stream)
	{
		string buf;
		if (stream->gets(buf) == false)
		{
			format("gets error: %s", acl::last_serror());
			return false;
		}
		if (buf == "quit")
		{
			stream->puts("bye!");
			return false;
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
//////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[])
{
	master_proc_test& mp = singleton2<master_proc_test>::get_instance();

	// 设置配置参数表
	mp.set_cfg_int(var_conf_int_tab);
	mp.set_cfg_int64(NULL);
	mp.set_cfg_str(var_conf_str_tab);
	mp.set_cfg_bool(var_conf_bool_tab);

	// 开始运行

	if (argc >= 2 && strcmp(argv[1], "alone") == 0)
	{
		format = (void (*)(const char*, ...)) printf;
		mp.run_alone("127.0.0.1:8888", NULL, 5);  // 单独运行方式
	}
	else
	{
#ifdef	WIN32
		format = (void (*)(const char*, ...)) printf;
		mp.run_alone("127.0.0.1:8888", NULL, 5);  // 单独运行方式
#else
		mp.run_daemon(argc, argv);  // acl_master 控制模式运行
#endif
	}

	return 0;
}

