#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "acl_cpp/lib_acl.hpp"
#include "fiber/go_fiber.hpp"

#include "rfc1035.h"
#include "argv.h"

static RFC1035_MESSAGE* parse_request(const char* data, size_t len)
{
	RFC1035_MESSAGE *request;
	request = rfc1035_request_unpack(data, len);
	if (request == NULL) {
		logger_error("unpack request error, size=%d", (int) len);
		return NULL;
	}
	RFC1035_QUERY *query = request->query;
	if (query == NULL || query->name[0] == 0) {
		rfc1035_message_destroy(request);
		return NULL;
	}

#if 0
	printf(">>>ask name=%s, query type=%d, class=%d\n",
		query->name, query->qtype, query->qclass);
#endif

	switch (query->qtype) {
	case RFC1035_TYPE_A:
		break;
	case RFC1035_TYPE_AAAA:
		break;
	default:
		logger_error("unsport qtype=%d, qclass=%d",
			query->qtype, query->qclass);
		rfc1035_message_destroy(request);
		return NULL;
	}

	return request;
}

static size_t build_reply(const RFC1035_MESSAGE* request, char* buf, size_t size)
{
	ARGV* ips = argv_alloc(5);
	argv_add(ips, "192.168.1.1", NULL);
	argv_add(ips, "192.168.1.2", NULL);
	argv_add(ips, "192.168.1.3", NULL);

	RFC1035_REPLY reply;
	reply.hostname = request->query->name;

#if 1
	reply.domain_root = "test.com";
	reply.dns_name = "ipv6-static.dns.test.com";
	reply.dns_ip = "127.0.0.1";
#else
	reply.domain_root = NULL;
	reply.dns_name = NULL;
	reply.dns_ip = NULL;
#endif

	reply.ips = ips;

	reply.ip_type = RFC1035_TYPE_A;
	reply.ttl = 600;
	reply.qid = request->id;

	size_t n = rfc1035_build_reply(&reply, buf, size);
	argv_free(ips);
	return n;
}

static void handle_pkt(acl::socket_stream& stream, const char* data, size_t len)
{
	RFC1035_MESSAGE* request = parse_request(data, len);
	if (request == NULL) {
		logger_error("parse request error");
		return;
	}

	char buf[1024];
	size_t ret = build_reply(request, buf, sizeof(buf));
	rfc1035_message_destroy(request);
	if (!ret) {
		logger_error("build reply error");
		return;
	}

	if (stream.write(buf, ret) == -1) {
		logger_error("write error");
	}
}

static void dns_server(acl::socket_stream& stream)
{
	char buf[1024];
	int ret;

	while (true) {
		ret = stream.read(buf, sizeof(buf), false);
		if (ret == -1) {
			continue;
		}
		buf[ret] = 0;
		handle_pkt(stream, buf, ret);
	}
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help]\r\n"
		" -s addr\r\n"
		, procname);
}

int main(int argc, char *argv[])
{
	int   ch;
	acl::string addr("127.0.0.1:53");

	while ((ch = getopt(argc, argv, "hs:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			addr = optarg;
			break;
		default:
			break;
		}
	}

	if (addr.empty()) {
		usage(argv[0]);
		return 0;
	}

	acl::log::stdout_open(true);
	acl::fiber::stdout_open(true);

	acl::socket_stream stream;
	if (!stream.bind_udp(addr)) {
		printf("bind %s error %s\r\n", addr.c_str(), acl::last_serror());
		return 1;
	}

	go[&] {
		dns_server(stream);
	};

	acl::fiber::schedule();
	return 0;
}
