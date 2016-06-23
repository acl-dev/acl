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

	if (argc >= 2 && strcasecmp(argv[1], "alone") == 0)
	{
		const char* addr = ":8887";

		acl::log::stdout_open(true);
		printf("listen: %s\r\n", addr);
		ms.run_alone(addr, argc >= 3 ? argv[2] : NULL, 0);
	}
	else
		ms.run_daemon(argc, argv);

	return 0;
}
