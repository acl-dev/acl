#include "lib_acl.h"

int dns_lookup(const char *domain, const char *dns_ip,
	unsigned short dns_port, ACL_VSTRING *sbuf)
{
	const char *myname = "dns_lookup";
	ACL_RES *res = NULL;
	ACL_DNS_DB *dns_db = NULL;
	ACL_ITER iter;

#define RETURN(_x_) do { \
	if (res) \
		acl_res_free(res); \
	if (dns_db) \
		acl_netdb_free(dns_db); \
	return (_x_); \
} while (0)

	if (sbuf == NULL)
		acl_msg_fatal("%s: sbuf null", myname);

	res = acl_res_new(dns_ip, dns_port);

	dns_db = acl_res_lookup(res, domain);
	if (dns_db == NULL) {
		acl_vstring_sprintf(sbuf,
			"failed for domain %s, %s", domain, acl_res_errmsg(res));
		RETURN (-1);
	}

	(void) acl_netdb_size(dns_db);
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

	RETURN (0);
}
