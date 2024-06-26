#include "lib_acl.h"
#include <string>
#include <resolv.h>
#include <arpa/nameser.h>

#if 1
void parse_svcb_record(const unsigned char *rdata, int rdlen) {
	int offset = 0;

	// 优先级（2字节）
	uint16_t priority;
	memcpy(&priority, rdata + offset, sizeof(priority));
	priority = ntohs(priority);
	offset += 2;

	// 目标主机名
	char target[NS_MAXDNAME];
	int name_len = dn_expand(rdata, rdata + rdlen, rdata + offset, target, sizeof(target));
	if (name_len < 0) {
		fprintf(stderr, "dn_expand failed\n");
		return;
	}
	offset += name_len;

	printf("Priority: %d\n", priority);
	printf("Target: %s\n", target);

	// 解析key=value对
	while (offset < rdlen) {
		// 参数key（2字节）
		uint16_t key;
		memcpy(&key, rdata + offset, sizeof(key));
		key = ntohs(key);
		offset += 2;

		// 参数值长度（2字节）
		uint16_t value_len;
		memcpy(&value_len, rdata + offset, sizeof(value_len));
		value_len = ntohs(value_len);
		offset += 2;

		// 参数值
		const unsigned char *value = rdata + offset;
		offset += value_len;

		printf("Parameter Key: %d\n", key);
		if (key == 1) { // alpn
			printf("ALPN: ");
			for (int i = 0; i < value_len; i++) {
				if (value[i] >= 32 && value[i] <= 126) {
					printf("%c", value[i]);
				} else {
					printf(".");
				}
			}
			printf("\n");
		} else if (key == 2) { // no-default-alpn
			printf("NO-DEFAULT-ALPN\n");
		} else if (key == 3) { // port
			if (value_len == 2) {
				uint16_t port;
				memcpy(&port, value, sizeof(port));
				port = ntohs(port);
				printf("Port: %d\n", port);
			}
		} else if (key == 4) { // ipv4hint
			printf("IPv4 Hints: ");
			for (int i = 0; i < value_len; i += 4) {
				char ip_str[INET_ADDRSTRLEN];
				inet_ntop(AF_INET, value + i, ip_str, sizeof(ip_str));
				printf("%s ", ip_str);
			}
			printf("\n");
		} else if (key == 6) { // echconfig
			printf("ECH Config (encoded): ");
			for (int i = 0; i < value_len; i++) {
				printf("%02x", value[i]);
			}
			printf("\n");
		} else if (key == 8) { // ipv6hint
			printf("IPv6 Hints: ");
			for (int i = 0; i < value_len; i += 16) {
				char ip_str[INET6_ADDRSTRLEN];
				inet_ntop(AF_INET6, value + i, ip_str, sizeof(ip_str));
				printf("%s ", ip_str);
			}
			printf("\n");
		} else {
			printf("Unknown Parameter Value\n");
		}
	}
}
#else
void parse_svcb_record(const unsigned char *rdata, int rdlen) {
	int offset = 0;

	// 优先级（2字节）
	uint16_t priority;
	memcpy(&priority, rdata + offset, sizeof(priority));
	priority = ntohs(priority);
	offset += 2;

	// 目标主机名
	char target[NS_MAXDNAME];
	int name_len = dn_expand(rdata, rdata + rdlen, rdata + offset, target, sizeof(target));
	if (name_len < 0) {
		fprintf(stderr, "dn_expand failed\n");
		return;
	}
	offset += name_len;

	printf("Priority: %d\n", priority);
	printf("Target: %s\n", target);

	// 解析key=value对
	while (offset < rdlen) {
		// 参数key（2字节）
		uint16_t key;
		memcpy(&key, rdata + offset, sizeof(key));
		key = ntohs(key);
		offset += 2;

		// 参数值长度（2字节）
		uint16_t value_len;
		memcpy(&value_len, rdata + offset, sizeof(value_len));
		value_len = ntohs(value_len);
		offset += 2;

		// 参数值
		//unsigned char value[value_len];
		unsigned char value[512];
		assert(value_len < 512);
		memcpy(value, rdata + offset, value_len);
		offset += value_len;

		printf("Parameter Key: %d\n", key);
		printf("Parameter Value: ");
		for (int i = 0; i < value_len; ++i) {
			printf("%02x ", value[i]);
		}
		printf("\n");
	}
}
#endif

static void test_res_query(const char *domain)
{
	unsigned char response[NS_PACKETSZ];
	int response_len = res_query(domain, C_IN, 65 /* DNS_TYPE_SVCB */, response, sizeof(response));

	if (response_len < 0) {
		perror("res_query failed");
		return;
	}

	ns_msg handle;
	if (ns_initparse(response, response_len, &handle) < 0) {
		fprintf(stderr, "ns_initparse failed\n");
		return;
	}

	int msg_count = ns_msg_count(handle, ns_s_an);
	for (int i = 0; i < msg_count; i++) {
		ns_rr rr;
		if (ns_parserr(&handle, ns_s_an, i, &rr) < 0) {
			fprintf(stderr, "ns_parserr failed\n");
			continue;
		}

		printf("Found SVCB record for %s\n", domain);

		if (ns_rr_type(rr) == ns_t_a && ns_rr_rdlen(rr) == 4) {
			struct in_addr addr;
			memcpy(&addr, ns_rr_rdata(rr), sizeof(addr));
			char ip_str[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, &addr, ip_str, sizeof(ip_str));
			printf("IPv4 Address: %s\n", ip_str);
		} else if (ns_rr_type(rr) == ns_t_aaaa && ns_rr_rdlen(rr) == 16) {
			struct in6_addr addr;
			memcpy(&addr, ns_rr_rdata(rr), sizeof(addr));
			char ip_str[INET6_ADDRSTRLEN];
			inet_ntop(AF_INET6, &addr, ip_str, sizeof(ip_str));
			printf("IPv6 Address: %s\n", ip_str);
		} else if (ns_rr_type(rr) == ns_t_cname) {
			char cname[NS_MAXDNAME];
			if (ns_name_uncompress(ns_msg_base(handle), ns_msg_end(handle),
				ns_rr_rdata(rr), cname, sizeof(cname)) < 0) {
				printf("s_name_uncompress failed\n");
			} else {
				printf("CNAME: %s\r\n", cname);
			}
		} else if (ns_rr_type(rr) == 65) {
			printf("svcb Record\n");
			parse_svcb_record(ns_rr_rdata(rr), ns_rr_rdlen(rr));
		} else {
			char buf[128];
			int n = (int) sizeof(buf) - 1;
			if (ns_rr_rdlen(rr) < n) {
				n = ns_rr_rdlen(rr);
			}
			memcpy(buf, ns_rr_rdata(rr), ns_rr_rdlen(rr));
			buf[n] = 0;

			printf("ns type=%d, rdlen=%d, buf=%s\n", ns_rr_type(rr), ns_rr_rdlen(rr), buf);
		}
	}
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help] -n domain\r\n", procname);
}

int main(int argc, char *argv[])
{
	char  ch;
	std::string name, service;

	while ((ch = getopt(argc, argv, "hn:s:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'n':
			name = optarg;
			break;
		case 's':
			service = optarg;
			break;
		default:
			usage(argv[0]);
			return 0;
		}
	}

	test_res_query(name.c_str());

	return 0;
}
