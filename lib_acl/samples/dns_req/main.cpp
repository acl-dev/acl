#include "lib_acl.h"
#if  defined(ACL_MS_WINDOWS)
#pragma comment(lib,"ws2_32")
#pragma comment(lib, "wsock32")
#endif

static int __nresult = 0;

static void dns_lookup_callback(ACL_DNS_DB *dns_db, void *ctx, int errnum)
{
	ACL_ITER iter;
	char *domain = (char*) ctx;

	if (dns_db == NULL) {
		printf(">>>lookup result error(%s), domain(%s)\n",
			acl_dns_serror(errnum), domain);
		__nresult++;
		return;
	}

	printf(">>>%s: lookup result, domain(%s)\n", __FUNCTION__, dns_db->name);

	acl_foreach(iter, dns_db) {
		const ACL_HOST_INFO *info;

		info = (const ACL_HOST_INFO*) iter.data;
		const char *name = "unknown";
		switch (info->type) {
		case ACL_HOSTNAME_TYPE_IPV4:
			name = "ipv4";
			break;
		case ACL_HOSTNAME_TYPE_IPV6:
			name = "ipv6";
			break;
		case ACL_HOSTNAME_TYPE_CNAME:
			name = "cname";
			break;
		default:
			break;
		}
		printf("\t%s: addr=%s, ttl=%d\n", name, info->ip, info->ttl);
	}     

	__nresult++;
}

static void dns_lookup(char *domains, const char *dns_ips, int dns_port)
{
	ACL_AIO  *aio     = acl_aio_create(ACL_EVENT_SELECT);
	int       timeout = 5;
	ACL_DNS  *dns     = acl_dns_create(aio, timeout);
	ACL_ARGV *argv    = acl_argv_split(domains, ",:;|");
	ACL_ARGV *ip_argv = acl_argv_split(dns_ips, ",:;|");
	ACL_ITER  iter;

	/* 打开DNS缓存功能 */
	acl_dns_open_cache(dns, 100);

	/* 添加DNS服务器地址 */
	acl_foreach(iter, ip_argv) {
		char *ip = (char*) iter.data;
		printf("add one dns, ip=%s, port=%d\r\n", ip, dns_port);
		acl_dns_add_dns(dns, ip, dns_port, 24);
	}

	acl_argv_free(ip_argv);

	/* 设置校验DNS地址有效性标记位 */
	acl_dns_check_dns_ip(dns);

	/* 查询域名所对应的IP地址 */
	acl_foreach(iter, argv) {
		char *domain = (char*) iter.data;
		printf(">>>call dns lookup for: %s\n", domain);
		acl_dns_lookup(dns, domain, dns_lookup_callback, domain);
	}

	/* 进入事件循环中 */
	while (__nresult < iter.size) {
		acl_aio_loop(aio);
	}

	printf("---------------------------------------------------\n");
	printf(">>>DNS cache result search\n\n");
	/* 查询结果清零 */
	__nresult = 0;

	/* 通过缓存查询域名所对应的IP地址 */
	acl_foreach(iter, argv) {
		char *domain = (char*) iter.data;
		printf(">>>call dns lookup from cache for: %s\n", domain);
		acl_dns_lookup(dns, domain, dns_lookup_callback, domain);
	}

	while (__nresult < iter.size) {
		acl_aio_loop(aio);
	}

	acl_argv_free(argv);
	acl_dns_close(dns);
	acl_aio_check(aio);
	acl_aio_free(aio);
}

static void usage(const char *procname)
{
	printf("usage: %s -s dns_ip_list[192.168.0.1:192.168.0.2]"
		" -p dns_port -d domain_list[www.sina.com.cn:www.sohu.com]\r\n", procname);
}

int main(int argc, char *argv[])
{
	int   ch;
	char  dns_ips[512], domains[1024];
	int   dns_port = 53;

	dns_ips[0] = 0;
	domains[0] = 0;

	acl_lib_init();

	while ((ch = getopt(argc, argv, "hs:p:d:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			ACL_SAFE_STRNCPY(dns_ips, optarg, sizeof(dns_ips));
			break;
		case 'p':
			dns_port = atoi(optarg);
			break;
		case 'd':
			ACL_SAFE_STRNCPY(domains, optarg, sizeof(domains));
			break;
		default:
			usage(argv[0]);
			return 0;
		}
	}

	if (dns_ips[0] == 0 || domains[0] == 0) {
		usage(argv[0]);
		return 0;
	}

	acl_msg_stdout_enable(1);
	dns_lookup(domains, dns_ips, dns_port);
#ifdef ACL_MS_WINDOWS
	printf("Enter any key to exit\n");
	getchar();
#endif
	return 0;
}

