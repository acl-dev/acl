#include "lib_acl.h"

static int dns_lookup(const char *domain, const char *dns_ip,
	unsigned short dns_port, ACL_VSTRING *sbuf)
{
	ACL_RES *res;		/* DNS 查询句柄 */
	ACL_DNS_DB *dns_db;	/* DNS 查询结果 */
	ACL_ITER iter;		/* 遍历句柄 */

	/* 创建 DNS 客户端查询对象 */
	res = acl_res_new(dns_ip, dns_port);

	/* 向 DNS 服务器发送域名查询信息 */
	dns_db = acl_res_lookup(res, domain);
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
		struct in_addr in;
		char  buf[32];

		info = (ACL_HOST_INFO*) iter.data;
		in.s_addr = info->saddr.sin_addr.s_addr;
		acl_mask_addr((unsigned char*) &in.s_addr, sizeof(in.s_addr), 24);
		acl_inet_ntoa(in, buf, sizeof(buf));

		acl_vstring_sprintf_append(sbuf, "A\t%d\t%s\t%s\t%d\r\n",
			info->ttl, info->ip, buf, res->cur_qid);
	}

	/* 释放 DNS 查询句柄 */
	acl_res_free(res);

	/* 释放域名查询结果 */
	acl_netdb_free(dns_db);
	return 0;
}

static void usage(const char *procname)
{
	printf("usage: %s -h dns_ip -p dns_port -d domain_name\r\n", procname);
}

int main(int argc, char *argv[])
{
	char  ch;
	char  dns_ip[64], domain[128];
	int   dns_port = 53, ret;
	ACL_VSTRING *sbuf;

	dns_ip[0] = 0;
	domain[0] = 0;

	while ((ch = getopt(argc, argv, "h:p:d:")) > 0) {
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
	ret = dns_lookup(domain, dns_ip, dns_port, sbuf);
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
