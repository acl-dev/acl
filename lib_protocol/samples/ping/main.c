#include "lib_acl.h"
#include "lib_protocol.h"
#include "lib_acl.h"
#include <signal.h>

static int __delay = 1;

static void add_ip_list(ICMP_CHAT *chat, const ACL_ARGV *domain_list, int npkt)
{
	ACL_DNS_DB* dns_db;
	const char* ptr;
	int   i, j;
	char *pdomain, *pip;
	ACL_ARGV* ip_list = acl_argv_alloc(10);

	for (i = 0; i < domain_list->argc; i++) {
		dns_db = acl_gethostbyname(domain_list->argv[i], NULL);
		if (dns_db == NULL) {
			acl_msg_warn("Can't find domain %s", domain_list->argv[i]);
			continue;
		}

		for (j = 0; j < acl_netdb_size(dns_db); j++) {
			ptr = acl_netdb_index_ip(dns_db, j);
			if (ptr == NULL)
				continue;
			acl_argv_add(ip_list, domain_list->argv[i], ptr, NULL);
		}
		acl_netdb_free(dns_db);
	}

	for (i = 0; i < ip_list->argc;) {
		pdomain = ip_list->argv[i++];
		pip = ip_list->argv[i++];

		if (strcmp(pdomain, pip) == 0)
			icmp_ping_one(chat, NULL, pip, npkt, __delay, 1);
		else
			icmp_ping_one(chat, pdomain, pip, npkt, __delay, 1);
	}
}

static ICMP_CHAT *__chat = NULL;

static void display_res2(ICMP_CHAT *chat)
{
	if (chat) {
		/* 显示 PING 的结果总结 */
		icmp_stat(chat);
		printf(">>>max pkts: %d\r\n", icmp_chat_seqno(chat));
	}
}

static void display_res(void)
{
	if (__chat) {
		display_res2(__chat);

		/* 释放 ICMP 对象 */
		icmp_chat_free(__chat);
		__chat = NULL;
	}
}

/* 单线程异步 PING 多个地址的函数入口 */
static void ping_main_async(const ACL_ARGV *ip_list, int npkt)
{
	ACL_AIO *aio;

	/* 创建非阻塞异步通信句柄 */
	aio = acl_aio_create(ACL_EVENT_SELECT);
	acl_aio_set_keep_read(aio, 0);

	/* 创建 ICMP 对象 */
	__chat = icmp_chat_create(aio, 1);

	/* 添加需要 PING 的地址列表 */
	add_ip_list(__chat, ip_list, npkt);

	while (1) {
		/* 如果 PING 结束，则退出循环 */
		if (icmp_chat_finish(__chat)) {
			printf("over now!, hosts' size=%d, count=%d\r\n",
				icmp_chat_size(__chat), icmp_chat_count(__chat));
			break;
		}

		/* 异步事件循环过程 */
		acl_aio_loop(aio);
	}

	/* 显示 PING 结果 */
	display_res();

	/* 销毁非阻塞句柄 */
	acl_aio_free(aio);
}

/* 单线程 PING 单个地址的函数入口 */
static void ping_main_sync(const char *dest, int npkt)
{
	ACL_DNS_DB* dns_db;
	const char* ip;

	/* 创建 ICMP 对象 */
	__chat = icmp_chat_create(NULL, 1);

	/* 由域名解析出 IP 地址 */
	dns_db = acl_gethostbyname(dest, NULL);
	if (dns_db == NULL) {
		acl_msg_warn("Can't find domain %s", dest);
		return;
	}

	ip = acl_netdb_index_ip(dns_db, 0);
	if (ip == NULL || *ip == 0)
		acl_msg_fatal("ip invalid");

	/* 开始 PING 一个 IP 地址 */
	if (strcmp(dest, ip) == 0)
		icmp_ping_one(__chat, NULL, ip, npkt, __delay, 1);
	else
		icmp_ping_one(__chat, dest, ip, npkt, __delay, 1);

	/* 释放 DNS 查询结果 */
	acl_netdb_free(dns_db);

	/* 显示 PING 结果小结 */
	display_res();
}

/* PING 线程入口 */
static int __npkt = 10;
static void *ping_thread(void *arg)
{
	const char *ip, *dest = (char *) arg;
	ACL_DNS_DB* dns_db;
	ICMP_CHAT *chat;

	/* 通过域名解析出IP地址 */
	dns_db = acl_gethostbyname(dest, NULL);
	if (dns_db == NULL) {
		acl_msg_warn("Can't find domain %s", dest);
		return (NULL);
	}

	/* 只取出域名第一个 IP 地址 PING */
	ip = acl_netdb_index_ip(dns_db, 0);
	if (ip == NULL || *ip == 0) {
		acl_msg_error("ip invalid");
		acl_netdb_free(dns_db);
		return (NULL);
	}

	/* 创建 ICMP 对象 */
	chat = icmp_chat_create(NULL, 1);

	/* 开始 PING */
	if (strcmp(dest, ip) == 0)
		icmp_ping_one(chat, NULL, ip, __npkt, __delay, 1);
	else
		icmp_ping_one(chat, dest, ip, __npkt, __delay, 1);
	acl_netdb_free(dns_db);  /* 释放域名解析对象 */
	display_res2(chat);  /* 显示 PING 结果 */
	icmp_chat_free(chat);  /* 释放 ICMP 对象 */
	return (NULL);
}

/* 多线程方式 PING 多个目标地址，每个线程采用同步 PING 方式 */
static void ping_main_threads(const ACL_ARGV *ip_list, int npkt)
{
	int   i, n;
	acl_pthread_t tids[128];
	acl_pthread_attr_t attr;

	__npkt = npkt;
	acl_pthread_attr_init(&attr);
	acl_pthread_attr_setdetachstate(&attr, 0);

	/* 限定每次最大的线程数，防止系统开销太大 */
	n = ip_list->argc > 128 ? 128 : ip_list->argc;
	for (i = 0; i < n; i++)
		/* 创建 PING 线程 */
		acl_pthread_create(&tids[i], &attr, ping_thread, ip_list->argv[i]);

	for (i = 0; i < n; i++)
		/* 回收线程状态 */
		acl_pthread_join(tids[i], NULL);
}

static void usage(const char* progname)
{
	printf("usage: %s [-h help] -s [sync] -d delay -t [use thread mode] [-n npkt] [\"dest1 dest2 dest3...\"]\r\n", progname);
	printf("example: %s -n 10 www.sina.com.cn www.baidu.com www.qq.com\r\n", progname);
	printf("example: %s -s -n 10 www.sina.com.cn\r\n", progname);
#ifdef WIN32
	printf("please enter any key to exit\r\n");
	getchar();
#endif
}

/* 当收到 SIGINT 信号(即在 PING 过程中用户按下 ctrl + c)时的信号处理函数 */
static void OnSigInt(int signo acl_unused)
{
	display_res();
	exit(0);
}

int main(int argc, char* argv[])
{
	char  ch;
	int   npkt = 5, i, syn = 0, thread = 0;
	ACL_ARGV* dest_list = acl_argv_alloc(10);

	signal(SIGINT, OnSigInt);  /* 用户按下 ctr + c 时中断 PING 程序 */
	acl_socket_init();  /* 在 WIN32 下需要初始化全局套接字库 */
	acl_msg_stdout_enable(1);  /* 允许 acl_msg_xxx 记录的信息输出至屏幕 */

	while ((ch = getopt(argc, argv, "htsl:n:d:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return (0);
		case 's':
			syn = 1;
			break;
		case 't':
			thread = 1;
			break;
		case 'n':
			npkt = atoi(optarg);
			break;
		case 'd':
			__delay = atoi(optarg);
			break;
		default:
			usage(argv[0]);
			return (0);
		}
	}

	if (optind == argc) {
		usage(argv[0]);
		return (0);
	}

	for (i = optind; i < argc; i++) {
		acl_argv_add(dest_list, argv[i], NULL);
	}

	if (npkt <= 0)
		npkt = 0;

	/* 同步 PING 方式，对于多个目标地址，采用一个线程 PING 一个地址 */
	if (thread)
		ping_main_threads(dest_list, npkt);

	/* 同步 PING 方式，只能同时 PING 一个地址 */
	else if (syn)
		ping_main_sync(dest_list->argv[0], npkt);

	/* 异步 PING 方式，可以在一个线程中同时 PING 多个地址 */
	else
		ping_main_async(dest_list, npkt);

	acl_argv_free(dest_list);

#ifdef WIN32
	printf("please enter any key to exit\r\n");
	getchar();
#endif

	acl_socket_end();
	return 0;
}
