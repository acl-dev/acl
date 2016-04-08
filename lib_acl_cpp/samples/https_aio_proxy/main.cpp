#include "stdafx.h"
#include "master_service.h"

int main(int argc, char* argv[])
{
	// 初始化 acl 库
	acl::acl_cpp_init();

	master_service& ms = acl::singleton2<master_service>::get_instance();

	// 设置配置参数表
	ms.set_cfg_int(var_conf_int_tab);
	ms.set_cfg_str(var_conf_str_tab);
	ms.set_cfg_bool(var_conf_bool_tab);

	// 开始运行
	if (argc >= 2 && strcmp(argv[1], "alone") == 0)
	{
		acl::log::stdout_open(true);  // 日志输出至标准输出
		const char* addr = "127.0.0.1:5200";
		printf("listen on: %s\r\n", addr);
		if (argc >= 3)
			ms.run_alone(addr, argv[2], acl::ENGINE_SELECT);
		else
			ms.run_alone(addr, NULL, acl::ENGINE_SELECT);  // 单独运行方式
	}
	else
	{
#ifdef	WIN32
		acl::log::stdout_open(true);  // 日志输出至标准输出
		const char* addr = "127.0.0.1:5200";
		printf("listen on: %s\r\n", addr);

		ms.run_alone(addr, NULL, acl::ENGINE_SELECT);  // 单独运行方式
#else
		ms.run_daemon(argc, argv);  // acl_master 控制模式运行
#endif
	}

	return 0;
}
