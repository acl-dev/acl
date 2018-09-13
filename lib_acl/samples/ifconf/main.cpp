#include "lib_acl.h"

static void test1(void)
{
        ACL_IFCONF *ifconf;	/* 网卡查询结果对象 */
        ACL_IFADDR *ifaddr;	/* 每个网卡信息对象 */
        ACL_ITER iter;		/* 遍历对象 */

	/* 查询本机所有网卡信息 */
	ifconf = acl_get_ifaddrs();

	if (ifconf == NULL) {
		printf("acl_get_ifaddrs error: %s\r\n", acl_last_serror());
		return;
	}

	printf("acl_get_ifaddrs:\r\n");

	/* 遍历所有网卡的信息 */
	acl_foreach(iter, ifconf) {
		ifaddr = (ACL_IFADDR*) iter.data;
		printf(">>>ip: %s, name: %s\r\n", ifaddr->ip, ifaddr->name);
	}

	/* 释放查询结果 */
	acl_free_ifaddrs(ifconf);
}

static void test2(const char *pattern)
{
        ACL_ITER iter;		/* 遍历对象 */
	ACL_ARGV   *addrs;

	addrs = acl_ifconf_search(pattern);
	if (addrs == NULL) {
		printf("acl_ifconf_search error\r\n");
		return;
	}

	printf("pattern=%s\r\n", pattern);
	acl_foreach(iter, addrs) {
		const char *addr = (const char *)iter.data;
		printf(">>>ip: %s\r\n", addr);
	}

	acl_argv_free(addrs);
}

int main(void)
{
	const char *patterns[] = {
		"8190",
		"#8190",
		":8190",
		"*#8190",
		"*.*.*.*:8190",
		"0.0.0.0:8190",
		"127.*.*.*:8290, 192.168.*.*:8291, 172.16.*.*.:8292, 172.17.*.*.:8293, /unix_server@unix",
		"127.*.*.*:8290, 0.0.0.0:8191, *.*.*.*:8292",
		":8191, #8192, *:8193, *#8194",
		NULL,
	};
	int i;

	test1();

	for (i = 0; patterns[i]; i++) {
		if (i >= 0) {
			printf("\r\n-----------------------------------\r\n");
			test2(patterns[i]);
		}
	}

	return 0;
}
