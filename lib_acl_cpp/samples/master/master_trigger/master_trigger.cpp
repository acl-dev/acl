// master_trigger.cpp : 定义控制台应用程序的入口点。
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

class master_trigger_test : public master_trigger
{
public:
	master_trigger_test() {}
	~master_trigger_test() {}

protected:
	// 基类纯虚函数：当触发器时间到时调用此函数
	virtual void on_trigger()
	{
		format("on trigger now\r\n");
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
protected:
private:
};
//////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[])
{
	master_trigger_test mt;

	// 设置配置参数表
	mt.set_cfg_int(var_conf_int_tab);
	mt.set_cfg_int64(NULL);
	mt.set_cfg_str(var_conf_str_tab);
	mt.set_cfg_bool(var_conf_bool_tab);

	// 开始运行

	if (argc >= 2 && strcmp(argv[1], "alone") == 0)
	{
		format = (void (*)(const char*, ...)) printf;
		mt.run_alone(NULL, 5, 1);  // 单独运行方式
	}
	else
	{
#ifdef	WIN32
		format = (void (*)(const char*, ...)) printf;
		mt.run_alone(NULL, 5, 1);  // 单独运行方式
#else
		mt.run_daemon(argc, argv);  // acl_master 控制模式运行
#endif
	}

	return 0;
}

