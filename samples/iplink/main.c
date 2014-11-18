#include "lib_acl.h"

int main(void)
{
	ACL_IPLINK *lnk = acl_iplink_create(10);
	const char *ip1_from = "127.0.0.1", *ip1_to = "127.0.0.1";
	const char *ip2_from = "10.0.250.218", *ip2_to = "10.0.250.218";
	const char *ip3_from = "10.0.250.38", *ip3_to = "10.0.250.38";

	acl_iplink_insert(lnk, ip1_from, ip1_to);
	printf("add from: %s to: %s\r\n", ip1_from, ip1_to);
	acl_iplink_insert(lnk, ip2_from, ip2_to);
	printf("add from: %s to: %s\r\n", ip2_from, ip2_to);
	acl_iplink_insert(lnk, ip3_from, ip3_to);
	printf("add from: %s to: %s\r\n", ip3_from, ip3_to);

	printf("==================================================\r\n");

	acl_iplink_list(lnk);

	printf("==================================================\r\n");

	acl_dlink_list(lnk);

	printf("==================================================\r\n");

	if (acl_iplink_lookup_str(lnk, ip1_from) == NULL)
		printf("ip: %s not included!\r\n", ip1_from);
	else
		printf("ip: %s included!\r\n", ip1_from);

	acl_iplink_free(lnk);
	return 0;
}
