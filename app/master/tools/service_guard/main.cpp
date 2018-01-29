#include "stdafx.h"
#include "configure.h"
#include "tcp_service.h"
#include "udp_service.h"

static void init_configure(acl::master_base& base)
{
	// 设置配置参数表
	base.set_cfg_int(var_conf_int_tab);
	base.set_cfg_int64(var_conf_int64_tab);
	base.set_cfg_str(var_conf_str_tab);
	base.set_cfg_bool(var_conf_bool_tab);
}

static void run_tcp_service(int argc, char* argv[])
{
	tcp_service& ts = acl::singleton2<tcp_service>::get_instance();
	init_configure(ts);

	if (argc >= 2 && strcmp(argv[1], "alone") == 0)
	{
		// 日志输出至标准输出
		acl::log::stdout_open(true);

		// 监听的地址列表，格式：ip:port1,ip:port2,...
		const char* addrs = ":8888";
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
			ts.run_alone(addrs, argv[2], count, max_threads);
		else
			ts.run_alone(addrs, NULL, count, max_threads);

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
		ts.run_alone(addrs, NULL, count, max_threads);
		printf("Enter any key to exit now\r\n");
		getchar();
#else
		// acl_master 控制模式运行
		ts.run_daemon(argc, argv);
#endif
	}
}

static void run_udp_service(int argc, char* argv[])
{
	udp_service& us = acl::singleton2<udp_service>::get_instance();
	init_configure(us);

	if (argc >= 2 && strcmp(argv[1], "alone") == 0)
	{
		// 日志输出至标准输出
		acl::log::stdout_open(true);

		// 监听的地址列表，格式：ip:port1,ip:port2,...
		const char* addrs = "0.0.0.0:8390";
		printf("bind on: %s\r\n", addrs);

		// 测试时设置该值 > 0 则指定服务器处理客户端连接过程的
		// 会话总数（一个连接从接收到关闭称之为一个会话），当
		// 处理的连接会话数超过此值，测试过程结束；如果该值设
		// 为 0，则测试过程永远不结束
		unsigned int count = 0;

		// 单独运行方式
		if (argc >= 3)
			us.run_alone(addrs, argv[2], count);
		else
			us.run_alone(addrs, NULL, count);

		printf("Enter any key to exit now\r\n");
		getchar();
	}
	else
	{
#ifdef	WIN32
		// 日志输出至标准输出
		acl::log::stdout_open(true);

		// 监听的地址列表，格式：ip:port1,ip:port2,...
		const char* addrs = "0.0.0.0:8390";
		printf("bind on: %s\r\n", addrs);

		// 测试时设置该值 > 0 则指定服务器处理客户端连接过程的
		// 会话总数（一个连接从接收到关闭称之为一个会话），当
		// 处理的连接会话数超过此值，测试过程结束；如果该值设
		// 为 0，则测试过程永远不结束
		unsigned int count = 0;

		// 单独运行方式
		us.run_alone(addrs, NULL, count);
		printf("Enter any key to exit now\r\n");
		getchar();
#else
		// acl_master 控制模式运行
		us.run_daemon(argc, argv);
#endif
	}
}

static bool check_service(int argc, char* argv[])
{
	bool is_tcp_service = false;

	for (int i = 0; i < argc; i++)
	{
		if (argv[i][0] != '-')
			continue;
		switch (argv[i][1])
		{
		case 'M':
			if (i + 1 < argc && !strcasecmp(argv[i + 1], "tcp"))
			{
				is_tcp_service = true;
				break;
			}
		default:
			break;
		}
	}

	return is_tcp_service;
}

int main(int argc, char* argv[])
{
	// 初始化 acl 库
	acl::acl_cpp_init();

	bool tcp_mode = check_service(argc, argv);

	if (tcp_mode)
		run_tcp_service(argc, argv);
	else
		run_udp_service(argc, argv);
	return 0;
}
