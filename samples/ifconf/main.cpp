#include "lib_acl.h"

int main(void)
{
        ACL_IFCONF *ifconf;
        ACL_IFADDR *ifaddr;
        ACL_ITER iter;
	char  host_ip[64];

	acl_msg_stdout_enable(1);
	ifconf = acl_get_ifaddrs();

	if (ifconf == NULL) {
		printf("acl_get_ifaddrs error: %s\r\n", acl_last_serror());
		return (1);
	}

	host_ip[0] = 0;
	acl_foreach(iter, ifconf) {
		ifaddr = (ACL_IFADDR*) iter.data;

		if (strcmp(ifaddr->ip, "127.0.0.1") == 0)
			continue;

		acl_msg_info(">>>ip: %s", ifaddr->ip);
		/* ÍâÍøIPÓÅÏÈ */
		if (strncmp(ifaddr->ip, "10.", 3) != 0
			&& strncmp(ifaddr->ip, "192.", 4) != 0)
		{
			snprintf(host_ip, sizeof(host_ip), "%s", ifaddr->ip);
		} else if (host_ip == NULL) {
			snprintf(host_ip, sizeof(host_ip), "%s", ifaddr->ip);
		}
	}

	if (host_ip[0])
		printf(">>host_ip: %s\n", host_ip);

	acl_free_ifaddrs(ifconf);
	return (0);
}
