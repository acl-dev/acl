#include "stdafx.h"
#include "master_service.h"

int main(int argc, char* argv[])
{
	// 初始化 acl 库
	acl::acl_cpp_init();

	master_service& ms = acl::singleton2<master_service>::get_instance();

	// 设置配置参数表
	ms.set_cfg_int(var_conf_int_tab);
	ms.set_cfg_int64(var_conf_int64_tab);
	ms.set_cfg_str(var_conf_str_tab);
	ms.set_cfg_bool(var_conf_bool_tab);

	// 开始运行

	if (argc >= 2 && strcmp(argv[1], "alone") == 0)
	{
		// 日志输出至标准输出
		acl::log::stdout_open(true);

		// 监听的地址列表，格式：ip:port1,ip:port2,...
		const char* addrs = ":8886";
		printf("listen on: %s\r\n", addrs);

		// 测试时设置该值 > 0 则指定服务器处理客户端连接过程的
		// 会话总数（一个连接从接收到关闭称之为一个会话），当
		// 处理的连接会话数超过此值，测试过程结束；如果该值设
		// 为 0，则测试过程永远不结束
		unsigned int count = 0;

		// 测试过程中指定线程池最大线程个数
		unsigned int max_threads = 100;

		// 单独运行方式

		if (argc >= 3)
			ms.run_alone(addrs, argv[2], count, max_threads);
		else
			ms.run_alone(addrs, NULL, count, max_threads);

		printf("Enter any key to exit now\r\n");
		getchar();
	}
	else
	{
#ifdef	WIN32
		// 日志输出至标准输出
		acl::log::stdout_open(true);

		// 监听的地址列表，格式：ip:port1,ip:port2,...
		const char* addrs = "127.0.0.1:8888";
		printf("listen on: %s\r\n", addrs);

		// 测试时设置该值 > 0 则指定服务器处理客户端连接过程的
		// 会话总数（一个连接从接收到关闭称之为一个会话），当
		// 处理的连接会话数超过此值，测试过程结束；如果该值设
		// 为 0，则测试过程永远不结束
		unsigned int count = 0;

		// 测试过程中指定线程池最大线程个数
		unsigned int max_threads = 100;

		// 单独运行方式
		ms.run_alone(addrs, NULL, count, max_threads);
		printf("Enter any key to exit now\r\n");
		getchar();
#else
		// acl_master 控制模式运行
		ms.run_daemon(argc, argv);
#endif
	}

	return 0;
}
