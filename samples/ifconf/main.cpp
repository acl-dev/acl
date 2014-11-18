#include "lib_acl.h"

int main(void)
{
        ACL_IFCONF *ifconf;	/* 网卡查询结果对象 */
        ACL_IFADDR *ifaddr;	/* 每个网卡信息对象 */
        ACL_ITER iter;		/* 遍历对象 */

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

	return 0;
}
