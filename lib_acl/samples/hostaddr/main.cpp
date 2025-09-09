#include "lib_acl.h"

static int test1(void) {
	struct {
		const char *addr;
		int type;
		int has_port;
		int ok;
	} addrs[] = {
		{ "127.0.0.1:80",       4, 1, 1 },
		{ "127.0.0.1|80",       4, 1, 1 },
		{ "192.168.0.1:65535",  4, 1, 1 },
		{ "[2001:db8::1]:443",  6, 1, 1 },
		{ "[::1]:22",           6, 1, 1 },
		{ "[abcd:1234:5678:9abc:def0:1234:5678:9abc]:8080", 6, 1, 1 },
		{ "256.1.1.1:80",      -4, 1, 0 }, // invalid IPv4
		{ "[2001:db8:::1]:80", -6, 1, 0 }, // invalid IPv6
		{ "192.168.0.1",        4, 0, 1 }, // missing port
		{ "[::1]",              6, 0, 1 }, // missing port
		{ "[::1]:80",           6, 1, 1 },
		{ "[::1]|80",           6, 1, 1 },
		{ "192.168.0.1|65535",  4, 1, 1 },
		{ "2001:db8::1|443",    6, 1, 1 },
		{ "2001:db8:::1|80",   -6, 1, 0 }, // invalid IPv6
		{ "*.*.*.1:80",        -4, 1, 1 },
		{ "*.*.*.1|80",        -4, 1, 1 },
		{ NULL,                 0, 0, 0 },
	};
	size_t i;

	for (i = 0; addrs[i].addr != NULL; i++) {
		if (acl_valid_hostaddr(addrs[i].addr, 1)) {
			printf("Valid addr=%s\r\n", addrs[i].addr);
		} else {
			printf("%d: Invalid addr=%s\r\n", __LINE__, addrs[i].addr);
		}

		printf("-----------------------------------------------\r\n");
	}

	printf("\r\n================================================\r\n\r\n");

	for (i = 0; addrs[i].addr != NULL; i++) {
		switch (addrs[i].type) {
		case 4:
			if (acl_valid_ipv4_hostaddr(addrs[i].addr, 1) == 0) {
				printf("%d: Check %s error\r\n", __LINE__, addrs[i].addr);
				return -1;
			}
			printf("Check %s ok\r\n", addrs[i].addr);
			break;
		case 6:
			if (acl_valid_ipv6_hostaddr(addrs[i].addr, 1) == 0) {
				printf("%d: Check %s error\r\n", __LINE__, addrs[i].addr);
				return -1;
			}
			printf("Check %s ok\r\n", addrs[i].addr);
			break;
		case -4:
			if (acl_valid_ipv4_hostaddr(addrs[i].addr, 1) == 1) {
				printf("%d: Check %s error\r\n", __LINE__, addrs[i].addr);
				return -1;
			}
			printf("Check %s ok\r\n", addrs[i].addr);
			break;
		case -6:
			if (acl_valid_ipv6_hostaddr(addrs[i].addr, 1) == 1) {
				printf("%d: Check %s error\r\n", __LINE__, addrs[i].addr);
				return -1;
			}
			printf("Check %s ok\r\n", addrs[i].addr);
			break;
		default:
			break;
		}

		printf("-----------------------------------------------\r\n");
	}


	printf("\r\n================================================\r\n\r\n");

	for (i = 0; addrs[i].addr != NULL; i++) {
		char ip[128];
		int  port;

		memset(ip, 0, sizeof(ip));

		if (acl_parse_hostaddr(addrs[i].addr, ip, sizeof(ip), &port)) {
			if (addrs[i].ok == 0) {
				printf("%d: Parse error, addr=%s, ip=%s, port=%d\r\n",
					__LINE__, addrs[i].addr, ip, port);
				return -1;
			}

			if (!addrs[i].has_port || port >= 0) {
				printf("Parse ok, addr=%s, ip=%s, port=%d\r\n",
					addrs[i].addr, ip, port);
			} else {
				printf("%d: Parse error, addr=%s, ip=%s, port=%d\r\n",
					__LINE__, addrs[i].addr, ip, port);
				return -1;
			}
		} else {
			if (addrs[i].ok == 1) {
				printf("%d: Parse error, addr=%s, ip=%s, port=%d\r\n",
					__LINE__, addrs[i].addr, ip, port);
				return -1;
			}

			printf("Parse ok, invalid addr=%s, ip=%s, port=%d\r\n",
				addrs[i].addr, ip, port);
		}
	}


	printf("\r\n================================================\r\n\r\n");

	for (i = 0; addrs[i].addr != NULL; i++) {
		char ip[128];
		int  port;

		memset(ip, 0, sizeof(ip));

		if (addrs[i].type == 4) {
			if (acl_parse_ipv4_hostaddr(addrs[i].addr, ip, sizeof(ip), &port)) {
				if (addrs[i].ok == 0) {
					printf("%d: Parse IPv4 error, addr=%s, ip=%s, port=%d\r\n",
						__LINE__, addrs[i].addr, ip, port);
					return -1;
				}
			} else {
				printf("%d: Parse IPv4 error, addr=%s, ip=%s, port=%d\r\n",
					__LINE__, addrs[i].addr, ip, port);
				return -1;
			}

			printf("Parse IPv4 ok, addr=%s, ip=%s, port=%d\r\n",
				addrs[i].addr, ip, port);
		} else if (addrs[i].type == 6) {
			if (acl_parse_ipv6_hostaddr(addrs[i].addr, ip, sizeof(ip), &port)) {
				if (addrs[i].ok == 0) {
					printf("%d: Parse IPv6 error, addr=%s, ip=%s, port=%d\r\n",
						__LINE__, addrs[i].addr, ip, port);
					return -1;
				}
			} else {
				printf("%d: Parse IPv6 error, addr=%s, ip=%s, port=%d\r\n",
					__LINE__, addrs[i].addr, ip, port);
				return -1;
			}

			printf("Parse IPv6 ok, addr=%s, ip=%s, port=%d\r\n",
				addrs[i].addr, ip, port);
		}
	}

	return 0;
}

int main(void) {
	acl_msg_stdout_enable(1);

	if (test1() == 0) {
		printf("\r\nAll over!\r\n");
	} else {
		printf("\r\nTest failed\r\n");
	}

	return 0;
}
