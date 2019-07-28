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
		printf(">>>name=%s, addr=%s, type=%s\r\n",
			ifaddr->name, ifaddr->addr,
			ifaddr->saddr.sa.sa_family == AF_INET ?
			 "AF_INET" : (ifaddr->saddr.sa.sa_family == AF_INET6 ?
				 "AF_INET6" : "unknown"));
	}

	/* 释放查询结果 */
	acl_free_ifaddrs(ifconf);
}

static void test2(const char *pattern)
{
        ACL_ITER    iter;		/* 遍历对象 */
	ACL_IFCONF *ifconf;

	printf("pattern=%s\r\n", pattern);

	ifconf = acl_ifconf_search(pattern);
	if (ifconf == NULL) {
		printf("acl_ifconf_search error\r\n");
		return;
	}

	acl_foreach(iter, ifconf) {
		const ACL_IFADDR *ifaddr = (const ACL_IFADDR *) iter.data;
		const char *type;
		if (ifaddr->saddr.sa.sa_family == AF_INET)
			type = "AF_INET";
		else if (ifaddr->saddr.sa.sa_family == AF_INET6)
			type = "AF_INET6";
#ifdef AF_UNIX
		else if (ifaddr->saddr.sa.sa_family == AF_UNIX)
			type = "AF_UNIX";
#endif
		else
			type = "unknown";
		printf(">>>name=%s, addr=%s, type=%s\r\n",
			ifaddr->name, ifaddr->addr, type);
	}

	acl_free_ifaddrs(ifconf);
}

int main(void)
{
	const char *patterns[] = {
		/* 0 */ "127.0.0.1",
		/* 1 */ "127.0.0.1:8190",
		/* 2 */ "127.*.*.1:8190",
		/* 3 */ "127.0.0.1:8290, master_ctld.sock, 10.*.*.*:8390",
		/* 4 */ "8190",
		/* 5 */ "|8190",
		/* 6 */ ":8190",
		/* 7 */ "*|8190",
		/* 8 */ "*.*.*.*:8190",
		/* 9 */ "0.0.0.0:8190",
		/* 10 */ "aio.sock",
		/* 11 */ "/aio.sock",
		/* 12 */ "127.*.*.*:8290, 192.168.*.*:8291, 172.16.*.*:8292, 172.17.*.*:8293, /unix_server@unix",
		/* 13 */ "127.*.*.*:8290, 0.0.0.0:8191, *.*.*.*:8292",
		/* 14 */ ":8191, |8192, *:8193, *|8194",
		/* 15 */ "127.0.0.1:8290, master_ctld.sock, 10.*.*.*:8390",
		NULL,
	};

	test1();

	for (int i = 0; patterns[i]; i++) {
		if (i == 15) {
			printf("\r\n-----------------------------------\r\n");
			test2(patterns[i]);
		}
	}

	ACL_ARGV *tokens = acl_argv_alloc(10);
	for (int i = 0; patterns[i]; i++) {
		ACL_ARGV *fields = acl_argv_split(patterns[i], ",; \t\r\n");
		ACL_ITER  iter;

		acl_foreach(iter, fields) {
			acl_argv_add(tokens, (const char*) iter.data, NULL);
		}

		acl_argv_free(fields);
	}

	printf("\r\n-----------------------------------\r\n");

	ACL_ITER iter;
	acl_foreach(iter, tokens) {
		const char *ptr = (const char *) iter.data;
		if (!acl_valid_hostaddr(ptr, 0)) {
			printf(">>not valid host addr for: %s\r\n", ptr);
		}
	}

	acl_argv_free(tokens);

	printf("All over!\r\n");
	return 0;
}
