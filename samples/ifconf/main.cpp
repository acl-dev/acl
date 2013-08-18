#include "lib_acl.h"

int main(void)
{
        ACL_IFCONF *ifconf;
        ACL_IFADDR *ifaddr;
        ACL_ITER iter;
	char *host_ip = NULL;

	acl_msg_stdout_enable(1);
	ifconf = acl_get_ifaddrs();

	if (ifconf == NULL) {
		printf("acl_get_ifaddrs error: %s\r\n", acl_last_serror());
		return (1);
	}

	acl_foreach(iter, ifconf) {
		ifaddr = (ACL_IFADDR*) iter.data;

		if (strcmp(ifaddr->ip, "127.0.0.1") == 0)
			continue;

		acl_msg_info(">>>ip: %s", ifaddr->ip);
		/* ÍâÍøIPÓÅÏÈ */
		if (strncmp(ifaddr->ip, "10.", 3) != 0
				&& strncmp(ifaddr->ip, "192.", 4) != 0)
		{
			host_ip = acl_mystrdup(ifaddr->ip);
		} else if (host_ip == NULL) {
			host_ip = acl_mystrdup(ifaddr->ip);
		}
	}

	if (host_ip)
		printf(">>host_ip: %s\n", host_ip);
	return (0);
}
