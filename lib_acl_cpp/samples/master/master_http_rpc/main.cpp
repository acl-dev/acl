#include "stdafx.h"
#include "master_service.h"

bool var_mem_slice_on = false;

int main(int argc, char* argv[])
{
	if (0)
	{
		var_mem_slice_on = true;
		acl_mem_slice_init(8, 1024, 100000,
			ACL_SLICE_FLAG_GC2 |
			ACL_SLICE_FLAG_RTGC_OFF |
			ACL_SLICE_FLAG_LP64_ALIGN);
	}

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
		const char* addr = "127.0.0.1:8888";
		printf("listen on: %s\r\n", addr);
		ms.run_alone(addr, NULL, acl::ENGINE_KERNEL);  // 单独运行方式
	}
	else
	{
#ifdef	WIN32
		const char* addr = "127.0.0.1:8888";
		printf("listen on: %s\r\n", addr);

		ms.run_alone(addr, NULL, acl::ENGINE_KERNEL);  // 单独运行方式
#else
		ms.run_daemon(argc, argv);  // acl_master 控制模式运行
#endif
	}

	return 0;
}
