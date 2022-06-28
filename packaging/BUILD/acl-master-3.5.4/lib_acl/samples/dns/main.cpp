#include "lib_acl.h"

static int dns_lookup(const char *domain, const char *dns_ip,
	unsigned short dns_port, ACL_VSTRING *sbuf, int use_in6)
{
	ACL_RES *res;		/* DNS 查询句柄 */
	ACL_DNS_DB *dns_db;	/* DNS 查询结果 */
	ACL_ITER iter;		/* 遍历句柄 */

	/* 创建 DNS 客户端查询对象 */
	res = acl_res_new(dns_ip, dns_port);

	/* 向 DNS 服务器发送域名查询信息 */
	if (use_in6) {
		dns_db = acl_res_lookup6(res, domain);
	} else {
		dns_db = acl_res_lookup(res, domain);
	}

	if (dns_db == NULL) {
		acl_vstring_sprintf(sbuf, "failed for domain %s, %s",
			domain, acl_res_errmsg(res));
		acl_res_free(res);
		return -1;
	}

	/* 打印查询结果个数 */
	printf("domain: %s, count: %d\r\n", domain, acl_netdb_size(dns_db));

	/* 遍历所有的域名查询结果 */
	acl_vstring_sprintf_append(sbuf, "type\tttl\tip\t\tnet\t\tqid\t\n");
	acl_foreach(iter, dns_db) {
		ACL_HOST_INFO *info;
		char  buf[32];
		const char *type;

		info = (ACL_HOST_INFO*) iter.data;

		if (info->saddr.sa.sa_family == AF_INET) {
			struct in_addr in;
			in.s_addr = info->saddr.in.sin_addr.s_addr;
			acl_mask_addr((unsigned char*) &in.s_addr, sizeof(in.s_addr), 24);
			acl_inet_ntoa(in, buf, sizeof(buf));
			type = "A";
#ifdef AF_INET6
		} else if (info->saddr.sa.sa_family == AF_INET6) {
			struct in6_addr in6;
			memcpy(&in6, &info->saddr.in6.sin6_addr, sizeof(in6));
			acl_mask_addr((unsigned char*) &in6, sizeof(in6), 64);
			acl_inet6_ntoa(in6, buf, sizeof(buf));
			type = "AAAA";
#endif
		} else {
			buf[0] = 0;
			type = "Unknown";
		}

		acl_vstring_sprintf_append(sbuf, "%s\t%d\t%s\t%s\t%d\r\n",
			type, info->ttl, info->ip, buf, res->cur_qid);
	}

	/* 释放 DNS 查询句柄 */
	acl_res_free(res);

	/* 释放域名查询结果 */
	acl_netdb_free(dns_db);
	return 0;
}

static void usage(const char *procname)
{
	printf("usage: %s -h dns_ip -p dns_port -x [if lookup for inet6 addr] -d domain_name\r\n", procname);
}

int main(int argc, char *argv[])
{
	char  ch;
	char  dns_ip[64], domain[128];
	int   dns_port = 53, ret, use_in6 = 0;
	ACL_VSTRING *sbuf;

	dns_ip[0] = 0;
	domain[0] = 0;

	while ((ch = getopt(argc, argv, "h:p:d:x")) > 0) {
		switch (ch) {
		case 'h':
			ACL_SAFE_STRNCPY(dns_ip, optarg, sizeof(dns_ip));
			break;
		case 'p':
			dns_port = atoi(optarg);
			break;
		case 'd':
			ACL_SAFE_STRNCPY(domain, optarg, sizeof(domain));
			break;
		case 'x':
			use_in6 = 1;
			break;
		default:
			usage(argv[0]);
			return 0;
		}
	}

	if (dns_ip[0] == 0 || domain[0] == 0) {
		usage(argv[0]);
		return 0;
	}

	sbuf = acl_vstring_alloc(128);
	ret = dns_lookup(domain, dns_ip, dns_port, sbuf, use_in6);
	if (ret < 0) {
		printf("dns lookup(%s) error(%s)\r\n",
			domain, acl_vstring_str(sbuf));
		acl_vstring_free(sbuf);
		return 0;
	}

	printf("domain: %s\r\n%s", domain, acl_vstring_str(sbuf));
	acl_vstring_free(sbuf);
	return 0;
}
