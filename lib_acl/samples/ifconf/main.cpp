#include "lib_acl.h"

int main(void)
{
        ACL_IFCONF *ifconf;	/* 网卡查询结果对象 */
        ACL_IFADDR *ifaddr;	/* 每个网卡信息对象 */
        ACL_ITER iter;		/* 遍历对象 */
	const char *pattern = "127.*.*.*:8290, 192.168.*.*:8290, 172.16.*.*.:8290, 172.17.*.*.:8290, /unix_server@unix";
	ACL_ARGV   *addrs;

	/* 查询本机所有网卡信息 */
	ifconf = acl_get_ifaddrs();

	if (ifconf == NULL) {
		printf("acl_get_ifaddrs error: %s\r\n", acl_last_serror());
		return 1;
	}

	/* 遍历所有网卡的信息 */
	acl_foreach(iter, ifconf) {
		ifaddr = (ACL_IFADDR*) iter.data;
		printf(">>>ip: %s, name: %s\r\n", ifaddr->ip, ifaddr->name);
	}

	/* 释放查询结果 */
	acl_free_ifaddrs(ifconf);

	printf("\r\n----------------------------------------------\r\n");

	addrs = acl_ifconf_search(pattern);
	if (addrs == NULL) {
		printf("acl_ifconf_search error\r\n");
		return 1;
	}

	printf("pattern=%s\r\n", pattern);
	acl_foreach(iter, addrs) {
		const char *addr = (const char *)iter.data;
		printf(">>>ip: %s\r\n", addr);
	}
	acl_argv_free(addrs);


	return 0;
}
