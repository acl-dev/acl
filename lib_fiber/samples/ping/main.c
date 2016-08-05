#include "lib_acl.h"
#include "lib_protocol.h"
#include "fiber/lib_fiber.h"
#include <signal.h>

static int __nfibers = 0;
static int __npkt = 10;

static void display_res(ICMP_CHAT *chat)
{
	if (chat) {
		/* 显示 PING 的结果总结 */
		icmp_stat(chat);
		printf(">>>max pkts: %d\r\n", icmp_chat_seqno(chat));
	}
}

/* PING 线程入口 */
static void fiber_ping(ACL_FIBER *fiber acl_unused, void *arg)
{
	const char *dest = (const char *) arg;
	ACL_DNS_DB* dns_db;
	const char *ip;
	int   delay = 1;  /* 发送 ping 包的时间间隔（秒）*/
	ICMP_CHAT *chat;

	/* 通过域名解析出IP地址 */
	dns_db = acl_gethostbyname(dest, NULL);
	if (dns_db == NULL) {
		acl_msg_warn("Can't find domain %s", dest);
		return;
	}

	/* 只取出域名第一个 IP 地址 PING */
	ip = acl_netdb_index_ip(dns_db, 0);
	if (ip == NULL || *ip == 0) {
		acl_msg_error("ip invalid");
		acl_netdb_free(dns_db);
		return;
	}

	/* 创建 ICMP 对象 */
	chat = icmp_chat_create(NULL, 1);

	/* 开始 PING */
	if (strcmp(dest, ip) == 0)
		icmp_ping_one(chat, NULL, ip, __npkt, delay, 1);
	else
		icmp_ping_one(chat, dest, ip, __npkt, delay, 1);

	acl_netdb_free(dns_db);  /* 释放域名解析对象 */
	display_res(chat);  /* 显示 PING 结果 */
	icmp_chat_free(chat);  /* 释放 ICMP 对象 */

	if (--__nfibers == 0)
		acl_fiber_stop();
}

static void usage(const char* progname)
{
	printf("usage: %s [-h help] [-n npkt] [\"dest1 dest2 dest3...\"]\r\n",
		progname);
	printf("example: %s -n 10 www.sina.com.cn www.qq.com\r\n", progname);
}

/* 当收到 SIGINT 信号(即在 PING 过程中用户按下 ctrl + c)时的信号处理函数 */
static void on_sigint(int signo acl_unused)
{
	exit(0);
}

int main(int argc, char* argv[])
{
	char  ch;
	int   i;

	signal(SIGINT, on_sigint);  /* 用户按下 ctr + c 时中断 PING 程序 */
	acl_msg_stdout_enable(1);  /* 允许 acl_msg_xxx 记录的信息输出至屏幕 */

	while ((ch = getopt(argc, argv, "hn:")) > 0) {
		switch (ch) {
		case 'n':
			__npkt = atoi(optarg);
			break;
		case 'h':
			usage(argv[0]);
			return 0;
		default:
			break;
		}
	}

	if (optind == argc) {
		usage(argv[0]);
		return 0;
	}

	if (__npkt <= 0)
		__npkt = 10;

	/* 记录要启动的协程的总数 */
	__nfibers = argc - optind;

	for (i = optind; i < argc; i++)
		acl_fiber_create(fiber_ping, argv[i], 32000);

	acl_fiber_schedule();

	return 0;
}
