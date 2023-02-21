#include "stdafx.h"
#include "master_service.h"

int main(int argc, char *argv[])
{
	acl::acl_cpp_init();
	master_service& ms = acl::singleton2<master_service>::get_instance();

	// 设置配置参数表
	ms.set_cfg_int(var_conf_int_tab);
	ms.set_cfg_int64(var_conf_int64_tab);
	ms.set_cfg_str(var_conf_str_tab);
	ms.set_cfg_bool(var_conf_bool_tab);

	if (argc == 1 || (argc >= 2 && strcasecmp(argv[1], "alone") == 0)) {
		// 日志输出至标准输出
		acl::log::stdout_open(true);
		// 禁止生成 acl_master.log 日志
		acl::master_log_enable(false);

		//const char* addr = "127.0.0.1|16379";
		//printf("listen: %s\r\n", addr);
		const char* addr = NULL;
		ms.run_alone(addr, argc >= 3 ? argv[2] : NULL);
	} else {
#if defined(_WIN32) || defined(_WIN64)
		const char* addr = "|16379";

		acl::log::stdout_open(true);
		printf("listen on: %s\r\n", addrs);
		ms.run_alone(addr, argc >= 3 ? argv[2] : NULL);
#else
		ms.run_daemon(argc, argv);
#endif
	}

	return 0;
}
